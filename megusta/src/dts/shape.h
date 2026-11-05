#pragma once
#include <cstdint>
#include <deque>
#include <vector>
#include <string>
#include <memory>
#include "sequence.h"
#include "legacy_alloc.h"
#include "material_list.h"
#include "mesh.h"
#include "matrix.h"
#include "vector_storage.h"
namespace megusta { class ResourceFile; namespace dts {
class TSShapeAlloc;
class TSShape;
struct EngineBitmapView;
extern ResourceFile* (*iflOpenFile)(const char* path,const char* mode);
extern void (*iflCloseFile)(ResourceFile* file);
extern const char* iflBaseTextures;
extern const char* iflSkinPrefix;
extern float lastDetailFadeOut,billboardFadeIn,billboardFadeOut,normalFadeIn,normalFadeOut;
// Compatibility name for the same signed minimum-detail global used by
// assembly, initialization and instance detail selection (0054eeb4).
extern int& shapeSkipLoadDetails;
// DAT_0052354c: the saved global data byte is initialized to 1 and gates
// TSShape_init after shape reads and sequence imports.
extern bool dtsShapeInitializeAfterRead;
extern int minimumDetailLevel;
struct DTSNode { std::int32_t nameIndex,parentIndex,firstObject=0,firstChild=0,nextSibling=0; std::uint8_t legacyFlag=0; };
struct DTSObject { std::int32_t nameIndex,numMeshes,startMeshIndex,nodeIndex,nextSibling=0,firstDecal=0; };
struct DTSDecal { std::int32_t nameIndex,numMeshes,startMeshIndex,objectIndex,nextSibling=0; };
struct DTSIflMaterial { std::int32_t nameIndex,materialSlot,firstFrame=0,firstFrameOffTimeIndex=0,numFrames=0; };
struct DTSQuatF { float x,y,z,w; float* setMatrix(float* matrix) const; };
struct DTSQuat16 {
    std::int16_t x,y,z,w;
    void getQuatF(float* output) const;
    void set(const float* input);
};
struct DTSPoint3 { float x,y,z; };
struct DTSArbitraryScale { DTSQuatF rotation; DTSPoint3 factors; };
struct DTSBillboardServices {
    bool (*loadBitmap)(const char*,int,bool,EngineBitmapView**,void**,std::uint32_t*)=nullptr;
    void* (*createDynamicEntry)(EngineBitmapView*,bool,std::uint32_t*)=nullptr;
    void (*destroyTextureEntry)(void*)=nullptr;
};
void bindBillboardServices(const DTSBillboardServices& services);

class TSLastDetail {
public:
    ~TSLastDetail();
    std::uint32_t numEquatorSteps=0,numPolarSteps=0;
    float polarAngle=0;
    char includePoles=0;
    int textureSize=0,atlasIndex=0;
    std::string bitmapSource;
    float center[3]={};
    float points[12]={};
    int bitmapIndex=0;
    float rotation=0;
    std::vector<EngineBitmapView*> bitmaps;
    std::vector<void*> textureEntries;
    std::vector<std::uint32_t> textureNames;
    void chooseView(const float* matrix);
    void initializeBillboardMetadata(const TSShape* shape,std::uint32_t equatorSteps,
        std::uint32_t polarSteps,float angle,char poles,int index,int size,const char* source);
    void construct(TSShape* shape,std::uint32_t equatorSteps,std::uint32_t polarSteps,
        float angle,char poles,int index,int size,const char* source);
    void destroy();
    bool loadBitmapViews(const char* source);
    void render(float alpha,bool fog);
    void renderNoFog(float alpha);
    void renderFogCombine(float alpha);
};
void interpolatePoint3F(const float* first,const float* second,float position,float* output);
std::uint32_t bitmapPowerOfTwoDimension(std::uint32_t value);
struct DTSKeyframe { std::int32_t first,second,third; };
// Original reader/importer share this legacy keyframe scratch table.
extern std::vector<DTSKeyframe> legacyKeyframeScratch;
struct DTSNodeState { std::int16_t rotation[4]; float translation[3]; };
struct DTSObjectState { float visibility; std::int32_t frameIndex,materialFrameIndex; };
struct DTSTrigger { std::uint32_t state; float position; };
struct DTSDetail { std::int32_t nameIndex,subShape,objectDetail; float size; float averageError=0,maxError=0; std::int32_t polyCount=0; };
struct DTSLegacyMeshRange {
    DTSMeshKind type;
    std::uint32_t word32Offset,word16Offset,byte8Offset;
};
// Stable native view of the legacy payload streams at the reader/assembler
// boundary. Counts are in allocator elements, matching the recovered
// getDWordCount32/16/8 outputs; normalized header records remain TSShape-owned.
struct DTSLegacyShapeBuffers {
    std::uint32_t* words32=nullptr;
    std::uint16_t* words16=nullptr;
    std::uint8_t* bytes8=nullptr;
    std::uint32_t word32Count=0,word16Count=0,byte8Count=0;
};
struct DTSAssembledMesh {
    DTSMeshKind type=DTSMeshNull;
    TSMesh base;
    TSSkinMesh skin;
    TSSortedMesh sorted;
    TSDecalMesh decal;
};
class TSShape {
public:
    enum class HeaderResult { readError,unsupportedVersion,legacyUnified,splitBuffer };
    int exporterVersion;
    std::int32_t readVersion=-1;
    float radius,tubeRadius,center[3],bounds[6];
    std::vector<DTSNode> nodes;
    std::vector<DTSObject> objects;
    std::vector<DTSDecal> decals;
    std::vector<DTSIflMaterial> iflMaterials;
    std::vector<float> iflFrameOffTimes;
    std::vector<std::int32_t> firstNodes,firstObjects,firstDecals;
    std::vector<std::int32_t> numNodes,numObjects,numDecals;
    std::vector<std::int32_t> firstTranslucentObjects;
    std::vector<DTSQuat16> defaultRotations,nodeRotations;
    std::vector<DTSPoint3> defaultTranslations,nodeTranslations;
    std::vector<float> nodeUniformScales;
    std::vector<DTSPoint3> nodeAlignedScales,nodeArbitraryScaleFactors,groundTranslations;
    std::vector<DTSQuat16> nodeArbitraryScaleRotations,groundRotations;
    float smallestVisibleSize;
    std::int32_t smallestVisibleDetail;
    std::vector<DTSKeyframe> legacyKeyframes;
    std::vector<std::uint32_t> legacyPre16Words;
    std::vector<DTSNodeState> nodeStates;
    std::vector<DTSObjectState> objectStates;
    std::vector<std::int32_t> decalStates;
    std::vector<DTSTrigger> triggers;
    std::vector<DTSDetail> details;
    // One entry per DTS detail index; ordinary mesh details remain null.
    std::vector<std::unique_ptr<TSLastDetail>> billboardDetails;
    std::vector<float> alphaIn,alphaOut;
    std::vector<TSShapeSequence> sequences;
    std::vector<std::int32_t> legacyFirstKeyframes;
    DTSLegacyAlloc meshStreams;
    DTSLegacyShapeBuffers legacyShapeBuffers;
    std::vector<DTSLegacyMeshRange> meshes,skins;
    // The executable owns each name string separately, so pointers returned
    // by getName survive appending another name. deque keeps element
    // references stable while retaining normalized strings.
    std::deque<std::string> names;
    // Indices corresponding to the executable ownedNameStrings pointer table.
    std::vector<std::size_t> ownedNameIndices;
    TSMaterialList materialList;
    bool hasMaterials=false;
    std::vector<std::int32_t> detailFirstSkin,detailNumSkins;
    std::vector<std::uint32_t> splitWords;
    // Backing storage for the reconstructed split-stream assembly passes.
    std::vector<std::uint64_t> assemblyArena;
    std::uint32_t assemblyArenaBytes=0;
    std::uint32_t split16Offset=0,split8Offset=0;
    std::uint32_t assemblyMeshCount=0,assemblySkinCount=0,assemblyNameCount=0;
    std::vector<DTSAssembledMesh> assembledMeshes,assembledSkins;
    std::uint32_t legacyGuardMismatches=0;
    std::uint32_t shapeFlags=0;
    void destroy();
    void setupBillboardDetails(char* bitmapBaseName);
    char* getName(int index);
    int findName(char* name);
    int findNode(int nameIndex);
    int readName(ResourceFile* file,char addIfMissing);
    // Orchestrates legacy/split reading and both assembly passes; the saved
    // DAT_0052354c global controls post-read hierarchy initialization. The
    // in-memory shape remains normalized into C++ storage.
    bool read(ResourceFile& file,const char* legacyMaterialPrefix,bool useEncodedNormals,unsigned skipDetail=0,bool convertTriangles=false,int minStripTriangles=-1);
    // Reads only the first stage of TSShape_read_impl. The caller must still
    // convert or assemble the shape and read its sequences and materials.
    HeaderResult readHeader(ResourceFile& file);
    // Parses the legacy stream through the sequence records only; mesh and
    // material data immediately following it remain unread.
    bool readLegacyPrefix(ResourceFile& file);
    // Continues after readLegacyPrefix and rearranges pre-v17 keyframes.
    bool readLegacyTail(ResourceFile& file,const char* legacyMaterialPrefix,DTSLegacyShapeBuffers* output=nullptr);
    // Imports a DSQ stream using the recovered v0-v22 version branches. The
    // shipped game assets are v16; later-version branches have no asset-level
    // runtime fixture in this repository.
    bool importSequences(ResourceFile& file,bool initializeAfterImport=false);
    // Converts pre-v17 frame-major states to sequence track-major order.
    bool rearrangeLegacyKeyframes();
    // 004c9f50: original sequence, data-buffer and stride interface.
    void rearrangeKeyframeData(TSShapeSequence* sequence,int firstKeyframe,
        unsigned char* rotations,unsigned char* translations,unsigned char* objectStates,unsigned char* decalStates,
        int rotationStride,int translationStride,int objectStride,int decalStride);
    // Builds default, animated and ground pose arrays from legacy node states.
    bool normalizeLegacyPoses();
    // Assembles mesh-only legacy streams produced by readLegacyTail. Primitive
    // conversion, influence reduction and complete shape initialization remain.
    bool assembleLegacyMeshes(bool useEncodedNormals,unsigned skipDetail=0,bool convertTriangles=false,int minStripTriangles=-1);
    // One allocator-state pass over either legacy payload ranges or split
    // streams. The caller performs the executable's sizing pass and final pass.
    bool assembleShapePass(TSShapeAlloc& allocator,bool legacy,bool useEncodedNormals,
        unsigned skipDetail,bool convertTriangles,int minStripTriangles);
    bool convertMeshesToTriangles();
    bool convertMeshesToMultipleStrips(int minStripTriangles);
    bool convertMeshesToSingleStrip(int minStripTriangles);
    // The hierarchy-building portion of TSShape::init; other init work remains.
    bool initializeHierarchy();
    // Calculates sequence scale flags and per-detail polygon counts.
    bool initializeDetailMetadata();
    void initializeDetailAlpha();
    bool initializeMaterialList();
    // Original void entry points; the checked adapter below reports failures.
    void init();
    void initMaterialList();
    bool initializeShape(unsigned skipDetail);
    void readIflMaterials();
    // 004cc380: returns the skip decision; callers retain both cursors.
    bool checkSkip(int meshIndex,int* objectCursor,unsigned* decalCursor,unsigned detailIndex);
    // Recovered mesh skip predicate. Cursors must be retained in mesh order.
    bool checkSkip(int meshIndex,int& objectCursor,int& decalCursor,unsigned detailIndex,bool& skip) const;
    bool applyDetailSkip(unsigned skipDetail);
    // Reads the version-19–22 streams and metadata without assembling a shape.
    bool readSplitData(ResourceFile& file,const char* legacyMaterialPrefix);
    void setSplitAssemblyRead(TSShapeAlloc& allocator,bool sizing);
    // First portion of the common shape assembler, through default/node poses.
    bool readSplitAssemblyCore(TSShapeAlloc& allocator);
    bool readSplitAssemblyTail(TSShapeAlloc& allocator,bool useEncodedNormals,unsigned skipDetail=0,bool convertTriangles=false,int minStripTriangles=-1,bool initialize=true);
};
} }

// 004cd340: __thiscall TSShape::read(TSShape*, ResourceFile*).
extern "C" void FUN_004d0130(float* matrix);
extern "C" long double FUN_004d01f0(const float* matrix);
extern "C" void FUN_004d0450(const float* matrix,const float* point,float* output);
extern "C" float* MEGUSTA_DTS_THISCALL FUN_004d0530(float* matrix,const float* angles);
extern "C" void MEGUSTA_DTS_THISCALL FUN_004d0550(const float* matrix,int column,float* output);
extern "C" void MEGUSTA_DTS_THISCALL FUN_004d0580(const float* matrix,int column,float* output);
extern "C" int FUN_004d71a0(std::uint32_t value);
