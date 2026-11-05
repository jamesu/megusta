#include "../src/dts/mesh.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    const std::uint16_t ranges[][2]={{0,4},{4,5},{9,3}};
    const std::uint32_t materials[]={0x40000001u,0x40000001u,2};
    const std::uint16_t raw[]={0,1,2,3,4,5,6,7,8,9,10,11};
    minimumStripTriangles=3; int pc=0,ic=0;
    leaveAsMultipleStrips(ranges,materials,raw,3,&pc,&ic,nullptr,nullptr);
    assert(pc==3 && ic==14);
    TSDrawPrimitive rawPrimitives[3]; std::uint16_t rawIndices[14];
    leaveAsMultipleStrips(ranges,materials,raw,3,&pc,&ic,rawPrimitives,rawIndices);
    assert(rawPrimitives[0].count==5 && rawPrimitives[1].count==6 && rawPrimitives[2].count==3);
    TSMesh mesh; mesh.indices={0,1,2,3,4,5,6,7,8,9,10,11};
    mesh.primitives={{0,4,0x40000001u},{4,5,0x40000001u},{9,3,2}};
    assert(mesh.leaveAsMultipleStrips(3));
    assert(mesh.primitives.size()==3);
    assert(mesh.primitives[0].material==0x40000001u && mesh.primitives[0].count==5);
    assert(mesh.primitives[1].material==1 && mesh.primitives[1].count==6);
    assert(mesh.primitives[2].material==2 && mesh.primitives[2].count==3);
    const std::vector<std::uint16_t> expected={4,5,6,7,8,0,1,2,2,1,3,9,10,11};
    assert(mesh.indices==expected);
    const std::uint16_t shortRange[][2]={{0,3}}; const std::uint32_t shortMaterial[]={0x40000005u};
    const std::uint16_t shortSource[]={12,13,14}; TSDrawPrimitive partialOutput[1];
    partialOutput[0]={0xaaaa,0xbbbb,0xccccdddd}; minimumStripTriangles=5;
    leaveAsMultipleStrips(shortRange,shortMaterial,shortSource,1,&pc,&ic,partialOutput,nullptr);
    assert(pc==1 && ic==3);
    assert(partialOutput[0].start==0xaaaa && partialOutput[0].count==0xbbbb && partialOutput[0].material==0xccccdddd);
    TSMesh bad; bad.indices={0,1}; bad.primitives={{0,3,0}};
    assert(!bad.leaveAsMultipleStrips(3) && bad.indices.size()==2);
}
