#include "../src/dts/mesh.h"
#include "../src/dts/material_list.h"
#include "../src/dts/graphics_state.h"
#include "../src/dts/wrappers/graphics_state_wrappers.h"
#include "../src/engine/opengl_adapter.h"
#include <cassert>

static const void* vertexData;
static const void* indexData;
static int draws,modeSeen,countSeen;
static int colors,materialsUpdated;
static float alphaSeen;
static std::vector<int> stateCalls;
static int extensionReads;
static bool lightingEnabled;
static const char* extensionText="GL_ARB_multitexture GL_EXT_texture_env_combine";
// Exercise the actual material loader -> handle -> render binding path.
static int textureEntry,textureLoads,textureReferences;
static int fogTextureEntry;
static std::uint32_t lastTextureName;
static void* loadMaterialTexture(const char* name,int type,bool clamp) {
    assert(std::string(name)=="skins/test" && type==3 && clamp);
    ++textureLoads; return &textureEntry;
}
static bool hasMaterialBitmap(void* entry) { return entry==&textureEntry; }
static void retainMaterialTexture(void* entry) { assert(entry==&textureEntry); ++textureReferences; }
static void* releaseMaterialTexture(void* entry) {
    assert(entry==&textureEntry && textureReferences>0);
    return --textureReferences ? entry : nullptr;
}
static std::uint32_t materialTextureName(void* entry) {
    assert(entry==&textureEntry || entry==&fogTextureEntry);
    return entry==&fogTextureEntry ? 88 : 77;
}
namespace megusta {
void OpenGLAdapter::generateTextures(int,std::uint32_t*) { assert(false && "unexpected texture allocation in fog fixture"); }
void OpenGLAdapter::deleteTextures(int,const std::uint32_t*) { assert(false && "unexpected texture deletion in fog fixture"); }
void OpenGLAdapter::textureImage2D(int,int,int,int,int,int,int,int,const void*,int) {
    assert(false && "unexpected texture upload in fog fixture");
}
const char* OpenGLAdapter::getStringOrPlatform(int token) { assert(token==0xa8); ++extensionReads; return extensionText; }
void OpenGLAdapter::vertexPointer(int size,int type,int stride,const void* data) {
    assert(size==3 && type==3 && stride==0); vertexData=data;
}
void OpenGLAdapter::drawElements(int mode,int count,int type,const void* indices) {
    assert(type==2); ++draws; modeSeen=mode; countSeen=count; indexData=indices;
}
void OpenGLAdapter::color4fv(const float* color) { ++colors; alphaSeen=color[3]; }
void OpenGLAdapter::materialfv(int face,int mode,const float* color) {
    assert(face==0x15 && mode==0x74 && color[3]==alphaSeen); ++materialsUpdated;
}
void OpenGLAdapter::enable(int token) { stateCalls.push_back(0x100+token); if (token==0x67) lightingEnabled=true; }
void OpenGLAdapter::disable(int token) { stateCalls.push_back(0x200+token); if (token==0x67) lightingEnabled=false; }
void OpenGLAdapter::disableClientState(int token) { stateCalls.push_back(0x300+token); }
void OpenGLAdapter::depthMask(bool value) { stateCalls.push_back(value ? 0x401 : 0x400); }
void OpenGLAdapter::frontFace(int token) { assert(token==0x14); stateCalls.push_back(0x500); }
void OpenGLAdapter::enableClientState(int token) { stateCalls.push_back(0x600+token); }
void OpenGLAdapter::texGeni(int coordinate,int name,int value) {
    assert(name==0x5c && (value==0x61 || value==0x5f)); stateCalls.push_back(0x700+coordinate);
}
void OpenGLAdapter::texParameteri(int target,int name,int value) { assert(target==0x4a && (name==0x4b || name==0x4c) && value==0x58); }
void OpenGLAdapter::bindTexture(int target,std::uint32_t texture) { assert(target==0x4a && (texture==77 || texture==88)); lastTextureName=texture; stateCalls.push_back(0xd00); }
void OpenGLAdapter::polygonOffset(float factor,float units) { assert(factor==-2 && units==-2); stateCalls.push_back(0x800); }
void OpenGLAdapter::texEnvi(int target,int name,int value) { assert(target==0x48); if (name==0x49) stateCalls.push_back(0x900+value); }
void OpenGLAdapter::texEnvfv(int target,int name,const float* values) {
    assert(target==0x48 && name==100 && values==megusta::dts::dtsFogColor);
}
void OpenGLAdapter::blendFunc(int source,int dest) { assert(source==0x37 && dest==0x38); stateCalls.push_back(0x802); }
void OpenGLAdapter::activeTexture(int token) { stateCalls.push_back(0xa00+token); }
void OpenGLAdapter::clientActiveTexture(int token) { stateCalls.push_back(0xb00+token); }
bool OpenGLAdapter::isEnabled(int token) { assert(token==0x67); stateCalls.push_back(0xc67); return lightingEnabled; }
}
int main() {
    using namespace megusta::dts;
    megusta::OpenGLAdapter adapter(static_cast<const std::uint32_t*>(nullptr));
    dtsRenderGL=&adapter;
    dtsExtensionsDetected=false;
    assert(hasMultitexture() && hasTextureEnvCombine() && extensionReads==1);
    extensionText=nullptr; detectTextureExtensions();
    assert(!hasMultitexture() && !hasTextureEnvCombine() && extensionReads==2);
    extensionText="GL_ARB_multitexture GL_EXT_texture_env_combine"; detectTextureExtensions();
    TSMesh mesh; mesh.vertices.resize(4); mesh.verticesPerFrame=2;
    mesh.indices={0,1,2,3,2,1};
    mesh.primitives={{0,3,0},{3,3,0x40000001u}};
    TSMaterialList materials; char materialA[]="a",materialB[]="b";
    materials.push_back(materialA,4,-1,-1,-1,1,1); materials.push_back(materialB,0,-1,-1,-1,1,1);
    mesh.renderFog(1,&materials);
    assert(vertexData==mesh.vertices.data()+2 && draws==1);
    assert(modeSeen==dtsPrimitiveModes[1] && countSeen==3 && indexData==mesh.indices.data()+3);
    mesh.primitives={{0,3,0x80000001u}}; draws=0;
    mesh.renderFog(0,&materials);
    assert(draws==1 && modeSeen==0x7fff);
    mesh.primitives[0].material=0xc0000001u; draws=0;
    mesh.renderFog(0,&materials);
    assert(draws==1 && modeSeen==0xffff);
    meshReflectionFactor=0.5f; meshFogFactor=1; meshAlphaFactor=1; meshCurrentAlpha=0.5f;
    TSMesh::setFade(0.5f);
    assert(meshFadeActive && meshCurrentAlpha==0.25f && colors==1 && materialsUpdated==1 && alphaSeen==0.25f);
    TSMesh::setFade(0.5001f);
    assert(colors==1 && materialsUpdated==1);
    TSMesh::clearFade();
    assert(!meshFadeActive && meshFadeFactor==1 && meshCurrentAlpha==0.5f && colors==2 && materialsUpdated==2);
    TSMesh::resetEnvironmentMapMaterials();
    const std::vector<int> environmentReset={0x24a,0x22e,0x401,0x118,0x31b,0x31d,0x31e,0x262,0x263,0x24a};
    assert(stateCalls==environmentReset);
    stateCalls.clear(); dtsTextureMode=9; dtsRenderAlpha=0.25f;
    TSDecalMesh::initDecalMaterials();
    const std::vector<int> decalInit={0x118,0x500,0x61b,0x61d,0x75d,0x75e,0x162,0x163,0x119,0x800,0x956,0x802,0x22e,0x401,0x24a};
    assert(stateCalls==decalInit && dtsSavedDecalTextureMode==9 && dtsSavedDecalAlpha==0.25f && dtsTextureMode==11);
    assert(dtsCurrentMaterialFlags==3 && dtsCurrentMaterialIndex==0x10000000u && dtsEnvironmentMode==0 && dtsDetailMode==4 && dtsActiveTextureUnit==0);
    stateCalls.clear(); dtsRenderAlpha=0.75f;
    TSDecalMesh::resetDecalMaterials();
    const std::vector<int> decalReset={0x31b,0x31d,0x262,0x263,0x219,0x24a,0x22e,0x401};
    assert(stateCalls==decalReset && dtsTextureMode==9 && dtsRenderAlpha==0.25f);
    dtsSavedDecalTextureMode=8; dtsRenderAlpha=0.75f; TSDecalMesh::resetDecalMaterials();
    assert(dtsTextureMode==8 && dtsRenderAlpha==0.75f);
    stateCalls.clear(); dtsEnvironmentMode=1; dtsEnvironmentTextureUnit=1;
    dtsTextureMode=9; dtsFogTextureUnit=2; dtsActiveTextureUnit=3;
    dtsMultitextureSupported=true; dtsFogPreviouslyEnabled=true; dtsForceMaterialUpdate=true;
    TSMesh::resetMaterials();
    const std::vector<int> materialReset={0xa8b,0x965,0x24a,0x262,0x263,0xa8c,0x965,0x24a,0xa8d,
        0x31b,0x31d,0x31e,0x24a,0x22e,0x401,0x118,0xc67,0x167,0xa8a,0xb8a};
    assert(stateCalls==materialReset && !dtsForceMaterialUpdate);
    stateCalls.clear(); dtsEnvironmentTextureName=77; dtsEnvironmentAlpha=0.5f; dtsRenderAlpha=0.25f;
    TSMesh::initEnvironmentMapMaterials();
    assert(meshCurrentAlpha==0.125f && alphaSeen==0.125f && meshFadeFactor==1);
    const std::vector<int> environmentInit={0xd00,0x75d,0x75e,0x162,0x163,0x14a,0x118,0x500,0x61b,0x61d,0x802,0x956,0x12e};
    assert(stateCalls==environmentInit);
    TSMaterialList loadedMaterials; char name[]="test.png";
    loadedMaterials.push_back(name,0,-1,-1,-1,1,1);
    bindMaterialTextureServices({loadMaterialTexture,hasMaterialBitmap,
        retainMaterialTexture,releaseMaterialTexture,materialTextureName});
    loadedMaterials.tsmlTransform(); loadedMaterials.clampToEdge=true;
    loadedMaterials.loadAll(); loadedMaterials.loadAll();
    assert(textureLoads==1 && textureReferences==1);
    dtsEnvironmentMode=0; dtsMaterialAlphaState=false; dtsMaterialAlphaToggle=false;
    dtsCurrentMaterialIndex=0; dtsCurrentMaterialFlags=0;
    meshAlphaFactor=1; meshFogFactor=1; meshFadeFactor=1;
    dtsEnvironmentAlpha=1; meshCurrentAlpha=1;
    stateCalls.clear(); TSMesh meshForMaterial;
    meshForMaterial.setMaterial(0,&loadedMaterials);
    assert(stateCalls==std::vector<int>{0xd00});
    // Exercise the shared state across real producer/consumer calls: saved
    // lighting must survive an unlit material, and setFade must affect the
    // following opaque material's alpha/depth state.
    lightingEnabled=true; dtsTextureMode=8;
    dtsTextureEnvCombineSupported=false; dtsInitialAlpha=0.5f;
    TSMesh::initMaterials();
    assert(dtsSavedLightingState && meshAlphaFactor==0.5f);
    loadedMaterials.setFlags(0,0x20u);
    TSMesh::setMaterial(0,&loadedMaterials);
    assert(!lightingEnabled);
    TSMesh::resetMaterials();
    assert(lightingEnabled);
    loadedMaterials.setFlags(0,0);
    dtsTextureMode=11; dtsFogColor[3]=0.75f;
    dtsInitialAlpha=0.5f; meshAlphaFactor=0.5f;
    dtsCurrentMaterialFlags=0; dtsCurrentMaterialIndex=0;
    TSMesh::setFade(0.5f);
    TSMesh::setMaterial(0,&loadedMaterials);
    assert((dtsCurrentMaterialFlags&4u) && meshFogFactor==0.25f);
    assert(meshCurrentAlpha==0.0625f);
    TSMesh::clearFade();
    DTSMaterialTextureHandle fogHandle{&fogTextureEntry};
    auto* previousFogHandle=dtsCurrentEnvironmentTexture;
    dtsCurrentEnvironmentTexture=&fogHandle;
    dtsTextureEnvCombineSupported=true; dtsTextureMode=9;
    dtsSecondaryTextureUnit=2;
    TSMesh::initMaterials();
    assert(lastTextureName==88 && dtsEnvironmentTextureName==77);
    assert(dtsFogTextureUnit==2 && meshAlphaFactor==0.5f);
    dtsCurrentEnvironmentTexture=previousFogHandle;
    TSLastDetail_initMaterials_impl();
    assert(dtsTextureMode==8 && dtsFogTextureUnit==0 && dtsActiveTextureUnit==0);
    assert(dtsCurrentMaterialFlags==0 && dtsCurrentMaterialIndex==0x10000000u);
    loadedMaterials.unload(0);
    assert(textureReferences==0 && loadedMaterials.getTextureName(0)==0);
    loadedMaterials.free(); bindMaterialTextureServices({});
    dtsRenderGL=nullptr;
}
