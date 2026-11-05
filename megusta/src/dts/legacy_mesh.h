#pragma once
#include <cstdint>
#include "vector_storage.h"
#include "mesh.h"
namespace megusta { class ResourceFile; namespace dts {
class DTSLegacyAlloc;
// Appends a serialized legacy mesh to the three assembly streams.
bool readLegacyMesh(ResourceFile& file,DTSLegacyAlloc& streams,std::uint32_t version,DTSMeshKind meshType);
} }

