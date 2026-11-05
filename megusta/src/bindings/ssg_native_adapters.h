#pragma once
#include <cstdint>
class ssgEntity;
class ssgBaseTransform;
class ssgAnim;

namespace megusta::bindings::native {
int ssgEntity_getNumKids_binding_adapter(ssgEntity* entity);
ssgEntity* ssgEntity_getKid_binding_adapter(ssgEntity* entity,std::uint32_t index);
void ssgEntity_addKid_binding_adapter(ssgEntity* entity,ssgEntity* child);
void ssgEntity_removeKid_binding_adapter(ssgEntity* entity,ssgEntity* child);
void ssgEntity_removeAllKids_binding_adapter(ssgEntity* entity);
void ssgTransform_set_binding_adapter(ssgBaseTransform* transform,float x,float y,float z,float angle);
void ssgTransform_fastSet_binding_adapter(ssgBaseTransform* transform,float x,float y,float z);
float* ssgTransform_getPos_binding_adapter(ssgBaseTransform* transform);
void ssgAnim_setFrame_impl(ssgAnim* animation,std::uint32_t frame);
}
