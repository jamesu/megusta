#include "../src/dts/shape_instance.h"
#include "../src/dts/shape.h"
#include "../src/dts/thread.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSShape noSequenceShape;
    TSShapeInstance noSequenceInstance(&noSequenceShape,0);
    assert(noSequenceInstance.addThread()==nullptr);

    TSShape billboardShape; billboardShape.details.resize(1);
    billboardShape.details[0].subShape=-2;
    TSShapeInstance billboardInstance(&billboardShape,0);
    billboardInstance.animate(0);

    TSShape shape; shape.sequences.resize(2); auto& sequence=shape.sequences[0];
    shape.nodes.resize(1); shape.nodes[0].parentIndex=-1;
    shape.firstNodes={0}; shape.numNodes={1}; shape.defaultRotations.push_back({0,0,0,32767});
    shape.objects.resize(1); shape.objects[0].nodeIndex=0; shape.objects[0].numMeshes=0; shape.objects[0].startMeshIndex=0;
    shape.firstObjects={0}; shape.numObjects={1}; shape.firstDecals={0}; shape.numDecals={0};
    shape.objectStates={{1,0,0},{0,10,20},{1,11,21},{1,12,22},{1,13,23}};
    shape.iflMaterials.resize(1); shape.iflMaterials[0].numFrames=3; shape.iflMaterials[0].firstFrameOffTimeIndex=0;
    shape.iflFrameOffTimes={0.2f,0.4f,0.6f};
    shape.details.resize(1); shape.details[0].subShape=0;
    shape.defaultTranslations.push_back({0,0,0});
    shape.nodeTranslations={{0,0,0},{2,0,0},{4,0,0},{6,0,0}};
    sequence.numKeyframes=4; sequence.duration=2; sequence.flags=0x10; sequence.dirtyFlags=0x0f;
    sequence.translationMatters.bits[0]=1;
    sequence.baseObjectState=1; sequence.visMatters.bits[0]=sequence.frameMatters.bits[0]=sequence.matFrameMatters.bits[0]=1;
    sequence.iflMatters.bits[0]=1;
    TSShapeInstance instance(&shape,1); auto* thread=instance.addThread(); assert(thread);
    thread->advanceTime(0.5f); assert(thread->pos==0.25f && thread->keyNum1==1 && thread->keyNum2==2);
    instance.animate(0); assert(instance.nodeTransforms[0].values[3]==2.0f && !(instance.dirtyFlags[0]&1));
    assert(instance.meshObjects[0].visible==1 && instance.meshObjects[0].frame==11 && instance.meshObjects[0].materialFrame==21);
    assert(instance.iflMaterialInstances[0].frame==2 && !(instance.dirtyFlags[0]&0x20));
    thread->advancePos(-0.5f); assert(thread->pos==0.75f && thread->keyNum1==3 && thread->keyNum2==0);
    thread->setPos(0.5f); assert(thread->getTime()==1.0f);
    assert(TSShapeInstance_getTime(thread)==1.0L);
    assert(TSShapeInstance_getDuration(thread)==2.0L);
    TSShapeInstance_setPos(thread,0.25f); assert(thread->pos==0.25f);
    assert(TSShapeInstance_isInTransition(thread)==0);
    instance.setNodeAnimationState(0,0x20u); assert(instance.getNodeAnimationState(0)==0x30u);
    instance.setNodeAnimationState(0,0); assert(instance.getNodeAnimationState(0)==0);
    shape.sequences[1].priority=3; shape.sequences[0].priority=1;
    TSThread higher(&instance); higher.setSequence(1,0.0f);
    assert(DTS_compareThreads(&higher,thread)==-1);
    shape.sequences[1].flags=8u;
    assert(DTS_compareThreads(thread,&higher)==-1);
    sequence.flags=0x30; sequence.firstTrigger=0; sequence.numTriggers=1;
    shape.triggers.push_back({0x80000001u,0.5f});
    thread->setSequence(0,0.25f); thread->advancePos(0.5f);
    assert(instance.triggerStates==1);
    sequence.flags=0; thread->setSequence(0,0.5f);
    instance.transitionToSequence(thread,0,0.5f,1.0f,false);
    assert(thread->isInTransition() && thread->getPos()==0.0f && thread->getDuration()==1.0f);
    thread->advancePos(0.25f);
    assert(thread->isInTransition() && thread->transitionPosition==0.25f && thread->pos==0.5f);
    thread->advancePos(0.75f);
    assert(!thread->isInTransition() && thread->pos==0.5f && instance.transitionThreads.empty());
    instance.destroyThread(thread); assert(instance.threads.empty());
}
