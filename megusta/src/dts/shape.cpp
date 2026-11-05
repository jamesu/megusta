#include "shape.h"
#include "shape_instance.h"
#include "../engine/resource_file.h"
#include "../engine/string_services.h"
#include "legacy_mesh.h"
#include "shape_alloc.h"
#include "state_transpose.h"
#include "mesh_lifecycle.h"
#include "bitmap_support.h"
#include "texture_manager.h"
#include "graphics_state.h"
#include "../engine/opengl_adapter.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <utility>
#include <new>
namespace megusta { namespace dts {
namespace { constexpr std::uint32_t maximumDTSReadVersion=22; }
std::vector<DTSKeyframe> legacyKeyframeScratch;
ResourceFile* (*iflOpenFile)(const char*,const char*)=nullptr;
void (*iflCloseFile)(ResourceFile*)=nullptr;
const char* iflBaseTextures="data/textures/";
const char* iflSkinPrefix="skins/";
float lastDetailFadeOut=0.1f,billboardFadeIn=0.15f,billboardFadeOut=0.1f;
float normalFadeIn=-1.0f,normalFadeOut=-1.0f;
int& shapeSkipLoadDetails=minimumDetailLevel;
bool dtsShapeInitializeAfterRead=true;
namespace { DTSBillboardServices billboardServices{}; }
void bindBillboardServices(const DTSBillboardServices& services) { billboardServices=services; }
char* TSShape::getName(int index) {
    auto& name=names[index]; return name.empty() ? const_cast<char*>(name.c_str()) : &name[0];
}
int TSShape::findName(char* name) {
    for (std::size_t i=0;i<names.size();++i)
        if (!strings::compareInsensitive(name,names[i].c_str())) return static_cast<int>(i);
    return -1;
}
int TSShape::findNode(int nameIndex) {
    for (std::size_t i=0;i<nodes.size();++i) if (nodes[i].nameIndex==nameIndex) return static_cast<int>(i);
    return -1;
}
int TSShape::readName(ResourceFile* file,char addIfMissing) {
    std::uint32_t length=0;
    // The saved body ignores both callback return counts. Only a zero length
    // is treated as the missing-name sentinel.
    file->read(&length,4,1);
    if (!length) return -1;
    std::string name(length,'\0');
    file->read(&name[0],1,length);
    int found=findName(&name[0]);
    if (found<0 && addIfMissing) {
        found=static_cast<int>(names.size());
        name.resize(std::strlen(name.c_str()));
        names.push_back(name);
        ownedNameIndices.push_back(static_cast<std::size_t>(found));
    }
    return found;
}
void DTSQuat16::getQuatF(float* output) const {
    constexpr float scale=1.0f/32767.0f;
    output[0]=x*scale; output[1]=y*scale; output[2]=z*scale; output[3]=w*scale;
}
void DTSQuat16::set(const float* input) {
    // The recovered VC6 __ftol helper temporarily selects round-toward-zero
    // before fistp, independently of the caller's x87 rounding mode.
    x=static_cast<std::int16_t>(static_cast<std::int64_t>(static_cast<long double>(input[0])*32767.0L));
    y=static_cast<std::int16_t>(static_cast<std::int64_t>(static_cast<long double>(input[1])*32767.0L));
    z=static_cast<std::int16_t>(static_cast<std::int64_t>(static_cast<long double>(input[2])*32767.0L));
    w=static_cast<std::int16_t>(static_cast<std::int64_t>(static_cast<long double>(input[3])*32767.0L));
}
void interpolatePoint3F(const float* first,const float* second,float position,float* output) {
    output[0]=(second[0]-first[0])*position+first[0];
    output[1]=(second[1]-first[1])*position+first[1];
    output[2]=(second[2]-first[2])*position+first[2];
}
std::uint32_t bitmapPowerOfTwoDimension(std::uint32_t value) {
    std::uint8_t exponent=0xff,previous=0xff;
    unsigned setBits=0;
    if (value) {
        do {
            previous=exponent;
            if (value&1u) ++setBits;
            exponent=static_cast<std::uint8_t>(previous+1u);
            value>>=1;
        } while (value);
        if (setBits>1) exponent=static_cast<std::uint8_t>(previous+2u);
    }
    return 1u<<(exponent&31u);
}
void TSLastDetail::chooseView(const float* matrix) {
    const float translation[3]={matrix[3],matrix[7],matrix[11]}; float direction[3];
    transformVector3(matrix,translation,direction);
    const float x=direction[0],y=direction[1],z=direction[2];
    float angle=0.0f;
    if (includePoles || numPolarSteps) {
        // The saved decompilation loses the acos argument. The existing
        // instruction export shows FDIVR Z after SQRT, then a float argument
        // store before the acos call (004d6395..004d63a1).
        const float squaredLength=z*z+y*y+x*x;
        const long double length=std::sqrt(static_cast<long double>(squaredLength));
        const float cosine=static_cast<float>(static_cast<long double>(z)/length);
        angle=static_cast<float>(std::acos(static_cast<long double>(cosine)));
    }
    constexpr long double pi=static_cast<long double>(3.141592653589793);
    if (includePoles && (!(angle>=polarAngle) ||
        !(pi-static_cast<long double>(polarAngle)>=static_cast<long double>(angle)))) {
        bitmapIndex=static_cast<int>((numPolarSteps*2+1)*numEquatorSteps);
        if (polarAngle<angle) ++bitmapIndex;
        rotation=static_cast<float>(matrixAtan2X87(matrix[5],matrix[4]));
        return;
    }
    float equatorStep=static_cast<float>((2.0L*pi)/numEquatorSteps);
    // 004d6449 stores the polar step as double, unlike the equator's float.
    const double polarStep=numPolarSteps
        ? static_cast<double>((static_cast<long double>(1.5707963267948966)-polarAngle)/
                              static_cast<std::int32_t>(numPolarSteps)) : 0.0;
    const long double equatorCoordinate=
        (matrixAtan2X87(x,y)+pi)*
        static_cast<long double>(0.9990000128746033)+static_cast<long double>(0.0010000000474974513);
    const auto legacyIndex=[](long double value) {
        constexpr long double limit=9223372036854775808.0L;
        const auto wide=(value>=-limit && value<limit) ? static_cast<std::int64_t>(value)
            : std::numeric_limits<std::int64_t>::min();
        const auto bits=static_cast<std::uint32_t>(wide);
        std::int32_t result; std::memcpy(&result,&bits,sizeof(result)); return result;
    };
    // The saved __ftol conversions truncate toward zero; rounding to nearest
    // moves views at sector boundaries into the next billboard slot.
    const int equatorIndex=legacyIndex(equatorCoordinate/equatorStep);
    bitmapIndex=equatorIndex*static_cast<int>(numPolarSteps*2+1);
    if (numPolarSteps)
        bitmapIndex=legacyIndex(
            (static_cast<long double>(angle)-polarAngle)/polarStep+bitmapIndex);
    rotation=static_cast<float>(matrixAtan2X87(matrix[2],matrix[10]));
}
void TSLastDetail::initializeBillboardMetadata(const TSShape* shape,
        std::uint32_t equatorSteps,std::uint32_t polarSteps,float angle,char poles,
        int index,int size,const char* source) {
    numEquatorSteps=equatorSteps;
    numPolarSteps=polarSteps;
    polarAngle=angle;
    includePoles=poles;
    atlasIndex=index;
    textureSize=size;
    bitmapSource=source ? source : "";
    center[0]=shape->center[0]; center[1]=shape->center[1]; center[2]=shape->center[2];
    const float radius=shape->radius;
    const float corners[12]={static_cast<float>(-radius),0.0f,radius, radius,0.0f,radius,
                             radius,0.0f,static_cast<float>(-radius),
                             static_cast<float>(-radius),0.0f,static_cast<float>(-radius)};
    std::copy(corners,corners+12,points);
}
TSLastDetail::~TSLastDetail() { destroy(); }
void TSLastDetail::destroy() {
    for (std::size_t index=0;index<textureEntries.size();++index) {
        if (index<bitmaps.size() && bitmaps[index]) {
            // 004d62a0 gates on the bitmap slot, then calls the texture-entry
            // destructor even when the corresponding entry is null. The saved
            // owner never directly destroys the bitmap view here.
            if (billboardServices.destroyTextureEntry)
                billboardServices.destroyTextureEntry(textureEntries[index]);
            else
                destroyTextureEntry(static_cast<EngineTextureEntry*>(textureEntries[index]));
        }
    }
    textureEntries.clear(); textureNames.clear(); bitmaps.clear();
}
bool TSLastDetail::loadBitmapViews(const char* source) {
    if (!source || !*source) return false;
    // 004d6ae0 formats base_textures + "billboards/" + source. Use the
    // shared texture root so callers that redirect the shape's asset tree
    // load the matching billboard images as well.
    std::string base=iflBaseTextures ? iflBaseTextures : "";
    base+="billboards/";
    base+=source;
    const std::size_t dot=base.find_last_of('.');
    if (dot!=std::string::npos) base.resize(dot);
    const std::uint32_t count=(numPolarSteps*2u+1u)*numEquatorSteps+
        (includePoles ? 2u : 0u);
    bitmaps.reserve(count); textureEntries.reserve(count); textureNames.reserve(count);
    for (std::uint32_t index=0;index<count;++index) {
        const std::string path=base+"_"+std::to_string(index)+".png";
        EngineBitmapView* bitmap=nullptr; void* entry=nullptr; std::uint32_t texture=0;
        bool loaded=false;
        if (billboardServices.loadBitmap) {
            loaded=billboardServices.loadBitmap(path.c_str(),3,true,&bitmap,&entry,&texture);
        } else {
            // 004d6ae0 passes the fully formatted base_textures/billboards
            // path, including .png, to engineTexture_loadByName. Keep that
            // cache key and call argument exact; the loader itself performs
            // its own base_textures prefix operation.
            auto* textureEntry=loadTextureByName(path.c_str(),3,true);
            entry=textureEntry;
            if (textureEntry) {
                bitmap=reinterpret_cast<EngineBitmapView*>(textureEntry);
                texture=textureEntry->textureName;
                loaded=true;
            }
        }
        if (!loaded || !entry || !bitmap) {
            // The saved loader's failed filename lookup yields a null entry;
            // cleanup visits only the earlier successfully loaded entries.
            destroy();
            return false;
        }
        bitmaps.push_back(bitmap); textureEntries.push_back(entry); textureNames.push_back(texture);
    }
    return true;
}
void TSLastDetail::construct(TSShape* shape,std::uint32_t equatorSteps,
        std::uint32_t polarSteps,float angle,char poles,int index,int size,const char* source) {
    initializeBillboardMetadata(shape,equatorSteps,polarSteps,angle,poles,index,size,source);
    if (!loadBitmapViews(source)) {
        const float polarStep=polarSteps ? (1.5707964f-angle)/static_cast<float>(polarSteps) : 0.0f;
        float elevation=polarSteps ? angle-1.5707964f : 0.0f;
        float azimuth=0.0f;
        const float equatorStep=equatorSteps ? static_cast<float>(
            static_cast<long double>(6.283185307179586)/equatorSteps) : 0.0f;
        for (std::uint32_t equator=0;equator<equatorSteps;++equator) {
            for (std::uint32_t polar=0;polar<polarSteps*2u+1u;++polar) {
                float elevationMatrix[16],azimuthMatrix[16],matrix[16];
                const float elevationAngles[3]={elevation,0.0f,0.0f};
                const float azimuthAngles[3]={0.0f,0.0f,static_cast<float>(azimuth-3.1415927f)};
                initializeEulerMatrix(elevationMatrix,elevationAngles);
                initializeEulerMatrix(azimuthMatrix,azimuthAngles);
                multiplyMatrix4x4(azimuthMatrix,elevationMatrix,matrix);
                EngineBitmapView* bitmap=snapshotShapeStatic(
                    shape,size,size,1,matrix,index,1.0f,1);
                bitmaps.push_back(bitmap);
                elevation+=polarStep;
            }
            azimuth+=equatorStep;
            elevation=polarSteps ? angle-1.5707964f : 0.0f;
        }
        if (poles) {
            const int poleSigns[2]={1,-1};
            for (int sign : poleSigns) {
                float matrix[16]; const float angles[3]={static_cast<float>(static_cast<float>(sign)*1.5707964f),0.0f,0.0f};
                initializeEulerMatrix(matrix,angles);
                bitmaps.push_back(snapshotShapeStatic(
                    shape,size,size,1,matrix,index,1.0f,1));
            }
        }
    }
    // The saved constructor creates fresh dynamic texture entries for every
    // bitmap after loadBitmapViews succeeds or the snapshot fallback runs.
    // In the loaded-view case this replaces the temporary handles written by
    // loadBitmapViews; the original does not release those cache entries here.
    textureEntries.assign(bitmaps.size(),nullptr);
    textureNames.assign(bitmaps.size(),0);
    for (std::size_t i=0;i<bitmaps.size();++i) {
        if (!bitmaps[i]) continue;
        if (billboardServices.createDynamicEntry) {
            textureEntries[i]=billboardServices.createDynamicEntry(
                bitmaps[i],true,&textureNames[i]);
        } else {
            auto* entry=createDynamicTextureEntry(nullptr,bitmaps[i],true);
            textureEntries[i]=entry;
            if (entry) textureNames[i]=entry->textureName;
        }
    }
}
void TSLastDetail::renderNoFog(float alpha) {
    static const float texcoords[8]={0.0f,0.0f,1.0f,0.0f,1.0f,1.0f,0.0f,1.0f};
    static const float normal[3]={0.0f,0.0f,1.0f};
    if (bitmapIndex<0 || static_cast<std::size_t>(bitmapIndex)>=textureNames.size()) return;
    dtsRenderGL->vertexPointer(3,3,0,points);
    dtsRenderGL->texCoordPointer(2,3,0,texcoords);
    dtsRenderGL->normalPointer(3,0,normal);
    const float color[4]={1.0f,1.0f,1.0f,alpha};
    dtsRenderGL->materialfv(0x15,0x74,color);
    dtsRenderGL->color4f(1.0f,1.0f,1.0f,alpha);
    const auto& state=lastDetailMaterialState;
    if ((state.materialState&4u)==0 || (state.materialState&0x18u)!=0) {
        dtsRenderGL->depthMask(false);
        dtsRenderGL->enable(0x2e);
        dtsRenderGL->blendFunc(0x37,0x38);
    }
    if ((state.textureState&0x10000000u)!=0) dtsRenderGL->enable(0x4a);
    dtsRenderGL->bindTexture(0x4a,textureNames[static_cast<std::size_t>(bitmapIndex)]);
    dtsRenderGL->drawArrays(10,0,4);
    lastDetailMaterialState.textureState=99;
    lastDetailMaterialState.materialState|=4u;
}
void TSLastDetail::renderFogCombine(float alpha) {
    static const float texcoords[8]={0.0f,0.0f,1.0f,0.0f,1.0f,1.0f,0.0f,1.0f};
    static const float normal[3]={0.0f,0.0f,1.0f};
    if (bitmapIndex<0 || static_cast<std::size_t>(bitmapIndex)>=textureNames.size()) return;
    dtsRenderGL->vertexPointer(3,3,0,points);
    dtsRenderGL->texCoordPointer(2,3,0,texcoords);
    dtsRenderGL->normalPointer(3,0,normal);
    const float color[4]={1.0f,1.0f,1.0f,alpha};
    dtsRenderGL->materialfv(0x15,0x74,color);
    dtsRenderGL->color4f(1.0f,1.0f,1.0f,alpha);
    const auto& state=lastDetailMaterialState;
    if ((state.materialState&4u)==0 || (state.materialState&0x18u)!=0) {
        dtsRenderGL->depthMask(false);
        dtsRenderGL->enable(0x2e);
        dtsRenderGL->blendFunc(0x37,0x38);
    }
    if ((state.textureState&0x10000000u)!=0) dtsRenderGL->enable(0x4a);
    dtsRenderGL->bindTexture(0x4a,textureNames[static_cast<std::size_t>(bitmapIndex)]);
    if (dtsTextureMode!=9) {
        dtsRenderGL->activeTexture(dtsSecondaryTextureUnit+0x8a);
        dtsRenderGL->texEnvfv(0x48,100,dtsFogColor);
        dtsRenderGL->enable(0x4a);
        dtsRenderGL->texEnvi(0x48,0x49,0x8e);
        dtsRenderGL->texEnvi(0x48,0x8f,0x9f);
        dtsRenderGL->texEnvi(0x48,0x91,0xa0);
        dtsRenderGL->texEnvi(0x48,0x97,0x33);
        dtsRenderGL->texEnvi(0x48,0x92,0xa2);
        dtsRenderGL->texEnvi(0x48,0x98,0x33);
        dtsRenderGL->texEnvi(0x48,0x93,0xa0);
        dtsRenderGL->texEnvi(0x48,0x99,0x37);
        dtsRenderGL->texEnvi(0x48,0x90,0x65);
        dtsRenderGL->texEnvi(0x48,0x94,0xa2);
        dtsRenderGL->texEnvi(0x48,0x9a,0x37);
    }
    dtsRenderGL->drawArrays(10,0,4);
    if (dtsTextureMode!=9) {
        dtsRenderGL->disable(0x4a);
        dtsRenderGL->activeTexture(0x8a);
    }
    lastDetailMaterialState.textureState=99;
    lastDetailMaterialState.materialState|=4u;
}
void TSLastDetail::render(float alpha,bool fog) {
    dtsRenderGL->pushMatrix();
    float view[16]; FUN_004d46e0(view);
    float centerInView[3]={static_cast<float>(view[0]*center[0]+view[4]*center[1]+view[8]*center[2]),
                           static_cast<float>(view[1]*center[0]+view[5]*center[1]+view[9]*center[2]),
                           static_cast<float>(view[2]*center[0]+view[6]*center[1]+view[10]*center[2])};
    view[3]+=centerInView[0]; view[7]+=centerInView[1]; view[11]+=centerInView[2];
    chooseView(view);
    dtsRenderGL->loadMatrix(view);
    dtsRenderGL->rotate(rotation*(180.0f/3.14159265358979323846f),0.0f,1.0f,0.0f);
    if (billboardRenderState.postScale)
        dtsRenderGL->scale(billboardRenderState.postScale[0],billboardRenderState.postScale[1],
                           billboardRenderState.postScale[2]);
    if (fog && hasTextureEnvCombine() && hasMultitexture()) {
        renderFogCombine(alpha);
        dtsRenderGL->popMatrix();
        return;
    }
    if (fog) alpha=(1.0f-dtsFogColor[3])*alpha;
    renderNoFog(alpha);
    dtsRenderGL->popMatrix();
}
namespace {
bool assembleMeshWithContext(DTSAssembledMesh& mesh,TSShapeAlloc& allocator,std::uint32_t version,bool encoded,bool skip,bool triangles,int minimum,
        const std::vector<TSMesh>& previous,const std::vector<TSSkinMesh>& previousSkins,const std::vector<std::uint8_t>& retained) {
    DTSMeshAssemblyContext context{&allocator,version,encoded,&previous,&previousSkins,false,skip,retained.data(),&mesh,triangles,minimum};
    struct RestoreContext { DTSMeshAssemblyContext* saved; ~RestoreContext() { activeMeshAssembly=saved; } } restore{activeMeshAssembly};
    activeMeshAssembly=&context;
    TSMesh::assembleMesh(mesh.type,static_cast<unsigned char>(skip));
    return context.succeeded;
}
// The original scratch tables hold backing pointers, not the shorter counts
// of the current mesh. Preserve the ancestor backing data in the normalized
// scratch copies until the pointer-based two-pass assembler replaces them.
void appendMeshScratch(const DTSAssembledMesh& mesh,std::vector<TSMesh>& bases,std::vector<TSSkinMesh>& skins,bool skinTable=false) {
    TSMesh base=mesh.base;
    TSSkinMesh skin=mesh.skin;
    int parent=base.parentMesh;
    if (parent>=0 && static_cast<std::size_t>(parent)<bases.size()) {
        const auto& source=bases[parent];
        base.vertices=source.vertices; base.texcoords=source.texcoords;
        base.normals=source.normals; base.encodedNormals=source.encodedNormals;
        if (mesh.type==DTSMeshSkin && static_cast<std::size_t>(parent)<skins.size()) {
            const auto& original=skins[parent];
            skin.initialVertices=original.initialVertices; skin.initialNormals=original.initialNormals;
            skin.encodedNormals=original.encodedNormals; skin.initialTransforms=original.initialTransforms;
            skin.vertexIndices=original.vertexIndices; skin.boneIndices=original.boneIndices;
            skin.weights=original.weights; skin.nodeIndices=original.nodeIndices;
        }
    }
    // The separate skin list registers initial vertices and normals in the
    // common parent tables (004cc4a0), rather than the skin's base arrays.
    if (skinTable) {
        base.vertices=skin.initialVertices;
        base.normals=skin.initialNormals;
        base.encodedNormals=skin.encodedNormals;
    }
    bases.push_back(std::move(base)); skins.push_back(std::move(skin));
}
void* assembledMeshObject(DTSAssembledMesh& mesh) {
    switch (mesh.type) {
    case DTSMeshStandard: return &mesh.base;
    case DTSMeshSkin: return &mesh.skin;
    case DTSMeshDecal: return &mesh.decal;
    case DTSMeshSorted: return &mesh.sorted;
    default: return nullptr;
    }
}
}
bool TSShape::read(ResourceFile& file,const char* legacyMaterialPrefix,bool useEncodedNormals,unsigned skipDetail,bool convertTriangles,int minStripTriangles) {
    // Do not retain data from an earlier load if a shape object is reused.
    // Release externally-owned texture entries before replacing the normalized
    // material records; vector assignment alone only destroys their strings.
    destroy();
    *this=TSShape();
    const HeaderResult header=readHeader(file);
    if (header==HeaderResult::legacyUnified) {
        TSShapeAlloc legacyAllocator;
        bool ok=readLegacyPrefix(file) && readLegacyTail(file,legacyMaterialPrefix,&legacyShapeBuffers) &&
            assembleShapePass(legacyAllocator,true,useEncodedNormals,skipDetail,convertTriangles,minStripTriangles) &&
            (!dtsShapeInitializeAfterRead || initializeShape(skipDetail));
        if (ok) {
            // 004cd340 releases the three temporary legacy streams after
            // assembly. These ranges point into them and are staging only.
            meshStreams=DTSLegacyAlloc();
            legacyShapeBuffers=DTSLegacyShapeBuffers{};
            meshes.clear(); skins.clear();
        }
        return ok;
    }
    if (header==HeaderResult::splitBuffer) {
        if (!readSplitData(file,legacyMaterialPrefix)) return false;
        // The first pass sizes the shape block. Preserve sequence input because
        // the normalized core currently derives ground frames in both passes.
        auto inputSequences=sequences;
        TSShapeAlloc allocator; setSplitAssemblyRead(allocator,true);
        bool ok=assembleShapePass(allocator,false,useEncodedNormals,skipDetail,convertTriangles,minStripTriangles);
        if (!ok) return false;
        assemblyArenaBytes=allocator.size;
        assemblyArena.resize((static_cast<std::size_t>(assemblyArenaBytes)+7)/8+1);
        sequences=std::move(inputSequences);
        allocator.destination=reinterpret_cast<std::uint8_t*>(assemblyArena.data());
        allocator.size=0;
        setSplitAssemblyRead(allocator,false);
        ok=assembleShapePass(allocator,false,useEncodedNormals,skipDetail,convertTriangles,minStripTriangles);
        if (ok && allocator.size!=assemblyArenaBytes) ok=false;
        if (ok && dtsShapeInitializeAfterRead) ok=initializeShape(skipDetail);
        if (ok) {
            // The original discards its unified input buffer after assembly;
            // the normalized output currently owns copies of its data.
            std::vector<std::uint32_t>().swap(splitWords);
            split16Offset=split8Offset=0;
        }
        return ok;
    }
    return false;
}
TSShape::HeaderResult TSShape::readHeader(ResourceFile& file) {
    std::uint32_t header=0;
    if (file.read(&header,4,1)==0) return HeaderResult::readError;
    exporterVersion=static_cast<std::int32_t>(header)>>16;
    std::uint32_t version=header&0xffu;
    currentShapeReadVersion=version;
    if (version>maximumDTSReadVersion) return HeaderResult::unsupportedVersion;
    readVersion=version;
    return version<19 ? HeaderResult::legacyUnified : HeaderResult::splitBuffer;
}
bool TSShape::readLegacyPrefix(ResourceFile& file) {
    if (readVersion>=19) return false;
    auto block=[&](void* p,std::uint32_t width,std::uint32_t count) { return !count || file.read(p,width,count)==count; };
    auto count=[&](std::uint32_t& n) { return block(&n,4,1); };
    if (!block(&radius,4,1) || !block(&tubeRadius,4,1) || !block(center,4,3) || !block(bounds,4,6)) return false;
    std::uint32_t n=0;
    if (!count(n)) return false;
    nodes.resize(n);
    for (auto& node:nodes) {
        if (!block(&node.nameIndex,4,1) || !block(&node.parentIndex,4,1)) return false;
        if (readVersion<17 && !block(&node.legacyFlag,1,1)) return false;
    }
    if (!count(n)) return false; objects.resize(n);
    for (auto& item:objects) if (!block(&item,4,4)) return false;
    if (!count(n)) return false; decals.resize(n);
    for (auto& item:decals) if (!block(&item,4,4)) return false;
    if (!count(n)) return false; iflMaterials.resize(n);
    for (auto& item:iflMaterials) if (!block(&item,4,2)) return false;
    if (!count(n)) return false;
    firstNodes.resize(n); firstObjects.resize(n); firstDecals.resize(n);
    std::uint32_t discarded=0;
    if (!block(firstNodes.data(),4,n) || !block(&discarded,4,1) || !block(firstObjects.data(),4,n) || !block(&discarded,4,1) || !block(firstDecals.data(),4,n)) return false;
    auto makeCounts=[](const std::vector<std::int32_t>& first,std::size_t total,std::vector<std::int32_t>& result) {
        result.resize(first.size());
        std::int32_t end=static_cast<std::int32_t>(total);
        for (std::size_t i=first.size();i>0;--i) {
            auto start=first[i-1]; if (start<0 || start>end) return false;
            result[i-1]=end-start; end=start;
        }
        return true;
    };
    if (!makeCounts(firstNodes,nodes.size(),numNodes) || !makeCounts(firstObjects,objects.size(),numObjects) || !makeCounts(firstDecals,decals.size(),numDecals)) return false;
    if (readVersion<16) {
        if (!count(n)) return false;
        legacyPre16Words.resize(n);
        if (!block(legacyPre16Words.data(),4,n)) return false;
    } else {
        legacyPre16Words.clear();
    }
    if (readVersion<17) {
        if (!count(n)) return false; legacyKeyframes.resize(n);
        for (auto& item:legacyKeyframes) if (!block(&item,4,3)) return false;
    }
    if (!count(n)) return false; nodeStates.resize(n);
    for (auto& item:nodeStates) if (!block(item.rotation,2,4) || !block(item.translation,4,3)) return false;
    // The legacy-header field named numNodeTranslations in the saved decomp
    // is actually the object-state count: 004cc4a0 consumes this header word
    // after the combined node-state count, then copies three words per state.
    if (!count(n)) return false; objectStates.resize(n);
    for (auto& item:objectStates) if (!block(&item,4,3)) return false;
    if (!count(n)) return false;
    if (readVersion<14) {
        // 004c93d0 inserts one -1 default per decal before consuming the
        // serialized state words. The assembler still exposes the header's
        // original state count, so only that prefix of the combined stream
        // becomes the final state table.
        const auto defaults=decals.size();
        decalStates.assign(defaults+static_cast<std::size_t>(n),-1);
        if (!block(decalStates.data()+defaults,4,n)) return false;
        decalStates.resize(n);
    } else {
        decalStates.resize(n); if (!block(decalStates.data(),4,n)) return false;
    }
    if (!count(n)) return false; triggers.resize(n); for (auto& item:triggers) if (!block(&item,4,2)) return false;
    if (!count(n)) return false; details.resize(n);
    smallestVisibleSize=0; smallestVisibleDetail=0;
    for (std::size_t i=0;i<details.size();++i) {
        auto& item=details[i]; if (!block(&item,4,4)) return false;
        if (!std::isfinite(item.size) || static_cast<double>(item.size)<std::numeric_limits<std::int32_t>::min() || static_cast<double>(item.size)>std::numeric_limits<std::int32_t>::max()) return false;
        int size=static_cast<int>(item.size);
        if (size>=0) { smallestVisibleSize=static_cast<float>(size); smallestVisibleDetail=static_cast<std::int32_t>(i); }
        item.averageError=item.maxError=-1.0f; item.polyCount=0;
    }
    if (!count(n)) return false; sequences.clear(); sequences.resize(n); legacyFirstKeyframeScratch.clear();
    currentShapeReadVersion=readVersion;
    for (auto& sequence:sequences) { sequence.read(&file,1); if (file.eof() || file.error()) return false; }
    legacyFirstKeyframes=legacyFirstKeyframeScratch;
    return true;
}
bool TSShape::importSequences(ResourceFile& file,bool initializeAfterImport) {
    // 004ca240 has distinct record paths below v17, v17-v21, and v22. The
    // shipped DSQ inventory is v16; later paths follow the saved reader but
    // have no shipped-asset runtime coverage.
    auto read=[&](void* value,std::uint32_t size=4) {
        return file.read(value,size,1)==1;
    };
    std::uint32_t version=0,nodeNameCount=0;
    if (!read(&version) || version>maximumDTSReadVersion || !read(&nodeNameCount)) return false;
    currentShapeReadVersion=version;

    std::vector<std::int32_t> nodeMap(nodeNameCount,-1);
    std::vector<std::uint32_t> duplicateOrdinal(names.size(),0);
    for (std::uint32_t i=0;i<nodeNameCount;++i) {
        const std::size_t namesBefore=names.size();
        const std::size_t ownedNamesBefore=ownedNameIndices.size();
        int nameIndex=readName(&file,1);
        if (nameIndex<0) return false;
        if (duplicateOrdinal.size()<names.size()) duplicateOrdinal.resize(names.size(),0);
        std::uint32_t occurrence=duplicateOrdinal[static_cast<std::size_t>(nameIndex)]++;
        for (std::size_t node=0;node<nodes.size();++node) {
            if (nodes[node].nameIndex!=nameIndex) continue;
            if (occurrence--==0) { nodeMap[i]=static_cast<std::int32_t>(node); break; }
        }
        if (nodeMap[i]<0 && names.size()!=namesBefore) {
            names.resize(namesBefore);
            ownedNameIndices.resize(ownedNamesBefore);
        }
        // The saved importer abandons the import if the incoming node name
        // cannot be resolved to the shape (including a duplicate occurrence
        // beyond the shape's matching nodes). Keeping -1 and continuing
        // would silently drop that node's animation masks while accepting the
        // rest of the DSQ.
        if (nodeMap[i]<0) return false;
    }

    // These two words precede the pre-v17 keyframe records. The second is
    // used to rebase their object-state indices.
    std::uint32_t unusedNodeStateBase=0,objectStateBase=0,groundRecordCount=0;
    if (!read(&unusedNodeStateBase) || !read(&objectStateBase)) return false;
    (void)unusedNodeStateBase;
    legacyKeyframeScratch.clear();
    if (version<17) {
        if (!read(&groundRecordCount)) return false;
        legacyKeyframeScratch.resize(groundRecordCount);
        for (std::size_t i=0;i<legacyKeyframeScratch.size();++i) {
            auto& key=legacyKeyframeScratch[i];
            if (!read(&key.first) || !read(&key.second) || !read(&key.third)) return false;
        }
    }

    const std::size_t rotationBase=nodeRotations.size();
    const std::size_t translationBase=nodeTranslations.size();
    const std::size_t uniformScaleBase=nodeUniformScales.size();
    const std::size_t alignedScaleBase=nodeAlignedScales.size();
    const std::size_t arbitraryScaleBase=nodeArbitraryScaleFactors.size();
    const std::size_t groundTranslationBase=groundTranslations.size();
    // 004ca240 rebases pre-v22 input indices by the existing animated count
    // minus the incoming node-name count. Default poses already live in
    // separate arrays after assembly; do not add the shape's node count here.
    const std::int32_t rotationFrameBase=static_cast<std::int32_t>(rotationBase)+
        (version<22 ? -static_cast<std::int32_t>(nodeNameCount) : 0);
    const std::int32_t translationFrameBase=static_cast<std::int32_t>(translationBase)+
        (version<22 ? -static_cast<std::int32_t>(nodeNameCount) : 0);
    std::uint32_t stateCount=0;
    if (version<22) {
        if (!read(&stateCount)) return false;
        nodeRotations.resize(rotationBase+stateCount);
        nodeTranslations.resize(translationBase+stateCount);
        for (std::uint32_t i=0;i<stateCount;++i) {
            auto& q=nodeRotations[rotationBase+i]; auto& p=nodeTranslations[translationBase+i];
            if (!read(&q.x,2) || !read(&q.y,2) || !read(&q.z,2) || !read(&q.w,2) ||
                !read(&p.x) || !read(&p.y) || !read(&p.z)) return false;
        }
    } else {
        auto readQuaternions=[&](std::vector<DTSQuat16>& values,std::uint32_t count) {
            const std::size_t base=values.size(); values.resize(base+count);
            for (std::uint32_t i=0;i<count;++i) {
                auto& q=values[base+i];
                if (!read(&q.x,2) || !read(&q.y,2) || !read(&q.z,2) || !read(&q.w,2)) return false;
            }
            return true;
        };
        auto readPoints=[&](std::vector<DTSPoint3>& values,std::uint32_t count) {
            const std::size_t base=values.size(); values.resize(base+count);
            for (std::uint32_t i=0;i<count;++i) {
                auto& p=values[base+i];
                if (!read(&p.x) || !read(&p.y) || !read(&p.z)) return false;
            }
            return true;
        };
        auto readFloats=[&](std::vector<float>& values,std::uint32_t count) {
            const std::size_t base=values.size(); values.resize(base+count);
            for (std::uint32_t i=0;i<count;++i) if (!read(&values[base+i])) return false;
            return true;
        };
        std::uint32_t count=0;
        if (!read(&count) || !readQuaternions(nodeRotations,count) ||
            !read(&count) || !readPoints(nodeTranslations,count) ||
            !read(&count) || !readFloats(nodeUniformScales,count) ||
            !read(&count) || !readPoints(nodeAlignedScales,count)) return false;
        // Each of these two pairs has one shared count in the saved reader.
        if (!read(&count) || !readQuaternions(nodeArbitraryScaleRotations,count) ||
            !readPoints(nodeArbitraryScaleFactors,count) ||
            !read(&count) || !readPoints(groundTranslations,count) ||
            !readQuaternions(groundRotations,count)) return false;
    }

    // The object-state adjustment word and sequence count follow the imported
    // transform streams; object states themselves are not embedded here.
    std::uint32_t importedObjectStateCount=0,sequenceCount=0;
    if (!read(&importedObjectStateCount) || !read(&sequenceCount)) return false;
    const std::size_t sequenceBase=sequences.size();
    const std::size_t triggerBase=triggers.size();
    sequences.resize(sequenceBase+sequenceCount);
    legacyFirstKeyframeScratch.clear();
    for (std::size_t index=sequenceBase;index<sequences.size();++index) {
        TSShapeSequence& sequence=sequences[index];
        sequence.nameIndex=readName(&file,1);
        if (sequence.nameIndex<0) return false;
        sequence.read(file,version,false,legacyFirstKeyframeScratch);
        if (file.error() || file.eof()) return false;
        if (version>=17) {
            sequence.baseRotation+=rotationFrameBase;
            sequence.baseTranslation+=translationFrameBase;
            if (version>=22) {
                if (sequence.flags&1u) sequence.baseScale+=static_cast<std::int32_t>(uniformScaleBase);
                else if (sequence.flags&2u) sequence.baseScale+=static_cast<std::int32_t>(alignedScaleBase);
                else if (sequence.flags&4u) sequence.baseScale+=static_cast<std::int32_t>(arbitraryScaleBase);
            }
        }
        const std::int32_t groundFrameBase=version>=22 ?
            static_cast<std::int32_t>(groundTranslationBase) : rotationFrameBase;
        sequence.firstGroundFrame+=groundFrameBase;
        if (version>=9) sequence.firstTrigger+=static_cast<std::int32_t>(triggerBase);

        TSIntegerSet mappedRotation,mappedTranslation,mappedScale;
        mappedRotation.clearAll(TSIntegerSet::capacity); mappedTranslation.clearAll(TSIntegerSet::capacity); mappedScale.clearAll(TSIntegerSet::capacity);
        for (std::uint32_t source=0;source<nodeMap.size() && source<TSIntegerSet::capacity;++source) {
            const int destination=nodeMap[source];
            if (destination<0 || destination>=TSIntegerSet::capacity) continue;
            if (sequence.rotationMatters.test(source)) mappedRotation.set(static_cast<unsigned>(destination));
            if (sequence.translationMatters.test(source)) mappedTranslation.set(static_cast<unsigned>(destination));
            if (sequence.scaleMatters.test(source)) mappedScale.set(static_cast<unsigned>(destination));
        }
        sequence.rotationMatters.copy(mappedRotation);
        sequence.translationMatters.copy(mappedTranslation);
        sequence.scaleMatters.copy(mappedScale);
    }

    // Object state count is a base-adjustment field, not a second payload.
    // For v16, keyframe first-index records drive the legacy track transpose.
    (void)importedObjectStateCount;
    if (version>=9) {
        std::uint32_t triggerCount=0;
        if (!read(&triggerCount)) return false;
        const std::size_t oldTriggerCount=triggers.size();
        triggers.resize(oldTriggerCount+triggerCount);
        for (std::size_t i=oldTriggerCount;i<triggers.size();++i)
            if (!read(&triggers[i].state) || !read(&triggers[i].position)) return false;
    }

    // Normalize the importer-owned scratch indices here, where the default
    // prefix is known. The original-address transpose entry must take its
    // supplied indices literally, including when explicit buffers are passed.
    if (version<17) for (auto& key:legacyKeyframeScratch) {
        key.first+=rotationFrameBase;
        key.second+=static_cast<std::int32_t>(objectStates.size())-
            static_cast<std::int32_t>(objectStateBase);
    }

    for (std::size_t i=sequenceBase;version<17 && i<sequences.size();++i) {
        const auto& sequence=sequences[i];
        if (!sequence.numKeyframes) continue;
        const std::size_t firstIndex=legacyFirstKeyframeScratch.size()-sequenceCount+(i-sequenceBase);
        if (firstIndex>=legacyFirstKeyframeScratch.size()) return false;
        if (legacyFirstKeyframeScratch[firstIndex]<0 ||
            static_cast<std::size_t>(legacyFirstKeyframeScratch[firstIndex])>=legacyKeyframeScratch.size()) return false;
        const std::int32_t first=legacyKeyframeScratch[static_cast<std::size_t>(legacyFirstKeyframeScratch[firstIndex])].first;
        int rotationTracks=0,objectTracks=0,decalTracks=0;
        for (int node=0;node<TSIntegerSet::capacity;++node) {
            rotationTracks+=sequence.rotationMatters.test(node);
            objectTracks+=sequence.visMatters.test(node)||sequence.frameMatters.test(node)||sequence.matFrameMatters.test(node);
            decalTracks+=sequence.decalMatters.test(node);
        }
        const std::size_t frames=static_cast<std::size_t>(sequence.numKeyframes);
        auto fits=[&](std::size_t size,int base,int tracks) {
            if (base<0) return false;
            const std::size_t start=static_cast<std::size_t>(base);
            return start<=size && (!tracks || frames<=(size-start)/static_cast<unsigned>(tracks));
        };
        const auto& offsets=legacyKeyframeScratch[static_cast<std::size_t>(legacyFirstKeyframeScratch[firstIndex])];
        if (sequence.numKeyframes<0 ||
            (rotationTracks && (!fits(nodeRotations.size(),first,rotationTracks) ||
                                !fits(nodeTranslations.size(),first,rotationTracks))) ||
            (objectTracks && !fits(objectStates.size(),offsets.second,objectTracks)) ||
            (decalTracks && !fits(decalStates.size(),offsets.third,decalTracks))) return false;
        rearrangeKeyframeData(&sequences[i],legacyFirstKeyframeScratch[firstIndex],nullptr,nullptr,nullptr,nullptr,-1,-1,-1,-1);
    }
    for (std::size_t i=sequenceBase;version<22 && i<sequences.size();++i) {
        auto& sequence=sequences[i];
        if (sequence.numGroundFrames<0 || sequence.firstGroundFrame<0) return false;
        if (!sequence.numGroundFrames) {
            sequence.firstGroundFrame=static_cast<std::int32_t>(groundRotations.size());
            continue;
        }
        const std::size_t first=static_cast<std::size_t>(sequence.firstGroundFrame);
        const std::size_t count=static_cast<std::size_t>(sequence.numGroundFrames);
        if (first>nodeRotations.size() || count>nodeRotations.size()-first ||
            first>nodeTranslations.size() || count>nodeTranslations.size()-first) return false;
        sequence.firstGroundFrame=static_cast<std::int32_t>(groundRotations.size());
        groundRotations.insert(groundRotations.end(),nodeRotations.begin()+first,nodeRotations.begin()+first+count);
        groundTranslations.insert(groundTranslations.end(),nodeTranslations.begin()+first,nodeTranslations.begin()+first+count);
    }
    (void)objectStateBase;
    if (initializeAfterImport) init();
    return true;
}
bool TSShape::readLegacyTail(ResourceFile& file,const char* legacyMaterialPrefix,DTSLegacyShapeBuffers* output) {
    if (readVersion>=19) return false;
    auto readWord=[&](void* value) { return file.read(value,4,1)==1; };
    auto count=[&](std::uint32_t& n) { return readWord(&n); };
    meshStreams.init32(); meshStreams.init16(); meshStreams.init8();
    meshes.clear(); skins.clear(); names.clear(); ownedNameIndices.clear(); materialList.free(); hasMaterials=false;
    std::uint32_t n=0;
    if (!count(n)) return false;
    meshes.reserve(n);
    for (std::uint32_t i=0;i<n;++i) {
        std::int32_t type=0; if (!readWord(&type)) return false;
        DTSLegacyMeshRange range{static_cast<DTSMeshKind>(type),static_cast<std::uint32_t>(meshStreams.words32.size()),
            static_cast<std::uint32_t>(meshStreams.words16.size()),static_cast<std::uint32_t>(meshStreams.bytes8.size())};
        *meshStreams.alloc32(1)=static_cast<std::uint32_t>(type);
        if (!readLegacyMesh(file,meshStreams,readVersion,static_cast<DTSMeshKind>(type))) return false;
        meshes.push_back(range);
    }
    if (readVersion<16) {
        // The old slot table marks absent meshes with negative words. Its
        // nonnegative values select the next serialized mesh, rather than
        // acting as indices into the serialized mesh array (004cc4a0).
        std::vector<DTSLegacyMeshRange> slots;
        slots.reserve(legacyPre16Words.size());
        std::size_t next=0;
        for (const auto word:legacyPre16Words) {
            if (static_cast<std::int32_t>(word)<0) slots.push_back({DTSMeshNull,0,0,0});
            else {
                if (next>=meshes.size()) return false;
                slots.push_back(meshes[next++]);
            }
        }
        if (next!=meshes.size()) return false;
        meshes=std::move(slots);
    }
    meshStreams.guard();
    if (!count(n)) return false;
    for (std::uint32_t i=0;i<n;++i) {
        std::uint32_t length=0; if (!count(length)) return false;
        std::string name(length,'\0');
        if (length && file.read(&name[0],1,length)!=length) return false;
        auto* output=meshStreams.alloc8(length+1);
        for (std::uint32_t j=0;j<length;++j) output[j]=static_cast<std::uint8_t>(name[j]);
        output[length]=0; names.push_back(name);
    }
    meshStreams.guard();
    std::uint32_t present=0; if (!readWord(&present)) return false;
    if (present) {
        dts::legacyMaterialPrefix=legacyMaterialPrefix;
        currentShapeReadVersion=readVersion;
        if (!materialList.read(&file)) return false;
        hasMaterials=true;
        if (exporterVersion<116) for (std::size_t i=0;i<materialList.size();++i) {
            auto flags=materialList.getFlags(static_cast<int>(i));
            if (hasMaterialFlag(flags,TSMaterialFlag::Translucent) &&
                !(flags&(materialFlag(TSMaterialFlag::WrapS)|materialFlag(TSMaterialFlag::WrapT))))
                materialList.setFlags(static_cast<int>(i),flags|materialFlag(TSMaterialFlag::SelectTextureType6));
        }
    }
    if (!count(n)) return false;
    skins.reserve(n);
    for (std::uint32_t i=0;i<n;++i) {
        DTSLegacyMeshRange range{DTSMeshSkin,static_cast<std::uint32_t>(meshStreams.words32.size()),
            static_cast<std::uint32_t>(meshStreams.words16.size()),static_cast<std::uint32_t>(meshStreams.bytes8.size())};
        if (!readLegacyMesh(file,meshStreams,readVersion,DTSMeshSkin)) return false;
        skins.push_back(range);
    }
    meshStreams.guard();
    detailFirstSkin.assign(details.size(),0); detailNumSkins.assign(details.size(),0);
    if (n) {
        std::uint32_t ignored=0; if (!readWord(&ignored)) return false;
        if (!details.empty() && file.read(detailFirstSkin.data(),4,static_cast<std::uint32_t>(details.size()))!=details.size()) return false;
        std::int32_t next=static_cast<std::int32_t>(n);
        for (std::size_t i=details.size();i>0;--i) { detailNumSkins[i-1]=next-detailFirstSkin[i-1]; next=detailFirstSkin[i-1]; }
    }
    if (readVersion<17 && !rearrangeLegacyKeyframes()) return false;
    if (!normalizeLegacyPoses()) return false;
    // The recovered reader publishes the three legacy allocator cursors and
    // their element counts to TSShape::read. Keep that handoff explicit while
    // the normalized header/pose tables stay owned by this TSShape.
    if (output) {
        output->words32=meshStreams.words32.data(); output->word32Count=meshStreams.dwordCount32();
        output->words16=meshStreams.words16.data(); output->word16Count=meshStreams.dwordCount16();
        output->bytes8=meshStreams.bytes8.data(); output->byte8Count=meshStreams.dwordCount8();
    }
    return true;
}
bool TSShape::rearrangeLegacyKeyframes() {
    if (readVersion>=17 || legacyFirstKeyframes.size()!=sequences.size()) return false;
    struct Plan { int rotation,object,decal,frames,rotTracks,objTracks,decalTracks; };
    std::vector<Plan> plans;
    auto fits=[](int first,int frames,int tracks,std::size_t size) {
        return first>=0 && frames>=0 && tracks>=0 && static_cast<std::size_t>(first)<=size &&
            (!tracks || static_cast<std::size_t>(frames)<=(size-static_cast<std::size_t>(first))/static_cast<unsigned>(tracks));
    };
    for (std::size_t i=0;i<sequences.size();++i) {
        auto& sequence=sequences[i];
        if (sequence.numKeyframes<0) return false;
        if (!sequence.numKeyframes) { plans.push_back({0,0,0,0,0,0,0}); continue; }
        int rotations=0,objects=0,decals=0;
        for (unsigned node=0;node<TSIntegerSet::capacity;++node) {
            rotations+=sequence.rotationMatters.test(node);
            objects+=sequence.frameMatters.test(node)||sequence.matFrameMatters.test(node)||sequence.visMatters.test(node);
            decals+=sequence.decalMatters.test(node);
        }
        int key=legacyFirstKeyframes[i];
        if ((rotations || objects || decals) && (key<0 || static_cast<std::size_t>(key)>=legacyKeyframes.size())) return false;
        DTSKeyframe frame{};
        if (rotations || objects || decals) frame=legacyKeyframes[key];
        Plan plan{rotations?frame.first:0,objects?frame.second:0,decals?frame.third:0,sequence.numKeyframes,rotations,objects,decals};
        if (!fits(plan.rotation,plan.frames,rotations,nodeStates.size()) ||
            !fits(plan.object,plan.frames,objects,objectStates.size()) ||
            !fits(plan.decal,plan.frames,decals,decalStates.size())) return false;
        plans.push_back(plan);
    }
    for (std::size_t i=0;i<plans.size();++i) {
        const auto& p=plans[i]; if (!p.frames) continue;
        auto& sequence=sequences[i];
        sequence.baseRotation=sequence.baseTranslation=p.rotation;
        sequence.baseObjectState=p.object; sequence.baseDecalState=p.decal;
        // Both pose components use the same keyframe base and permutation.
        // The binary stores them in separate 16-bit and 32-bit streams;
        // transposing complete normalized records applies that permutation
        // to both components without introducing a third pose stream.
        if (!rearrangeStates(nodeStates.data(),nodeStates.size()*sizeof(DTSNodeState),p.rotation,p.frames,p.rotTracks,sizeof(DTSNodeState)) ||
            !rearrangeStates(objectStates.data(),objectStates.size()*sizeof(DTSObjectState),p.object,p.frames,p.objTracks,sizeof(DTSObjectState)) ||
            !rearrangeStates(decalStates.data(),decalStates.size()*sizeof(std::int32_t),p.decal,p.frames,p.decalTracks,sizeof(std::int32_t))) return false;
    }
    return true;
}
void TSShape::rearrangeKeyframeData(TSShapeSequence* sequence,int firstKeyframe,
        unsigned char* rotations,unsigned char* translations,unsigned char* objectData,unsigned char* decalData,
        int rotationStride,int translationStride,int objectStride,int decalStride) {
    int rotationTracks=0,objectTracks=0,decalTracks=0;
    for (int i=0;i<TSIntegerSet::capacity;++i) {
        if (sequence->rotationMatters.test(i)) ++rotationTracks;
        if (sequence->frameMatters.test(i)||sequence->matFrameMatters.test(i)||sequence->visMatters.test(i)) ++objectTracks;
        if (sequence->decalMatters.test(i)) ++decalTracks;
    }
    int frames=sequence->numKeyframes;
    if (!rotations && frames*rotationTracks) rotations=reinterpret_cast<unsigned char*>(nodeRotations.data());
    if (!translations && frames*rotationTracks) translations=reinterpret_cast<unsigned char*>(nodeTranslations.data());
    if (!objectData && frames*objectTracks) objectData=reinterpret_cast<unsigned char*>(objectStates.data());
    if (!decalData && frames*decalTracks) decalData=reinterpret_cast<unsigned char*>(decalStates.data());
    if (rotationStride<0) rotationStride=8;
    if (translationStride<0) translationStride=12;
    if (objectStride<0) objectStride=12;
    if (decalStride<0) decalStride=4;
    if (!frames) return;
    sequence->baseRotation=sequence->baseTranslation=rotationTracks ?
        legacyKeyframeScratch[firstKeyframe].first : 0;
    sequence->baseObjectState=objectTracks ? legacyKeyframeScratch[firstKeyframe].second : 0;
    sequence->baseDecalState=decalTracks ? legacyKeyframeScratch[firstKeyframe].third : 0;
    rearrangeStates(sequence->baseRotation,frames,rotationTracks,rotations,rotationStride);
    rearrangeStates(sequence->baseTranslation,frames,rotationTracks,translations,translationStride);
    rearrangeStates(sequence->baseObjectState,frames,objectTracks,objectData,objectStride);
    rearrangeStates(sequence->baseDecalState,frames,decalTracks,decalData,decalStride);
}
bool TSShape::normalizeLegacyPoses() {
    if (readVersion>=19 || nodeStates.size()<nodes.size()) return false;
    const std::size_t defaults=nodes.size();
    // Ground indices in old files address the combined default/animated array.
    for (const auto& sequence:sequences) {
        if (sequence.numGroundFrames<0) return false;
        if (sequence.numGroundFrames) {
            if (sequence.firstGroundFrame<static_cast<std::int32_t>(defaults)) return false;
            std::size_t start=static_cast<std::size_t>(sequence.firstGroundFrame);
            if (start>nodeStates.size() || static_cast<std::size_t>(sequence.numGroundFrames)>nodeStates.size()-start) return false;
        }
    }
    defaultRotations.resize(defaults); defaultTranslations.resize(defaults);
    nodeRotations.resize(nodeStates.size()-defaults);
    nodeTranslations.resize(nodeStates.size()-defaults);
    for (std::size_t i=0;i<nodeStates.size();++i) {
        const auto& state=nodeStates[i];
        DTSQuat16 rotation{state.rotation[0],state.rotation[1],state.rotation[2],state.rotation[3]};
        DTSPoint3 translation{state.translation[0],state.translation[1],state.translation[2]};
        if (i<defaults) { defaultRotations[i]=rotation; defaultTranslations[i]=translation; }
        else { nodeRotations[i-defaults]=rotation; nodeTranslations[i-defaults]=translation; }
    }
    groundRotations.clear(); groundTranslations.clear();
    for (auto& sequence:sequences) {
        int first=sequence.firstGroundFrame;
        sequence.firstGroundFrame=static_cast<std::int32_t>(groundRotations.size());
        for (int i=0;i<sequence.numGroundFrames;++i) {
            std::size_t index=static_cast<std::size_t>(first+i)-defaults;
            groundRotations.push_back(nodeRotations[index]); groundTranslations.push_back(nodeTranslations[index]);
        }
        sequence.baseRotation-=static_cast<std::int32_t>(defaults);
        sequence.baseTranslation-=static_cast<std::int32_t>(defaults);
        sequence.baseScale=0;
    }
    return true;
}
bool TSShape::initializeHierarchy() {
    // Validate references before replacing any of the existing links.
    for (const auto& node:nodes)
        if (node.parentIndex < -1 || (node.parentIndex >= 0 && static_cast<std::size_t>(node.parentIndex)>=nodes.size())) return false;
    for (const auto& object:objects)
        if (object.nodeIndex<0 || static_cast<std::size_t>(object.nodeIndex)>=nodes.size()) return false;
    for (const auto& decal:decals)
        if (decal.objectIndex<0 || static_cast<std::size_t>(decal.objectIndex)>=objects.size() ||
            decal.startMeshIndex<0 || decal.numMeshes<0 || static_cast<std::size_t>(decal.startMeshIndex)>assembledMeshes.size() ||
            static_cast<std::size_t>(decal.numMeshes)>assembledMeshes.size()-static_cast<std::size_t>(decal.startMeshIndex)) return false;
    for (auto& node:nodes) node.firstChild=node.firstObject=node.nextSibling=-1;
    std::vector<int> lastChild(nodes.size(),-1),lastObject(nodes.size(),-1),lastDecal(objects.size(),-1);
    for (std::size_t i=0;i<nodes.size();++i) {
        int parent=nodes[i].parentIndex; if (parent<0) continue;
        int& last=lastChild[parent];
        if (last<0) nodes[parent].firstChild=static_cast<int>(i);
        else nodes[last].nextSibling=static_cast<int>(i);
        last=static_cast<int>(i);
    }
    for (std::size_t i=0;i<objects.size();++i) {
        auto& object=objects[i]; object.nextSibling=object.firstDecal=-1;
        int& last=lastObject[object.nodeIndex];
        if (last<0) nodes[object.nodeIndex].firstObject=static_cast<int>(i);
        else objects[last].nextSibling=static_cast<int>(i);
        last=static_cast<int>(i);
    }
    for (std::size_t i=0;i<decals.size();++i) {
        auto& decal=decals[i]; decal.nextSibling=-1;
        int& last=lastDecal[decal.objectIndex];
        if (last<0) objects[decal.objectIndex].firstDecal=static_cast<int>(i);
        else decals[last].nextSibling=static_cast<int>(i);
        last=static_cast<int>(i);
        // The decompiler casts this to TSMesh::mCenter[1]; offset 0x24 is
        // TSDecalMesh::texgenSCount in the recovered decal layout.
        for (int j=0;j<decal.numMeshes;++j) {
            auto& mesh=assembledMeshes[static_cast<std::size_t>(decal.startMeshIndex+j)];
            if (mesh.type==DTSMeshDecal && mesh.decal.texgenS.empty()) {
                destroyTSDecalMesh(&mesh.decal);
                mesh.type=DTSMeshNull;
            }
        }
    }
    return true;
}
bool TSShape::initializeDetailMetadata() {
    for (const auto& detail:details) {
        if (detail.subShape<0) continue;
        const std::size_t sub=static_cast<std::size_t>(detail.subShape);
        if (sub>=firstObjects.size() || sub>=numObjects.size() || firstObjects[sub]<0 || numObjects[sub]<0 ||
            static_cast<std::size_t>(firstObjects[sub])>objects.size() ||
            static_cast<std::size_t>(numObjects[sub])>objects.size()-static_cast<std::size_t>(firstObjects[sub]) || detail.objectDetail<0) return false;
        for (int j=0;j<numObjects[sub];++j) {
            const auto& object=objects[static_cast<std::size_t>(firstObjects[sub]+j)];
            if (object.numMeshes<0 || object.startMeshIndex<0) return false;
            if (detail.objectDetail<object.numMeshes &&
                static_cast<std::size_t>(object.startMeshIndex)+static_cast<std::size_t>(detail.objectDetail)>=assembledMeshes.size()) return false;
        }
    }
    shapeFlags=0;
    for (const auto& sequence:sequences) {
        std::uint32_t scale=sequence.flags & 7u;
        if (scale>(shapeFlags&7u)) shapeFlags=(shapeFlags&~7u)|scale;
    }
    for (auto& detail:details) {
        if (detail.subShape<0) continue;
        int polygons=0;
        const auto sub=static_cast<std::size_t>(detail.subShape);
        for (int j=0;j<numObjects[sub];++j) {
            const auto& object=objects[static_cast<std::size_t>(firstObjects[sub]+j)];
            if (detail.objectDetail>=object.numMeshes) continue;
            const auto& mesh=assembledMeshes[static_cast<std::size_t>(object.startMeshIndex+detail.objectDetail)];
            switch (mesh.type) {
            case DTSMeshStandard: polygons+=mesh.base.getNumPolys(); break;
            case DTSMeshSkin: polygons+=mesh.skin.getNumPolys(); break;
            case DTSMeshDecal: polygons+=mesh.decal.getNumPolys(); break;
            case DTSMeshSorted: polygons+=mesh.sorted.getNumPolys(); break;
            case DTSMeshNull: break;
            default: return false;
            }
        }
        detail.polyCount=polygons;
    }
    return true;
}
void TSShape::initializeDetailAlpha() {
    alphaIn.resize(details.size()); alphaOut.resize(details.size());
    for (std::size_t i=0;i<details.size();++i) {
        if (!(details[i].size>=0.0f)) { alphaIn[i]=alphaOut[i]=0.0f; continue; }
        if (i+1==details.size() || details[i+1].size<0.0f) {
            alphaIn[i]=0.0f; alphaOut[i]=lastDetailFadeOut;
        } else if (details[i+1].subShape<0) {
            alphaIn[i]=billboardFadeIn; alphaOut[i]=billboardFadeOut;
        } else {
            alphaIn[i]=normalFadeIn; alphaOut[i]=normalFadeOut;
        }
    }
}
bool TSShape::initializeShape(unsigned skipDetail) {
    if (!initializeHierarchy()) return false;
    // The alpha values use the original detail descriptors before skip remaps.
    initializeDetailAlpha();
    // Assembly has already omitted skipped mesh slots. 004cbcd0 only aliases
    // detail descriptors and skin ranges here; it does not run checkSkip or
    // discard another set of meshes while initializing an existing shape.
    const int minimum=static_cast<int>(skipDetail);
    if (smallestVisibleDetail>0) {
        if (static_cast<std::size_t>(smallestVisibleDetail)>=details.size()) return false;
        for (int detail=smallestVisibleDetail-1;detail>=0;--detail) {
            if (detail>=minimum) continue;
            const DTSDetail& next=details[static_cast<std::size_t>(detail+1)];
            DTSDetail& current=details[static_cast<std::size_t>(detail)];
            if (current.subShape!=next.subShape || current.objectDetail!=next.objectDetail) {
                current.subShape=next.subShape;
                current.objectDetail=next.objectDetail;
            }
        }
        const int skinCutoff=std::min(minimum,smallestVisibleDetail);
        if (skinCutoff>0) {
            if (detailFirstSkin.size()!=details.size() || detailNumSkins.size()!=details.size()) return false;
            for (int detail=skinCutoff-1;detail>=0;--detail) {
                detailFirstSkin[static_cast<std::size_t>(detail)]=
                    detailFirstSkin[static_cast<std::size_t>(detail+1)];
                detailNumSkins[static_cast<std::size_t>(detail)]=
                    detailNumSkins[static_cast<std::size_t>(detail+1)];
            }
        }
    }
    return initializeDetailMetadata() && initializeMaterialList();
}
void TSShape::init() { initializeShape(shapeSkipLoadDetails); }
void TSShape::destroy() {
    materialList.free();
    // 004cb9b0 destroys decal-owned slots first and clears those slots before
    // visiting the remaining mesh table. Preserve that lifetime ordering even
    // though the normalized records own their storage independently.
    for (const auto& decal:decals) {
        for (int index=0;index<decal.numMeshes;++index) {
            const auto slot=static_cast<std::size_t>(decal.startMeshIndex+index);
            // A failed normalized read can leave hierarchy records populated
            // before any mesh records have been assembled.
            if (slot>=assembledMeshes.size()) break;
            auto& mesh=assembledMeshes[slot];
            if (mesh.type!=DTSMeshNull) {
                destroyTSDecalMesh(&mesh.decal);
                mesh.type=DTSMeshNull;
            }
        }
    }
    for (auto& mesh:assembledMeshes) {
        switch (mesh.type) {
        case DTSMeshStandard: destroyTSMesh(&mesh.base); break;
        case DTSMeshSkin: destroyTSSkinMesh(&mesh.skin); break;
        case DTSMeshDecal: destroyTSDecalMesh(&mesh.decal); break;
        case DTSMeshSorted: destroyTSMesh(&mesh.sorted); break;
        default: break;
        }
    }
    for (auto& skin:assembledSkins)
        if (skin.type!=DTSMeshNull) destroyTSSkinMesh(&skin.skin);
    // These normalized records own vectors where the executable owns the
    // single shape arena. Drop them at the same point the original releases
    // its memory block so an explicit destroy does not retain mesh storage.
    assembledMeshes.clear();
    assembledSkins.clear();
    billboardDetails.clear();
    ownedNameIndices.clear();
    names.clear();
    // The original frees the single shape memory block and each separately
    // allocated table here. Replacing the normalized record with its default
    // value releases the corresponding vectors and resets all read state.
    *this=TSShape();
}
void TSShape::setupBillboardDetails(char* bitmapBaseName) {
    // The original allocates a pointer slot for every detail through the last
    // negative-subshape detail, leaving ordinary-detail slots null.
    if (!billboardDetails.empty()) return;
    for (std::size_t index=0;index<details.size();++index) {
        const DTSDetail& detail=details[index];
        if (detail.subShape>=0) continue;
        if (billboardDetails.size()<=index) billboardDetails.resize(index+1);
        const std::uint32_t packed=static_cast<std::uint32_t>(detail.objectDetail);
        std::unique_ptr<TSLastDetail> billboard(new (std::nothrow) TSLastDetail());
        if (billboard) {
            billboard->construct(this,packed&0x7fu,(packed>>7)&0x3fu,
                static_cast<float>(static_cast<long double>((packed>>13)&0x3fu)*
                                   0.02454369260617026L),
                static_cast<char>(packed>>31),static_cast<int>((packed>>19)&0xfu),
                static_cast<int>((packed>>23)&0xffu),bitmapBaseName);
        }
        billboardDetails[index]=std::move(billboard);
    }
}
void TSShape::initMaterialList() { initializeMaterialList(); }
bool TSShape::initializeMaterialList() {
    // Preserve the saved 004cc1e0 loop literally: it initializes and compares
    // the cursor against subShapeNumObjects[sub] without adding
    // subShapeFirstObject[sub]. Other shape paths treat that field as a count
    // and use first + count; this initializer's direct bound is executable-
    // evidenced behavior.
    firstTranslucentObjects.resize(firstObjects.size());
    for (std::size_t sub=0;sub<firstObjects.size();++sub) {
        int objectIndex=firstObjects[sub];
        const int objectEnd=numObjects[sub];
        firstTranslucentObjects[sub]=objectEnd;
        // The recovered outer do/while advances to the next subshape only
        // when the inner cursor equals the stored bound. A start greater than
        // the bound terminates the outer traversal.
        if (objectIndex>objectEnd) break;
        if (objectIndex==objectEnd) continue;
        do {
            const auto& object=objects[static_cast<std::size_t>(objectIndex)];
            int meshIndex=0;
            for (;meshIndex<object.numMeshes;++meshIndex) {
                const auto& mesh=assembledMeshes[static_cast<std::size_t>(object.startMeshIndex+meshIndex)];
                // The original mesh pointer table can contain null slots.
                // The saved loop skips those slots and keeps scanning this
                // object's remaining mesh entries.
                if (mesh.type==DTSMeshNull) continue;
                const std::vector<TSDrawPrimitive>* primitives=nullptr;
                switch (mesh.type) {
                case DTSMeshStandard: case DTSMeshSkin: case DTSMeshSorted: primitives=&mesh.base.primitives; break;
                case DTSMeshDecal: primitives=&mesh.decal.primitives; break;
                default: break;
                }
                if (!primitives) continue;
                bool foundTranslucent=false;
                for (const auto& primitive:*primitives) {
                    const std::uint32_t material=primitive.material;
                    if (hasDrawPrimitiveFlag(material,TSDrawPrimitiveFlag::DynamicMaterial)) continue;
                    const std::uint32_t flags=materialList.getFlags(
                        static_cast<int>(drawPrimitiveMaterialIndex(material)));
                    if (!(flags&0xe0000000u) && (flags&4u)) {
                        shapeFlags|=0x80u;
                        firstTranslucentObjects[sub]=objectIndex;
                        foundTranslucent=true;
                        break;
                    }
                }
                if (foundTranslucent) break;
            }
            if (meshIndex!=object.numMeshes) break;
            ++objectIndex;
        } while (objectIndex<objectEnd);
        if (objectIndex!=objectEnd) break;
    }
    return true;
}

void TSShape::readIflMaterials() {
    if ((shapeFlags&0x40u) || !iflOpenFile || !iflCloseFile) return;
    for (auto& ifl:iflMaterials) {
        ifl.firstFrame=static_cast<int>(materialList.size());
        ifl.firstFrameOffTimeIndex=static_cast<int>(iflFrameOffTimes.size()); ifl.numFrames=0;
        if (ifl.nameIndex<0 || static_cast<std::size_t>(ifl.nameIndex)>=names.size()) { ifl.firstFrame=ifl.materialSlot; continue; }
        auto& name=names[ifl.nameIndex]; for (auto& c:name) if (c=='\\') c='/';
        bool defaultSkin=std::strcmp(iflSkinPrefix,materialSkinPrefix)==0;
        std::string path=iflBaseTextures;
        // The saved branch uses base_textures/<name> only when the read
        // version is outside 0..15 AND the configured skin prefix differs
        // from the default. In particular, the pre-read sentinel -1 does not
        // take the legacy skins/ path unless the prefix itself is default.
        const bool useSkinSubdirectory=defaultSkin || (readVersion>=0 && readVersion<=15);
        if (useSkinSubdirectory) path+=iflSkinPrefix;
        path+=name;
        ResourceFile* file=iflOpenFile(path.c_str(),"r");
        if (!file) { ifl.firstFrame=ifl.materialSlot; continue; }
        char line[512]; std::uint32_t accumulated=0;
        while (file->getLine(line,sizeof(line))) {
            if (file->error()) continue;
            for (char* p=line;*p;++p) if (*p=='\t') *p=' ';
            char* nameStart=line; while (*nameStart==' ') ++nameStart;
            char* space=std::strchr(nameStart,' '); int duration=1;
            if (space) { *space++='\0'; while (*space==' ') ++space; duration=static_cast<int>(std::strtol(space,nullptr,10)); if (!duration) duration=1; }
            if (!*nameStart) continue;
            const std::size_t frameNameLength=std::min(std::strlen(nameStart),std::size_t(255));
            const std::string frameName(nameStart,frameNameLength);
            accumulated+=static_cast<std::uint32_t>(duration);
            iflFrameOffTimes.push_back(static_cast<float>(accumulated)*0.033333335f);
            std::string materialName=defaultSkin ? frameName : std::string(iflSkinPrefix)+frameName;
            materialList.push_back(&materialName[0],materialFlag(TSMaterialFlag::DynamicMaterial),
                -1,-1,-1,1,1); ++ifl.numFrames;
        }
        iflCloseFile(file);
    }
    initMaterialList(); shapeFlags|=0x40u;
}
bool TSShape::checkSkip(int meshIndex,int* objectCursor,unsigned* decalCursor,unsigned detailIndex) {
    if (!detailIndex) return false;
    const auto& detail=details[detailIndex];
    const int sub=detail.subShape;
    while (*objectCursor<static_cast<int>(objects.size())) {
        const auto& object=objects[*objectCursor];
        if (meshIndex<object.startMeshIndex) break;
        if (meshIndex<object.startMeshIndex+object.numMeshes) {
            if (*objectCursor<firstObjects[sub]) return true;
            return (sub+1==static_cast<int>(firstObjects.size()) || *objectCursor<firstObjects[sub+1]) &&
                meshIndex-object.startMeshIndex<detail.objectDetail;
        }
        ++*objectCursor;
    }
    while (*decalCursor<decals.size()) {
        const auto& decal=decals[*decalCursor];
        if (meshIndex<decal.startMeshIndex) break;
        if (meshIndex<decal.startMeshIndex+decal.numMeshes) {
            if (static_cast<int>(*decalCursor)<firstDecals[sub]) return true;
            return (sub+1==static_cast<int>(firstDecals.size()) || static_cast<int>(*decalCursor)<firstDecals[sub+1]) &&
                meshIndex-decal.startMeshIndex<detail.objectDetail;
        }
        ++*decalCursor;
    }
    return false;
}
bool TSShape::checkSkip(int meshIndex,int& objectCursor,int& decalCursor,unsigned detailIndex,bool& skip) const {
    skip=false;
    if (!detailIndex) return true;
    if (detailIndex>=details.size() || meshIndex<0 || objectCursor<0 || decalCursor<0) return false;
    int sub=details[detailIndex].subShape, level=details[detailIndex].objectDetail;
    if (sub<0 || static_cast<std::size_t>(sub)>=firstObjects.size() || static_cast<std::size_t>(sub)>=firstDecals.size()) return false;
    auto select=[&](int index,int first,const std::vector<std::int32_t>& firsts) {
        if (index<firsts[sub]) return true;
        if (static_cast<std::size_t>(sub+1)==firsts.size() || index<firsts[sub+1]) return meshIndex-first<level;
        return false;
    };
    while (static_cast<std::size_t>(objectCursor)<objects.size()) {
        const auto& object=objects[objectCursor];
        if (meshIndex<object.startMeshIndex) break;
        if (object.startMeshIndex<0 || object.numMeshes<0) return false;
        if (meshIndex-object.startMeshIndex<object.numMeshes) {
            skip=select(objectCursor,object.startMeshIndex,firstObjects); return true;
        }
        ++objectCursor;
    }
    while (static_cast<std::size_t>(decalCursor)<decals.size()) {
        const auto& decal=decals[decalCursor];
        if (meshIndex<decal.startMeshIndex) break;
        if (decal.startMeshIndex<0 || decal.numMeshes<0) return false;
        if (meshIndex-decal.startMeshIndex<decal.numMeshes) {
            skip=select(decalCursor,decal.startMeshIndex,firstDecals); return true;
        }
        ++decalCursor;
    }
    return true;
}
bool TSShape::applyDetailSkip(unsigned requested) {
    if (!requested) return true;
    if (smallestVisibleDetail<0 || static_cast<std::size_t>(smallestVisibleDetail)>=details.size()) return false;
    unsigned effective=requested<static_cast<unsigned>(smallestVisibleDetail) ? requested : static_cast<unsigned>(smallestVisibleDetail);
    if (!effective) return true;
    if (detailFirstSkin.size()!=details.size() || detailNumSkins.size()!=details.size()) return false;
    std::vector<bool> skipped(assembledMeshes.size(),false);
    int object=0; unsigned decal=0;
    for (std::size_t i=0;i<assembledMeshes.size();++i) {
        int nextObject=object,nextDecal=static_cast<int>(decal); bool skip=false;
        if (!checkSkip(static_cast<int>(i),nextObject,nextDecal,effective,skip)) return false;
        object=nextObject; decal=static_cast<unsigned>(nextDecal);
        skipped[i]=skip;
    }
    for (std::size_t i=0;i<assembledMeshes.size();++i)
        if (skipped[i]) assembledMeshes[i].type=DTSMeshNull;
    // 004cc4a0 also skips skin meshes before the first skin for the selected
    // detail. Keep the normalized records in their slots, with null-mesh type.
    const int firstRetainedSkin=detailFirstSkin[effective];
    for (std::size_t i=0;i<assembledSkins.size() && static_cast<int>(i)<firstRetainedSkin;++i)
        assembledSkins[i].type=DTSMeshNull;
    for (int i=smallestVisibleDetail-1;i>=0;--i) if (static_cast<unsigned>(i)<requested) {
        details[i].subShape=details[i+1].subShape;
        details[i].objectDetail=details[i+1].objectDetail;
    }
    for (int i=static_cast<int>(effective)-1;i>=0;--i) {
        detailFirstSkin[i]=detailFirstSkin[i+1]; detailNumSkins[i]=detailNumSkins[i+1];
    }
    return true;
}
bool TSShape::convertMeshesToTriangles() {
    auto convert=[](DTSAssembledMesh& mesh) {
        switch (mesh.type) {
        case DTSMeshStandard: return mesh.base.convertToTriangles();
        case DTSMeshSkin: if (!mesh.skin.convertToTriangles()) return false; mesh.base=mesh.skin; return true;
        // The sorted-mesh assembler temporarily disables triangle and single-strip
        // conversion, selecting the multiple-strip path with the global threshold.
        case DTSMeshSorted: if (!mesh.sorted.leaveAsMultipleStrips(minimumStripTriangles)) return false; mesh.base=mesh.sorted; return true;
        case DTSMeshDecal: case DTSMeshNull: return true;
        default: return false;
        }
    };
    for (auto& mesh:assembledMeshes) if (!convert(mesh)) return false;
    for (auto& mesh:assembledSkins) if (!convert(mesh)) return false;
    return true;
}
bool TSShape::convertMeshesToMultipleStrips(int minimum) {
    auto convert=[&](DTSAssembledMesh& mesh) {
        switch (mesh.type) {
        case DTSMeshStandard: return mesh.base.leaveAsMultipleStrips(minimum);
        case DTSMeshSkin: if (!mesh.skin.leaveAsMultipleStrips(minimum)) return false; mesh.base=mesh.skin; return true;
        case DTSMeshSorted: if (!mesh.sorted.leaveAsMultipleStrips(minimum)) return false; mesh.base=mesh.sorted; return true;
        case DTSMeshDecal: case DTSMeshNull: return true;
        default: return false;
        }
    };
    for (auto& mesh:assembledMeshes) if (!convert(mesh)) return false;
    for (auto& mesh:assembledSkins) if (!convert(mesh)) return false;
    return true;
}
bool TSShape::convertMeshesToSingleStrip(int minimum) {
    auto convert=[&](DTSAssembledMesh& mesh) {
        switch (mesh.type) {
        case DTSMeshStandard: return mesh.base.convertToSingleStrip(minimum);
        case DTSMeshSkin: if (!mesh.skin.convertToSingleStrip(minimum)) return false; mesh.base=mesh.skin; return true;
        case DTSMeshSorted: if (!mesh.sorted.leaveAsMultipleStrips(minimumStripTriangles)) return false; mesh.base=mesh.sorted; return true;
        case DTSMeshDecal: case DTSMeshNull: return true;
        default: return false;
        }
    };
    for (auto& mesh:assembledMeshes) if (!convert(mesh)) return false;
    for (auto& mesh:assembledSkins) if (!convert(mesh)) return false;
    return true;
}
bool TSShape::assembleLegacyMeshes(bool encoded,unsigned skipDetail,bool convertTriangles,int minStripTriangles) {
    if (readVersion>=19) return false;
    auto setup=[&](TSShapeAlloc& allocator,const DTSLegacyMeshRange& range,bool hasType) {
        std::size_t first32=static_cast<std::size_t>(range.word32Offset)+(hasType ? 1u : 0u);
        if (first32>=meshStreams.words32.size() || range.word16Offset>=meshStreams.words16.size() || range.byte8Offset>=meshStreams.bytes8.size()) return false;
        // Each serialized mesh has its own input cursors, but all meshes
        // share one output reservation. setRead resets allocator size, so
        // preserve the pass-wide destination and byte count around it.
        auto* destination=allocator.destination;
        const std::uint32_t size=allocator.size,multiplier=allocator.multiplier;
        allocator.setRead(meshStreams.words32.data()+first32,meshStreams.words16.data()+range.word16Offset,meshStreams.bytes8.data()+range.byte8Offset,false);
        allocator.destination=destination; allocator.size=size; allocator.multiplier=multiplier;
        // Tail-only streams start at a later guard than the original full shape.
        allocator.guard32=meshStreams.words32[first32];
        allocator.guard16=meshStreams.words16[range.word16Offset];
        allocator.guard8=meshStreams.bytes8[range.byte8Offset];
        return true;
    };
    if (skipDetail && (smallestVisibleDetail<0 || static_cast<std::size_t>(smallestVisibleDetail)>=details.size())) return false;
    unsigned effectiveSkip=skipDetail ? std::min(skipDetail,static_cast<unsigned>(smallestVisibleDetail)) : 0;
    if (effectiveSkip && effectiveSkip>=detailFirstSkin.size()) return false;
    TSShapeAlloc allocator;
    auto assemblePass=[&]() {
    legacyGuardMismatches=0;
    int objectCursor=0,decalCursor=0;
    assembledMeshes.clear(); assembledMeshes.reserve(meshes.size());
    std::vector<TSMesh> priorBases; std::vector<TSSkinMesh> priorSkins; std::vector<std::uint8_t> retained;
    priorBases.reserve(meshes.size()); priorSkins.reserve(meshes.size());
    for (const auto& range:meshes) {
        DTSAssembledMesh record; record.type=range.type;
        bool skip=false;
        if (!checkSkip(static_cast<int>(assembledMeshes.size()),objectCursor,decalCursor,effectiveSkip,skip)) return false;
        if (range.type!=DTSMeshNull) {
            if (!setup(allocator,range,true)) return false;
            if (!assembleMeshWithContext(record,allocator,readVersion,encoded,skip,convertTriangles,minStripTriangles,priorBases,priorSkins,retained)) return false;
            if (allocator.guardMismatch) ++legacyGuardMismatches;
        }
        retained.push_back(!skip && record.type!=DTSMeshNull && record.type!=DTSMeshDecal); appendMeshScratch(record,priorBases,priorSkins); if (skip) record.type=DTSMeshNull; assembledMeshes.push_back(std::move(record));
    }
    assembledSkins.clear(); assembledSkins.reserve(skins.size()); priorBases.clear(); priorSkins.clear(); retained.clear();
    priorBases.reserve(skins.size()); priorSkins.reserve(skins.size());
    for (const auto& range:skins) {
        if (!setup(allocator,range,false)) return false;
        DTSAssembledMesh record; record.type=DTSMeshSkin;
        bool skip=effectiveSkip && static_cast<int>(assembledSkins.size())<detailFirstSkin[effectiveSkip];
        if (!assembleMeshWithContext(record,allocator,readVersion,encoded,skip,convertTriangles,minStripTriangles,priorBases,priorSkins,retained)) return false;
        if (allocator.guardMismatch) ++legacyGuardMismatches;
        retained.push_back(!skip); record.base=record.skin; appendMeshScratch(record,priorBases,priorSkins,true); if (skip) record.type=DTSMeshNull; assembledSkins.push_back(std::move(record));
    }
    return true;
    };
    if (!assemblePass()) return false;
    assemblyArenaBytes=allocator.size;
    assemblyArena.resize((static_cast<std::size_t>(assemblyArenaBytes)+7)/8+1);
    allocator.destination=reinterpret_cast<std::uint8_t*>(assemblyArena.data());
    allocator.size=0;
    if (!assemblePass() || allocator.size!=assemblyArenaBytes) return false;
    return true;
}
bool TSShape::assembleShapePass(TSShapeAlloc& allocator,bool legacy,bool encoded,
        unsigned skipDetail,bool convertTriangles,int minStripTriangles) {
    if (legacy) {
        // The legacy allocator has three independent input cursors and mesh
        // ranges, so its pass pair is encapsulated by the range assembler.
        // The common read entry still owns format dispatch and initialization.
        return assembleLegacyMeshes(encoded,skipDetail,convertTriangles,minStripTriangles);
    }
    return readSplitAssemblyCore(allocator) &&
        readSplitAssemblyTail(allocator,encoded,skipDetail,convertTriangles,minStripTriangles,false) &&
        !allocator.guardMismatch;
}
bool TSShape::readSplitData(ResourceFile& file,const char* legacyMaterialPrefix) {
    if (readVersion<19 || readVersion>22) return false;
    std::uint32_t totalWords=0,offset16=0,offset8=0;
    if (file.read(&totalWords,4,1)!=1 || file.read(&offset16,4,1)!=1 || file.read(&offset8,4,1)!=1) return false;
    if (!totalWords || offset16>offset8 || offset8>totalWords) return false;
    splitWords.resize(totalWords);
    if (file.read(splitWords.data(),4,totalWords)!=totalWords) return false;
    split16Offset=offset16; split8Offset=offset8;
    std::uint32_t count=0;
    if (file.read(&count,4,1)!=1) return false;
    sequences.clear(); sequences.resize(count); legacyFirstKeyframeScratch.clear();
    currentShapeReadVersion=readVersion;
    for (auto& sequence:sequences) { sequence.read(&file,1); if (file.eof() || file.error()) return false; }
    legacyFirstKeyframes=legacyFirstKeyframeScratch;
    dts::legacyMaterialPrefix=legacyMaterialPrefix;
    currentShapeReadVersion=readVersion;
    if (!materialList.read(&file)) return false;
    hasMaterials=true;
    return true;
}
void TSShape::setSplitAssemblyRead(TSShapeAlloc& allocator,bool sizing) {
    allocator.setRead(splitWords.data(),splitWords.data()+split16Offset,splitWords.data()+split8Offset,sizing);
}
bool TSShape::readSplitAssemblyCore(TSShapeAlloc& a) {
    if (readVersion<19 || readVersion>22) return false;
    auto number=[&]() { return a.get32(); };
    auto real=[&]() { std::uint32_t word=number(); float value; std::memcpy(&value,&word,4); return value; };
    // Until the normalized fields are replaced by arena-backed records, keep
    // their values while making the original allocator perform the copy/size.
    auto copiedNumber=[&](char returnSource) {
        std::uint32_t value; std::memcpy(&value,a.buffer32,4);
        a.copyToShape32(1,returnSource); return value;
    };
    auto copiedReal=[&]() { auto word=copiedNumber(0); float value; std::memcpy(&value,&word,4); return value; };
    auto copiedShort=[&]() {
        std::uint16_t value; std::memcpy(&value,a.buffer16,2);
        a.copyToShape16(1,0); return value;
    };
    std::uint32_t nodeCount=number(),objectCount=number(),decalCount=number(),subCount=number(),iflCount=number();
    std::uint32_t rotationCount=0,translationCount=0,uniformCount=0,alignedCount=0,arbitraryCount=0;
    if (readVersion<22) {
        std::uint32_t combined=number(); if (combined<nodeCount) return false;
        rotationCount=translationCount=combined-nodeCount;
    } else { rotationCount=number(); translationCount=number(); uniformCount=number(); alignedCount=number(); arbitraryCount=number(); }
    std::uint32_t objectStateCount=number(),decalStateCount=number(),triggerCount=number(),detailCount=number();
    assemblyMeshCount=number(); assemblySkinCount=number(); assemblyNameCount=number();
    smallestVisibleSize=static_cast<float>(static_cast<std::int32_t>(number()));
    smallestVisibleDetail=static_cast<std::int32_t>(number());
    a.checkGuard(); radius=real(); tubeRadius=real();
    for (float& v:center) v=real(); for (float& v:bounds) v=real(); a.checkGuard();
    nodes.resize(nodeCount);
    for (auto& n:nodes) { n.nameIndex=copiedNumber(0); n.parentIndex=copiedNumber(0); n.firstObject=copiedNumber(0); n.firstChild=copiedNumber(0); n.nextSibling=copiedNumber(0); }
    a.checkGuard(); objects.resize(objectCount);
    for (auto& o:objects) { o.nameIndex=copiedNumber(1); o.numMeshes=copiedNumber(1); o.startMeshIndex=copiedNumber(1); o.nodeIndex=copiedNumber(1); o.nextSibling=copiedNumber(1); o.firstDecal=copiedNumber(1); }
    a.checkGuard(); decals.resize(decalCount);
    for (auto& d:decals) { d.nameIndex=copiedNumber(1); d.numMeshes=copiedNumber(1); d.startMeshIndex=copiedNumber(1); d.objectIndex=copiedNumber(1); d.nextSibling=copiedNumber(1); }
    a.checkGuard(); iflMaterials.resize(iflCount);
    for (auto& m:iflMaterials) { m.nameIndex=copiedNumber(0); m.materialSlot=copiedNumber(0); m.firstFrame=copiedNumber(0); m.firstFrameOffTimeIndex=copiedNumber(0); m.numFrames=copiedNumber(0); }
    a.checkGuard();
    firstNodes.resize(subCount); firstObjects.resize(subCount); firstDecals.resize(subCount);
    for (auto& v:firstNodes) v=copiedNumber(1); for (auto& v:firstObjects) v=copiedNumber(1); for (auto& v:firstDecals) v=copiedNumber(1);
    a.checkGuard(); numNodes.resize(subCount); numObjects.resize(subCount); numDecals.resize(subCount);
    for (auto& v:numNodes) v=copiedNumber(0); for (auto& v:numObjects) v=copiedNumber(0); for (auto& v:numDecals) v=copiedNumber(0);
    a.checkGuard();
    a.allocShape32(static_cast<int>(subCount)); // first translucent object per subshape
    defaultRotations.resize(nodeCount);
    for (auto& q:defaultRotations) { q.x=copiedShort(); q.y=copiedShort(); q.z=copiedShort(); q.w=copiedShort(); }
    a.align32();
    defaultTranslations.resize(nodeCount);
    for (auto& p:defaultTranslations) { p.x=copiedReal(); p.y=copiedReal(); p.z=copiedReal(); }
    nodeTranslations.resize(translationCount);
    for (auto& p:nodeTranslations) { p.x=real(); p.y=real(); p.z=real(); }
    nodeRotations.resize(rotationCount);
    for (auto& q:nodeRotations) { q.x=a.get16(); q.y=a.get16(); q.z=a.get16(); q.w=a.get16(); }
    a.align32();
    a.checkGuard();
    nodeUniformScales.clear(); nodeAlignedScales.clear(); nodeArbitraryScaleFactors.clear(); nodeArbitraryScaleRotations.clear();
    if (readVersion>=22) {
        nodeUniformScales.resize(uniformCount); for (auto& value:nodeUniformScales) value=real();
        nodeAlignedScales.resize(alignedCount); for (auto& p:nodeAlignedScales) { p.x=real(); p.y=real(); p.z=real(); }
        nodeArbitraryScaleFactors.resize(arbitraryCount); for (auto& p:nodeArbitraryScaleFactors) { p.x=real(); p.y=real(); p.z=real(); }
        nodeArbitraryScaleRotations.resize(arbitraryCount);
        for (auto& q:nodeArbitraryScaleRotations) { q.x=a.get16(); q.y=a.get16(); q.z=a.get16(); q.w=a.get16(); }
        a.align32();
        a.checkGuard();
    }
    groundTranslations.clear(); groundRotations.clear();
    if (readVersion<22) {
        for (auto& sequence:sequences) {
            if (sequence.numGroundFrames<0 || (sequence.numGroundFrames && sequence.firstGroundFrame<static_cast<std::int32_t>(nodeCount))) return false;
            std::size_t first=sequence.numGroundFrames ? static_cast<std::size_t>(sequence.firstGroundFrame-static_cast<std::int32_t>(nodeCount)) : 0;
            std::size_t count=static_cast<std::size_t>(sequence.numGroundFrames);
            if (first>nodeTranslations.size() || count>nodeTranslations.size()-first || first>nodeRotations.size() || count>nodeRotations.size()-first) return false;
            sequence.firstGroundFrame=static_cast<std::int32_t>(groundTranslations.size());
            groundTranslations.insert(groundTranslations.end(),nodeTranslations.begin()+first,nodeTranslations.begin()+first+count);
            groundRotations.insert(groundRotations.end(),nodeRotations.begin()+first,nodeRotations.begin()+first+count);
            sequence.baseRotation-=static_cast<std::int32_t>(nodeCount); sequence.baseTranslation-=static_cast<std::int32_t>(nodeCount); sequence.baseScale=0;
        }
    }
    objectStates.resize(objectStateCount);
    for (auto& state:objectStates) { state.visibility=copiedReal(); state.frameIndex=copiedNumber(0); state.materialFrameIndex=copiedNumber(0); }
    a.checkGuard(); decalStates.resize(decalStateCount); for (auto& state:decalStates) state=copiedNumber(0);
    a.checkGuard(); triggers.resize(triggerCount); for (auto& trigger:triggers) { trigger.state=number(); trigger.position=real(); }
    a.checkGuard(); details.resize(detailCount);
    for (auto& detail:details) { detail.nameIndex=copiedNumber(1); detail.subShape=copiedNumber(1); detail.objectDetail=copiedNumber(1); auto readDetailFloat=[&]() { auto word=copiedNumber(1); float v; std::memcpy(&v,&word,4); return v; }; detail.size=readDetailFloat(); detail.averageError=readDetailFloat(); detail.maxError=readDetailFloat(); detail.polyCount=copiedNumber(1); }
    a.checkGuard();
    return true;
}
bool TSShape::readSplitAssemblyTail(TSShapeAlloc& a,bool encoded,unsigned skipDetail,bool convertTriangles,int minStripTriangles,bool initialize) {
    if (readVersion<19 || readVersion>22) return false;
    if (skipDetail && (smallestVisibleDetail<0 || static_cast<std::size_t>(smallestVisibleDetail)>=details.size())) return false;
    unsigned effectiveSkip=skipDetail ? std::min(skipDetail,static_cast<unsigned>(smallestVisibleDetail)) : 0;
    int objectCursor=0,decalCursor=0;
    assembledMeshes.clear(); assembledMeshes.reserve(assemblyMeshCount);
    auto** meshSlots=static_cast<void**>(a.allocShapePointers(static_cast<int>(assemblyMeshCount)));
    std::vector<TSMesh> priorBases; std::vector<TSSkinMesh> priorSkins; std::vector<std::uint8_t> retained;
    priorBases.reserve(assemblyMeshCount); priorSkins.reserve(assemblyMeshCount);
    for (std::uint32_t i=0;i<assemblyMeshCount;++i) {
        DTSAssembledMesh record; record.type=static_cast<DTSMeshKind>(a.get32());
        bool skip=false; if (!checkSkip(static_cast<int>(i),objectCursor,decalCursor,effectiveSkip,skip)) return false;
        if (!assembleMeshWithContext(record,a,readVersion,encoded,skip,convertTriangles,minStripTriangles,priorBases,priorSkins,retained)) return false;
        retained.push_back(!skip && record.type!=DTSMeshNull && record.type!=DTSMeshDecal); appendMeshScratch(record,priorBases,priorSkins); if (skip) record.type=DTSMeshNull; assembledMeshes.push_back(std::move(record));
        if (meshSlots) meshSlots[i]=skip ? nullptr : assembledMeshObject(assembledMeshes.back());
    }
    a.checkGuard(); names.clear(); ownedNameIndices.clear();
    for (std::uint32_t i=0;i<assemblyNameCount;++i) {
        const auto* start=a.buffer8; std::size_t length=0;
        while (start[length]) ++length;
        names.emplace_back(reinterpret_cast<const char*>(start),length);
        a.copyToShape8(static_cast<int>(length+1),0);
    }
    a.align32();
    a.checkGuard(); detailFirstSkin.resize(details.size()); detailNumSkins.resize(details.size());
    for (auto& value:detailFirstSkin) { std::memcpy(&value,a.buffer32,4); a.copyToShape32(1,1); }
    for (auto& value:detailNumSkins) { std::memcpy(&value,a.buffer32,4); a.copyToShape32(1,0); }
    a.checkGuard(); assembledSkins.clear(); assembledSkins.reserve(assemblySkinCount);
    auto** skinSlots=static_cast<void**>(a.allocShapePointers(static_cast<int>(assemblySkinCount)));
    priorBases.clear(); priorSkins.clear(); retained.clear(); priorBases.reserve(assemblySkinCount); priorSkins.reserve(assemblySkinCount);
    for (std::uint32_t i=0;i<assemblySkinCount;++i) {
        DTSAssembledMesh record; record.type=DTSMeshSkin;
        bool skip=effectiveSkip && static_cast<int>(i)<detailFirstSkin[effectiveSkip];
        if (!assembleMeshWithContext(record,a,readVersion,encoded,skip,convertTriangles,minStripTriangles,priorBases,priorSkins,retained)) return false;
        retained.push_back(!skip); record.base=record.skin; appendMeshScratch(record,priorBases,priorSkins,true); if (skip) record.type=DTSMeshNull; assembledSkins.push_back(std::move(record));
        if (skinSlots) skinSlots[i]=skip ? nullptr : &assembledSkins.back().skin;
    }
    a.checkGuard();
    a.allocShape32(static_cast<int>(details.size())); // alphaIn
    a.allocShape32(static_cast<int>(details.size())); // alphaOut
    return !initialize || initializeShape(skipDetail);
}
} }

// Address-level ABI adapters below use C linkage only to keep recovered
// symbols stable and searchable. This says nothing about the original
// executable's C/C++ linkage; the MEGUSTA_DTS_* annotation separately records
// the calling convention we are modeling. Inventory: docs/DTS_ABI_ADAPTERS.md.
extern "C" int FUN_004d71a0(std::uint32_t value) {
    return static_cast<int>(megusta::dts::bitmapPowerOfTwoDimension(value));
}
