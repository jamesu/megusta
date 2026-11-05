#include "../src/dts/mesh.h"
#include "../src/dts/shape.h"
#include "../src/dts/shape_alloc.h"
#include "../src/engine/resource_file.h"
#include <cassert>
#include <cmath>
#include <cstring>
using namespace megusta::dts;
static void checkEmptySplit(unsigned version) {
    std::vector<std::uint32_t> words;
    // Five record counts; v19-21 combined pose count or v22's five counts;
    // then state, detail, mesh, skin, name and smallest-visible fields.
    words.insert(words.end(),5,0);
    words.insert(words.end(),version<22 ? 1 : 5,0);
    words.insert(words.end(),9,0);
    unsigned guard=0;
    auto emitGuard=[&]() { words.push_back(guard++); };
    emitGuard(); words.insert(words.end(),11,0); emitGuard();
    for (int i=0;i<7;++i) emitGuard(); // nodes through pose arrays
    if (version>=22) emitGuard(); // scale arrays
    for (int i=0;i<8;++i) emitGuard(); // states, details and tail sections
    const std::uint32_t offset16=static_cast<std::uint32_t>(words.size());
    std::vector<std::uint8_t> file;
    auto put32=[&](std::uint32_t value) { for (int i=0;i<4;++i) file.push_back(static_cast<std::uint8_t>(value>>(8*i))); };
    for (unsigned i=0;i<guard;i+=2) words.push_back(i | ((i+1<guard ? i+1 : 0)<<16));
    const std::uint32_t offset8=static_cast<std::uint32_t>(words.size());
    for (unsigned i=0;i<guard;i+=4) {
        std::uint32_t packed=0; for (unsigned j=0;j<4 && i+j<guard;++j) packed|=(i+j)<<(8*j);
        words.push_back(packed);
    }
    put32(version); put32(static_cast<std::uint32_t>(words.size())); put32(offset16); put32(offset8);
    for (auto value:words) put32(value);
    put32(0); // sequences
    file.push_back(1); put32(0); // empty binary material list
    megusta::ResourceFile input(file.data(),static_cast<std::int32_t>(file.size()));
    TSShape shape; assert(shape.read(input,"",false));
    assert(shape.readVersion==version && shape.nodes.empty() && shape.assembledMeshes.empty());
    assert(shape.splitWords.empty() && input.tell()==static_cast<std::int32_t>(file.size()));
}
static void checkCoreAllocator() {
    std::vector<std::uint32_t> words={1,0,0,0,0,1, 0,0,0,0,0,0,0,0,0};
    words.push_back(0); words.insert(words.end(),11,0); words.push_back(1);
    words.push_back(7); words.insert(words.end(),4,0xffffffffu);
    for (unsigned i=2;i<=7;++i) words.push_back(i);
    words.push_back(0x3f800000u); words.push_back(0x40000000u); words.push_back(0x40400000u);
    for (unsigned i=8;i<=12;++i) words.push_back(i);
    std::uint16_t halves[]={0,1,2,3,4,5,6,7, 1,2,3,4, 8,9,10,11,12};
    std::uint8_t bytes[]={0,1,2,3,4,5,6,7,8,9,10,11,12};
    TSShapeAlloc allocator; TSShape shape; shape.readVersion=21;
    allocator.setRead(words.data(),halves,bytes,1);
    assert(shape.readSplitAssemblyCore(allocator) && !allocator.guardMismatch && allocator.size==40);
    allocator.doAlloc(); auto* block=allocator.destination;
    allocator.setRead(words.data(),halves,bytes,0);
    assert(shape.readSplitAssemblyCore(allocator) && !allocator.guardMismatch);
    assert(allocator.size==40 && allocator.destination==block+40);
    assert(*reinterpret_cast<std::uint32_t*>(block)==7);
    assert(shape.defaultRotations[0].w==4 && shape.defaultTranslations[0].z==3.0f);
    ::operator delete(block);
}
static void checkSplitReadArena() {
    std::vector<std::uint32_t> words={1,0,0,0,0,1, 0,0,0,0,0,0,0,0,0};
    words.push_back(0); words.insert(words.end(),11,0); words.push_back(1);
    words.push_back(7); words.insert(words.end(),4,0xffffffffu);
    for (unsigned i=2;i<=7;++i) words.push_back(i);
    words.insert(words.end(),{0x3f800000u,0x40000000u,0x40400000u});
    for (unsigned i=8;i<=16;++i) words.push_back(i);
    const auto offset16=static_cast<std::uint32_t>(words.size());
    std::vector<std::uint16_t> halves={0,1,2,3,4,5,6,7,1,2,3,4,8,9,10,11,12,13,14,15,16};
    for (std::size_t i=0;i<halves.size();i+=2)
        words.push_back(halves[i] | (static_cast<std::uint32_t>(i+1<halves.size() ? halves[i+1] : 0)<<16));
    const auto offset8=static_cast<std::uint32_t>(words.size());
    for (unsigned i=0;i<=16;i+=4) {
        std::uint32_t packed=0; for (unsigned j=0;j<4 && i+j<=16;++j) packed|=(i+j)<<(8*j);
        words.push_back(packed);
    }
    std::vector<std::uint8_t> file;
    auto put=[&](std::uint32_t n) { for (int i=0;i<4;++i) file.push_back(static_cast<std::uint8_t>(n>>(8*i))); };
    put(21); put(static_cast<std::uint32_t>(words.size())); put(offset16); put(offset8);
    for (auto word:words) put(word);
    put(0); file.push_back(1); put(0);
    megusta::ResourceFile input(file.data(),static_cast<std::int32_t>(file.size()));
    TSShape shape; assert(shape.read(input,"",false));
    assert(shape.assemblyArenaBytes==40 && shape.nodes[0].nameIndex==7);
    assert(*reinterpret_cast<const std::uint32_t*>(shape.assemblyArena.data())==7);
    assert(shape.defaultRotations[0].w==4 && shape.defaultTranslations[0].z==3.0f);
}
static void checkTailAllocator() {
    std::uint32_t words[]={4,0,1,0,0,2,3}; // null mesh, guards and skin ranges
    std::uint16_t halves[]={0,1,2,3};
    std::uint8_t bytes[]={0,'x',0,1,2,3};
    TSShapeAlloc allocator; TSShape shape; shape.readVersion=21;
    shape.assemblyMeshCount=1; shape.assemblyNameCount=1;
    shape.details.resize(1); shape.details[0].subShape=-1;
    allocator.setRead(words,halves,bytes,1);
    assert(shape.readSplitAssemblyTail(allocator,false) && !allocator.guardMismatch);
    const auto expected=static_cast<unsigned>(sizeof(void*)+20);
    assert(allocator.size==expected);
    allocator.doAlloc(); auto* block=allocator.destination;
    allocator.setRead(words,halves,bytes,0);
    assert(shape.readSplitAssemblyTail(allocator,false) && !allocator.guardMismatch);
    assert(allocator.size==expected && allocator.destination==block+expected);
    assert(*reinterpret_cast<void**>(block)==nullptr);
    assert(block[sizeof(void*)]=='x' && block[sizeof(void*)+1]==0 && shape.names[0]=="x");
    ::operator delete(block);
}
static void checkMeshAllocator() {
    std::vector<std::uint32_t> words={0,1,1,0xffffffffu};
    words.insert(words.end(),9,0); words.push_back(1);
    words.push_back(1); words.push_back(0x3f800000u); words.push_back(0); words.push_back(0);
    words.push_back(1); words.push_back(0); words.push_back(0);
    words.push_back(0); words.push_back(0); words.push_back(0x3f800000u);
    words.push_back(0); words.push_back(0); words.push_back(2); words.push_back(1); words.push_back(0); words.push_back(1);
    std::uint16_t halves[]={0,10,11,1}; std::uint8_t bytes[]={0,1};
    TSShapeAlloc allocator; TSMesh mesh;
    allocator.setRead(words.data(),halves,bytes,1);
    assert(mesh.assembleBase(allocator,21,false,{}) && !allocator.guardMismatch && allocator.size==36);
    allocator.doAlloc(); auto* block=allocator.destination;
    allocator.setRead(words.data(),halves,bytes,0);
    assert(mesh.assembleBase(allocator,21,false,{}) && !allocator.guardMismatch);
    assert(allocator.size==36 && allocator.destination==block+36);
    assert(mesh.vertices[0].x==1.0f && mesh.normals[0].z==1.0f && mesh.mergeIndices[1]==11);
    ::operator delete(block);
    // The legacy reader retains the serialized radius during sizing and
    // recomputes it only after a destination has been allocated.
    allocator.setRead(words.data(),halves,bytes,1);
    assert(mesh.assembleBase(allocator,18,false,{}) && mesh.radius==1.0f);
    allocator.doAlloc(); block=allocator.destination;
    allocator.setRead(words.data(),halves,bytes,0);
    assert(mesh.assembleBase(allocator,18,false,{}) && mesh.radius==0.0f && mesh.center[0]==1.0f);
    ::operator delete(block);
}
int main() {
    TSMesh normalMesh; normalMesh.normals={{1,2,3},{4,5,6}};
    assert(normalMesh.getNormals(1)==normalMesh.normals.data()+1 && normalMesh.getNormals(1)->z==6);
    normalMesh.meshType=0x10000000u; normalMesh.verticesPerFrame=2;
    normalMesh.encodedNormals={3,7}; normalDecodeTable[3]={1,2,3}; normalDecodeTable[7]={4,5,6};
    const auto* decoded=normalMesh.getNormals(0);
    assert(decoded[0].x==1 && decoded[1].z==6);
    const float shadowMatrix[]={1,0,0,0.75f, 0,0,0,0, 0,0,1,-0.75f, 0,0,0,1};
    const TSMeshPoint shadowVertices[]={{-2,0,3},{20,0,-2},{2,0,20}};
    shadowCoordinates.resize(6);
    shadowTransform(shadowMatrix,shadowVertices,shadowVertices+3,10);
    assert(shadowCoordinates[0]==0 && shadowCoordinates[1]==2);
    assert(shadowCoordinates[2]==10 && shadowCoordinates[3]==0);
    assert(shadowCoordinates[4]==2 && shadowCoordinates[5]==10);
    const TSMeshPoint oversizedProjection={1.0e30f,0,0};
    shadowTransform(shadowMatrix,&oversizedProjection,&oversizedProjection+1,10);
    assert(shadowCoordinates[0]==0 && shadowCoordinates[1]==0);
    for (unsigned version=19;version<=22;++version) checkEmptySplit(version);
    checkCoreAllocator();
    checkSplitReadArena();
    checkTailAllocator();
    checkMeshAllocator();
    TSShape names; names.names={"Root","Wheel"}; names.nodes.resize(2); names.nodes[1].nameIndex=1;
    char query[]="wHeEl"; assert(names.findName(query)==1 && names.findNode(1)==1);
    assert(std::strcmp(names.getName(0),"Root")==0 && names.findNode(99)==-1);
    const std::uint8_t nameBytes[]={5,0,0,0,'w','h','e','e','l',3,0,0,0,'N','e','w'};
    megusta::ResourceFile nameInput(nameBytes,sizeof(nameBytes));
    assert(names.readName(&nameInput,0)==1 && names.readName(&nameInput,1)==2);
    assert(names.names[2]=="New");
    assert(names.ownedNameIndices.size()==1 && names.ownedNameIndices[0]==2);
    DTSQuat16 packed{32767,-32767,0,0}; float quat[4]; packed.getQuatF(quat);
    assert(quat[0]==1.0f && quat[1]==-1.0f);
    float half[]={0.5f,-0.5f,0,1}; packed.set(half);
    assert(packed.x==16383 && packed.y==-16383 && packed.w==32767);
    DTSQuatF rotation{0,0,0.70710678f,0.70710678f}; float rotationMatrix[16];
    assert(rotation.setMatrix(rotationMatrix)==rotationMatrix);
    assert(std::fabs(rotationMatrix[0])<0.00001f && std::fabs(rotationMatrix[1]-1.0f)<0.00001f);
    float first[]={1,2,3},second[]={5,10,15},point[3];
    interpolatePoint3F(first,second,0.25f,point);
    assert(point[0]==2 && point[1]==4 && point[2]==6);
    float transform[]={1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    const float factors[]={2,3,4}; applyAlignedScale(factors,transform);
    assert(transform[0]==2 && transform[1]==6 && transform[2]==12 && transform[3]==4 && transform[12]==26);
    applyUniformScale(0.5f,transform); assert(transform[0]==1 && transform[1]==3 && transform[3]==4);
    DTSArbitraryScale arbitrary{{0,0,0,1},{2,3,4}}; float arbitraryMatrix[16]={};
    arbitraryMatrix[0]=arbitraryMatrix[5]=arbitraryMatrix[10]=arbitraryMatrix[15]=1;
    applyArbitraryScale(&arbitrary,arbitraryMatrix);
    assert(arbitraryMatrix[0]==2 && arbitraryMatrix[5]==3 && arbitraryMatrix[10]==4 && arbitraryMatrix[15]==1);
    arbitrary.rotation={0,0,0.70710678f,0.70710678f};
    for (int i=0;i<16;++i) arbitraryMatrix[i]=(i%5==0) ? 1.0f : 0.0f;
    applyArbitraryScale(&arbitrary,arbitraryMatrix);
    assert(std::fabs(arbitraryMatrix[0]-3)<0.00001f && std::fabs(arbitraryMatrix[5]-2)<0.00001f);
    assert(std::fabs(arbitraryMatrix[1])<0.00001f && std::fabs(arbitraryMatrix[4])<0.00001f);
    TSMaterialList materials; char materialA[]="a",materialB[]="b";
    materials.push_back(materialA,0,-1,-1,-1,1,1); materials.push_back(materialB,0x08000000u,-1,-1,-1,1,1);
    TSMesh dynamicMesh; dynamicMesh.primitives={{0,0,1}};
    TSDecalMesh dynamicDecal; dynamicDecal.primitives={{0,0,0}};
    assert(dynamicMesh.hasDynamicMaterials(&materials) && !dynamicDecal.hasDynamicMaterials(&materials));
    dynamicDecal.primitives[0].material=1; assert(dynamicDecal.hasDynamicMaterials(&materials));
    TSLastDetail billboard; billboard.numEquatorSteps=4;
    float camera[]={1,0,0,0, 0,1,0,1, 0,0,1,0, 0,0,0,1};
    billboard.chooseView(camera); assert(billboard.bitmapIndex==1 && billboard.rotation==0);
    billboard.numPolarSteps=2; billboard.includePoles=1; billboard.polarAngle=0.3f;
    camera[7]=0; camera[11]=1; billboard.chooseView(camera);
    assert(billboard.bitmapIndex==20 && std::fabs(billboard.rotation-1.5707963f)<0.00001f);
    // The saved instruction export divides Z by length before acos, so the
    // two poles remain distinct at both unit and ordinary camera distances.
    camera[11]=-1; billboard.chooseView(camera); assert(billboard.bitmapIndex==21);
    camera[11]=10; billboard.chooseView(camera); assert(billboard.bitmapIndex==20);
    camera[11]=-10; billboard.chooseView(camera); assert(billboard.bitmapIndex==21);
    TSMesh mesh; mesh.vertices={{-1,0,0},{3,2,0},{0,0,10},{0,0,14}}; mesh.verticesPerFrame=2;
    mesh.computeBounds();
    assert(mesh.bounds[0]==-1 && mesh.bounds[3]==3 && mesh.bounds[5]==14 && mesh.center[2]==7);
    const float matrix[16]={2,0,0,1, 0,1,0,2, 0,0,1,3, 0,0,0,1};
    float box[6],center[3],radius;
    assert(mesh.computeBoundsFrame(matrix,0,box,center,&radius));
    assert(box[0]==-1 && box[3]==7 && center[0]==3 && center[1]==3 && center[2]==3);
    assert(std::fabs(radius-std::sqrt(17.0f))<0.00001f);
    assert(!mesh.computeBoundsFrame(matrix,2,box,center,&radius));
    mesh.computeBounds(matrix,box,0,center,&radius); assert(box[0]==-1 && box[3]==7);
    TSSkinMesh skin; skin.vertices={{100,100,100}}; skin.initialVertices={{1,2,3},{3,2,3}};
    skin.computeBounds(); assert(skin.center[0]==2 && skin.radius==1);
    skin.computeBounds(matrix,box,99,center,&radius); assert(box[0]==3 && box[3]==7 && radius==2);
    skin.initialVertices.clear(); skin.computeBounds(); assert(skin.radius==0 && skin.bounds[0]==0);
    // On v19+ the stored radius is converted from a signed integer by FILD.
    std::uint32_t words[]={0,1,1,0xffffffffu, 0,0,0,0,0,0, 0,0,0,
                           0xfffffff9u, 0,0, 0,0,0,0,0,1};
    std::uint16_t halves[]={0,1}; std::uint8_t bytes[]={0,1};
    TSShapeAlloc allocator; allocator.setRead(words,halves,bytes,true);
    TSMesh stored; assert(stored.assembleBase(allocator,21,false,{}));
    assert(!allocator.guardMismatch && stored.radius==-7.0f);
    // Shared data retains the child's stream counts, not the parent's lengths.
    TSMesh parent; parent.vertices.resize(4); parent.normals.resize(4); parent.texcoords.resize(3);
    parent.vertices[1].x=17; parent.texcoords[0].u=9;
    std::uint32_t childWords[]={0,1,1,0, 0,0,0,0,0,0, 0,0,0,
                                1, 2,1, 0,0,0,0,0,1};
    allocator.setRead(childWords,halves,bytes,true); TSMesh child;
    assert(child.assembleBase(allocator,21,false,{parent}));
    assert(!allocator.guardMismatch && child.vertices.size()==2 && child.normals.size()==2 && child.texcoords.size()==1);
    assert(child.vertices[1].x==17 && child.texcoords[0].u==9);
    // Three meshes: grandchild references a shorter child but uses more of
    // the same ancestor backing data. This exercises the loader scratch table.
    std::vector<std::uint32_t> stream;
    auto word=[&](std::uint32_t n) { stream.push_back(n); };
    for (int i=0;i<3;++i) {
        word(0); word(2*i); word(1); word(1); word(i ? i-1 : 0xffffffffu);
        for (int j=0;j<9;++j) word(0);
        word(1); int n=i==0 ? 4 : i==1 ? 2 : 3;
        word(n); if (!i) for (int j=0;j<n*3;++j) word(0);
        word(n); if (!i) for (int j=0;j<n*2;++j) word(0);
        if (!i) for (int j=0;j<n*3;++j) word(0);
        word(0); word(0); word(0); word(0); word(0);
        word(2*i+1);
    }
    word(6); word(7); word(8); word(9);
    std::uint16_t guards16[]={0,1,2,3,4,5,6,7,8,9};
    std::uint8_t guards8[]={0,1,2,3,4,5,6,7,8,9};
    allocator.setRead(stream.data(),guards16,guards8,true);
    TSShape shape; shape.readVersion=21; shape.assemblyMeshCount=3;
    assert(shape.readSplitAssemblyTail(allocator,false));
    assert(shape.assembledMeshes[1].base.vertices.size()==2);
    assert(shape.assembledMeshes[2].base.vertices.size()==3);
    // Separate skin scratch tables use initial vertices as the common parent
    // backing, even when the skin's base vertices differ.
    stream.clear(); word(0); word(1); word(2);
    for (int i=0;i<2;++i) {
        word(3+i*3); word(1); word(1); word(i ? 0 : 0xffffffffu);
        for (int j=0;j<9;++j) word(0);
        word(1); word(1);
        if (!i) { word(0x3f800000u); word(0); word(0); }
        word(0);
        if (!i) { word(0); word(0); word(0); }
        word(0); word(0); word(0); word(0); word(0); word(4+i*3);
        word(1);
        if (!i) { word(0x40000000u); word(0); word(0); word(0); word(0); word(0); }
        word(0); word(0); word(0); word(5+i*3);
    }
    word(9); allocator.setRead(stream.data(),guards16,guards8,true);
    TSShape skinShape; skinShape.readVersion=21; skinShape.assemblySkinCount=2;
    assert(skinShape.readSplitAssemblyTail(allocator,false));
    assert(skinShape.assembledSkins[0].skin.vertices[0].x==1.0f);
    assert(skinShape.assembledSkins[1].skin.vertices[0].x==2.0f);
    assert(skinShape.assembledSkins[1].skin.initialVertices[0].x==2.0f);
    // A skin in the ordinary mesh table can use an ordinary mesh parent.
    // Its initial vertices and normals come from the common mesh table.
    stream.clear(); word(0); word(1); word(1); word(0);
    for (int j=0;j<9;++j) word(0);
    word(1); word(1); word(0); word(0); word(0); word(0); word(0); word(0); word(1);
    word(1); word(0); word(0); word(0); word(2);
    allocator.setRead(stream.data(),guards16,guards8,true);
    parent.vertices[0].x=42; TSSkinMesh skinChild;
    const std::uint8_t retainedParent[]={0};
    DTSMeshAssemblyContext assemblyContext{&allocator,21,false,nullptr,nullptr,
        false,false,retainedParent,nullptr,false,-1};
    activeMeshAssembly=&assemblyContext;
    assert(skinChild.assemble(allocator,21,false,{parent},{TSSkinMesh{}}));
    assert(!allocator.guardMismatch && skinChild.initialVertices.size()==1 && skinChild.initialVertices[0].x==42);
    activeMeshAssembly=nullptr;
}
