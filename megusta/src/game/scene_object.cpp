#include "scene_object.h"
#include "sounds.h"
#include <cmath>
#include "ssgASX.h"
namespace megusta {
void accumulateEntityTransform(ssgEntity* entity, std::uint32_t frame, sgMat4 matrix) {
    if ((entity->getType() & SSG_TYPE_TRANSFORM) == SSG_TYPE_TRANSFORM) {
        if (entity->getType() & 0x1000000)
            static_cast<asx::GameAnimatedTransform*>(entity)->setFrame(static_cast<int>(frame));
        sgMat4 local;
        static_cast<ssgTransform*>(entity)->getTransform(local);
        sgPreMultMat4(matrix, local);
    }
}
void accumulateAncestorTransforms(ssgEntity* stop, ssgEntity* entity, std::uint32_t frame, sgMat4 matrix) {
    ssgEntity* first = entity->getParent(0);
    if (!first) return;
    ssgEntity* current = first;
    while (current != stop) {
        ssgEntity* next = entity->getNextParent();
        if (!next) { current = first; break; }
        current = next;
    }
    accumulateAncestorTransforms(stop, current, frame, matrix);
    accumulateEntityTransform(current, frame, matrix);
}
void SceneObjectRecord::refreshFrameTransform() {
    if (flags & 1) { transformDirty = false; return; }
    sgMakeIdentMat4(frameTransform);
    accumulateAncestorTransforms(nullptr, entity, currentFrame, frameTransform);
    sgCopyMat4(baseFrameTransform, frameTransform);
    accumulateEntityTransform(entity, currentFrame, frameTransform);
    transformDirty = false;
}
float* SceneObjectRecord::getFrameTransform() {
    if (transformDirty) refreshFrameTransform();
    return frameTransform[0];
}
float* SceneObjectRecord::getBaseFrameTransform() {
    if (transformDirty) refreshFrameTransform();
    return baseFrameTransform[0];
}
float* SceneObjectRecord::getPosition() {
    const float* m = getFrameTransform();
    const float x=localCenter[0], y=localCenter[1], z=localCenter[2];
    worldPositionScratch[0]=x*m[0]+y*m[4]+z*m[8]+m[12];
    worldPositionScratch[1]=x*m[1]+y*m[5]+z*m[9]+m[13];
    worldPositionScratch[2]=x*m[2]+y*m[6]+z*m[10]+m[14];
    return worldPositionScratch;
}
SceneTriangleServices SceneObjectCollection::triangleServices() {
    return {this,
        [](void* context, std::uint16_t index) -> const float* {
            return static_cast<SceneObjectCollection*>(context)->objects[index].getFrameTransform();
        },
        [](void* context, std::uint16_t index) -> std::uint32_t {
            return static_cast<SceneObjectCollection*>(context)->objects[index].flags;
        }};
}
}

#include "../engine/string_services.h"
#include <cstdlib>
#include <cstring>
namespace megusta {
void SceneObjectRecord::setWire(bool enabled, void (*applyWireDebug)()) {
    if (enabled) flags |= 2; else flags &= ~2u;
    applyWireDebug();
}
void SceneObjectRecord::setSound(const char* sound) {
    if (impactSound) { std::free(impactSound); impactSound = nullptr; }
    if (sound && *sound) impactSound = strings::duplicate(sound);
}
void SceneObjectRecord::playImpactSound(const float* position,const float* velocity,Sounds& sounds) {
    if (impactSound) {
        const float squared=velocity[2]*velocity[2]+velocity[1]*velocity[1]+velocity[0]*velocity[0];
        if (std::sqrt(static_cast<long double>(squared))>1.0L && !sounds.isPlaying(impactSound,0))
            sounds.play3d(impactSound,position[0],position[1],position[2],0,0);
    }
}
void SceneObjectCollection::setDefaultSound(const char* sound) {
    for (int i=1;i<=objectCount;++i) {
        auto& object=objects[i];
        if (object.name && !(object.flags&1) && !object.impactSound && !object.contactTask) object.setSound(sound);
    }
}
bool sceneHasTransparency(ssgEntity* entity) {
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch = static_cast<ssgBranch*>(entity);
        for (int i = 0; i < branch->getNumKids(); ++i)
            if (sceneHasTransparency(branch->getKid(i))) return true;
    } else if ((entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        auto* leaf = static_cast<ssgLeaf*>(entity);
        for (int i = 0; i < leaf->getNumColours(); ++i) {
            std::uint32_t alphaBits;
            std::memcpy(&alphaBits, &leaf->getColour(i)[3], sizeof(alphaBits));
            if (alphaBits != 0x3f800000u) return true;
        }
        ssgState* state = leaf->getState();
        if (state && (state->getType() & SSG_TYPE_SIMPLESTATE) == SSG_TYPE_SIMPLESTATE && state->isTranslucent() > 0)
            return true;
    }
    return false;
}
void updateSceneTransparency(ssgEntity* entity, float alpha) {
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch = static_cast<ssgBranch*>(entity);
        for (int i = 0; i < branch->getNumKids(); ++i) updateSceneTransparency(branch->getKid(i), alpha);
    } else if ((entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        auto* leaf = static_cast<ssgLeaf*>(entity);
        ssgState* state = leaf->getState();
        if (state && (state->getType() & SSG_TYPE_SIMPLESTATE) == SSG_TYPE_SIMPLESTATE) {
            auto* simple = static_cast<ssgSimpleState*>(state);
            // Original token 0x2e maps to the cached blend-enable bit 3.
            if (alpha >= 1.0f) { simple->disable(GL_BLEND); simple->setOpaque(); }
            else { simple->enable(GL_BLEND); simple->setTranslucent(); }
            for (int i = 0; i < leaf->getNumColours(); ++i) leaf->getColour(i)[3] = alpha;
#ifdef _SSG_USE_DLIST
            leaf->deleteDList();
#endif
        }
    }
}
void SceneObjectRecord::detectTransparency() {
    if (sceneHasTransparency(entity)) flags |= 0x100;
}
void SceneObjectRecord::setAlpha(float transparency) {
    if (transparency != transparencyAmount) {
        float alpha = 1.0f - transparency;
        transparencyAmount = transparency;
        if (alpha >= 1.0f) updateSceneTransparency(entity, 1.0f);
        else if (alpha <= 0.0f) updateSceneTransparency(entity, 0.0f);
        else updateSceneTransparency(entity, alpha);
    }
}
void SceneObjectRecord::hide() { flags |= 0x10; entity->setTraversalMask(entity->getTraversalMask() & ~1); }
void SceneObjectRecord::show() { flags &= ~0x10u; entity->setTraversalMask(entity->getTraversalMask() | 1); }
namespace {
void extendRecoveredBox(sgBox& box, const sgVec3 point) {
    const float* lo=box.getMin(); const float* hi=box.getMax();
    if (lo[0] <= hi[0] && lo[1] <= hi[1] && lo[2] <= hi[2]) box.extend(point);
    else { box.setMin(point); box.setMax(point); }
}
}
sgBox* SceneObjectRecord::updateWorldBounds() {
    worldBounds.empty();
    const float* low = localBounds.getMin(); const float* high = localBounds.getMax();
    if (low[0] <= high[0] && low[1] <= high[1] && low[2] <= high[2]) {
        const float* m=getBaseFrameTransform();
        sgVec3 a, b;
        for (int i=0;i<3;++i) {
            a[i]=low[0]*m[i]+low[1]*m[4+i]+low[2]*m[8+i]+m[12+i];
            b[i]=high[0]*m[i]+high[1]*m[4+i]+high[2]*m[8+i]+m[12+i];
        }
        extendRecoveredBox(worldBounds,a); extendRecoveredBox(worldBounds,b);
    }
    return &worldBounds;
}
}

#include "ssgGameAnimation.h"
#include <cmath>
namespace megusta {
ssgLeaf* findFirstLeaf(ssgEntity* entity) {
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch = static_cast<ssgBranch*>(entity);
        for (int i=0; i<branch->getNumKids(); ++i)
            if (ssgLeaf* leaf=findFirstLeaf(branch->getKid(i))) return leaf;
    } else if ((entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF)
        return static_cast<ssgLeaf*>(entity);
    return nullptr;
}
const float* SceneObjectRecord::getDirection() {
    // 0053d158 is shared scratch and overwritten on every call.
    static float direction[3];
    direction[0]=0.3f; direction[1]=-0.1f; direction[2]=1.0f;
    ssgLeaf* leaf=findFirstLeaf(entity);
    if (leaf && leaf->getNumVertices()>1) {
        const float* first=leaf->getVertex(0);
        const float* second=leaf->getVertex(1);
        const float* m=getFrameTransform();
        float p0[3], p1[3];
        for (int i=0;i<3;++i) {
            p0[i]=first[0]*m[i]+first[1]*m[4+i]+first[2]*m[8+i]+m[12+i];
            p1[i]=second[0]*m[i]+second[1]*m[4+i]+second[2]*m[8+i]+m[12+i];
        }
        for (int i=0;i<3;++i) direction[i]=p1[i]-p0[i];
    }
    return direction;
}
float SceneObjectRecord::getBallDistance(const float* ballPosition) {
    const float* p=getPosition();
    const float x=p[0]-ballPosition[0], y=p[1]-ballPosition[1], z=p[2]-ballPosition[2];
    return static_cast<float>(std::sqrt(static_cast<double>(x*x+z*z+y*y)));
}
void SceneObjectRecord::selectKey(SceneObjectRecord& target) {
    sgMakeIdentMat4(target.frameTransform);
    accumulateAncestorTransforms(entity, target.entity, currentFrame, target.frameTransform);
    sgCopyMat4(target.baseFrameTransform, target.frameTransform);
    accumulateEntityTransform(target.entity, currentFrame, target.frameTransform);
}
void SceneObjectRecord::setFrame(std::uint32_t frame, const SceneObjectAnimationServices& services) {
    const std::uint32_t count=frameCount;
    if (!count) currentFrame=0;
    else {
        std::uint32_t last=count-1;
        currentFrame=static_cast<std::int32_t>(frame)<static_cast<std::int32_t>(last)
            ? (static_cast<std::int32_t>(frame)<1 ? 0 : frame) : last;
    }
    if (currentFrame && !animator && count) {
        animator=new ssgAnim(entity, count);
        animator->ref();
        if (ssgAnim::isSingleSharedChild(entity)) {
            ssgEntity* child=static_cast<ssgBranch*>(entity)->getKid(0);
            for (int i=0; i<child->getNumParents(); ++i) {
                ssgEntity* parent=child->getParent(i);
                if (parent != entity && parent->getName()) {
                    SceneObjectRecord* other=services.findByName(services.context,parent->getName());
                    if (other->entity==parent && !other->animator) {
                        other->animator=new ssgAnim(other->entity,other->frameCount);
                        other->animator->ref();
                    }
                }
            }
        }
    }
    if (animator) animator->setFrame(currentFrame);
    if (!(flags & 4)) transformDirty=true;
    if (!(flags & 1)) {
        services.forgetContactObject(services.context,objectIndex);
        if (!(flags & 4)) services.triggerContactTask(services.context,objectIndex);
    }
}
void SceneObjectRecord::move(const SceneObjectAnimationServices& services) {
    ++currentFrame;
    if (frameCount <= currentFrame) currentFrame=0;
    setFrame(currentFrame,services);
}
}
namespace megusta {
bool containsSceneEntity(ssgEntity* entity, ssgEntity* target) {
    if (entity == target) return true;
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0; i<branch->getNumKids(); ++i)
            if (containsSceneEntity(branch->getKid(i),target)) return true;
    }
    return false;
}
void SceneObjectCollection::propagateCloak(ssgEntity* entity, bool inherited) {
    if (entity->getName()) {
        for (int i=1; i<=objectCount; ++i) {
            SceneObjectRecord& object=objects[i];
            if (object.entity == entity) {
                if (!inherited) inherited=(object.flags & 0x200u) != 0;
                if (inherited) object.flags |= 1; else object.flags &= ~1u;
                break;
            }
        }
    }
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0; i<branch->getNumKids(); ++i) propagateCloak(branch->getKid(i), inherited);
    }
}
void SceneObjectRecord::cloak(SceneObjectCollection& objects, const SceneObjectAnimationServices& services) {
    flags |= 0x200;
    objects.propagateCloak(entity, false);
    services.forgetContactObject(services.context, objectIndex);
}
void SceneObjectRecord::uncloak(SceneObjectCollection& objects, const SceneObjectAnimationServices& services) {
    flags &= ~0x200u;
    objects.propagateCloak(entity, false);
    services.forgetContactObject(services.context, objectIndex);
}
SceneObjectRecord* SceneObjectCollection::findWithinRoot(ssgEntity* root, const char* name) {
    for (int i=1; i<=objectCount; ++i) {
        SceneObjectRecord& object=objects[i];
        if (object.name && strings::compare(name,object.name)==0 && containsSceneEntity(root,object.entity))
            return &object;
    }
    return sentinelObject;
}
}

#include "scene_object_literals.h"
#include "../engine/resource_runtime.h"
namespace megusta {
void extendSceneBoundsFromVertices(ssgEntity* entity, sgBox& bounds) {
    while ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(entity);
        if ((entity->getType() & SSG_TYPE_SELECTOR) || (entity->getType() & 0x1000000)) {
            entity=branch->getKid(0);
        } else {
            for (int i=0; i<branch->getNumKids(); ++i) extendSceneBoundsFromVertices(branch->getKid(i), bounds);
            return;
        }
    }
    if ((entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        auto* leaf=static_cast<ssgLeaf*>(entity);
        for (int i=0; i<leaf->getNumVertices(); ++i) extendRecoveredBox(bounds, leaf->getVertex(i));
    }
}
void setSceneRenderOrderMode(ssgEntity* entity, int mode) {
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0; i<branch->getNumKids(); ++i) setSceneRenderOrderMode(branch->getKid(i),mode);
    } else if ((entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        static_cast<ssgLeaf*>(entity)->setRenderOrderMode(mode);
    }
}
void SceneObjectRecord::initializeFromEntity(int index, ssgEntity* source, const SceneObjectAnimationServices& services) {
    objectIndex=index; transparencyAmount=0.0f; transparencyAux=0; flags=0;
    entity=source; name=source->getName(); coefficientOfRestitution=0.5f; coefficientOfFriction=0.02f;
    frameCount=0; localBounds.empty(); localCenter[0]=localCenter[1]=localCenter[2]=0.0f;
    detectTransparency();
    if (name && strings::compare("miniHole",name) && strings::compare("mainHole",name)) {
        flags |= 4;
        extendSceneBoundsFromVertices(entity,localBounds);
        const float* low=localBounds.getMin(); const float* high=localBounds.getMax();
        if (low[0]<=high[0] && low[1]<=high[1] && low[2]<=high[2])
            for (int i=0;i<3;++i) localCenter[i]=(low[i]+high[i])*0.5f;
        frameCount=ssgAnim::countFrames(entity);
        if (!strings::compareBounded(name,scene_object_literals::hiddenPrefixHp,3) ||
            !strings::compareBounded(name,scene_object_literals::hiddenPrefixCo,3)) hide();
        if (strings::findSubstring(name,scene_object_literals::billLower) ||
            strings::findSubstring(name,scene_object_literals::billUpper)) {
            sharedIo->stdioPrintf(scene_object_literals::billboardLog,name);
            setSceneRenderOrderMode(entity,2);
        }
    }
    sgMakeIdentMat4(frameTransform);
    transformDirty=true;
    setFrame(0,services);
    refreshFrameTransform();
}
namespace {
int compareNamedObject(const void* first, const void* second) {
    const void* leftRecord;
    const void* rightRecord;
    const char* leftName;
    const char* rightName;
    std::memcpy(&leftRecord, first, sizeof(leftRecord));
    std::memcpy(&rightRecord, second, sizeof(rightRecord));
    std::memcpy(&leftName, leftRecord, sizeof(leftName));
    std::memcpy(&rightName, rightRecord, sizeof(rightName));
    return strings::compare(leftName,rightName);
}
}
SceneObjectRecord* SceneObjectCollection::findByName(const char* name) const {
    const char* key=name;
    const char** keyPointer=&key;
    auto* found=static_cast<SceneObjectRecord* const*>(std::bsearch(&keyPointer,sortedNamedObjects,
        static_cast<std::size_t>(namedObjectCount),sizeof(*sortedNamedObjects),compareNamedObject));
    return found && *found ? *found : sentinelObject;
}
}
namespace megusta {
void SceneObjectCollection::resetAnimationFrames(const SceneObjectAnimationServices& services) {
    for (int i=1; i<=objectCount; ++i)
        if (objects[i].frameCount) objects[i].setFrame(0,services);
}
void SceneObjectCollection::countSceneRecords(ssgEntity* entity, bool renderOnly) {
    const char* name=entity->getName();
    if (name) {
        ++objectCount;
        if (!renderOnly) renderOnly = strings::compareBounded(name,scene_object_literals::renderOnlyPrefix,3)==0;
    }
    if ((entity->getType() & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(entity);
        for (int i=0; i<branch->getNumKids(); ++i) countSceneRecords(branch->getKid(i),renderOnly);
    } else if (!renderOnly && (entity->getType() & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        triangleCount += static_cast<ssgLeaf*>(entity)->getNumTriangles();
    }
}
void SceneObjectCollection::collectSceneRecords(ssgEntity* source, int ownerIndex, std::uint32_t frames,
                                                 bool renderOnly, const SceneObjectAnimationServices& services) {
    for (int i=1; i<objectCount; ++i) if (objects[i].entity==source) return;
    SceneObjectRecord* current=nullptr;
    if (source->getName()) {
        int index=objectCount++;
        current=&objects[index];
        current->initializeFromEntity(index,source,services);
        current->triangles=triangleStorage+triangleCount;
        ownerIndex=index;
        if (!renderOnly) renderOnly = strings::compareBounded(current->name,scene_object_literals::renderOnlyPrefix,3)==0;
        if (renderOnly) current->flags |= 0x201;
    }
    const std::uint32_t inheritedFrames=frames;
    const int type=source->getType();
    if ((type & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        if (type & SSG_TYPE_SELECTOR) frames=static_cast<ssgBranch*>(source)->getNumKids();
        else if (type & 0x1000000) frames=static_cast<std::uint32_t>(static_cast<asx::GameAnimatedTransform*>(source)->frameCount());
    } else if ((type & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        ssgState* state=static_cast<ssgLeaf*>(source)->getState();
        if (state && (state->getType() & SSG_TYPE_STATESELECTOR))
            frames=static_cast<ssgStateSelector*>(state)->getNumSteps();
    }
    if (!inheritedFrames) {
        if (frames && ownerIndex) objects[ownerIndex].flags=(objects[ownerIndex].flags & ~4u) | 8u;
    } else if (frames < inheritedFrames) frames=inheritedFrames;
    if ((type & SSG_TYPE_BRANCH) == SSG_TYPE_BRANCH) {
        auto* branch=static_cast<ssgBranch*>(source);
        for (int i=0; i<branch->getNumKids(); ++i) collectSceneRecords(branch->getKid(i),ownerIndex,frames,renderOnly,services);
    } else if ((type & SSG_TYPE_LEAF) == SSG_TYPE_LEAF) {
        auto* leaf=static_cast<ssgLeaf*>(source);
        if (!renderOnly) {
            const int count=leaf->getNumTriangles();
            const SceneTriangleServices bridge=triangleServices();
            for (int i=0; i<count; ++i) triangleStorage[triangleCount++].initialize(leaf,static_cast<std::uint16_t>(i),static_cast<std::uint16_t>(ownerIndex),bridge);
        } else if (source->getNumParents()==1 && leaf->getDListIndex()!=0) {
            ssgBranch* parent=source->getParent(0);
            parent->replaceKid(source,new ssgCompiledLeaf(leaf));
        }
    }
    if (current) {
        current->triangleCount=static_cast<int>(triangleStorage+triangleCount-current->triangles);
        if (!renderOnly && !current->triangleCount) current->hide();
        if (frames || (current->flags & 1)) current->flags &= ~4u;
    }
}
void SceneObjectCollection::buildFromScene(ssgEntity* root, const SceneObjectAnimationServices& services) {
    sceneRoot=root; objectCount=1; triangleCount=0;
    countSceneRecords(root,false);
    const int capacity=objectCount+1;
    const int triangleCapacity=triangleCount;
    objects=new SceneObjectRecord[capacity]();
    triangleStorage=new SceneTriangleRecord[triangleCapacity];
    // The original clears every object record after its allocation constructors.
    for (int i=0;i<capacity;++i) {
        objects[i].localBounds.setMin(0.0f,0.0f,0.0f); objects[i].localBounds.setMax(0.0f,0.0f,0.0f);
        objects[i].worldBounds.setMin(0.0f,0.0f,0.0f); objects[i].worldBounds.setMax(0.0f,0.0f,0.0f);
    }
    sentinelObject=&objects[capacity-1];
    objects[0].initializeFromEntity(0,root,services);
    objectCount=1; triangleCount=0;
    collectSceneRecords(root,0,0,false,services);
    namedObjectCount=0;
    --objectCount;
    for (int i=1;i<=objectCount;++i) if (objects[i].name) ++namedObjectCount;
    sortedNamedObjects=new SceneObjectRecord*[namedObjectCount];
    namedObjectCount=0;
    for (int i=1;i<=objectCount;++i) if (objects[i].name) sortedNamedObjects[namedObjectCount++]=&objects[i];
    std::qsort(sortedNamedObjects,static_cast<std::size_t>(namedObjectCount),sizeof(*sortedNamedObjects),compareNamedObject);
    sharedIo->standardOutput->printf(scene_object_literals::collectionLog,objectCount,triangleCount);
}
void SceneObjectCollection::releaseSceneRecords() {
    sceneRoot=nullptr;
    for (int i=1;i<=objectCount;++i) {
        if (objects[i].animator) { ssgDeRefDelete(objects[i].animator); objects[i].animator=nullptr; }
        objects[i].setSound(nullptr);
    }
    if (objects) {
        delete[] objects; objects=nullptr; objectCount=0;
        delete[] sortedNamedObjects; sortedNamedObjects=nullptr; namedObjectCount=0; sentinelObject=nullptr;
    }
    if (triangleStorage) { delete[] triangleStorage; triangleStorage=nullptr; triangleCount=0; }
}
}
