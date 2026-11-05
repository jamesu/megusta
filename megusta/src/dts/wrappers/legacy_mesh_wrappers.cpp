#include "legacy_mesh_wrappers.h"
using namespace megusta::dts;

extern "C" void MEGUSTA_DTS_CDECL DTSLegacy_readAllocMesh_impl(
        megusta::ResourceFile* file,std::uint32_t** words32,
        std::uint16_t** words16,std::uint8_t** bytes8,int meshType) {
    auto& streams=DTSLegacy_addressAllocator();
    streams.words32.adoptExternal(words32);
    streams.words16.adoptExternal(words16);
    streams.bytes8.adoptExternal(bytes8);
    megusta::dts::readLegacyMesh(*file,streams,megusta::dts::currentShapeReadVersion,
        static_cast<megusta::dts::DTSMeshKind>(meshType));
    streams.words32.publishExternal(words32);
    streams.words16.publishExternal(words16);
    streams.bytes8.publishExternal(bytes8);
}
