#include "ssg_native_adapters.h"
#include "ssg.h"
#include "ssgGameAnimation.h"

namespace megusta::bindings::native {
namespace {
ssgBranch* asBranch(ssgEntity* entity) {
    return entity->isAKindOf(ssgTypeBranch()) ? static_cast<ssgBranch*>(entity) : nullptr;
}
}

int ssgEntity_getNumKids_binding_adapter(ssgEntity* entity) {
    auto* branch=asBranch(entity);
    return branch ? branch->getNumKids() : 0;
}
ssgEntity* ssgEntity_getKid_binding_adapter(ssgEntity* entity,std::uint32_t index) {
    auto* branch=asBranch(entity);
    return branch ? branch->getKid(static_cast<int>(index)) : nullptr;
}
void ssgEntity_addKid_binding_adapter(ssgEntity* entity,ssgEntity* child) {
    if(auto* branch=asBranch(entity)) branch->addKid(child);
}
void ssgEntity_removeKid_binding_adapter(ssgEntity* entity,ssgEntity* child) {
    if(auto* branch=asBranch(entity)) branch->removeKid(child);
}
void ssgEntity_removeAllKids_binding_adapter(ssgEntity* entity) {
    if(auto* branch=asBranch(entity)) branch->removeAllKids();
}

void ssgTransform_set_binding_adapter(ssgBaseTransform* transform,
        float x,float y,float z,float angle) {
    sgVec3 axis={0.0f,0.0f,1.0f};
    sgMat4 matrix,translation;
    sgMakeRotMat4(matrix,angle,axis);
    sgMakeTransMat4(translation,x,y,z);
    sgPostMultMat4(matrix,translation);
    transform->setTransform(matrix);
}
void ssgTransform_fastSet_binding_adapter(ssgBaseTransform* transform,float x,float y,float z) {
    sgMat4 matrix;
    sgMakeIdentMat4(matrix);
    matrix[3][0]=x; matrix[3][1]=y; matrix[3][2]=z;
    transform->setTransform(matrix);
}
float* ssgTransform_getPos_binding_adapter(ssgBaseTransform* transform) {
    static float position[3];
    sgMat4 matrix;
    transform->getTransform(matrix);
    position[0]=matrix[3][0]; position[1]=matrix[3][1]; position[2]=matrix[3][2];
    return position;
}
void ssgAnim_setFrame_impl(ssgAnim* animation,std::uint32_t frame) {
    animation->setFrame(frame);
}
} // namespace megusta::bindings::native
