#include "sequence_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_THISCALL TSShape_Sequence_read_impl(
        megusta::dts::TSShapeSequence* self,megusta::ResourceFile* file,int readNameIndex) {
    self->read(file,readNameIndex);
}
