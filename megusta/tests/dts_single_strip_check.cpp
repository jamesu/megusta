#include "../src/dts/mesh.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    TSMesh mesh;
    mesh.indices={0,1,2,3,4,5,6,7,8};
    mesh.primitives={{0,4,0x40000001u},{4,5,0x40000001u}};
    assert(mesh.convertToSingleStrip(2));
    const std::vector<std::uint16_t> expected={0,1,2,3,3,4,4,5,6,7,8};
    assert(mesh.indices==expected && mesh.primitives.size()==1 && mesh.primitives[0].count==11);
    TSMesh invalid; invalid.indices={0}; invalid.primitives={{0,3,0}};
    assert(!invalid.convertToSingleStrip(2) && invalid.indices.size()==1);
    const std::uint16_t ranges[][2]={{0,4}}; const std::uint32_t material[]={0x40000001u};
    const std::uint16_t source[]={0,1,2,3}; int pc=0; std::uint32_t ic=0;
    minimumStripTriangles=3;
    convertToSingleStrip(ranges,material,source,1,&pc,&ic,nullptr,nullptr);
    assert(pc==2 && ic==10);
    TSDrawPrimitive output[2]{}; std::uint16_t result[10]; for (auto& value:result) value=0xeeee;
    convertToSingleStrip(ranges,material,source,1,&pc,&ic,output,result);
    assert(output[0].count==4 && output[1].start==4 && output[1].count==6 && output[1].material==1);
    for (int i=0;i<4;++i) assert(result[i]==0xeeee);
    const std::uint16_t triangles[]={0,1,2,2,1,3};
    for (int i=0;i<6;++i) assert(result[4+i]==triangles[i]);
    const std::uint16_t shortRange[][2]={{0,3}}; const std::uint32_t shortMaterial[]={0x40000005u};
    const std::uint16_t shortSource[]={12,13,14}; TSDrawPrimitive partialOutput[2];
    partialOutput[0]={0xaaaa,0xbbbb,0xccccdddd};
    partialOutput[1]={0x1111,0x2222,0x33334444}; minimumStripTriangles=5;
    convertToSingleStrip(shortRange,shortMaterial,shortSource,1,&pc,&ic,partialOutput,nullptr);
    assert(pc==2 && ic==6);
    assert(partialOutput[1].start==0x1111 && partialOutput[1].count==0x2222);
    assert(partialOutput[1].material==5);
}
