#include "../src/dts/shape_alloc.h"
#include "../src/dts/mesh.h"
#include "../src/dts/shape.h"
#include "../src/dts/legacy_alloc.h"
#include "../src/engine/resource_file.h"
#include <cassert>
#include <cstdint>
#include <new>
using namespace megusta::dts;
int main() {
    DTSLegacyAlloc legacy;
    legacy.init32(); legacy.init16(); legacy.init8();
    const std::uint32_t inputValue=0x12345678u; megusta::ResourceFile input(&inputValue,4);
    std::uint32_t stored=0; assert(legacy.readStore32(input,stored)==inputValue && stored==inputValue);
    std::uint32_t words[4]={1,2,3,4}; std::uint16_t shorts[2]={}; std::uint8_t bytes[2]={};
    TSShapeAlloc allocator; allocator.setRead(words,shorts,bytes,true);
    assert(allocator.getPointer32(2)==words && allocator.buffer32==reinterpret_cast<const std::uint8_t*>(words+2));
    assert(allocator.allocShape32(2)==nullptr && allocator.size==8);
    assert(allocator.allocShape16(2)==nullptr && allocator.allocShape8(4)==nullptr && allocator.size==16);
    allocator.doAlloc(); auto* start=allocator.destination;
    assert(allocator.allocShape32(1)==start && allocator.destination==start+4);
    assert(allocator.allocShape16(2)==start+4 && allocator.destination==start+8);
    assert(allocator.allocShape8(3)==start+8 && allocator.destination==start+11);
    ::operator delete(start);
    // A retained child of a skipped parent must copy; a skipped child aliases.
    std::uint8_t retained[]={0,1};
    DTSMeshAssemblyContext context{&allocator,21,false,nullptr,nullptr,false,false,retained,nullptr,false,-1};
    activeMeshAssembly=&context; TSMesh mesh;
    allocator.setRead(words,shorts,bytes,true);
    assert(mesh.getSharedData32(-1,2,nullptr,1)==words && allocator.buffer32==reinterpret_cast<const std::uint8_t*>(words+2));
    void* parents[]={words,words+1};
    assert(mesh.getSharedData32(1,1,parents,0)==words+1 && allocator.size==0);
    assert(mesh.getSharedData32(0,1,parents,1)==words && allocator.size==0);
    assert(mesh.getSharedData32(0,1,parents,0)==nullptr && allocator.size==4);
    allocator.doAlloc(); start=allocator.destination;
    assert(mesh.getSharedData32(0,1,parents,0)==start && *reinterpret_cast<std::uint32_t*>(start)==words[0]);
    ::operator delete(start);
    // Reserve extra backing storage because the original 8-bit path copies 4*count.
    std::uint8_t source8[]={1,2,3,4,5,6,7,8}; void* parents8[]={source8};
    std::uint8_t output8[8]={}; allocator.setRead(words,shorts,bytes,true);
    allocator.destination=output8;
    assert(mesh.getSharedData8(0,2,parents8,0)==output8 && allocator.size==2);
    for (int i=0;i<8;++i) assert(output8[i]==source8[i]);
    allocator.setRead(words,shorts,bytes,true); allocator.multiplier=0;
    assert(allocator.copyToShape32(2,true)==words && allocator.size==0);
    assert(allocator.buffer32==reinterpret_cast<const std::uint8_t*>(words+2));
    DTSAssembledMesh noMesh; context.currentMesh=&noMesh;
    assert(TSMesh::assembleMesh(DTSMeshNull,1)==nullptr && context.succeeded && allocator.multiplier==1);
    assert(TSMesh::assembleMesh(static_cast<DTSMeshKind>(9),0)==nullptr && !context.succeeded);
    std::uint32_t meshWords[]={0,1,1,0xffffffffu, 0,0,0,0,0,0,0,0,0, 1,
                               0,0,1,0x40000001u,4,0,0,0,1};
    std::uint16_t meshHalves[]={0,0,4,0,1,2,3,1}; std::uint8_t meshBytes[]={0,1};
    std::vector<TSMesh> priorMeshes; std::vector<TSSkinMesh> priorSkins;
    DTSAssembledMesh converted; converted.type=DTSMeshStandard;
    allocator.setRead(meshWords,meshHalves,meshBytes,1);
    context.currentMesh=&converted; context.previousMeshes=&priorMeshes; context.previousSkins=&priorSkins;
    context.convertTriangles=true;
    assert(TSMesh::assembleMesh(DTSMeshStandard,0)==&converted.base && context.succeeded && !allocator.guardMismatch);
    assert(converted.base.primitives.size()==1 && converted.base.primitives[0].count==6 && converted.base.indices.size()==6);
    assert(allocator.size==128+20); allocator.doAlloc(); auto* meshBlock=allocator.destination;
    allocator.setRead(meshWords,meshHalves,meshBytes,0);
    assert(TSMesh::assembleMesh(DTSMeshStandard,0)==&converted.base && context.succeeded && allocator.size==128+20);
    auto* arenaPrimitive=reinterpret_cast<TSDrawPrimitive*>(meshBlock+128);
    auto* arenaIndices=reinterpret_cast<std::uint16_t*>(meshBlock+128+8);
    assert(arenaPrimitive->count==6 && arenaIndices[0]==0 && arenaIndices[5]==3);
    ::operator delete(meshBlock);
    allocator.setRead(meshWords,meshHalves,meshBytes,true);
    assert(TSMesh::assembleMesh(DTSMeshStandard,1)==&converted.base && context.succeeded);
    assert(allocator.size==128 && !allocator.guardMismatch && allocator.multiplier==1);
    // Sorted meshes force multiple-strip conversion even if the caller has
    // selected the normalized pass-through mode.
    std::vector<std::uint32_t> sortedWords(meshWords,meshWords+sizeof(meshWords)/sizeof(*meshWords));
    sortedWords.insert(sortedWords.end(),{0,0,0,0,0,0,2});
    std::uint16_t sortedHalves[]={0,0,4,0,1,2,3,1,2};
    std::uint8_t sortedBytes[]={0,1,2};
    DTSAssembledMesh sorted; sorted.type=DTSMeshSorted; context.currentMesh=&sorted;
    context.convertTriangles=false; context.minimumStripTriangles=-1;
    minimumStripTriangles=3;
    allocator.setRead(sortedWords.data(),sortedHalves,sortedBytes,true);
    assert(TSMesh::assembleMesh(DTSMeshSorted,0)==&sorted.sorted && context.succeeded && !allocator.guardMismatch);
    assert(sorted.sorted.primitives.size()==1 && sorted.sorted.primitives[0].count==6 && sorted.sorted.indices.size()==6);
    // With no explicit mode, base assembly likewise selects multiple strips
    // using the process-wide threshold instead of passing input through.
    dtsPrimitiveModes[3]=0;
    DTSAssembledMesh regular; regular.type=DTSMeshStandard; context.currentMesh=&regular;
    allocator.setRead(meshWords,meshHalves,meshBytes,true);
    assert(TSMesh::assembleMesh(DTSMeshStandard,0)==&regular.base && context.succeeded && !allocator.guardMismatch);
    assert(regular.base.primitives.size()==1 && regular.base.primitives[0].count==6 && regular.base.indices.size()==6);
    minimumStripTriangles=0;
    activeMeshAssembly=nullptr;
    // Decal payloads occupy the same arena in both sizing and copy passes.
    std::uint32_t decalWords[]={1,0x123,3,1,0, 10,11,12,13, 20,21,22,23, 7,0};
    std::uint16_t decalHalves[]={2,3,4,5,6,0}; std::uint8_t decalBytes[]={0};
    TSDecalMesh decal;
    allocator.setRead(decalWords,decalHalves,decalBytes,true);
    assert(decal.assemble(allocator,20) && !allocator.guardMismatch && allocator.size==52);
    allocator.doAlloc(); auto* decalBlock=allocator.destination;
    allocator.setRead(decalWords,decalHalves,decalBytes,false);
    assert(decal.assemble(allocator,20) && !allocator.guardMismatch && allocator.size==52);
    auto* decalPrimitive=reinterpret_cast<TSDrawPrimitive*>(decalBlock);
    assert(decalPrimitive->start==2 && decalPrimitive->count==3 && decalPrimitive->material==0x123);
    assert(*reinterpret_cast<std::uint16_t*>(decalBlock+8)==4);
    assert(*reinterpret_cast<std::uint32_t*>(decalBlock+20)==10);
    assert(*reinterpret_cast<std::uint32_t*>(decalBlock+36)==20);
    assert(decal.materialIndex==7 && decal.meshType==0x08000000u);
    ::operator delete(decalBlock);
    // Skin data with no influences: initial vertices, normals, transforms and
    // node indices each contribute to the arena in stream order.
    std::vector<std::uint32_t> skinWords={0,1,1,0xffffffffu, 0,0,0,0,0,0,0,0,0,1,
                                         0,0,0,0,0,0,0,1,
                                         1,11,12,13,21,22,23,1};
    for (std::uint32_t i=0;i<16;++i) skinWords.push_back(100+i);
    skinWords.insert(skinWords.end(),{0,1,77,2});
    std::uint16_t skinHalves[]={0,1,2}; std::uint8_t skinBytes[]={0,1,2};
    TSSkinMesh skin;
    activeMeshAssembly=&context;
    allocator.setRead(skinWords.data(),skinHalves,skinBytes,true);
    assert(skin.assemble(allocator,21,false,priorMeshes,priorSkins));
    assert(!allocator.guardMismatch && allocator.size==92);
    allocator.doAlloc(); auto* skinBlock=allocator.destination;
    allocator.setRead(skinWords.data(),skinHalves,skinBytes,false);
    assert(skin.assemble(allocator,21,false,priorMeshes,priorSkins));
    assert(!allocator.guardMismatch && allocator.size==92);
    assert(*reinterpret_cast<std::uint32_t*>(skinBlock)==11);
    assert(*reinterpret_cast<std::uint32_t*>(skinBlock+12)==21);
    assert(*reinterpret_cast<std::uint32_t*>(skinBlock+24)==100);
    assert(*reinterpret_cast<std::uint32_t*>(skinBlock+88)==77);
    ::operator delete(skinBlock);
    // A retained child of a skipped parent must reserve copies of the shared
    // base vertex, texture-coordinate and normal arrays.
    TSMesh parent; parent.vertices.push_back({1,2,3});
    parent.texcoords.push_back({4,5}); parent.normals.push_back({6,7,8});
    std::vector<TSMesh> parentsForMesh{parent};
    std::uint32_t childWords[]={0,1,1,0, 0,0,0,0,0,0,0,0,0,1,
                                1,1,0,0,0,0,0,1};
    std::uint16_t childHalves[]={0,1}; std::uint8_t childBytes[]={0,1};
    TSMesh child; retained[0]=0; activeMeshAssembly=&context;
    allocator.setRead(childWords,childHalves,childBytes,true);
    assert(child.assembleBase(allocator,21,false,parentsForMesh));
    assert(!allocator.guardMismatch && allocator.size==32);
    allocator.doAlloc(); auto* childBlock=allocator.destination;
    allocator.setRead(childWords,childHalves,childBytes,false);
    assert(child.assembleBase(allocator,21,false,parentsForMesh));
    assert(allocator.size==32 && reinterpret_cast<float*>(childBlock)[0]==1);
    assert(reinterpret_cast<float*>(childBlock+12)[0]==4);
    assert(reinterpret_cast<float*>(childBlock+20)[0]==6);
    ::operator delete(childBlock);
    retained[0]=1; allocator.setRead(childWords,childHalves,childBytes,true);
    assert(child.assembleBase(allocator,21,false,parentsForMesh) && allocator.size==0);
    // Shared skin influences stop at the first parent index outside the
    // child's initial vertex range when the parent is retained.
    TSSkinMesh skinParent; skinParent.vertexIndices={0,0,1};
    skinParent.boneIndices={2,3,4}; skinParent.weights={0.75f,0.25f,1.0f};
    std::vector<TSSkinMesh> parentSkins{skinParent};
    std::uint32_t childSkinWords[]={0,1,1,0, 0,0,0,0,0,0,0,0,0,1,
                                    0,0,0,0,0,0,0,1, 1,0,3,0,2};
    std::uint16_t childSkinHalves[]={0,1,2}; std::uint8_t childSkinBytes[]={0,1,2};
    TSSkinMesh childSkin;
    allocator.setRead(childSkinWords,childSkinHalves,childSkinBytes,true);
    assert(childSkin.assemble(allocator,21,false,parentsForMesh,parentSkins));
    assert(!allocator.guardMismatch && childSkin.vertexIndices.size()==2 && childSkin.boneIndices[1]==3);
    activeMeshAssembly=nullptr;
}
