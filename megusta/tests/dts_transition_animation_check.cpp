#include "../src/dts/shape_instance.h"
#include "../src/dts/shape.h"
#include "../src/dts/thread.h"
#include <cassert>
#include <cmath>
using namespace megusta::dts;
static const float* callbackTransform(int,void*,int) {
    static const float matrix[16]={1,0,0,77, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    return matrix;
}
int main() {
    TSShape shape;
    shape.nodes.push_back({0,-1}); shape.nodes.push_back({1,-1});
    shape.firstNodes={0,1}; shape.numNodes={1,1};
    shape.firstObjects={0}; shape.numObjects={0};
    shape.firstDecals={0}; shape.numDecals={0};
    shape.details.push_back({0,0,0,0.0f}); shape.details.push_back({0,1,0,0.0f});
    shape.defaultRotations.push_back({0,0,0,32767}); shape.defaultRotations.push_back({0,0,0,32767});
    shape.defaultTranslations.push_back({2,0,0}); shape.defaultTranslations.push_back({0,0,0});
    shape.nodeTranslations={{20,0,0},{20,0,0}};
    shape.sequences.resize(2);
    shape.sequences[0].numKeyframes=1; shape.sequences[0].duration=1;
    shape.sequences[1].numKeyframes=2; shape.sequences[1].duration=1;
    shape.sequences[1].baseTranslation=0; shape.sequences[1].translationMatters.bits[0]=1;
    TSShapeInstance instance(&shape,0); TSThread* thread=instance.addThread(); assert(thread);
    assert(instance.animateNodeTransforms()); assert(instance.nodeTransforms[0].values[3]==2.0f);
    instance.transitionToSequence(thread,1,0.0f,1.0f,false);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-2.0f)<0.0001f);
    thread->advanceTime(0.5f);
    // The saved transition helper processes earlier translation bits while
    // animating a later subshape; the channel storage is shared between them.
    instance.nodeLocalTranslations[0]={20,0,0};
    instance.handleTransitionNodes(1,2);
    assert(std::fabs(instance.nodeLocalTranslations[0].x-11.0f)<0.0001f);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-11.0f)<0.0001f);
    instance.setNodeAnimationState(0,2u);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-2.0f)<0.0001f);

    // Callback-owned nodes bypass ordinary tracks in the handled-set pass;
    // their complete local transform is installed after scale and transitions.
    TSShape callbackShape;
    callbackShape.nodes.push_back({0,-1}); callbackShape.firstNodes={0}; callbackShape.numNodes={1};
    callbackShape.firstObjects={0}; callbackShape.numObjects={0};
    callbackShape.firstDecals={0}; callbackShape.numDecals={0};
    callbackShape.details.push_back({0,0,0,0.0f});
    callbackShape.defaultRotations.push_back({0,0,0,32767});
    callbackShape.defaultTranslations.push_back({3,0,0});
    callbackShape.nodeTranslations={{30,0,0},{30,0,0}};
    callbackShape.sequences.resize(2);
    callbackShape.sequences[0].numKeyframes=1;
    callbackShape.sequences[1].numKeyframes=2;
    callbackShape.sequences[1].baseTranslation=0;
    callbackShape.sequences[1].translationMatters.set(0);
    TSShapeInstance callbackInstance(&callbackShape,0);
    auto* callbackThread=callbackInstance.addThread(); assert(callbackThread);
    callbackInstance.setSequence(callbackThread,1,0.0f);
    callbackInstance.setNodeTransformCallback(callbackTransform,nullptr);
    callbackInstance.setNodeAnimationState(0,0x40u);
    assert(callbackInstance.animateNodeTransforms());
    assert(callbackInstance.nodeTransforms[0].values[3]==77.0f);
    assert(callbackInstance.nodeTranslationThreads[0]==nullptr);
}
