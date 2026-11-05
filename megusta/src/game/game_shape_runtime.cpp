#include "game_shape_adapter.h"
#include "gui_shape_previews.h"
#include "ground_shadow.h"
#include "native_game_helpers.h"
#include "camera.h"
#include "player.h"
#include "scene_object.h"
#include "dts/shape.h"
#include "dts/shape_instance.h"
#include "dts/texture_manager.h"
#include "dts/scratch_release.h"
#include "engine/resource_runtime.h"
#include "engine/string_services.h"
#include "engine/engine_math.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace megusta {
namespace {
GameShapeRuntime* runtime=nullptr;
int loadedShapes=0;
float nodeAimFrame[16],nodeAimResult[16],nodeAimPitchBias=0.2f;
bool nodeAimInitialized=false,nodeAimMirrored=false;
void defaultNodeAimAngles(const float* direction,float& yaw,float& pitch) {
    // The callback normalizes the transformed direction, then passes
    // -direction.x to the first asin. Before its second asin, the saved code
    // overwrites the reused Point3F stack slot with the first rotation axis
    // (0,0,1); the second x87 input is therefore -1, not a direction
    // component. This yields a fixed quarter-pitch before the shape-specific
    // bias is applied below. These operands were checked against the saved
    // Ghidra instruction listing, including the VC6 stack-slot reuse.
    yaw=static_cast<float>(std::asin(-static_cast<long double>(direction[0])));
    pitch=static_cast<float>(std::asin(-1.0L));
}
GameShapeRuntime& shapeRuntime() {
    if (!runtime) throw std::logic_error("Game shape runtime is not bound"); return *runtime;
}
bool canAim() {
    auto& r=shapeRuntime();
    return Camera::canFollowMovingBall(r.cameraState,r.objects.sceneRoot!=nullptr,r.players.currentPlayer,r.balls,r.putter);
}
void assignTexture(dts::DTSMaterialTextureHandle& destination,void* source) {
    dts::releaseMaterialTextureEntry(destination.entry); destination.entry=source; dts::retainMaterialTextureEntry(source);
}
}
void bindGameShapeRuntime(GameShapeRuntime& state) { runtime=&state; }
void bindNativeGuiShapeServices(GuiShapeServices& services) {
    services.load=[](void*,GameShapeAdapter& shape,const char* name,unsigned mask,bool shadow){return shape.load(name,mask,shadow);};
    services.release=[](void*,GameShapeAdapter& shape){shape.release();};
    services.setSkin=[](void*,GameShapeAdapter& shape,int skin){shape.setSkin(skin);};
    services.draw=[](void*,GameShapeAdapter& shape,const float* view,const float* transform,bool mirrored){shape.drawPreview(view,transform,mirrored);};
    services.sceneActive=[](void*){return shapeRuntime().objects.sceneRoot!=nullptr;};
}
void GameShapeAdapter::release() {
    std::free(shapeName); shapeName=nullptr;
    delete optionalShadowObject; optionalShadowObject=nullptr;
    delete static_cast<dts::TSShapeInstance*>(instance); instance=nullptr;
    bool owned=false;
    if (shape) {
        if (!borrowedShape) { auto* value=static_cast<dts::TSShape*>(shape); value->destroy(); delete value; owned=true; }
        shape=nullptr;
    }
    if (owned) {
        if (--loadedShapes<0) std::abort();
        if (!loadedShapes) {
            dts::destroyTextureEntryList(); dts::releaseMeshScratchBuffers(); dts::releaseShapeReadScratchBuffers();
            dts::releaseLegacyScratchBuffers(); dts::releaseInstanceScratchBuffers();
        }
    }
    thread=nullptr; playbackFlags=0; importedSequenceCount=currentSequence=0;
}
bool GameShapeAdapter::load(const char* name,unsigned mask,bool shadow) {
    release();
    if (!loadedShapes) {
        dts::iflBaseTextures="data/textures/"; dts::initializeTextureEntryList();
        dts::normalFadeIn=dts::normalFadeOut=-1;
    }
    ++loadedShapes; shapeName=strings::duplicate(name); borrowedShape=false;
    char path[256]; std::sprintf(path,"data/shapes/%s.dts",name);
    auto* file=sharedIo->open(path,"rb"); if (!file) {std::fprintf(stderr,"DTS resource missing: %s\n",path);return false;}
    auto* value=new dts::TSShape; shape=value;
    value->read(*file,dts::legacyMaterialPrefix,dts::dtsUseEncodedNormals,dts::shapeSkipLoadDetails,dts::dtsConvertTriangles,dts::minimumStripTriangles); sharedIo->close(file);
    importedSequenceCount=0;
    if (mask) {
        const auto* names=shapeRuntime().sequenceNames;
        // The saved loader tests the table pointer before entering its loop;
        // a null table skips DSQ imports even when the caller supplied a mask.
        // Keep the same behavior instead of turning the optional table into a
        // runtime precondition.
        if (names) {
        for (unsigned i=0;names[i];++i) if (mask&(1u<<(i&31))) {
            std::sprintf(path,"data/shapes/%s%s.dsq",shapeName,names[i]);
            if (auto* sequence=sharedIo->open(path,"rb")) {
                value->importSequences(*sequence,dts::dtsShapeInitializeAfterRead); sharedIo->close(sequence); ++importedSequenceCount;
            }
        }
        }
    }
    auto* created=new dts::TSShapeInstance(value,1); instance=created; thread=created->addThread(); currentSequence=-1;
    if (shadow) optionalShadowObject=new ShapeShadow;
    baseSkinMaterial=activeSkinMaterial=0; char skin[256]; std::sprintf(skin,"skins/%s",shapeName);
    for (unsigned i=0;i<value->materialList.size();++i)
        if (!strings::compareInsensitive(value->materialList.materialNames[i],skin)) { baseSkinMaterial=activeSkinMaterial=i; break; }
    return true;
}
void GameShapeAdapter::setSkin(int skin) {
    if (!shape) return;
    auto& materials=static_cast<dts::TSShape*>(shape)->materialList; if (!materials.size()) return;
    char baseName[32],name[512]; std::sprintf(baseName,"skins/%s",shapeName);
    if (!skin) std::sprintf(name,"skins/%s",shapeName); else std::sprintf(name,"skins/%s0%d",shapeName,skin+1);
    unsigned index=0; for (;index<materials.size();++index) if (!strings::compareInsensitive(materials.materialNames[index],name)) break;
    if (index==materials.size()) {
        materials.push_back(name,0,-1,-1,-1,1,1); if (index==materials.size()) return; materials.loadIndex(materials.size()-1);
    }
    void* saved=materials.textureHandles[activeSkinMaterial].entry; dts::retainMaterialTextureEntry(saved);
    assignTexture(materials.textureHandles[activeSkinMaterial],materials.textureHandles[baseSkinMaterial].entry);
    assignTexture(materials.textureHandles[baseSkinMaterial],saved); activeSkinMaterial=index;
    dts::releaseMaterialTextureEntry(saved); saved=materials.textureHandles[index].entry; dts::retainMaterialTextureEntry(saved);
    assignTexture(materials.textureHandles[activeSkinMaterial],materials.textureHandles[baseSkinMaterial].entry);
    for (unsigned i=0;i<materials.size();++i) if (!strings::compareInsensitive(materials.materialNames[i],baseName)) assignTexture(materials.textureHandles[i],saved);
    dts::releaseMaterialTextureEntry(saved);
}
const float* gameShapeNodeAimCallback(int,void* data,int node) {
    auto* instance=static_cast<dts::TSShapeInstance*>(data);
    if (!nodeAimInitialized) { nodeAimInitialized=true; std::memcpy(nodeAimResult,instance->nodeTransforms[node].values,sizeof(nodeAimResult)); }
    if (canAim()) {
        float direction[3]={static_cast<float>(gameNodeAimVector[0]-nodeAimFrame[12]),
            static_cast<float>(gameNodeAimVector[1]-(nodeAimFrame[13]-2)),static_cast<float>(gameNodeAimVector[2]-(nodeAimFrame[14]+10))};
        transformShapeDirection(direction,direction,nodeAimFrame);
        if (direction[0]!=0 || direction[1]!=0 || direction[2]!=0) {
            engine_math::engineMath_normalizeVec3Unchecked_impl(direction);
            auto angles=shapeRuntime().nodeAimAngles;
            float yaw,pitch;
            if (angles) angles(direction,yaw,pitch); else defaultNodeAimAngles(direction,yaw,pitch);
            float first[16],second[16];
            makeShapeAxisAngleMatrix(first,yaw*0.5f,0,0,1); makeShapeAxisAngleMatrix(second,pitch*0.25f-nodeAimPitchBias,1,0,0);
            engine_math::engineMatrix_multiply4x4_impl(second,first,nodeAimResult);
        }
    }
    return nodeAimResult;
}
void GameShapeAdapter::configureNodeAim(bool mirrored) {
    nodeAimPitchBias=0.2f; nodeAimMirrored=mirrored; bool active=canAim();
    const char* names[]={"IN","EG","HA","WD","BG","CH","DI","SL","RU","IR","MU","PU","FW","HZ"};
    bool supported=false; for (const char* name:names) if (!std::strncmp(shapeName,name,2)) supported=true;
    if (!std::strncmp(shapeName,"RU",2)) nodeAimPitchBias=0.5f;
    else if (!std::strncmp(shapeName,"MU",2)) nodeAimPitchBias=0.8f;
    active=active&&supported; auto* value=static_cast<dts::TSShapeInstance*>(instance);
    if (active) { value->animateNodeSubtrees(true); value->setNodeTransformCallback(gameShapeNodeAimCallback,value); }
    const int first=value->shape->firstNodes[0],end=first+value->shape->numNodes[0];
    for (int i=first;i<end;++i) if (!std::strcmp(value->shape->names[i].c_str(),"codeNeck")) value->setNodeAnimationState(i,active?0x40:0);
}
void GameShapeAdapter::drawPreview(const float* view,const float* transform,bool mirrored) {
    configureNodeAim(mirrored); std::memcpy(nodeAimFrame,transform,sizeof(nodeAimFrame));
    if (!instance) return;
    auto& value=*static_cast<dts::TSShapeInstance*>(instance); value.animate(0); glLoadMatrixf(view); glMultMatrixf(transform);
    value.mirrorYRender=mirrored; dts::setMeshColor(gameShapeMeshColour[0],gameShapeMeshColour[1],gameShapeMeshColour[2]);
    value.alphaAlways=false; value.alphaAlwaysValue=1;
    dts::dtsSkinDisplayListsEnabled=false;dts::dtsSkinMode=1;
    value.renderCurrent(nullptr);dts::dtsSkinMode=0;dts::dtsSkinDisplayListsEnabled=true;
    dts::setMeshColor(1,1,1); value.mirrorYRender=false; glLoadIdentity(); glFrontFace(GL_CCW); glEnable(GL_CULL_FACE);
}
void GameShapeAdapter::drawWithAlpha(const float* view,const float* transform,bool mirrored,float alpha) {
    auto& value=*static_cast<dts::TSShapeInstance*>(instance); glLoadMatrixf(view); glMultMatrixf(transform); value.mirrorYRender=mirrored;
    dts::setMeshColor(gameShapeMeshColour[0],gameShapeMeshColour[1],gameShapeMeshColour[2]);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE); value.alphaAlways=false; value.alphaAlwaysValue=1; value.renderCurrent(nullptr);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); glDepthRange(0,0.99999); value.alphaAlways=alpha<0.99f; value.alphaAlwaysValue=alpha; value.renderCurrent(nullptr);
    glDepthRange(0,1); dts::setMeshColor(1,1,1); value.mirrorYRender=false; glLoadIdentity(); glFrontFace(GL_CCW); glEnable(GL_CULL_FACE);
}
void GameShapeAdapter::drawShadow(const float* view,const float* transform,bool mirrored) {
    if (!instance || !optionalShadowObject) return;
    shadowScale=0.9f;
    const auto prefix=[&](const char* name){return !std::strncmp(shapeName,name,2);};
    if (prefix("SL") || prefix("CH") || prefix("BG") || prefix("DI")) shadowScale=0.6f;
    else if (prefix("WD") || prefix("IN") || prefix("PU")) shadowScale=0.5f; else if (prefix("IR")) shadowScale=0.8f;
    float matrix[16]; std::memcpy(matrix,transform,sizeof(matrix)); dts::transposeMatrix4(matrix);
    const float scale[3]={shadowScale,shadowScale,shadowScale}; engine_math::engineMatrix_scaleRows3_impl(matrix,scale);
    if (mirrored) { const float mirror[3]={1,-1,1}; engine_math::engineMatrix_scaleRows3_impl(matrix,mirror); }
    const float direction[3]={-0.57f,0,-0.57f},unit[3]={1,1,1}; auto& value=*static_cast<dts::TSShapeInstance*>(instance);
    optionalShadowObject->setProjectionDirection(direction); optionalShadowObject->setShapeScale(value,unit); optionalShadowObject->prepareBitmap();
    optionalShadowObject->renderShapeMask(value,matrix); optionalShadowObject->uploadMask(); glBindTexture(GL_TEXTURE_2D,optionalShadowObject->textureName());
    glFrontFace(GL_CCW); glEnable(GL_CULL_FACE); renderGroundShadow(view,transform[12],transform[13],transform[14],-30,12,-20,20,-5,5,0.2f,false);
    glFrontFace(GL_CCW); glEnable(GL_CULL_FACE);
}
}
