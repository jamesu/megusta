#include "../src/dts/material_list.h"
#include "../src/engine/resource_file.h"
#include <cassert>
#include <cstdint>
using namespace megusta::dts;
int main() {
    TSMaterialList list; char first[]="a\\ball.BMP",second[]="other.PNG",third[]="x.jpg";
    list.push_back(first,4,-1,2,3,0.5f,0.75f);
    list.push_back(second,0,5,-1,-1,1,1);
    list.push_back(third,0,-1,-1,-1,1,1);
    assert(list.materialNames.count==3 && list.materialNames.capacity==16);
    assert(list.textureHandles.count==3 && list.textureHandles.capacity==16);
    assert(list.reflectanceMaps[0]==0 && list.bumpMaps[0]==2);
    list.tsmlTransform();
    assert(std::string(list.materialNames[0])=="skins/a/ball");
    assert(std::string(list.materialNames[1])=="skins/other" && std::string(list.materialNames[2])=="skins/x.jpg");
    list.tsmlTransform(); assert(std::string(list.materialNames[0])=="skins/a/ball");
    list.free(); assert(list.size()==0 && list.namesTransformed);
    assert(list.materialNames.capacity==16 && list.textureHandles.capacity==16);

    TSMaterialList remapped; char sourceName[]="source",destinationName[]="destination";
    remapped.push_back(sourceName,0,-1,-1,-1,1,1);
    remapped.push_back(destinationName,0x08000000u,-1,-1,-1,1,1);
    remapped.remap(1,0);
    assert(remapped.materialNames[1]==remapped.materialNames[0]);
    remapped.free();

    // 004cea50 completes each versioned read loop and checks the stream error
    // flag at the end. Resource-backed reads report EOF without ferror, so a
    // partial metadata word is retained and later metadata fields are still
    // visited instead of causing an early return.
    const std::uint8_t truncatedMetadata[]={1,1,0,0,0,1,'x',0x5a};
    megusta::ResourceFile file(truncatedMetadata,sizeof(truncatedMetadata));
    TSMaterialList partiallyRead;
    assert(partiallyRead.read(file,16,""));
    assert(partiallyRead.size()==1 && partiallyRead.flags[0]==0x5a);

    // The base binary reader also ignores per-name read counts, terminates
    // the reused 256-byte buffer at the declared length, and returns from the
    // stream error flag rather than the fread result.
    const std::uint8_t truncatedName[]={1,1,0,0,0,3,'y'};
    megusta::ResourceFile shortNameFile(truncatedName,sizeof(truncatedName));
    TSMaterialList shortNameList;
    assert(shortNameList.read(shortNameFile,16,""));
    assert(shortNameList.size()==1 && std::string(shortNameList.materialNames[0])=="y");
}
