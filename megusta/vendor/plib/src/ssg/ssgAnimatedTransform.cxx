// RECONSTRUCTED-EVIDENCE-DERIVED game transform conversion.
// Builder 00463330 (loop 0046384f..00463a98), frame setter 0047dc30.
#include "ssgASX.h"
#include <algorithm>
#include <cstdlib>

namespace megusta::asx {

void GameAnimatedTransform::setFrame(int value) {
    if(frames.empty())std::abort(); // Original dereferences a null frame here.
    current=std::clamp(value,0,static_cast<int>(frames.size())-1);
    // MG-PLIB-045: original0047dc30 updates history but does not dirty the
    // all-frame sphere. Calling the ordinary setter changes that contract.
    updateTransform();
    sg_matrix(transform,frame_at(current));
    firsttime();
}

void GameAnimatedTransform::recalcBSphere() {
    // Original0047de10 unions children once, then transforms that sphere's
    // center through every frame. Its radius is deliberately not scaled.
    sgSphere children;children.empty();bsphere.empty();
    for(ssgEntity* child=getKid(0);child;child=getNextKid())
        children.extend(child->getBSphere());
    if(!children.isEmpty())for(size_t i=0;i<frames.size();++i){
        sgSphere candidate=children;sgMat4 matrix;sg_matrix(matrix,frame_at(i));
        candidate.orthoXform(matrix);bsphere.extend(&candidate);
    }
    bsphere_is_invalid=FALSE;
}

std::vector<HostMatrix> reconstruct_transform_frames(
    const std::vector<std::array<float,10>>& source,const std::array<float,3>& pivot) {
    if (source.empty()) return {};
    std::vector<HostMatrix> frames{identity_matrix()};
    // The executable seeds frame 0 with identity and ignores raw sample 0.
    sgMat4 rotation_accumulator;
    sg_matrix(rotation_accumulator,identity_matrix());
    for (size_t i=1;i<source.size();++i) {
        const auto& f=source[i];
        sgVec3 axis={f[3],f[4],f[5]};sgMat4 rotation,local,translation;
        sgMakeRotMat4(rotation,f[6]*-57.29578f,axis);
        sgPostMultMat4(rotation_accumulator,rotation);
        sgVec3 negative_pivot={float(-pivot[0]),float(-pivot[1]),float(-pivot[2])};
        sgMakeTransMat4(local,negative_pivot);
        sgPostMultMat4(local,rotation_accumulator);
        // Scale columns AFTER translating the pivot; do not replace with a
        // conventional R*S*T(-pivot), whose translation differs under scale.
        for (int c=0;c<3;++c)for (int r=0;r<3;++r)local[c][r]*=f[7+c];
        sgVec3 position={float(pivot[0]+f[0]),float(pivot[1]+f[1]),float(pivot[2]+f[2])};
        sgMakeTransMat4(translation,position);
        sgPostMultMat4(local,translation);
        frames.push_back(host_matrix(local));
    }
    return frames;
}

} // namespace megusta::asx
