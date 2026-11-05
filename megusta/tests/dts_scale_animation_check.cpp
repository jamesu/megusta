#include "../src/dts/shape_instance.h"
#include "../src/dts/shape.h"
#include "../src/dts/thread.h"
#include <cassert>
#include <cmath>
using namespace megusta::dts;
static TSShape makeShape() {
    TSShape shape;
    shape.shapeFlags=1;
    shape.nodes.push_back({0,-1}); shape.firstNodes={0}; shape.numNodes={1};
    shape.firstObjects={0}; shape.numObjects={0};
    shape.firstDecals={0}; shape.numDecals={0};
    shape.details.push_back({0,0,0,0.0f});
    shape.defaultRotations.push_back({0,0,0,32767}); shape.defaultTranslations.push_back({0,0,0});
    // Sequence 0 interpolates key 0 to key 1; sequence 1 has its own 4.0 keys.
    shape.nodeUniformScales={2.0f,4.0f,4.0f,4.0f};
    shape.sequences.resize(2); auto& sequence=shape.sequences[0];
    sequence.numKeyframes=2; sequence.flags=1; sequence.baseScale=0; sequence.scaleMatters.bits[0]=1;
    shape.sequences[1].numKeyframes=2; shape.sequences[1].duration=1.0f;
    shape.sequences[1].flags=1; shape.sequences[1].baseScale=2; shape.sequences[1].scaleMatters.set(0);
    return shape;
}
int main() {
    TSShape shape=makeShape();
    TSShapeInstance instance(&shape,0);
    TSThread* thread=instance.addThread(); assert(thread);
    instance.setSequence(thread,0,0.5f);
    assert(instance.scaleCurrentlyAnimated);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[0]-3.0f)<0.0001f);
    assert(std::fabs(instance.nodeTransforms[0].values[5]-3.0f)<0.0001f);
    assert(std::fabs(instance.nodeTransforms[0].values[10]-3.0f)<0.0001f);
    instance.transitionToSequence(thread,1,0.0f,1.0f,false);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[0]-4.0f)<0.0001f);
    thread->advanceTime(0.5f);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[0]-3.5f)<0.0001f);
    instance.destroyThread(thread);
    assert(!instance.scaleCurrentlyAnimated);
}
