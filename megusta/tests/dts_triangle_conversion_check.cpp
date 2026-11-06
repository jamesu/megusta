#include "../src/dts/shape.h"
#include <cassert>
using namespace megusta::dts;
int main() {
    const std::uint16_t ranges[][2]={{0,4},{4,3}};
    const std::uint32_t materials[]={0x40000001u,1};
    const std::uint16_t source[]={0,1,2,3,4,5,6};
    int primitiveCount=-1,indexCount=-1;
    convertToTris(ranges,materials,source,2,&primitiveCount,&indexCount,nullptr,nullptr);
    assert(primitiveCount==1 && indexCount==9);
    TSDrawPrimitive rawPrimitive[1]; std::uint16_t rawIndices[9];
    convertToTris(ranges,materials,source,2,&primitiveCount,&indexCount,rawPrimitive,rawIndices);
    const std::uint16_t rawExpected[]={0,1,2,2,1,3,4,5,6};
    assert(rawPrimitive[0].start==0 && rawPrimitive[0].count==9 && rawPrimitive[0].material==1);
    for (int i=0;i<9;++i) assert(rawIndices[i]==rawExpected[i]);
    TSMesh mesh;
    mesh.indices={0,1,2,3,3,4,5,10,11,12,20,21,22};
    mesh.primitives={{0,7,0x40000002u},{7,3,2},{10,3,0x10000003u}};
    assert(mesh.convertToTriangles());
    const std::vector<std::uint16_t> expected={0,1,2,2,1,3,3,4,5,10,11,12};
    assert(mesh.indices==expected && mesh.primitives.size()==1);
    assert(mesh.primitives[0].start==0 && mesh.primitives[0].count==12 && mesh.primitives[0].material==2);
    TSMesh invalid; invalid.indices={1,2}; invalid.primitives={{0,3,0}};
    assert(!invalid.convertToTriangles() && invalid.indices.size()==2 && invalid.primitives.size()==1);
    // The original copies whole triples even when the stated count is one.
    TSMesh partial; partial.indices={5,6,7}; partial.primitives={{0,1,0}};
    assert(partial.convertToTriangles() && partial.indices.size()==3 && partial.primitives[0].count==3);
    // The executable skips the strip loop unless at least three indices are
    // present; it does not require two unused indices in the source buffer.
    TSMesh degenerateStrip; degenerateStrip.indices={9};
    degenerateStrip.primitives={{0,1,0x40000001u}};
    assert(degenerateStrip.convertToTriangles());
    assert(degenerateStrip.indices.empty() && degenerateStrip.primitives.size()==1);
    assert(degenerateStrip.primitives[0].count==0 && degenerateStrip.primitives[0].material==1);
    // A skipped material does not split the surrounding material group.
    TSMesh grouping; grouping.indices={0,1,2,3,4,5,6,7,8};
    grouping.primitives={{0,3,7},{3,3,0x10000008u},{6,3,7}};
    assert(grouping.convertToTriangles() && grouping.primitives.size()==1 && grouping.primitives[0].count==6);
    // Original primitive counts are 16-bit even though the output index count is wider.
    TSMesh large; large.indices.resize(65535,1);
    large.primitives={{0,65535,3},{0,3,3},{0,3,4}};
    assert(large.convertToTriangles() && large.indices.size()==65541);
    assert(large.primitives[0].count==2 && large.primitives[1].start==2);
    // Sorted meshes force the multiple-strip path even when triangles are requested.
    TSShape shape; shape.assembledMeshes.resize(1); auto& sorted=shape.assembledMeshes[0];
    sorted.type=DTSMeshSorted; sorted.sorted.indices={0,1,2,3};
    sorted.sorted.primitives={{0,4,0x40000001u}};
    minimumStripTriangles=0;
    assert(shape.convertMeshesToTriangles());
    assert(sorted.sorted.primitives.size()==1 && sorted.sorted.primitives[0].material==0x40000001u);
    assert(sorted.sorted.indices.size()==4);
    assert(shape.convertMeshesToSingleStrip(0));
    assert(sorted.sorted.primitives.size()==1 && sorted.sorted.primitives[0].material==0x40000001u);
    assert(sorted.sorted.indices.size()==4);
}
