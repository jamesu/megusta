#pragma once
#include <cstdint>
#include <vector>
#include "vector_storage.h"
namespace megusta { class OpenGLAdapter; namespace dts {
class TSShapeAlloc;
class TSMaterialList;
struct DTSAssembledMesh;
struct DTSMatrix4;
struct DTSMaterialTextureHandle;
// Serialized/assembled-shape discriminator. These are distinct from the
// high-bit runtime state flags stored in TSMesh::meshType.
enum DTSMeshKind : int {
    DTSMeshStandard = 0,
    DTSMeshSkin = 1,
    DTSMeshDecal = 2,
    DTSMeshSorted = 3,
    DTSMeshNull = 4,
};
// TSDrawPrimitive::material is a packed word: the material identity/marker
// occupies the low 30 bits and the encoded draw selector occupies the top 2.
// Only list and strip have confirmed topology meanings in the recovered code;
// the other top-bit patterns are preserved but left unnamed semantically.
enum class TSDrawPrimitiveType : std::uint32_t {
    TriangleList = 0x00000000u,
    TriangleStrip = 0x40000000u,
    UnresolvedMode2 = 0x80000000u,
    UnresolvedMode3 = 0xc0000000u,
};
enum class TSDrawPrimitiveFlag : std::uint32_t {
    DynamicMaterial = 0x10000000u,
    UnresolvedBit29 = 0x20000000u,
};
constexpr std::uint32_t drawPrimitiveTypeMask = 0xc0000000u;
constexpr std::uint32_t drawPrimitiveMaterialIndexMask = 0x0fffffffu;
constexpr std::uint32_t drawPrimitiveFlagMask = 0x30000000u;
constexpr std::uint32_t drawPrimitiveMaterialCacheMask = 0x1fffffffu;
constexpr std::uint32_t drawPrimitiveMaterialPayloadMask = 0x3fffffffu;
constexpr std::uint32_t drawPrimitiveFlag(TSDrawPrimitiveFlag flag) {
    return static_cast<std::uint32_t>(flag);
}
constexpr std::uint32_t drawPrimitiveTypeBits(TSDrawPrimitiveType type) {
    return static_cast<std::uint32_t>(type);
}
constexpr std::uint32_t operator|(TSDrawPrimitiveFlag left,TSDrawPrimitiveFlag right) {
    return drawPrimitiveFlag(left)|drawPrimitiveFlag(right);
}
constexpr std::uint32_t makeDrawPrimitiveMaterial(std::uint32_t materialIndex,
        TSDrawPrimitiveType type,std::uint32_t flags=0) {
    return (materialIndex&drawPrimitiveMaterialIndexMask) |
        (flags&drawPrimitiveFlagMask) | drawPrimitiveTypeBits(type);
}
constexpr bool hasDrawPrimitiveFlag(std::uint32_t packed,TSDrawPrimitiveFlag flag) {
    return (packed&drawPrimitiveFlag(flag))!=0;
}
constexpr TSDrawPrimitiveType drawPrimitiveType(std::uint32_t packed) {
    return static_cast<TSDrawPrimitiveType>(packed&drawPrimitiveTypeMask);
}
// Matches the saved signed SAR of the packed word by 30, including negative
// selectors for the two unresolved high-bit patterns.
constexpr int drawPrimitiveModeIndex(std::uint32_t packed) {
    return static_cast<std::int32_t>(packed)>>30;
}
constexpr std::uint32_t drawPrimitiveMaterialIndex(std::uint32_t packed) {
    return packed&drawPrimitiveMaterialIndexMask;
}
constexpr std::uint32_t clearDrawPrimitiveType(std::uint32_t packed) {
    return packed&~drawPrimitiveTypeMask;
}
constexpr std::uint32_t clearTriangleStripTypeBit(std::uint32_t packed) {
    return packed&~drawPrimitiveTypeBits(TSDrawPrimitiveType::TriangleStrip);
}
struct TSMeshPoint { float x,y,z; };
struct TSMeshUV { float u,v; };
struct TSDrawPrimitive { std::uint16_t start,count; std::uint32_t material; };
// 004c4ed0: caller-owned split source arrays and optional output buffers.
// Null output buffers perform the sizing pass.
void convertToTris(const std::uint16_t (*startsAndCounts)[2],const std::uint32_t* materials,
                   const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
                   int* outputIndexCount,TSDrawPrimitive* outputPrimitives,std::uint16_t* outputIndices);
// 004c5530 reads the original process-wide short-strip threshold.
extern int minimumStripTriangles;
// 004c6110 reads these recovered process globals while grouping influences.
extern int dtsMaximumSkinInfluences;
extern float dtsMinimumSkinInfluenceWeight;
// 004c5c90 reads these process-wide assembly mode switches.
extern bool dtsConvertTriangles;
extern bool dtsUseEncodedNormals;
void leaveAsMultipleStrips(const std::uint16_t (*startsAndCounts)[2],const std::uint32_t* materials,
                           const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
                           int* outputIndexCount,TSDrawPrimitive* outputPrimitives,std::uint16_t* outputIndices);
void convertToSingleStrip(const std::uint16_t (*startsAndCounts)[2],const std::uint32_t* materials,
                          const std::uint16_t* indices,int primitiveCount,int* outputPrimitiveCount,
                          std::uint32_t* outputIndexCount,TSDrawPrimitive* outputPrimitives,std::uint16_t* outputIndices);
// DTS_Vector layout used by the strip unwinder: count, capacity, data.
// This differs from the generic normalized DTSVectorStorage field order.
struct DTSUnwindStripVector {
    std::uint32_t count=0;
    std::uint32_t capacity=0;
    std::uint16_t* data=nullptr;
    void push_back(std::uint16_t value);
};
void unwindTriangleStrip(const std::uint16_t* indices,int count,DTSUnwindStripVector& output);
void computeVertexBounds(const TSMeshPoint* vertices,int count,const float* transform,float* bounds,float* center,float* radius);
// 004c2220 writes two clamped shadow coordinates per input vertex into the
// process-wide buffer sized by the shadow renderer.
extern std::vector<std::int32_t> shadowCoordinates;
void shadowTransform(const float* matrix,const TSMeshPoint* begin,const TSMeshPoint* end,int maximum);
void* sharedMeshData32(int parent,int count,void** parentData,char skip);
void* sharedMeshData8(int parent,int count,void** parentData,char skip);
int reduceSkinInfluences(const std::int32_t* vertexIndices,const std::int32_t* boneIndices,
                         const float* weights,std::int32_t* outputVertices,
                         std::int32_t* outputBones,float* outputWeights,int influenceCount);
void getSharedSkinInfluences(int initialVertexCount,int parentMesh,int* influenceCount,
        const std::int32_t* const* vertexData,const std::int32_t* const* boneData,
        const float* const* weightData,std::int32_t** outputVertices,
        std::int32_t** outputBones,float** outputWeights,char skip);
// The encoded-normal table is populated by the original startup routine.
extern TSMeshPoint normalDecodeTable[256];
extern std::vector<TSMeshPoint> decodedNormalScratch;
void initializeNormalDecodeTable();
extern OpenGLAdapter* dtsRenderGL;
extern int dtsPrimitiveModeStorage[6];
extern int* dtsPrimitiveModes;
extern float meshFadeFactor,meshReflectionFactor,meshFogFactor,meshAlphaFactor,meshCurrentAlpha;
extern bool meshFadeActive;
extern int dtsSavedDecalTextureMode;
extern float& dtsRenderAlpha; // 0054ec44, compatibility alias of dtsInitialAlpha
extern float dtsSavedDecalAlpha;
extern std::uint32_t dtsCurrentMaterialFlags,dtsCurrentMaterialIndex;
extern int dtsEnvironmentMode,dtsDetailMode,dtsActiveTextureUnit;
extern int dtsEnvironmentTextureUnit,dtsFogTextureUnit;
extern int& dtsSecondaryTextureUnit; // 0054ec5c, same unit as fog texturing
extern int dtsTextureMode,dtsCurrentDetailIndex;
extern float dtsCurrentIntraDetail,dtsDetailMapAlpha;
extern bool dtsTexturingPassEnabled;
extern bool dtsTranslucentObjectRange,dtsTranslucentObjectsOnly;
extern bool dtsDecalsEnabled;
extern std::uint32_t dtsEnvironmentTextureName;
extern DTSMaterialTextureHandle* dtsCurrentEnvironmentTexture; // fog-combine handle
extern float dtsEnvironmentAlpha,dtsInitialAlpha;
extern float& dtsFogRenderAlpha; // 0054ec6c, fourth component of dtsFogColor
extern float dtsFogColor[4];
extern bool dtsFogColorUsesAlpha;
extern float dtsMeshColor[4];
void setMeshColor(float red,float green,float blue);
void initializeMeshColor();
void rasterizeTriangleStripShadows(const TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
                                   const std::uint16_t* indices,const std::int32_t* coordinates,
                                   int dimension,std::uint32_t* mask);
void rasterizeTriangleListShadows(const TSDrawPrimitive* primitives,int primitiveCount,int primitiveStride,
                                  const std::uint16_t* indices,const std::int32_t* coordinates,
                                  int dimension,std::uint32_t* mask);
extern bool dtsMultitextureSupported,dtsForceMaterialUpdate;
extern bool& dtsFogPreviouslyEnabled; // same saved lighting byte, 0054ec41
extern bool dtsExtensionsDetected,dtsTextureEnvCombineSupported;
extern bool dtsDisplayListsEnabled,dtsMaterialAlphaState,dtsMaterialForceDepthWrite,dtsSavedLightingState;
extern bool& dtsMaterialAlphaToggle; // same fade-active byte, 0054ec40
extern bool dtsSkinDisplayListsEnabled;
extern int dtsSkinMode;
extern const DTSMatrix4* dtsCurrentNodeTransforms;
extern const DTSMatrix4* dtsCurrentModelTransform;
void detectTextureExtensions();
bool hasMultitexture();
bool hasTextureEnvCombine();
class TSMesh {
public:
    // Normalized dispatch identity used by recovered destructor adapters.
    const void* dispatch=nullptr;
    static void* assembleMesh(DTSMeshKind type,unsigned char skip);
    static void setFade(float value);
    static void clearFade();
    static void resetEnvironmentMapMaterials();
    static void initEnvironmentMapMaterials();
    static void resetMaterials();
    int meshType=0,numFrames=0,numMaterialFrames=0,parentMesh=-1,verticesPerFrame=0;
    float bounds[6]={},center[3]={},radius=0;
    std::vector<TSMeshPoint> vertices,normals;
    // The original base pointers are redirected by updateSkin. Keep those
    // non-owning aliases distinct from the storage that owns the input data.
    const TSMeshPoint* activeVertices=nullptr;
    const TSMeshPoint* activeNormals=nullptr;
    int activeVertexCount=0,activeNormalCount=0;
    bool hasActiveStreams=false;
    const TSMeshPoint* vertexData() const { return hasActiveStreams ? activeVertices : vertices.data(); }
    const TSMeshPoint* normalData() const { return hasActiveStreams ? activeNormals : normals.data(); }
    int vertexCount() const { return hasActiveStreams ? activeVertexCount : static_cast<int>(vertices.size()); }
    std::vector<TSMeshUV> texcoords;
    std::vector<std::uint8_t> encodedNormals;
    std::vector<TSDrawPrimitive> primitives;
    std::vector<std::uint16_t> indices,mergeIndices;
    std::uint32_t displayList=0,environmentDisplayList=0;
    int getNumPolys() const;
    std::uint32_t hasDynamicMaterials(TSMaterialList* materials) const;
    const TSMeshPoint* getNormals(int vertexOffset) const;
    void renderFog(int frame,TSMaterialList* materials);
    void renderShadow(int frame,const float* transform,int dimension,std::uint32_t* mask,
                      TSMaterialList* materials);
    void render(int frame,int materialFrame,TSMaterialList* materials);
    void renderUsingBuffers(int frame,int materialFrame,TSMaterialList* materials,
                            const TSMeshPoint* vertexBuffer,const TSMeshPoint* normalBuffer);
    void renderEnvironmentMap(int frame,int materialFrame,TSMaterialList* materials);
    static void initMaterials();
    static void setMaterial(std::uint32_t material,TSMaterialList* materials);
    // Original virtual-style frame bounds entry, including argument order.
    void computeBounds(const float* transform,float* bounds,int frame,float* center,float* radius) const;
    bool computeBoundsFrame(const float* transform,int frame,float* bounds,float* center,float* radius) const;
    void computeBounds();
    // Recovered optional triangle-list conversion; leaves mesh unchanged on error.
    bool convertToTriangles();
    // Preserve long strips and expand strips shorter than minStripTriangles.
    bool leaveAsMultipleStrips(int minStripTriangles);
    bool convertToSingleStrip(int minStripTriangles);
    // Recovered 004c5c90 interface; uses the active shared assembly context.
    void assemble(char skip);
    // 004c5b70 / 004c5c00: original parent scratch-table interfaces.
    void* getSharedData32(int parent,int count,void** parentData,char skip);
    void* getSharedData8(int parent,int count,void** parentData,char skip);
    // Reads the base mesh body; primitive conversion and subtype payloads are separate.
    bool assembleBase(TSShapeAlloc& allocator,std::uint32_t version,bool useEncodedNormals,const std::vector<TSMesh>& previousMeshes);
};
struct TSSortedMeshCluster {
    std::int32_t startPrimitive,endPrimitive;
    float normal[3],k;
    std::int32_t frontCluster,backCluster;
};
class TSSortedMesh : public TSMesh {
public:
    std::vector<TSSortedMeshCluster> clusters;
    std::vector<std::int32_t> startCluster,firstVertices,numVertices,firstTexcoords;
    bool alwaysWriteDepth=false;
    void render(int frame,int materialFrame,TSMaterialList* materials);
    bool assemble(TSShapeAlloc& allocator,std::uint32_t version,bool useEncodedNormals,const std::vector<TSMesh>& previousMeshes);
    void assemble(char skip);
};
struct DTSMatrix4 { float values[16]; };
struct DTSPlane4 { float values[4]; };
struct DTSSkinInfluencePolicy { int maxPerVertex; float minimumWeight; };
class TSSkinMesh : public TSMesh {
public:
    std::vector<TSMeshPoint> initialVertices,initialNormals;
    std::vector<DTSMatrix4> initialTransforms;
    std::vector<std::int32_t> vertexIndices,boneIndices,nodeIndices;
    std::vector<float> weights;
    std::vector<DTSMatrix4> boneTransforms;
    std::vector<TSMeshPoint> skinnedVertices,skinnedNormals;
    bool assemble(TSShapeAlloc& allocator,std::uint32_t version,bool useEncodedNormals,
                  const std::vector<TSMesh>& previousMeshes,const std::vector<TSSkinMesh>& previousSkins);
    void assemble(int skip);
    // The original uses process-wide limits. Callers must supply those values.
    bool reduceInfluences(const DTSSkinInfluencePolicy& policy);
    // Runtime skinning family recovered from 004c2820–004c2d30. Like the
    // original, it reads the active transform table and GL/environment state.
    void updateSkin();
    void skinVerticesAndNormals();
    void skinVerticesOnly();
    void render(int frame,int materialFrame,TSMaterialList* materials);
    void computeBounds(const float* transform,float* bounds,int frame,float* center,float* radius) const;
    void computeBounds();
};
class TSDecalMesh {
public:
    std::uint32_t displayList=0;
    static void initDecalMaterials();
    static void resetDecalMaterials();
    TSMesh* targetMesh=nullptr;
    std::vector<TSDrawPrimitive> primitives;
    std::vector<std::uint16_t> indices;
    std::vector<std::int32_t> startPrimitive;
    std::vector<DTSPlane4> texgenS,texgenT;
    std::int32_t materialIndex=0;
    std::uint32_t meshType=0;
    // Normalized equivalent of the shared getNumPolys vtable operation used
    // by TSShape initialization for decal meshes stored in the mesh slots.
    int getNumPolys() const;
    std::uint32_t hasDynamicMaterials(TSMaterialList* materials) const;
    void render(int frame,int materialFrame,TSMaterialList* materials);
    bool assemble(TSShapeAlloc& allocator,std::uint32_t version);
    void assemble();
};
// Temporary normalized bridge to the original process-wide assembly state.
// It does not reproduce the original pointer-array storage or two-pass allocator.
struct DTSMeshAssemblyContext {
    TSShapeAlloc* allocator;
    std::uint32_t version;
    bool encodedNormals;
    const std::vector<TSMesh>* previousMeshes;
    const std::vector<TSSkinMesh>* previousSkins;
    bool succeeded;
    bool skip;
    const std::uint8_t* retainedMeshes;
    DTSAssembledMesh* currentMesh;
    bool convertTriangles;
    int minimumStripTriangles;
};
extern DTSMeshAssemblyContext* activeMeshAssembly;
static_assert(sizeof(TSSortedMeshCluster)==32,"DTS sorted cluster width");
} }

extern "C" bool FUN_004d48d0(void);
extern "C" bool FUN_004d48f0(void);

extern "C" {
void MEGUSTA_DTS_CDECL DTS_registeredStartup_004c2dd0(void);
void MEGUSTA_DTS_CDECL DTS_registeredStartup_004c1550(void);
void MEGUSTA_DTS_CDECL DTS_staticInitializer_004c1560(void);
void MEGUSTA_DTS_CDECL FUN_004d3970(megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,
    int primitiveStride,const std::uint16_t* indices,const std::int32_t* coordinates,
    int dimension,std::uint32_t* mask);
void MEGUSTA_DTS_CDECL FUN_004d3ee0(megusta::dts::TSDrawPrimitive* primitives,int primitiveCount,
    int primitiveStride,const std::uint16_t* indices,const std::int32_t* coordinates,
    int dimension,std::uint32_t* mask);
}
