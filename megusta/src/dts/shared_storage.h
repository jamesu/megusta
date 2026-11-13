// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "vector_storage.h"

namespace megusta { namespace dts {

using SharedStorageFree = void (MEGUSTA_DTS_CDECL *)(void*);
void bindSharedStorageFree(SharedStorageFree freeFunction);
void freeSharedStorageAllocation(void* memory);

// These are the process-wide backing vectors recovered from the startup and
// cleanup routines in the TinyDTS image. Each vector retains the binary's
// {count, pointer, capacity} semantics.
extern DTSVectorStorage sharedMeshVerts;
extern DTSVectorStorage sharedMeshNorms;
extern DTSVectorStorage sharedEncodedNorms;
extern DTSVectorStorage sharedTextureVerts;
extern DTSVectorStorage sharedCopiedMeshData;
extern DTSVectorStorage sharedSkinTransforms;
extern DTSVectorStorage sharedSkinVertexIndices;
extern DTSVectorStorage sharedSkinBoneIndices;
extern DTSVectorStorage sharedSkinWeights;
extern DTSVectorStorage sharedSkinNodeIndices;
extern DTSVectorStorage sharedDecodedNormals;

} }

extern "C" {
#define MEGUSTA_DTS_DECLARE_SHARED_STORAGE(prefix) \
 void prefix##_initialize(void); \
 void prefix##_registerCleanup(void); \
 void prefix##_cleanup(void)

MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSMesh_sharedVerts);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSMesh_sharedNorms);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSMesh_sharedEncodedNorms);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSMesh_sharedTVerts);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSMesh_sharedDataCopied);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSSkinMesh_sharedTransforms);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSSkinMesh_sharedVertexIndices);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSSkinMesh_sharedBoneIndices);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSSkinMesh_sharedWeights);
MEGUSTA_DTS_DECLARE_SHARED_STORAGE(TSSkinMesh_sharedNodeIndices);
void TSMesh_decodedNormals_initialize(void);
void TSMesh_decodedNormals_registerCleanup(void);
void TSMesh_decodedNormals_cleanup(void);
void DTS_registeredStartup_004c0ce0(void);
void DTS_registeredStartup_004c0d50(void);
void DTS_registeredStartup_004c0dc0(void);
void DTS_registeredStartup_004c0e30(void);
void DTS_registeredStartup_004c0ea0(void);
void DTS_registeredStartup_004c0f10(void);
void DTS_registeredStartup_004c0f80(void);
void DTS_registeredStartup_004c0ff0(void);
void DTS_registeredStartup_004c1060(void);
void DTS_registeredStartup_004c10d0(void);
void DTS_registeredStartup_004c1140(void);
}

#undef MEGUSTA_DTS_DECLARE_SHARED_STORAGE
