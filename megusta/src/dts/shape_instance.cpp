#include "shape_instance.h"
#include "shape.h"
#include "material_list.h"
#include "thread.h"
#include "graphics_state.h"
#include "bitmap_support.h"
#include "../engine/opengl_adapter.h"
#include <algorithm>
#include <cmath>
#include <new>
namespace megusta { namespace dts {
int minimumDetailLevel=0;
int snapshotMaximumUpscale=2;
float snapshotProjectionCorrection[16]={1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1};
DTSProjectionState dtsProjectionState;
namespace {
void selectInstanceNodeTransform(int nodeIndex) {
    const DTSMatrix4* transform=dtsCurrentNodeTransforms+nodeIndex;
    if (transform==dtsCurrentModelTransform) return;
    if (dtsCurrentModelTransform) dtsRenderGL->popMatrix();
    dtsRenderGL->pushMatrix();
    dtsCurrentModelTransform=transform;
    float converted[16];
    std::copy(transform->values,transform->values+16,converted);
    transposeMatrix4(converted);
    dtsRenderGL->multiplyMatrix(converted);
}
bool meshIsHidden(const DTSAssembledMesh& mesh) {
    switch (mesh.type) {
    case DTSMeshStandard: return (static_cast<std::uint32_t>(mesh.base.meshType)&0x01000000u)!=0;
    case DTSMeshSkin: return (static_cast<std::uint32_t>(mesh.skin.meshType)&0x01000000u)!=0;
    case DTSMeshDecal: return (mesh.decal.meshType&0x01000000u)!=0;
    case DTSMeshSorted: return (static_cast<std::uint32_t>(mesh.sorted.meshType)&0x01000000u)!=0;
    default: return true;
    }
}
int compareThreadPointers(const TSThread* left,const TSThread* right) {
    const bool leftBlend=(left->sequence->flags&8u)!=0;
    const bool rightBlend=(right->sequence->flags&8u)!=0;
    if (leftBlend!=rightBlend) return leftBlend ? 1 : -1;
    if (right->priority<left->priority) return -1;
    if (left->priority<right->priority) return 1;
    return 0;
}
void sortThreadVector(std::vector<TSThread*>& threads) {
    std::sort(threads.begin(),threads.end(),[](const TSThread* left,const TSThread* right) {
        return compareThreadPointers(left,right)<0;
    });
}
EngineBitmapView* allocateSnapshotBitmap(std::uint32_t width,std::uint32_t height,
        char mipmaps,std::uint32_t format) {
    auto* bitmap=static_cast<EngineBitmapView*>(::operator new(sizeof(EngineBitmapView),std::nothrow));
    if (!bitmap) return nullptr;
    if (!constructEngineBitmap(bitmap,width,height,mipmaps!=0,format)) {
        ::operator delete(bitmap);
        return nullptr;
    }
    return bitmap;
}
std::uint8_t byteFromTruncate(long double value) {
    // The saved image conversion uses VC6 __ftol on each channel, which
    // truncates toward zero. Do not round filtered RGB or alpha upward.
    return static_cast<std::uint8_t>(static_cast<std::int64_t>(value));
}
int snapshotAlpha(const std::uint8_t* black,const std::uint8_t* white) {
    return 765+static_cast<int>(black[0])+black[1]+black[2]-
        static_cast<int>(white[0])-white[1]-white[2];
}
void renderInstanceMesh(DTSAssembledMesh& mesh,int frame,int materialFrame,TSMaterialList* materials) {
    switch (mesh.type) {
    case DTSMeshStandard: mesh.base.render(frame,materialFrame,materials); break;
    case DTSMeshSkin: mesh.skin.render(frame,materialFrame,materials); break;
    case DTSMeshDecal: mesh.decal.render(frame,materialFrame,materials); break;
    case DTSMeshSorted: mesh.sorted.render(frame,materialFrame,materials); break;
    default: break;
    }
}
}
void TSMeshObjectInstance::render(int meshIndex,TSMaterialList* materials) {
    if (visible<=0.01f || meshIndex>=object->numMeshes) return;
    DTSAssembledMesh& mesh=meshList[meshIndex];
    if (meshIsHidden(mesh)) return;
    selectInstanceNodeTransform(nodeIndex);
    if (visible>0.99f) {
        renderInstanceMesh(mesh,frame,materialFrame,materials);
        return;
    }
    TSMesh::setFade(visible);
    renderInstanceMesh(mesh,frame,materialFrame,materials);
    TSMesh::clearFade();
}
void TSDecalObjectInstance::render(int meshIndex,TSMaterialList* materials) {
    if (target->visible<=0.01f || meshIndex>=decal->numMeshes) return;
    DTSAssembledMesh& mesh=meshList[meshIndex];
    if (frame<0 || meshIsHidden(mesh) || mesh.type!=DTSMeshDecal) return;
    selectInstanceNodeTransform(nodeIndex);
    mesh.decal.render(target->frame,frame,materials);
}
void TSMeshObjectInstance::renderEnvironmentMap(int meshIndex,TSMaterialList* materials) {
    if (visible<=0.01f || meshIndex>=object->numMeshes) return;
    DTSAssembledMesh& mesh=meshList[meshIndex];
    if (mesh.type==DTSMeshDecal || mesh.type==DTSMeshNull) return;
    selectInstanceNodeTransform(nodeIndex);
    TSMesh& base=mesh.type==DTSMeshSkin ? static_cast<TSMesh&>(mesh.skin) :
                 mesh.type==DTSMeshSorted ? static_cast<TSMesh&>(mesh.sorted) : mesh.base;
    base.renderEnvironmentMap(frame,materialFrame,materials);
}
void TSMeshObjectInstance::renderFog(int meshIndex,TSMaterialList* materials) {
    if (visible<=0.01f || meshIndex>=object->numMeshes) return;
    DTSAssembledMesh& mesh=meshList[meshIndex];
    if (mesh.type==DTSMeshDecal || mesh.type==DTSMeshNull) return;
    selectInstanceNodeTransform(nodeIndex);
    TSMesh& base=mesh.type==DTSMeshSkin ? static_cast<TSMesh&>(mesh.skin) :
                 mesh.type==DTSMeshSorted ? static_cast<TSMesh&>(mesh.sorted) : mesh.base;
    base.renderFog(frame,materials);
}
void TSMeshObjectInstance::renderShadow(int meshIndex,const float* worldTransform,int dimension,
                                        std::uint32_t* mask,TSMaterialList* materials) {
    if (!(visible>0.01f) || meshIndex>=object->numMeshes) return;
    DTSAssembledMesh& mesh=meshList[meshIndex];
    if (mesh.type==DTSMeshNull) return;
    float objectTransform[16];
    multiplyMatrix4x4(worldTransform,dtsCurrentNodeTransforms[nodeIndex].values,objectTransform);
    TSMesh& base=mesh.type==DTSMeshSkin ? static_cast<TSMesh&>(mesh.skin) :
                 mesh.type==DTSMeshSorted ? static_cast<TSMesh&>(mesh.sorted) : mesh.base;
    // The saved object-instance routine dispatches every non-null mesh slot
    // through TSMesh_renderShadow. Decal slots have no base geometry here, so
    // the normalized empty base mesh supplies the same no-op behavior.
    base.renderShadow(frame,objectTransform,dimension,mask,materials);
}
TSShapeInstance::TSShapeInstance(TSShape* source,int loadMaterials) : shape(source) {
    instanceLinksReady=buildInstanceData(source,loadMaterials);
    // 004c6b30 clears mirrorY only after buildInstanceData returns.
    mirrorYRender=false;
}
TSShapeInstance::~TSShapeInstance() {
    // 004c6c80 repeatedly calls destroyThread on the last live entry, then
    // clears the material-list slot through setMaterialList(nullptr).
    while (!threads.empty()) destroyThread(threads.back());
    setMaterialList(nullptr);
    environmentTextureEntry=releaseMaterialTextureEntry(environmentTextureEntry);
    environmentTextureName=0;
    environmentMapEntryPresent=false;
}
TSThread* TSShapeInstance::addThread() {
    if (!shape || shape->sequences.empty()) return nullptr;
    auto* thread=new TSThread(this); threads.push_back(thread); setDirty(0x7f); return thread;
}
void TSShapeInstance::destroyThread(TSThread* thread) {
    if (!thread) return;
    clearTransition(thread);
    auto found=std::find(threads.begin(),threads.end(),thread);
    auto index=static_cast<std::size_t>(found-threads.begin());
    delete threads[index];
    threads.erase(found); setDirty(0x7f); checkScaleCurrentlyAnimated();
}
void TSShapeInstance::setDirty(std::uint32_t flags) { for (auto& value:dirtyFlags) value|=flags; }
void TSShapeInstance::clearDirty(std::uint32_t flags) {
    // Both recovered dirty-flag loops are bounded by the subshape-table
    // count. dirtyFlags is normalized to that same count during construction.
    for (auto& value:dirtyFlags) value&=~flags;
}
void TSShapeInstance::setTriggerStateBit(std::uint32_t mask,char enabled) {
    if (enabled) triggerStates|=mask; else triggerStates&=~mask;
}
bool TSShapeInstance::buildInstanceData(TSShape* source,int loadMaterials) {
    if (!source) return false;
    shape=source; bool load=static_cast<unsigned char>(loadMaterials)!=0;
    groundThread=nullptr;
    currentDetailLevel=0;
    maximumDetailMapDetail=0;
    materialList=nullptr;
    ownsMaterialList=false;
    instanceData=nullptr;
    // 004c6da0 loads the null-name default texture, stages a temporary
    // reference, replaces the instance handle, then drops the temporary one.
    void* loadedEnvironment=loadMaterialTextureEntry(nullptr,0,false);
    retainMaterialTextureEntry(loadedEnvironment);
    environmentTextureEntry=releaseMaterialTextureEntry(environmentTextureEntry);
    environmentTextureEntry=loadedEnvironment;
    retainMaterialTextureEntry(environmentTextureEntry);
    releaseMaterialTextureEntry(loadedEnvironment);
    environmentTextureName=materialTextureEntryName(environmentTextureEntry);
    environmentMapEntryPresent=environmentTextureEntry!=nullptr;
    environmentMapAlpha=0.0f;
    currentIntraDetailLevel=1.0f;
    alphaAlwaysValue=1.0f;
    environmentMapOn=false;
    allowTwoPassEnvironmentMap=true;
    allowTwoPassDetailMap=false;
    alphaIsReflectanceMap=true;
    maximumEnvironmentMapDetail=1;
    nodeTransformCallback=nullptr;
    nodeTransformCallbackData=nullptr;
    triggerStates=0;
    alphaAlways=false;
    scaleCurrentlyAnimated=false;
    // 004c6da0 clears DAT_0054ec54 after the instance flags and before
    // material-list/billboard initialization. 004c7920 later reads this
    // shared byte as the texturing-pass selector.
    dtsTexturingPassEnabled=false;
    if (load && shape->hasMaterials) setMaterialList(&shape->materialList);
    if (load) shape->setupBillboardDetails(nullptr);
    if (shape->objectStates.size()<shape->objects.size() || shape->decalStates.size()<shape->decals.size()) return false;
    for (const auto& object:shape->objects)
        if (object.numMeshes<0 || object.startMeshIndex<0 || static_cast<std::size_t>(object.startMeshIndex)>shape->assembledMeshes.size() ||
            static_cast<std::size_t>(object.numMeshes)>shape->assembledMeshes.size()-static_cast<std::size_t>(object.startMeshIndex)) return false;
    for (const auto& decal:shape->decals)
        if (decal.objectIndex<0 || static_cast<std::size_t>(decal.objectIndex)>=shape->objects.size() || decal.numMeshes<0 || decal.startMeshIndex<0 ||
            static_cast<std::size_t>(decal.startMeshIndex)>shape->assembledMeshes.size() ||
            static_cast<std::size_t>(decal.numMeshes)>shape->assembledMeshes.size()-static_cast<std::size_t>(decal.startMeshIndex)) return false;
    nodeTransforms.resize(shape->nodes.size());
    meshObjects.clear(); meshObjects.resize(shape->objects.size());
    for (std::size_t i=0;i<meshObjects.size();++i) {
        auto& out=meshObjects[i]; auto& object=shape->objects[i]; const auto& state=shape->objectStates[i];
        out.nodeIndex=object.nodeIndex; out.object=&object;
        out.meshList=object.numMeshes ? shape->assembledMeshes.data()+object.startMeshIndex : nullptr;
        out.visible=state.visibility; out.frame=state.frameIndex; out.materialFrame=state.materialFrameIndex;
    }
    decalObjects.clear(); decalObjects.resize(shape->decals.size());
    for (std::size_t i=0;i<decalObjects.size();++i) {
        auto& out=decalObjects[i]; auto& decal=shape->decals[i]; auto& target=meshObjects[decal.objectIndex];
        out.nodeIndex=target.nodeIndex; out.target=&target; out.decal=&decal; out.frame=shape->decalStates[i];
        out.meshList=decal.numMeshes ? shape->assembledMeshes.data()+decal.startMeshIndex : nullptr;
        for (int j=0;j<decal.numMeshes;++j) {
            auto& mesh=out.meshList[j]; if (mesh.type!=DTSMeshDecal) continue;
            mesh.decal.targetMesh=nullptr;
            if (j<target.object->numMeshes) {
                auto& targetMesh=target.meshList[j];
                // The saved pointer table refers to the actual mesh object,
                // including its redirected skin streams and subtype state.
                switch (targetMesh.type) {
                case DTSMeshStandard: mesh.decal.targetMesh=&targetMesh.base; break;
                case DTSMeshSkin: mesh.decal.targetMesh=&targetMesh.skin; break;
                case DTSMeshSorted: mesh.decal.targetMesh=&targetMesh.sorted; break;
                default: break;
                }
            }
        }
    }
    iflMaterialInstances.clear();
    if (load) for (const auto& material:shape->iflMaterials) { TSIflMaterialInstance instance; instance.material=&material; iflMaterialInstances.push_back(instance); }
    maximumDetailMapDetail=-1;
    if (load) setupDetailMapMeshes();
    dirtyFlags.assign(shape->firstNodes.size(),0x7f);
    // The original finishes construction by dispatching every dirty subtree.
    // This initializes object/decal state and IFL frames alongside the node pose.
    animateSubtrees(true);
    return true;
}
bool TSShapeInstance::setDefaultNodeTransforms() {
    return animateNodeTransforms();
}
void TSShapeInstance::handleDefaultScale(int firstNode,int endNode,TSIntegerSet& handled) {
    handled.takeAway(callbackNodes);
    handled.takeAway(handsOffNodes);
    uniformNodeScales.resize(shape->nodes.size(),1.0f);
    alignedNodeScales.resize(shape->nodes.size(),DTSPoint3{1.0f,1.0f,1.0f});
    arbitraryNodeScales.resize(shape->nodes.size(),DTSArbitraryScale{{0.0f,0.0f,0.0f,1.0f},{1.0f,1.0f,1.0f}});
    nodeScaleThreads.resize(shape->nodes.size(),nullptr);
    for (int node=firstNode;node<endNode && static_cast<unsigned>(node)<TSIntegerSet::capacity;++node) {
        if (!handled.test(static_cast<unsigned>(node))) continue;
        uniformNodeScales[node]=1.0f;
        alignedNodeScales[node]={1.0f,1.0f,1.0f};
        arbitraryNodeScales[node]={{0.0f,0.0f,0.0f,1.0f},{1.0f,1.0f,1.0f}};
        nodeScaleThreads[node]=nullptr;
    }
    handled.overlap(handsOffNodes);
    handled.overlap(callbackNodes);
}
bool TSShapeInstance::handleAnimatedScale(TSThread* thread,int firstNode,int endNode,TSIntegerSet& handled) {
    const auto& sequence=*thread->sequence;
    std::uint32_t rank=0;
    for (int node=0;node<endNode;++node) {
        if (!sequence.scaleMatters.test(static_cast<unsigned>(node))) continue;
        if (node>=firstNode && !handled.test(static_cast<unsigned>(node))) {
            int mode=(sequence.flags&2u) ? 1 : ((sequence.flags&4u) ? 2 : 0);
            if (shape->shapeFlags&2u) mode+=3;
            if (shape->shapeFlags&4u) mode+=6;
            long long base=static_cast<long long>(sequence.baseScale)+static_cast<long long>(sequence.numKeyframes)*rank;
            long long key1=base+thread->keyNum1,key2=base+thread->keyNum2;
            if (mode<=2) {
                float a=shape->nodeUniformScales[key1],b=shape->nodeUniformScales[key2],value=(b-a)*thread->keyPos+a;
                if (mode==0) uniformNodeScales[node]=value;
                else if (mode==1) alignedNodeScales[node]={value,value,value};
                else arbitraryNodeScales[node]={{0.0f,0.0f,0.0f,1.0f},{value,value,value}};
            } else if (mode==4 || mode==5) {
                float a[3]={shape->nodeAlignedScales[key1].x,shape->nodeAlignedScales[key1].y,shape->nodeAlignedScales[key1].z};
                float b[3]={shape->nodeAlignedScales[key2].x,shape->nodeAlignedScales[key2].y,shape->nodeAlignedScales[key2].z},value[3];
                interpolatePoint3F(a,b,thread->keyPos,value);
                if (mode==4) alignedNodeScales[node]={value[0],value[1],value[2]};
                else arbitraryNodeScales[node]={{0.0f,0.0f,0.0f,1.0f},{value[0],value[1],value[2]}};
            } else if (mode==8) {
                float a[3]={shape->nodeArbitraryScaleFactors[key1].x,shape->nodeArbitraryScaleFactors[key1].y,shape->nodeArbitraryScaleFactors[key1].z};
                float b[3]={shape->nodeArbitraryScaleFactors[key2].x,shape->nodeArbitraryScaleFactors[key2].y,shape->nodeArbitraryScaleFactors[key2].z},value[3],qa[4],qb[4],q[4];
                interpolatePoint3F(a,b,thread->keyPos,value);
                shape->nodeArbitraryScaleRotations[key1].getQuatF(qa); shape->nodeArbitraryScaleRotations[key2].getQuatF(qb);
                interpolateQuatF(qa,qb,thread->keyPos,q);
                arbitraryNodeScales[node]={{q[0],q[1],q[2],q[3]},{value[0],value[1],value[2]}};
            }
            nodeScaleThreads[node]=thread;
            handled.set(static_cast<unsigned>(node));
        }
        ++rank;
    }
    return true;
}
void TSShapeInstance::handleNodeScale(int firstNode,int endNode) {
    if ((shape->shapeFlags&7u)==0) return;
    for (int node=firstNode;node<endNode;++node) {
        if (handsOffNodes.test(static_cast<unsigned>(node))) continue;
        float* matrix=nodeTransforms[node].values;
        if (shape->shapeFlags&1u) applyUniformScale(uniformNodeScales[node],matrix);
        else if (shape->shapeFlags&2u) applyAlignedScale(&alignedNodeScales[node].x,matrix);
        else applyArbitraryScale(&arbitraryNodeScales[node],matrix);
    }
}
void TSShapeInstance::handleTransitionNodes(int firstNode,int endNode) {
    auto has=[&](const TSIntegerSet& set,int node) {
        if (static_cast<unsigned>(node)>=TSIntegerSet::capacity) return false;
        return set.test(static_cast<unsigned>(node));
    };
    // The original helper walks each channel's bitset from its first set bit.
    // Rotation alone checks the current subshape's lower bound; translation
    // and scale intentionally continue to process earlier set nodes. Preserve
    // that asymmetry from 004d10e0 because these channel arrays are shared
    // across subshape dispatches.
    for (std::uint32_t index=static_cast<std::uint32_t>(transitionRotationNodes.start());
         index<static_cast<std::uint32_t>(endNode);transitionRotationNodes.next(&index)) {
        const int node=static_cast<int>(index);
        if (node>=firstNode) {
            TSThread* rotationThread=nodeRotationThreads[node];
            if (!rotationThread || !rotationThread->transitionInProgress) {
                rotationThread=nullptr;
                for (auto* candidate:transitionThreads) {
                    if (has(candidate->transitionOldRotationNodes,node) ||
                        has(candidate->sequence->rotationMatters,node)) {
                        rotationThread=candidate; break;
                    }
                }
            }
            float current[4];
            float reference[4];
            for (int i=0;i<4;++i) current[i]=nodeRotationValues[node][i];
            nodeReferenceRotations[node].getQuatF(reference);
            interpolateQuatF(reference,current,
                rotationThread->transitionPosition,nodeRotationValues[node].data());
        }
    }
    for (std::uint32_t index=static_cast<std::uint32_t>(transitionTranslationNodes.start());
         index<static_cast<std::uint32_t>(endNode);transitionTranslationNodes.next(&index)) {
        const int node=static_cast<int>(index);
        TSThread* translationThread=nodeTranslationThreads[node];
        if (!translationThread || !translationThread->transitionInProgress) {
            translationThread=nullptr;
            for (auto* candidate:transitionThreads) {
                if (has(candidate->transitionOldTranslationNodes,node) ||
                    has(candidate->sequence->translationMatters,node)) {
                    translationThread=candidate; break;
                }
            }
        }
        const float amount=translationThread->transitionPosition;
        const auto& old=nodeReferenceTranslations[node]; auto& value=nodeLocalTranslations[node];
        value.x=(value.x-old.x)*amount+old.x;
        value.y=(value.y-old.y)*amount+old.y;
        value.z=(value.z-old.z)*amount+old.z;
    }
    if (scaleCurrentlyAnimated) {
        for (std::uint32_t index=static_cast<std::uint32_t>(transitionScaleNodes.start());
             index<static_cast<std::uint32_t>(endNode);transitionScaleNodes.next(&index)) {
            const int node=static_cast<int>(index);
            TSThread* scaleThread=nodeScaleThreads[node];
            if (!scaleThread || !scaleThread->transitionInProgress) {
                scaleThread=nullptr;
                for (auto* candidate:transitionThreads) {
                    if (has(candidate->transitionOldScaleNodes,node) ||
                        has(candidate->sequence->scaleMatters,node)) {
                        scaleThread=candidate; break;
                    }
                }
            }
            const float amount=scaleThread->transitionPosition;
            if (shape->shapeFlags&1u) {
                uniformNodeScales[node]+=(referenceUniformNodeScales[node]-uniformNodeScales[node])*amount;
            } else if (shape->shapeFlags&2u) {
                const auto& old=referenceAlignedNodeScales[node]; auto& value=alignedNodeScales[node];
                value.x=(value.x-old.x)*amount+old.x;
                value.y=(value.y-old.y)*amount+old.y;
                value.z=(value.z-old.z)*amount+old.z;
            } else {
                const auto& old=referenceArbitraryNodeScaleFactors[node];
                auto& value=arbitraryNodeScales[node];
                value.factors.x=(value.factors.x-old.x)*amount+old.x;
                value.factors.y=(value.factors.y-old.y)*amount+old.y;
                value.factors.z=(value.factors.z-old.z)*amount+old.z;
                float from[4];
                referenceArbitraryNodeScaleRotations[node].getQuatF(from);
                float to[4]={value.rotation.x,value.rotation.y,value.rotation.z,value.rotation.w},result[4];
                interpolateQuatF(from,to,amount,result);
                value.rotation={result[0],result[1],result[2],result[3]};
            }
        }
    }
}
void TSShapeInstance::updateTransitions() {
    if (transitionThreads.empty()) return;
    const std::size_t nodeCount=shape->nodes.size();
    nodeReferenceRotations.resize(nodeCount);
    nodeReferenceTranslations.resize(nodeCount);
    for (std::size_t node=0;node<nodeCount;++node) {
        const auto index=static_cast<unsigned>(node);
        if (transitionRotationNodes.test(index))
            nodeReferenceRotations[node].set(nodeRotationValues[node].data());
        if (transitionTranslationNodes.test(index))
            nodeReferenceTranslations[node]=nodeLocalTranslations[node];
    }
    if (shape->shapeFlags&7u) {
        if (shape->shapeFlags&1u) {
            referenceUniformNodeScales.resize(nodeCount);
            for (std::size_t node=0;node<nodeCount;++node)
                if (transitionScaleNodes.test(static_cast<unsigned>(node)))
                    referenceUniformNodeScales[node]=uniformNodeScales[node];
        } else if (shape->shapeFlags&2u) {
            referenceAlignedNodeScales.resize(nodeCount);
            for (std::size_t node=0;node<nodeCount;++node)
                if (transitionScaleNodes.test(static_cast<unsigned>(node)))
                    referenceAlignedNodeScales[node]=alignedNodeScales[node];
        } else {
            referenceArbitraryNodeScaleFactors.resize(nodeCount);
            referenceArbitraryNodeScaleRotations.resize(nodeCount);
            for (std::size_t node=0;node<nodeCount;++node) {
                if (!transitionScaleNodes.test(static_cast<unsigned>(node))) continue;
                referenceArbitraryNodeScaleFactors[node]=arbitraryNodeScales[node].factors;
                const auto& rotation=arbitraryNodeScales[node].rotation;
                const float components[4]={rotation.x,rotation.y,rotation.z,rotation.w};
                referenceArbitraryNodeScaleRotations[node].set(components);
            }
        }
    }
    for (auto* thread:transitionThreads) {
        if (!thread->transitionInProgress) continue;
        const float position=thread->transitionPosition;
        thread->transitionPosition=0.0f;
        thread->transitionDuration=(1.0f-position)*thread->transitionDuration;
    }
}
void TSShapeInstance::handleMaskedPositionNode(TSThread* thread,int node,int trackRank) {
    const auto& sequence=*thread->sequence;
    const long long base=static_cast<long long>(sequence.baseTranslation)+
        static_cast<long long>(sequence.numKeyframes)*trackRank;
    const long long key1=base+thread->keyNum1,key2=base+thread->keyNum2;
    const auto& first=shape->nodeTranslations[key1]; const auto& second=shape->nodeTranslations[key2];
    const bool maskX=maskPosXNodes.test(static_cast<unsigned>(node));
    const bool maskY=maskPosYNodes.test(static_cast<unsigned>(node));
    const bool maskZ=maskPosZNodes.test(static_cast<unsigned>(node));
    auto& translation=nodeLocalTranslations[node];
    if (!maskX) translation.x=(second.x-first.x)*thread->keyPos+first.x;
    if (!maskY) translation.y=(second.y-first.y)*thread->keyPos+first.y;
    if (!maskZ) translation.z=(second.z-first.z)*thread->keyPos+first.z;
}
void TSShapeInstance::handleBlendSequence(TSThread* thread,int firstNode,int endNode) {
    const auto& sequence=*thread->sequence;
    TSIntegerSet matters=sequence.translationMatters;
    matters.overlap(sequence.rotationMatters);
    matters.overlap(sequence.scaleMatters);
    int rotationRank=0,translationRank=0,scaleRank=0;
    for (int node=0;node<endNode && static_cast<unsigned>(node)<TSIntegerSet::capacity;++node) {
        if (!matters.test(static_cast<unsigned>(node))) continue;
        const bool apply=node>=firstNode && !disableBlendNodes.test(static_cast<unsigned>(node));
        float overlay[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
        if (sequence.rotationMatters.test(static_cast<unsigned>(node))) {
            if (apply) {
                const long long base=static_cast<long long>(sequence.baseRotation)+
                    static_cast<long long>(sequence.numKeyframes)*rotationRank;
                const long long key1=base+thread->keyNum1,key2=base+thread->keyNum2;
                float a[4],b[4],q[4];
                shape->nodeRotations[key1].getQuatF(a); shape->nodeRotations[key2].getQuatF(b);
                interpolateQuatF(a,b,thread->keyPos,q);
                DTSQuatF rotation{q[0],q[1],q[2],q[3]}; rotation.setMatrix(overlay);
            }
            ++rotationRank;
        }
        if (sequence.translationMatters.test(static_cast<unsigned>(node))) {
            if (apply) {
                const long long base=static_cast<long long>(sequence.baseTranslation)+
                    static_cast<long long>(sequence.numKeyframes)*translationRank;
                const long long key1=base+thread->keyNum1,key2=base+thread->keyNum2;
                const auto& a=shape->nodeTranslations[key1]; const auto& b=shape->nodeTranslations[key2];
                overlay[3]=(b.x-a.x)*thread->keyPos+a.x;
                overlay[7]=(b.y-a.y)*thread->keyPos+a.y;
                overlay[11]=(b.z-a.z)*thread->keyPos+a.z;
            }
            ++translationRank;
        }
        if (sequence.scaleMatters.test(static_cast<unsigned>(node))) {
            if (apply) {
                const long long base=static_cast<long long>(sequence.baseScale)+
                    static_cast<long long>(sequence.numKeyframes)*scaleRank;
                const long long key1=base+thread->keyNum1,key2=base+thread->keyNum2;
                if (sequence.flags&1u) {
                    const float a=shape->nodeUniformScales[key1],b=shape->nodeUniformScales[key2];
                    applyUniformScale((b-a)*thread->keyPos+a,overlay);
                } else if (shape->shapeFlags&2u) {
                    float a[3]={shape->nodeAlignedScales[key1].x,shape->nodeAlignedScales[key1].y,shape->nodeAlignedScales[key1].z};
                    float b[3]={shape->nodeAlignedScales[key2].x,shape->nodeAlignedScales[key2].y,shape->nodeAlignedScales[key2].z},scale[3];
                    interpolatePoint3F(a,b,thread->keyPos,scale); applyAlignedScale(scale,overlay);
                } else {
                    float qa[4],qb[4],q[4]; shape->nodeArbitraryScaleRotations[key1].getQuatF(qa); shape->nodeArbitraryScaleRotations[key2].getQuatF(qb);
                    interpolateQuatF(qa,qb,thread->keyPos,q);
                    const auto& a=shape->nodeArbitraryScaleFactors[key1]; const auto& b=shape->nodeArbitraryScaleFactors[key2];
                    DTSArbitraryScale scale{{q[0],q[1],q[2],q[3]},
                        {static_cast<float>((b.x-a.x)*thread->keyPos+a.x),
                         static_cast<float>((b.y-a.y)*thread->keyPos+a.y),
                         static_cast<float>((b.z-a.z)*thread->keyPos+a.z)}};
                    applyArbitraryScale(&scale,overlay);
                }
            }
            ++scaleRank;
        }
        if (apply) {
            float result[16];
            multiplyMatrix4x4(nodeTransforms[node].values,overlay,result);
            std::copy(result,result+16,nodeTransforms[node].values);
        }
    }
}
bool TSShapeInstance::animateNodeTransforms(int selectedSubShape) {
    if (!shape || shape->defaultRotations.size()<shape->nodes.size() || shape->defaultTranslations.size()<shape->nodes.size() ||
        shape->firstNodes.size()!=shape->numNodes.size()) return false;
    if (selectedSubShape < -1 || (selectedSubShape>=0 && static_cast<std::size_t>(selectedSubShape)>=shape->firstNodes.size())) return false;
    for (std::size_t sub=0;sub<shape->firstNodes.size();++sub) {
        int start=shape->firstNodes[sub],count=shape->numNodes[sub];
        if (start<0 || count<0 || static_cast<std::size_t>(start)>shape->nodes.size() || static_cast<std::size_t>(count)>shape->nodes.size()-static_cast<std::size_t>(start)) return false;
        for (int i=start;i<start+count;++i) if (shape->nodes[i].parentIndex>=i || shape->nodes[i].parentIndex < -1) return false;
    }
    nodeTransforms.resize(shape->nodes.size());
    nodeRotationValues.resize(shape->nodes.size());
    nodeLocalTranslations.resize(shape->nodes.size());
    // These correspond to the process-wide per-node channel-thread caches in
    // the saved implementation. Preserve earlier subshape entries: transition
    // evaluation can revisit earlier translation/scale bits while dispatching
    // a later subshape.
    nodeRotationThreads.resize(shape->nodes.size(),nullptr);
    nodeTranslationThreads.resize(shape->nodes.size(),nullptr);
    nodeScaleThreads.resize(shape->nodes.size(),nullptr);
    // 004d06e0 finds the first blend thread once. All normal-track passes
    // stop there; its final blend pass then visits the complete suffix and
    // gates only on blendDisabled.
    std::size_t firstBlendThread=0;
    while (firstBlendThread<threads.size() &&
           (threads[firstBlendThread]->sequence->flags&8u)==0)
        ++firstBlendThread;
    for (std::size_t sub=0;sub<shape->firstNodes.size();++sub) {
        if (selectedSubShape>=0 && static_cast<std::size_t>(selectedSubShape)!=sub) continue;
        int start=shape->firstNodes[sub],end=start+shape->numNodes[sub];
        TSIntegerSet rotationHandled,translationHandled,positionMasks;
        rotationHandled.setAll(static_cast<std::uint32_t>(shape->nodes.size()));
        translationHandled.setAll(static_cast<std::uint32_t>(shape->nodes.size()));
        for (std::size_t threadIndex=0;threadIndex<firstBlendThread;++threadIndex) {
            const auto* sequence=threads[threadIndex]->sequence;
            rotationHandled.takeAway(sequence->rotationMatters);
            translationHandled.takeAway(sequence->translationMatters);
        }
        rotationHandled.takeAway(callbackNodes);
        rotationHandled.takeAway(handsOffNodes);
        rotationHandled.overlap(maskRotationNodes);
        positionMasks.copy(maskPosXNodes);
        positionMasks.overlap(maskPosYNodes);
        positionMasks.overlap(maskPosZNodes);
        translationHandled.overlap(positionMasks);
        translationHandled.takeAway(callbackNodes);
        translationHandled.takeAway(handsOffNodes);

        // Initialize default channels only where the saved handled sets say
        // no ordinary sequence track owns them (or a callback/hands-off/mask
        // forces the default channel to be prepared).
        for (int node=start;node<end;++node) {
            const auto index=static_cast<unsigned>(node);
            if (rotationHandled.test(index)) {
                const auto& q=shape->defaultRotations[node];
                q.getQuatF(nodeRotationValues[node].data());
                nodeRotationThreads[node]=nullptr;
            }
            if (translationHandled.test(index)) {
                nodeLocalTranslations[node]=shape->defaultTranslations[node];
                nodeTranslationThreads[node]=nullptr;
            }
        }
        rotationHandled.overlap(handsOffNodes);
        rotationHandled.overlap(callbackNodes);
        translationHandled.takeAway(positionMasks);
        translationHandled.overlap(handsOffNodes);
        translationHandled.overlap(callbackNodes);

        if (scaleCurrentlyAnimated) {
            TSIntegerSet scaleHandled;
            scaleHandled.setAll(static_cast<std::uint32_t>(shape->nodes.size()));
            for (std::size_t threadIndex=0;threadIndex<firstBlendThread;++threadIndex) {
                const auto* thread=threads[threadIndex];
                scaleHandled.takeAway(thread->sequence->scaleMatters);
            }
            handleDefaultScale(start,end,scaleHandled);
            for (std::size_t threadIndex=0;threadIndex<firstBlendThread;++threadIndex) {
                auto* thread=threads[threadIndex];
                if (!handleAnimatedScale(thread,start,end,scaleHandled)) return false;
            }
        }
        for (std::size_t threadIndex=0;threadIndex<firstBlendThread;++threadIndex) {
            auto* thread=threads[threadIndex];
            const auto& sequence=*thread->sequence;
            int trackRank=0;
            for (std::uint32_t node=static_cast<std::uint32_t>(sequence.rotationMatters.start());
                 node<static_cast<std::uint32_t>(end);sequence.rotationMatters.next(&node),++trackRank) {
                if (node<static_cast<std::uint32_t>(start) || rotationHandled.test(node)) continue;
                const long long base=static_cast<long long>(sequence.baseRotation)+
                    static_cast<long long>(sequence.numKeyframes)*trackRank;
                float first[4],second[4];
                shape->nodeRotations[base+thread->keyNum1].getQuatF(first);
                shape->nodeRotations[base+thread->keyNum2].getQuatF(second);
                interpolateQuatF(first,second,thread->keyPos,nodeRotationValues[node].data());
                nodeRotationThreads[node]=thread;
                rotationHandled.set(node);
            }
            trackRank=0;
            for (std::uint32_t node=static_cast<std::uint32_t>(sequence.translationMatters.start());
                 node<static_cast<std::uint32_t>(end);sequence.translationMatters.next(&node),++trackRank) {
                if (node<static_cast<std::uint32_t>(start) || translationHandled.test(node)) continue;
                const int nodeIndex=static_cast<int>(node);
                if (positionMasks.test(node)) {
                    handleMaskedPositionNode(thread,nodeIndex,trackRank);
                } else {
                    const long long base=static_cast<long long>(sequence.baseTranslation)+
                        static_cast<long long>(sequence.numKeyframes)*trackRank;
                    const auto& first=shape->nodeTranslations[base+thread->keyNum1];
                    const auto& second=shape->nodeTranslations[base+thread->keyNum2];
                    auto& translation=nodeLocalTranslations[node];
                    translation.x=(second.x-first.x)*thread->keyPos+first.x;
                    translation.y=(second.y-first.y)*thread->keyPos+first.y;
                    translation.z=(second.z-first.z)*thread->keyPos+first.z;
                }
                nodeTranslationThreads[node]=thread;
                translationHandled.set(node);
            }
        }
        if (!transitionThreads.empty()) handleTransitionNodes(start,end);
        // The saved transition helper updates the channel arrays; the caller
        // then constructs local matrices from the transitioned rotation and
        // translation before scale, callback, blend, and parent passes.
        for (int node=start;node<end;++node) {
            if (handsOffNodes.test(static_cast<unsigned>(node))) continue;
            float* matrix=nodeTransforms[node].values;
            DTSQuatF rotation{nodeRotationValues[node][0],nodeRotationValues[node][1],
                nodeRotationValues[node][2],nodeRotationValues[node][3]};
            rotation.setMatrix(matrix);
            matrix[3]=nodeLocalTranslations[node].x;
            matrix[7]=nodeLocalTranslations[node].y;
            matrix[11]=nodeLocalTranslations[node].z;
        }
        if (scaleCurrentlyAnimated) handleNodeScale(start,end);
        for (int node=start;node<end;++node) {
            if (!callbackNodes.test(static_cast<unsigned>(node))) continue;
            const float* callbackTransform=nodeTransformCallback(0,nodeTransformCallbackData,node);
            std::copy(callbackTransform,callbackTransform+16,nodeTransforms[node].values);
        }
        for (std::size_t threadIndex=firstBlendThread;threadIndex<threads.size();++threadIndex) {
            auto* thread=threads[threadIndex];
            if (!thread->blendDisabled)
                handleBlendSequence(thread,start,end);
        }
        for (int i=start;i<end;++i) {
            int parent=shape->nodes[i].parentIndex; if (parent<0) continue;
            float local[16];
            std::copy(nodeTransforms[i].values,nodeTransforms[i].values+16,local);
            multiplyMatrix4x4(nodeTransforms[parent].values,local,nodeTransforms[i].values);
        }
    }
    return true;
}
std::uint8_t TSShapeInstance::getNodeAnimationState(int node) const {
    std::uint8_t state=maskRotationNodes.test(static_cast<unsigned>(node)) ? 1u : 0u;
    if (maskPosXNodes.test(static_cast<unsigned>(node))) state|=2u;
    if (maskPosYNodes.test(static_cast<unsigned>(node))) state|=4u;
    if (maskPosZNodes.test(static_cast<unsigned>(node))) state|=8u;
    if (disableBlendNodes.test(static_cast<unsigned>(node))) state|=0x10u;
    if (handsOffNodes.test(static_cast<unsigned>(node))) state|=0x20u;
    if (callbackNodes.test(static_cast<unsigned>(node))) state|=0x40u;
    return state;
}
void TSShapeInstance::setNodeAnimationState(int node,std::uint32_t state) {
    if (state&0x20u) state=0x30u;
    else if (state&0x40u) state=0x50u;
    if (state==getNodeAnimationState(node)) return;
    setDirty(0x7f);
    auto set=[&](TSIntegerSet& bits,bool enabled) {
        if (enabled) bits.set(static_cast<unsigned>(node)); else bits.clear(static_cast<unsigned>(node));
    };
    if ((state&0x0fu)==0) {
        set(maskRotationNodes,false); set(maskPosXNodes,false);
        set(maskPosYNodes,false); set(maskPosZNodes,false);
    } else {
        if (state&1u) set(maskRotationNodes,true);
        if (state&2u) set(maskPosXNodes,true);
        if (state&4u) set(maskPosYNodes,true);
        if (state&8u) set(maskPosZNodes,true);
    }
    set(disableBlendNodes,(state&0x10u)!=0);
    set(handsOffNodes,(state&0x20u)!=0);
    set(callbackNodes,(state&0x40u)!=0);
}
void TSShapeInstance::setNodeTransformCallback(NodeTransformCallback callback,void* callbackData) {
    nodeTransformCallback=callback;
    nodeTransformCallbackData=callbackData;
}
void TSShapeInstance::animateNodeSubtrees(bool markDirty) {
    if (markDirty) setDirty(1u);
    for (std::size_t sub=0;sub<shape->numNodes.size();++sub) {
        if (!(dirtyFlags[sub]&1u)) continue;
        animateNodeTransforms(static_cast<int>(sub));
        dirtyFlags[sub]&=~1u;
    }
}
void TSShapeInstance::animateSubtrees(bool markDirty) {
    if (markDirty) setDirty(0x7fu);
    for (std::size_t detail=0;detail<shape->numNodes.size();++detail) {
        if (!(dirtyFlags[detail]&1u)) continue;
        // The saved dispatcher passes its loop index as a detail index. The
        // detail path resolves the subshape, clears that dirty word, and the
        // outer loop then clears the word at its own index as well.
        animate(static_cast<int>(detail));
        dirtyFlags[detail]=0;
    }
}
void TSShapeInstance::setSequence(TSThread* thread,int sequence,float position) {
    // Preserve the outgoing blended node pose before TSThread::setSequence
    // removes this thread from the transition set.  The original performs
    // this evaluation whenever any transition is active, not only when this
    // particular thread is transitioning.
    if (!transitionThreads.empty()) animateNodeSubtrees(true);
    thread->setSequence(sequence,position);
    setDirty(0x7f);
    groundThread=nullptr;
    const bool sequenceHasScale=(thread->sequence->flags&7u)!=0;
    if (scaleCurrentlyAnimated) {
        if (!sequenceHasScale) checkScaleCurrentlyAnimated();
    } else if (sequenceHasScale) {
        scaleCurrentlyAnimated=true;
    }
    updateTransitions();
}
void TSShapeInstance::transitionToSequence(TSThread* thread,int sequence,float position,float duration,bool targetScale) {
    animateNodeSubtrees(true);
    thread->transitionToSequence(sequence,position,duration,targetScale);
    setDirty(0x7f);
    groundThread=nullptr;
    const bool sequenceHasScale=(thread->sequence->flags&7u)!=0;
    if (scaleCurrentlyAnimated) {
        if (!sequenceHasScale) checkScaleCurrentlyAnimated();
    } else if (sequenceHasScale) {
        scaleCurrentlyAnimated=true;
    }
    transitionRotationNodes.overlap(thread->transitionOldRotationNodes);
    transitionRotationNodes.overlap(thread->sequence->rotationMatters);
    transitionTranslationNodes.overlap(thread->transitionOldTranslationNodes);
    transitionTranslationNodes.overlap(thread->sequence->translationMatters);
    transitionScaleNodes.overlap(thread->transitionOldScaleNodes);
    transitionScaleNodes.overlap(thread->sequence->scaleMatters);
    if (std::find(transitionThreads.begin(),transitionThreads.end(),thread)==transitionThreads.end())
        transitionThreads.push_back(thread);
    updateTransitions();
}
void TSShapeInstance::clearTransition(TSThread* thread) {
    if (!thread || !thread->transitionInProgress) return;
    if (transitionThreads.size()>1) animateNodeSubtrees(true);
    thread->transitionInProgress=false;
    auto found=std::find(transitionThreads.begin(),transitionThreads.end(),thread);
    if (found!=transitionThreads.end()) transitionThreads.erase(found);
    transitionRotationNodes.clearAll(TSIntegerSet::capacity); transitionTranslationNodes.clearAll(TSIntegerSet::capacity); transitionScaleNodes.clearAll(TSIntegerSet::capacity);
    for (auto* active:transitionThreads) {
        transitionRotationNodes.overlap(active->transitionOldRotationNodes);
        transitionRotationNodes.overlap(active->sequence->rotationMatters);
        transitionTranslationNodes.overlap(active->transitionOldTranslationNodes);
        transitionTranslationNodes.overlap(active->sequence->translationMatters);
        transitionScaleNodes.overlap(active->transitionOldScaleNodes);
        transitionScaleNodes.overlap(active->sequence->scaleMatters);
    }
    setDirty(0x40);
    updateTransitions();
}
void TSShapeInstance::advanceTime(float delta) { for (auto* thread:threads) thread->advanceTime(delta); }
void TSShapeInstance::checkScaleCurrentlyAnimated() {
    scaleCurrentlyAnimated=true;
    for (const auto* thread:threads) {
        if (thread->sequence->flags&7u) return;
    }
    scaleCurrentlyAnimated=false;
}
bool TSShapeInstance::animateObjectStates(int sub,std::uint32_t mask) {
    int first=shape->firstObjects[sub],count=shape->numObjects[sub];
    int end=first+count;
    TSIntegerSet handledVisibility,handledFrame,handledMaterialFrame;
    const auto objectCount=static_cast<std::uint32_t>(meshObjects.size());
    handledVisibility.setAll(objectCount);
    handledFrame.setAll(objectCount);
    handledMaterialFrame.setAll(objectCount);
    for (auto* thread:threads) {
        const auto& sequence=*thread->sequence;
        handledVisibility.takeAway(sequence.visMatters);
        handledFrame.takeAway(sequence.frameMatters);
        handledMaterialFrame.takeAway(sequence.matFrameMatters);
    }
    for (int index=first;index<end;++index) {
        auto& object=meshObjects[index];
        const auto& state=shape->objectStates[index];
        if ((mask&2u) && handledVisibility.test(static_cast<unsigned>(index)))
            object.visible=state.visibility;
        if ((mask&4u) && handledFrame.test(static_cast<unsigned>(index)))
            object.frame=state.frameIndex;
        if ((mask&8u) && handledMaterialFrame.test(static_cast<unsigned>(index)))
            object.materialFrame=state.materialFrameIndex;
    }
    for (auto* thread:threads) {
        const auto& sequence=*thread->sequence;
        TSIntegerSet matters(sequence.frameMatters);
        matters.overlap(sequence.matFrameMatters);
        matters.overlap(sequence.visMatters);
        int rank=0;
        for (std::uint32_t index=static_cast<std::uint32_t>(matters.start());
             index<static_cast<std::uint32_t>(end);matters.next(&index),++rank) {
            const bool animateVisibility=(mask&2u) && sequence.visMatters.test(index) &&
                !handledVisibility.test(index);
            const bool animateFrame=(mask&4u) && sequence.frameMatters.test(index) &&
                !handledFrame.test(index);
            const bool animateMaterialFrame=(mask&8u) && sequence.matFrameMatters.test(index) &&
                !handledMaterialFrame.test(index);
            if (!animateVisibility && !animateFrame && !animateMaterialFrame) continue;
            auto& object=meshObjects[index];
            const int base=sequence.numKeyframes*rank+sequence.baseObjectState;
            const auto& firstState=shape->objectStates[base+thread->keyNum1];
            const auto& secondState=shape->objectStates[base+thread->keyNum2];
            if (animateVisibility) {
                const float difference=firstState.visibility-secondState.visibility;
                object.visible=difference*difference<=0.99f
                    ? secondState.visibility*thread->keyPos+
                      (1.0f-thread->keyPos)*firstState.visibility
                    : (thread->keyPos>=0.5f ? secondState.visibility : firstState.visibility);
                handledVisibility.set(index);
            }
            if (animateFrame) {
                object.frame=thread->keyPos>=0.5f ? secondState.frameIndex : firstState.frameIndex;
                handledFrame.set(index);
            }
            if (animateMaterialFrame) {
                object.materialFrame=thread->keyPos>=0.5f
                    ? secondState.materialFrameIndex : firstState.materialFrameIndex;
                handledMaterialFrame.set(index);
            }
        }
    }
    return true;
}
bool TSShapeInstance::animateDecalStates(int sub) {
    int first=shape->firstDecals[sub],count=shape->numDecals[sub];
    const int end=first+count;
    TSIntegerSet handled;
    handled.setAll(static_cast<std::uint32_t>(decalObjects.size()));
    for (auto* thread:threads) handled.takeAway(thread->sequence->decalMatters);
    for (int index=first;index<end;++index)
        if (handled.test(static_cast<unsigned>(index)))
            decalObjects[index].frame=shape->decalStates[index];
    for (auto* thread:threads) {
        const auto& sequence=*thread->sequence;
        int rank=0;
        for (std::uint32_t index=static_cast<std::uint32_t>(sequence.decalMatters.start());
             index<static_cast<std::uint32_t>(end);sequence.decalMatters.next(&index),++rank) {
            if (handled.test(index)) continue;
            const int key=sequence.numKeyframes*rank+sequence.baseDecalState+
                (thread->keyPos>=0.5f ? thread->keyNum2 : thread->keyNum1);
            decalObjects[index].frame=shape->decalStates[key];
            handled.set(index);
        }
    }
    return true;
}
void TSShapeInstance::animateVisibility(int sub) { animateObjectStates(sub,0x02u); }
void TSShapeInstance::animateFrame(int sub) { animateObjectStates(sub,0x04u); }
void TSShapeInstance::animateMatFrame(int sub) { animateObjectStates(sub,0x08u); }
void TSShapeInstance::animateDecals(std::uint32_t sub) { animateDecalStates(static_cast<int>(sub)); }
void TSShapeInstance::animateIfls() { updateIflFrames(); }
bool TSShapeInstance::updateIflFrames() {
    for (std::size_t i=0;i<iflMaterialInstances.size();++i) {
        auto& instance=iflMaterialInstances[i]; instance.frame=0;
        for (const auto* thread:threads) {
            if (!thread->sequence->iflMatters.test(static_cast<unsigned>(i))) continue;
            const auto& material=*instance.material;
            int count=material.numFrames,first=material.firstFrameOffTimeIndex;
            float end=count ? shape->iflFrameOffTimes[first+count-1] : 0;
            const long double extendedTime=
                static_cast<long double>(thread->sequence->duration)*thread->pos+
                thread->sequence->toolBegin;
            float time=static_cast<float>(extendedTime);
            if (static_cast<long double>(end)<extendedTime && end>0.0f) {
                const int cycles=static_cast<int>(static_cast<long double>(time)/end);
                time=static_cast<float>(static_cast<long double>(time)-
                    static_cast<long double>(cycles)*end);
            }
            int frame=0;
            while (frame<count-1 && !(time<=shape->iflFrameOffTimes[first+frame])) ++frame;
            instance.frame=frame; break;
        }
    }
    for (auto& flags:dirtyFlags) flags&=~0x20u;
    return true;
}
void TSShapeInstance::animate(int detail) {
    // 004d2970 checks only the -1 detail sentinel before indexing the saved
    // detail table. Preserve that contract rather than adding vector guards.
    if (detail==-1) return;
    int sub=shape->details[detail].subShape;
    if (sub<0) return;
    auto& dirty=dirtyFlags[sub];
    if (dirty&0x40u) {
        sortThreadVector(threads);
        sortThreadVector(transitionThreads);
    }
    if (dirty&0x20u) animateIfls();
    if (dirty&1u) animateNodeTransforms(sub);
    if (dirty&0x02u) animateVisibility(sub);
    if (dirty&0x04u) animateFrame(sub);
    if (dirty&0x08u) animateMatFrame(sub);
    if (dirty&0x10u) animateDecals(static_cast<std::uint32_t>(sub));
    // 004d2970 clears the entire subshape dirty word after running its set bits.
    dirty=0;
}
void TSShapeInstance::animate() { animate(currentDetailLevel); }
void TSShapeInstance::setMaterialList(TSMaterialList* materials) {
    if (ownsMaterialList && materialList) delete materialList;
    ownsMaterialList=false;
    materialList=materials;
    if (!materials) return;
    if (&shape->materialList==materials) shape->readIflMaterials();
    materials->tsmlTransform();
    materials->textureType=3;
    materials->clampToEdge=true;
    materials->loadAll();
    for (std::uint32_t i=0;i<materials->textureHandles.count;++i) {
        if (!(materials->getFlags(static_cast<int>(i))&
              (0xe0000000u|materialFlag(TSMaterialFlag::NeverEnvironmentMap))) &&
            materials->reflectanceMaps[i]!=-1 &&
            materials->reflectanceMaps[i]!=static_cast<int>(i)) {
            alphaIsReflectanceMap=false; break;
        }
    }
}
void TSShapeInstance::setupDetailMapMeshes() {
    maximumDetailMapDetail=-1;
    if (!shape || !materialList) return;
    auto materialHasDetailMap=[&](std::uint32_t materialIndex) {
        if (hasDrawPrimitiveFlag(materialIndex,TSDrawPrimitiveFlag::DynamicMaterial)) return false;
        materialIndex=drawPrimitiveMaterialIndex(materialIndex);
        const int detailMap=materialList->detailMaps[materialIndex];
        // The saved constructor only checks the -1 sentinel and whether the
        // texture-handle array exists; it does not range-check detailMap.
        return detailMap!=-1 && materialList->textureHandles.data!=nullptr;
    };
    auto markMesh=[&](DTSAssembledMesh& mesh,std::size_t detail) {
        bool hasDetailMap=false;
        switch (mesh.type) {
        case DTSMeshStandard:
            for (const auto& primitive:mesh.base.primitives)
                hasDetailMap=materialHasDetailMap(primitive.material)||hasDetailMap;
            if (hasDetailMap) mesh.base.meshType|=0x40000000;
            break;
        case DTSMeshSkin:
            for (const auto& primitive:mesh.skin.primitives)
                hasDetailMap=materialHasDetailMap(primitive.material)||hasDetailMap;
            if (hasDetailMap) mesh.skin.meshType|=0x40000000;
            break;
        case DTSMeshDecal:
            hasDetailMap=materialHasDetailMap(static_cast<std::uint32_t>(mesh.decal.materialIndex));
            if (hasDetailMap) mesh.decal.meshType|=0x40000000u;
            break;
        case DTSMeshSorted:
            for (const auto& primitive:mesh.sorted.primitives)
                hasDetailMap=materialHasDetailMap(primitive.material)||hasDetailMap;
            if (hasDetailMap) mesh.sorted.meshType|=0x40000000;
            break;
        default:
            break;
        }
        if (hasDetailMap && static_cast<int>(detail)>maximumDetailMapDetail)
            maximumDetailMapDetail=static_cast<int>(detail);
    };
    for (std::size_t detailIndex=0;detailIndex<shape->details.size();++detailIndex) {
        const DTSDetail& detail=shape->details[detailIndex];
        if (detail.subShape<0) continue;
        const std::size_t sub=static_cast<std::size_t>(detail.subShape);
        const int first=shape->firstObjects[sub],count=shape->numObjects[sub];
        for (int offset=0;offset<count;++offset) {
            const std::size_t objectIndex=static_cast<std::size_t>(first)+static_cast<std::size_t>(offset);
            const DTSObject& object=shape->objects[objectIndex];
            if (detail.objectDetail>=object.numMeshes) continue;
            const std::size_t meshIndex=static_cast<std::size_t>(object.startMeshIndex)+
                static_cast<std::size_t>(detail.objectDetail);
            markMesh(shape->assembledMeshes[meshIndex],detailIndex);
        }
        const int firstSkin=shape->detailFirstSkin[detailIndex];
        const int skinCount=shape->detailNumSkins[detailIndex];
        for (int offset=0;offset<skinCount;++offset) {
            const std::size_t skinIndex=static_cast<std::size_t>(firstSkin)+
                static_cast<std::size_t>(offset);
            markMesh(shape->assembledSkins[skinIndex],detailIndex);
        }
    }
}

void TSShapeInstance::setRenderStatics(int detail,float intraDetail,const float* postScale) {
    dtsCurrentNodeTransforms=reinterpret_cast<const DTSMatrix4*>(nodeTransforms.data());
    dtsCurrentDetailIndex=detail;
    dtsCurrentIntraDetail=intraDetail;
    billboardRenderState.postScale=postScale;
    dtsMaterialAlphaState=alphaAlways;
    dtsRenderAlpha=alphaAlways ? alphaAlwaysValue : 1.0f;
}

void TSShapeInstance::setupTexturing(int detail,float intraDetail) {
    dtsDetailMode=4;
    dtsActiveTextureUnit=0;
    const bool environmentActive=detail<=maximumEnvironmentMapDetail && environmentMapOn &&
        environmentTextureEntry && environmentMapAlpha>0.01f;
    dtsDetailMapAlpha=(detail<maximumDetailMapDetail || intraDetail>0.5f)
        ? 1.0f : intraDetail+intraDetail;
    if (environmentActive) {
        const float fade=(detail<maximumEnvironmentMapDetail || intraDetail>0.5f)
            ? 1.0f : intraDetail+intraDetail;
        dtsEnvironmentAlpha=fade*environmentMapAlpha;
        dtsEnvironmentTextureName=materialTextureEntryName(environmentTextureEntry);
    } else {
        dtsEnvironmentAlpha=1.0f;
        dtsEnvironmentTextureName=0;
    }

    int requiredUnits=1;
    if (hasMultitexture()) {
        int availableUnits=1;
        dtsRenderGL->getIntegerv(0x89,&availableUnits);
        if (hasTextureEnvCombine()) {
            if (environmentActive) {
                if (!alphaIsReflectanceMap) {
                    dtsEnvironmentMode=allowTwoPassEnvironmentMap ? 3 : 0;
                } else {
                    dtsEnvironmentTextureUnit=1;
                    dtsEnvironmentMode=1;
                    requiredUnits=2;
                }
            } else {
                dtsEnvironmentMode=0;
            }
            if (!dtsTexturingPassEnabled) {
                dtsTextureMode=8;
            } else if (availableUnits<requiredUnits+1 || dtsEnvironmentMode==3) {
                dtsTextureMode=11;
            } else {
                dtsTextureMode=9;
                dtsSecondaryTextureUnit=requiredUnits;
            }
        } else {
            dtsEnvironmentMode=environmentActive && allowTwoPassEnvironmentMap ? 3 : 0;
            dtsTextureMode=dtsTexturingPassEnabled ? 11 : 8;
        }
    } else {
        dtsEnvironmentMode=environmentActive && allowTwoPassEnvironmentMap ? 3 : 0;
        dtsTextureMode=dtsTexturingPassEnabled ? 11 : 8;
    }
    if (dtsEnvironmentMode==0) dtsEnvironmentAlpha=1.0f;
}
void TSShapeInstance::renderEnvironmentMap() {
    const int detail=dtsCurrentDetailIndex;
    const auto& descriptor=shape->details[detail];
    const int subShape=descriptor.subShape;
    const int objectDetail=descriptor.objectDetail;
    const int first=shape->firstObjects[subShape];
    const int end=first+shape->numObjects[subShape];
    if (first>=end) return;

    TSMesh::initEnvironmentMapMaterials();
    dtsCurrentModelTransform=nullptr;
    for (int object=first;object<end;++object)
        meshObjects[static_cast<std::size_t>(object)].renderEnvironmentMap(objectDetail,materialList);
    if (dtsCurrentModelTransform) {
        dtsRenderGL->popMatrix();
        dtsCurrentModelTransform=nullptr;
    }
    const int firstSkin=shape->detailFirstSkin[detail];
    const int endSkin=firstSkin+shape->detailNumSkins[detail];
    for (int skin=firstSkin;skin<endSkin;++skin)
        shape->assembledSkins[static_cast<std::size_t>(skin)].skin.renderEnvironmentMap(
            0,0,materialList);
    TSMesh::resetEnvironmentMapMaterials();
}
void TSShapeInstance::renderFog() {
    const int detail=dtsCurrentDetailIndex;
    const auto& descriptor=shape->details[detail];
    const int subShape=descriptor.subShape;
    const int objectDetail=descriptor.objectDetail;
    const bool lightingWasEnabled=dtsRenderGL->isEnabled(0x67);
    dtsRenderGL->disable(0x67);
    dtsRenderGL->color4fv(dtsFogColor);
    dtsRenderGL->enable(0x2e);
    dtsRenderGL->blendFunc(0x37,0x38);
    dtsRenderGL->enableClientState(0x1b);
    dtsCurrentModelTransform=nullptr;

    const int first=dtsTranslucentObjectRange
        ? shape->firstTranslucentObjects[subShape] : shape->firstObjects[subShape];
    const int end=dtsTranslucentObjectsOnly
        ? shape->firstTranslucentObjects[subShape]
        : shape->firstObjects[subShape]+shape->numObjects[subShape];
    for (int object=first;object<end;++object)
        meshObjects[static_cast<std::size_t>(object)].renderFog(objectDetail,materialList);
    if (dtsCurrentModelTransform) {
        dtsRenderGL->popMatrix();
        dtsCurrentModelTransform=nullptr;
    }
    const int firstSkin=shape->detailFirstSkin[detail];
    const int endSkin=firstSkin+shape->detailNumSkins[detail];
    for (int skin=firstSkin;skin<endSkin;++skin)
        shape->assembledSkins[static_cast<std::size_t>(skin)].skin.renderFog(0,materialList);
    dtsRenderGL->disable(0x2e);
    dtsRenderGL->blendFunc(0x32,0x31);
    dtsRenderGL->disableClientState(0x1b);
    if (lightingWasEnabled) dtsRenderGL->enable(0x67);
}
void TSShapeInstance::renderShadow(int detail,const float* worldTransform,int dimension,
                                   std::uint32_t* mask) {
    if (detail==-1) return;
    setRenderStatics(detail,0.0f,nullptr);
    dtsCurrentModelTransform=nullptr;
    const auto& descriptor=shape->details[static_cast<std::size_t>(detail)];
    const int subShape=descriptor.subShape;
    const int objectDetail=descriptor.objectDetail;
    const int first=shape->firstObjects[subShape];
    const int end=first+shape->numObjects[subShape];
    for (int object=first;object<end;++object)
        meshObjects[static_cast<std::size_t>(object)].renderShadow(
            objectDetail,worldTransform,dimension,mask,materialList);
    if (dtsCurrentModelTransform) dtsRenderGL->popMatrix();
    const int firstSkin=shape->detailFirstSkin[static_cast<std::size_t>(detail)];
    const int endSkin=firstSkin+shape->detailNumSkins[static_cast<std::size_t>(detail)];
    for (int skin=firstSkin;skin<endSkin;++skin)
        shape->assembledSkins[static_cast<std::size_t>(skin)].skin.renderShadow(
            0,worldTransform,dimension,mask,materialList);
    dtsCurrentNodeTransforms=nullptr;
}
void TSShapeInstance::renderDetail(int detail,float intraDetail,const float* postScale) {
    if (detail==-1) return;
    const auto& descriptor=shape->details[static_cast<std::size_t>(detail)];
    const int subShape=descriptor.subShape;
    const int objectDetail=descriptor.objectDetail;
    if (subShape<0) {
        if (dtsTranslucentObjectsOnly ||
            static_cast<std::size_t>(detail)>=shape->billboardDetails.size()) return;
        TSLastDetail* billboard=shape->billboardDetails[static_cast<std::size_t>(detail)].get();
        if (!billboard) return;
        const float alpha=alphaAlways ? alphaAlwaysValue : 1.0f;
        initializeLastDetailMaterials();
        // 004c75c0 passes DAT_0054ec54 to TSLastDetail_render_impl. It is
        // the texturing-pass byte reset by the instance constructor, not the
        // derived GL texture-mode selector (which may retain a prior mesh
        // pass value).
        billboard->render(alpha,dtsTexturingPassEnabled);
        resetLastDetailMaterials();
        return;
    }
    setRenderStatics(detail,intraDetail,postScale);
    for (const auto& instance:iflMaterialInstances)
        materialList->remap(instance.material->materialSlot,
                            instance.material->firstFrame+instance.frame);
    setupTexturing(detail,intraDetail);
    TSMesh::initMaterials();
    if (alphaAlways) dtsRenderGL->enable(0x2e);
    if (mirrorYRender) {
        dtsRenderGL->scale(1.0f,-1.0f,1.0f);
        dtsRenderGL->frontFace(0x13);
    }

    dtsCurrentModelTransform=nullptr;
    const int first=dtsTranslucentObjectRange
        ? shape->firstTranslucentObjects[subShape] : shape->firstObjects[subShape];
    const int end=dtsTranslucentObjectsOnly
        ? shape->firstTranslucentObjects[subShape]
        : shape->firstObjects[subShape]+shape->numObjects[subShape];
    for (int object=first;object<end;++object)
        meshObjects[static_cast<std::size_t>(object)].render(objectDetail,materialList);
    if (dtsCurrentModelTransform) {
        dtsRenderGL->popMatrix();
        dtsCurrentModelTransform=nullptr;
    }
    const int firstSkin=shape->detailFirstSkin[static_cast<std::size_t>(detail)];
    const int endSkin=firstSkin+shape->detailNumSkins[static_cast<std::size_t>(detail)];
    for (int skin=firstSkin;skin<endSkin;++skin)
        shape->assembledSkins[static_cast<std::size_t>(skin)].skin.render(0,0,materialList);
    if (mirrorYRender) dtsRenderGL->scale(1.0f,-1.0f,1.0f);
    TSMesh::resetMaterials();

    if (environmentMapOn && dtsEnvironmentMode==3) renderEnvironmentMap();
    if (dtsDecalsEnabled) {
        const int firstDecal=shape->firstDecals[subShape];
        const int endDecal=firstDecal+shape->numDecals[subShape];
        if (firstDecal<endDecal) {
            dtsCurrentModelTransform=nullptr;
            TSDecalMesh::initDecalMaterials();
            for (int decal=firstDecal;decal<endDecal;++decal)
                decalObjects[static_cast<std::size_t>(decal)].render(objectDetail,materialList);
            if (dtsCurrentModelTransform) {
                dtsRenderGL->popMatrix();
                dtsCurrentModelTransform=nullptr;
            }
            TSDecalMesh::resetDecalMaterials();
        }
    }
    if (dtsTextureMode==11) renderFog();
    if (alphaAlways) dtsRenderGL->disable(0x2e);
    dtsCurrentNodeTransforms=nullptr;
}
void TSShapeInstance::renderCurrent(const float* postScale) {
    const int detail=currentDetailLevel;
    if (detail<0) return;
    const float fadeOut=shape->alphaOut[static_cast<std::size_t>(detail)];
    const float baseAlpha=alphaAlways ? alphaAlwaysValue : 1.0f;
    const float alphaIn=shape->alphaIn[static_cast<std::size_t>(detail)];
    const float fadeEnd=alphaIn+fadeOut;
    if (fadeEnd<currentIntraDetailLevel) {
        renderDetail(detail,currentIntraDetailLevel,postScale);
        return;
    }
    const int next=detail+1;
    if (fadeOut<currentIntraDetailLevel) {
        if (next<static_cast<int>(shape->details.size()) && shape->details[next].size>0.0f) {
            const float alpha=((fadeEnd-currentIntraDetailLevel)*baseAlpha)/alphaIn;
            alphaAlwaysValue=alpha;
            alphaAlways=alpha<0.99f;
            renderDetail(next,0.0f,postScale);
        }
        alphaAlways=baseAlpha<0.99f;
        alphaAlwaysValue=baseAlpha;
        renderDetail(detail,currentIntraDetailLevel,postScale);
        return;
    }
    if (next<static_cast<int>(shape->details.size()) && shape->details[next].size>0.0f)
        renderDetail(next,0.0f,postScale);
    const float alpha=(currentIntraDetailLevel*baseAlpha)/fadeOut;
    alphaAlwaysValue=alpha;
    alphaAlways=alpha<0.99f;
    renderDetail(detail,currentIntraDetailLevel,postScale);
    alphaAlways=baseAlpha<0.99f;
    alphaAlwaysValue=baseAlpha;
}

int TSShapeInstance::getCurrentDetail() const { return currentDetailLevel; }
void TSShapeInstance::setCurrentDetail(int detail,float intraDetail) {
    currentDetailLevel=detail; currentIntraDetailLevel=intraDetail;
    int minimum=shape->smallestVisibleDetail;
    if (minimumDetailLevel<=minimum) minimum=minimumDetailLevel;
    if (detail>=0 && detail<minimum) { currentDetailLevel=minimum; currentIntraDetailLevel=1.0f; }
}
EngineBitmapView* TSShapeInstance::snapshot(int width,int height,char generateMipmaps,
        float* transform,char allowUpscale) {
    TSShapeInstance* self=this;
    using namespace megusta::dts;
    if (!self || !self->shape || !transform || !dtsRenderGL) return nullptr;
    const int displayWidth=queryDisplaySize()->width;
    const int displayHeight=queryDisplaySize()->height;
    if (displayWidth==0 || displayHeight==0) return nullptr;

    int upscale=1;
    if (allowUpscale && width*2<=displayWidth) {
        int candidate=2;
        for (;;) {
            const int candidateHeight=candidate*height;
            if (candidateHeight!=displayHeight && displayHeight<=candidateHeight) break;
            const int nextWidth=candidate*2*width;
            upscale=candidate;
            if (nextWidth!=displayWidth && displayWidth<nextWidth) break;
            candidate*=2;
        }
    }
    if (snapshotMaximumUpscale<upscale) upscale=snapshotMaximumUpscale;
    const std::uint32_t captureWidth=static_cast<std::uint32_t>(upscale*width);
    const std::uint32_t captureHeight=static_cast<std::uint32_t>(upscale*height);
    const int viewportX=(displayWidth>>1)-(captureWidth>>1);
    const int viewportY=(displayHeight>>1)-(captureHeight>>1);
    auto setViewport=[&](int x,int y,int w,int h) {
        auto& state=dtsProjectionState;
        state.viewport[0]=x; state.viewport[1]=y; state.viewport[2]=w; state.viewport[3]=h;
        dtsRenderGL->viewport(x,(displayHeight-y)-h,w,h);
        state.viewportWidthScale=static_cast<float>(w)*0.0015625f;
        state.pixelsPerUnit=(static_cast<float>(w)*state.nearPlane)/(state.right-state.left);
    };
    float savedClearColor[4];
    dtsRenderGL->getFloatv(0x47,savedClearColor);
    const bool savedStoredOrientation=billboardRenderState.useStoredOrientation;
    bool projectionPushed=false,modelviewPushed=false,clearColorRestored=false;
    EngineBitmapView* black=nullptr;
    EngineBitmapView* white=nullptr;
    EngineBitmapView* output=nullptr;
    auto restore=[&]() {
        if (black) FUN_004d2df0(black,1);
        if (white) FUN_004d2df0(white,1);
        setViewport(0,0,displayWidth,displayHeight);
        if (projectionPushed) {
            dtsRenderGL->matrixMode(0xf);
            dtsRenderGL->popMatrix();
        }
        if (modelviewPushed) {
            dtsRenderGL->matrixMode(0xe);
            dtsRenderGL->popMatrix();
        } else dtsRenderGL->matrixMode(0xe);
        if (!clearColorRestored) dtsRenderGL->clearColor(savedClearColor[0],savedClearColor[1],
            savedClearColor[2],savedClearColor[3]);
        billboardRenderState.useStoredOrientation=savedStoredOrientation;
        dtsCurrentModelTransform=nullptr;
    };

    setViewport(viewportX,viewportY,
        static_cast<int>(captureWidth),static_cast<int>(captureHeight));
    dtsRenderGL->matrixMode(0xf);
    dtsRenderGL->pushMatrix(); projectionPushed=true;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    dtsRenderGL->loadMatrix(identity);
    const float radius=self->shape->radius;
    auto& projection=dtsProjectionState;
    projection.left=-radius; projection.right=radius;
    projection.bottom=-radius; projection.top=radius;
    projection.nearPlane=1.0f; projection.farPlane=radius*20.0f;
    projection.orthographic=true;
    dtsRenderGL->ortho(-radius,radius,-radius,radius,1.0f,radius*20.0f);
    projection.pixelsPerUnit=static_cast<float>(captureWidth)/(projection.right-projection.left);
    dtsRenderGL->multiplyMatrix(snapshotProjectionCorrection);
    dtsRenderGL->matrixMode(0xe);
    dtsRenderGL->pushMatrix(); modelviewPushed=true;
    const float distance=radius*-10.0f;
    const float translated[3]={static_cast<float>(transform[1]*distance+self->shape->center[0]),
        static_cast<float>(transform[5]*distance+self->shape->center[1]),
        static_cast<float>(transform[9]*distance+self->shape->center[2])};
    transform[3]=translated[0]; transform[7]=translated[1]; transform[11]=translated[2];
    invertAffineMatrixInPlace(transform);
    float converted[16];
    std::copy(transform,transform+16,converted);
    transposeMatrix4(converted);
    dtsRenderGL->loadMatrix(converted);
    billboardRenderState.useStoredOrientation=false;
    dtsRenderGL->disable(0x18);
    dtsRenderGL->disable(0x67);
    dtsRenderGL->enable(0x21);
    dtsRenderGL->depthFunc(0x29);

    black=allocateSnapshotBitmap(captureWidth,captureHeight,0,2);
    if (!black) { restore(); return nullptr; }
    dtsRenderGL->clearColor(0.0f,0.0f,0.0f,0.0f);
    dtsRenderGL->clearFlags(3);
    self->renderDetail(self->currentDetailLevel,self->currentIntraDetailLevel,nullptr);
    dtsRenderGL->readPixels(viewportX,viewportY,static_cast<int>(captureWidth),
        static_cast<int>(captureHeight),0x3e,0,black->pixels+black->mipOffsets[0]);

    white=allocateSnapshotBitmap(captureWidth,captureHeight,0,2);
    if (!white) { restore(); return nullptr; }
    dtsRenderGL->clearColor(1.0f,1.0f,1.0f,1.0f);
    dtsRenderGL->clearFlags(3);
    self->renderDetail(self->currentDetailLevel,self->currentIntraDetailLevel,nullptr);
    dtsRenderGL->readPixels(viewportX,viewportY,static_cast<int>(captureWidth),
        static_cast<int>(captureHeight),0x3e,0,white->pixels+white->mipOffsets[0]);
    dtsRenderGL->disable(0x21);
    dtsRenderGL->clearColor(savedClearColor[0],savedClearColor[1],
        savedClearColor[2],savedClearColor[3]);
    clearColorRestored=true;

    output=allocateSnapshotBitmap(static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),generateMipmaps,3);
    if (!output) { restore(); return nullptr; }
    const std::uint8_t* blackPixels=black->pixels+black->mipOffsets[0];
    const std::uint8_t* whitePixels=white->pixels+white->mipOffsets[0];
    std::uint8_t* rgba=output->pixels+output->mipOffsets[0];
    if (!allowUpscale) {
        const int pixelCount=width*height;
        for (int pixel=0;pixel<pixelCount;++pixel) {
            const std::uint8_t* blackPixel=blackPixels+pixel*3;
            const std::uint8_t* whitePixel=whitePixels+pixel*3;
            const int alpha=snapshotAlpha(blackPixel,whitePixel);
            std::uint8_t* out=rgba+pixel*4;
            if (!alpha) out[0]=out[1]=out[2]=out[3]=0;
            else {
                const long double divisor=static_cast<long double>(alpha)*
                    static_cast<long double>(0.0013071896f);
                out[0]=byteFromTruncate(static_cast<long double>(blackPixel[0])/divisor);
                out[1]=byteFromTruncate(static_cast<long double>(blackPixel[1])/divisor);
                out[2]=byteFromTruncate(static_cast<long double>(blackPixel[2])/divisor);
                out[3]=static_cast<std::uint8_t>(alpha/3);
            }
        }
    } else {
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            long double red=0.0L;
            float green=0.0f,blue=0.0f,alphaWeight=0.0f;
            int alphaSum=0,covered=0;
            for (int dy=0;dy<upscale;++dy) for (int dx=0;dx<upscale;++dx) {
                const std::size_t source=(static_cast<std::size_t>(y*upscale+dy)*captureWidth+
                    static_cast<unsigned>(x*upscale+dx))*3;
                const auto* b=blackPixels+source;
                const int alpha=snapshotAlpha(b,whitePixels+source);
                if (alpha!=0) {
                    ++covered;
                    red+=b[0];
                    green=static_cast<float>(green+b[1]);
                    blue=static_cast<float>(blue+b[2]);
                }
                alphaSum+=alpha;
                alphaWeight=static_cast<float>(alpha*0.0013071896f+alphaWeight);
            }
            std::uint8_t* out=rgba+(static_cast<std::size_t>(y)*static_cast<unsigned>(width)+
                static_cast<unsigned>(x))*4;
            if (alphaWeight>0.01L) {
                const float inverseWeight=1.0f/alphaWeight;
                out[0]=byteFromTruncate(static_cast<long double>(inverseWeight)*red);
                out[1]=byteFromTruncate(static_cast<long double>(inverseWeight)*green);
                out[2]=byteFromTruncate(static_cast<long double>(inverseWeight)*blue);
            } else out[0]=out[1]=out[2]=0;
            out[3]=covered ? byteFromTruncate(
                static_cast<long double>(alphaSum)/static_cast<long double>(covered*3)) : 0;
        }
    }
    if (black) { FUN_004d2df0(black,1); black=nullptr; }
    if (white) { FUN_004d2df0(white,1); white=nullptr; }
    if (generateMipmaps) FUN_004d34f0(output,0);
    restore();
    EngineBitmapView* result=output;
    output=nullptr;
    return result;
}

EngineBitmapView* snapshotShapeStatic(TSShape* shape,int width,int height,
        char generateMipmaps,float* transform,int detail,float intraDetail,char allowUpscale) {
    if (!shape) return nullptr;
    auto* instance=new (std::nothrow) megusta::dts::TSShapeInstance(shape,1);
    if (!instance) return nullptr;
    instance->setCurrentDetail(detail,intraDetail);
    instance->animate();
    auto* result=instance->snapshot(width,height,generateMipmaps,transform,allowUpscale);
    delete instance;
    return result;
}

} }


