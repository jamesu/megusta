#include "../src/dts/shape.h"
#include "../src/dts/shape_instance.h"
#include "../src/dts/resource_bridge.h"
#include "../src/dts/texture_manager.h"
#include "../src/engine/resource_file.h"
#include <cassert>
#include <cstring>
using namespace megusta;
using namespace megusta::dts;
struct TextureListLifetime {
    TextureListLifetime() { initializeTextureEntryList(); }
    ~TextureListLifetime() { destroyTextureEntryList(); }
};
static ResourceFile* openIfl(const char* path,const char*) {
    if (std::strcmp(path,"data/textures/skins/test.ifl")!=0) return nullptr;
    static const char content[]="a.png 2\nb.png 1\n";
    return new ResourceFile(content,sizeof(content)-1);
}
static void closeIfl(ResourceFile* file) { delete file; }
static int closeShared(ResourceFile* file) { closeIfl(file); return 0; }
int main() {
    // The original game calls the texture-list startup routine before loading
    // shapes; material lookup hashes filenames through its 256-entry table.
    TextureListLifetime textureList;
    TSShape shape; shape.readVersion=16; shape.hasMaterials=true; shape.names={"test.ifl"};
    shape.iflMaterials.resize(1); shape.iflMaterials[0].nameIndex=0; shape.iflMaterials[0].materialSlot=0;
    char base[]="base"; shape.materialList.push_back(base,0,-1,-1,-1,1,1);
    bindIflResourceService(openIfl,closeShared);
    TSShapeInstance instance(&shape,1);
    assert(shape.iflMaterials[0].firstFrame==1 && shape.iflMaterials[0].numFrames==2);
    assert(shape.iflFrameOffTimes.size()==2 && shape.materialList.size()==3);
    assert(shape.iflFrameOffTimes[0]>0.066f && shape.iflFrameOffTimes[1]>0.099f);
    assert((shape.shapeFlags&0x40u)!=0 && instance.iflMaterialInstances.size()==1);
    bindIflResourceService(nullptr,nullptr);
}
