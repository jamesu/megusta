#include "../src/dts/shape_instance.h"
#include "../src/dts/shape.h"
#include "../src/dts/thread.h"
#include <cassert>
#include <cmath>
using namespace megusta::dts;
int main() {
    TSShape shape;
    shape.nodes.push_back({0,-1}); shape.firstNodes={0}; shape.numNodes={1};
    shape.firstObjects={0}; shape.numObjects={0};
    shape.firstDecals={0}; shape.numDecals={0};
    shape.details.push_back({0,0,0,0.0f});
    shape.defaultRotations.push_back({0,0,0,32767}); shape.defaultTranslations.push_back({2,0,0});
    shape.nodeTranslations={{10,0,0},{10,0,0}};
    shape.sequences.resize(3);
    shape.sequences[0].numKeyframes=1;
    shape.sequences[1].numKeyframes=2; shape.sequences[1].flags=8u;
    shape.sequences[1].baseTranslation=0; shape.sequences[1].translationMatters.set(0);
    shape.sequences[2].numKeyframes=2;
    shape.sequences[2].baseTranslation=2; shape.sequences[2].translationMatters.set(0);
    shape.nodeTranslations={{10,0,0},{10,0,0},{20,0,0},{20,0,0}};
    TSShapeInstance instance(&shape,0);
    TSThread* base=instance.addThread(); assert(base);
    TSThread* blend=instance.addThread(); assert(blend);
    instance.setSequence(blend,1,0.0f);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-12.0f)<0.0001f);
    instance.setNodeAnimationState(0,0x10u);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-2.0f)<0.0001f);

    // The saved 004d06e0 body finds the first blend thread, then runs its
    // final pass over the entire suffix, gating only on blendDisabled. Keep
    // a normal sequence after that boundary to pin the body-level behavior.
    TSThread* trailing=instance.addThread(); assert(trailing);
    instance.setSequence(trailing,2,0.0f);
    instance.setNodeAnimationState(0,0);
    assert(instance.animateNodeTransforms());
    assert(std::fabs(instance.nodeTransforms[0].values[3]-32.0f)<0.0001f);
}
