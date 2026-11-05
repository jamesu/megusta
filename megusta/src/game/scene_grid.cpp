#include "scene_grid.h"
#include "scene_grid_literals.h"
#include "../engine/resource_runtime.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdlib>
namespace megusta {
namespace {
// 0053d1a0 is scratch owned by the singleton build operation.
SceneGridBuildCell* buildCells=nullptr;
[[noreturn]] void fatal() { std::exit(1); }
int trunc32(long double value) { return static_cast<int>(static_cast<std::int64_t>(value)); }
int shiftedCell(int value, unsigned shift) { return static_cast<std::int32_t>(static_cast<std::uint32_t>(value) << (shift & 31)); }
}
SceneGridIndex sceneGrid;
SceneGridIndex* Grid_ctorTrivial_impl(SceneGridIndex* grid) { return grid; }
void Grid_cleanupGlobal_stub() { /* netc_serviceEnd_stub is an empty recovered callback. */ }
void Grid_registerGlobalCleanup_impl() { std::atexit(Grid_cleanupGlobal_stub); }
void Grid_initializeGlobal_impl() { (void)Grid_ctorTrivial_impl(&sceneGrid); }
SceneGridIndex::SceneGridIndex():cellShift(0),columns(0),rows(0),cellOffsets(nullptr),packedTriangleIds(nullptr),queryWord(0),candidateIds(nullptr),candidateCount(0) {
    // Original singleton is zero-initialized before its trivial constructor.
    bounds.setMin(0.0f,0.0f,0.0f); bounds.setMax(0.0f,0.0f,0.0f);
}
void SceneGridIndex::appendCellTriangle(int column, int row, int triangle) {
    if (column<0 || columns<=column || row<0 || rows<=row) fatal();
    SceneGridBuildCell& cell=buildCells[rows*column+row];
    if (cell.capacity <= cell.count) {
        int capacity=cell.capacity+256;
        auto* ids=new std::uint16_t[capacity];
        if (!ids) fatal();
        if (cell.capacity) std::memcpy(ids,cell.triangleIds,static_cast<unsigned>(cell.capacity)*2);
        delete[] cell.triangleIds;
        cell.capacity=static_cast<std::uint16_t>(capacity);
        cell.triangleIds=ids;
    }
    if (triangle>0xffff) fatal();
    cell.triangleIds[cell.count]=static_cast<std::uint16_t>(triangle);
    ++cell.count;
}
bool SceneGridIndex::triangleBoundsOverlap(const Point3F* vertices,int count,float x,float y,float width,float height) {
    unsigned common=15;
    while (count != 0) {
        unsigned bits;
        if (x <= vertices->x) bits = vertices->x <= x+width ? 0 : 2;
        else bits = 1;
        if (y <= vertices->y) { if (y+height < vertices->y) bits |= 8; }
        else bits |= 4;
        common &= bits; ++vertices; --count;
    }
    return common == 0;
}
void SceneGridIndex::distributeSceneTriangles(SceneObjectCollection& scene) {
    const SceneTriangleServices services=scene.triangleServices();
    for (int index=0;index<scene.triangleCount;++index) {
        SceneTriangleRecord& triangle=scene.triangleStorage[index];
        if (!(scene.objects[triangle.objectIndex].flags & 4)) continue;
        Point3F vertices[3]; triangle.copyVertices(vertices,services);
        float minX=vertices[0].x,maxX=minX,minY=vertices[0].y,maxY=minY;
        for (int i=1;i<3;++i) {
            if (vertices[i].x<minX) minX=vertices[i].x;
            if (vertices[i].y<minY) minY=vertices[i].y;
            if (maxX<vertices[i].x) maxX=vertices[i].x;
            if (maxY<vertices[i].y) maxY=vertices[i].y;
        }
        const float* base=bounds.getMin(); const unsigned shift=static_cast<unsigned>(cellShift)&31;
        int firstX=trunc32(static_cast<long double>(minX)-base[0]) >> shift;
        int firstY=trunc32(static_cast<long double>(minY)-base[1]) >> shift;
        int lastX=trunc32(static_cast<long double>(maxX)-base[0]) >> shift;
        int lastY=trunc32(static_cast<long double>(maxY)-base[1]) >> shift;
        float cellSize=static_cast<float>(shiftedCell(1,shift));
        for (int x=firstX;x<=lastX;++x) for (int y=firstY;y<=lastY;++y)
            if (triangleBoundsOverlap(vertices,3,static_cast<float>(shiftedCell(x,shift))+base[0],
                  static_cast<float>(shiftedCell(y,shift))+base[1],cellSize,cellSize))
                appendCellTriangle(x,y,index);
    }
}
void SceneGridIndex::buildSceneIndex(SceneObjectCollection& scene) {
    queryWord=0;
    if (this != &sceneGrid || scene.triangleCount>0xffff) fatal();
    sgBox box;
    for (int i=1;i<=scene.objectCount;++i) {
        if (scene.objects[i].flags & 4) {
            sgBox* candidate=scene.objects[i].updateWorldBounds();
            const float* lo=candidate->getMin(); const float* hi=candidate->getMax();
            if (lo[0]<=hi[0] && lo[1]<=hi[1] && lo[2]<=hi[2]) box.extend(candidate);
        }
    }
    bounds=box;
    const float* lo=bounds.getMin(); const float* hi=bounds.getMax();
    cellShift=0;
    int width=trunc32(static_cast<long double>(hi[0])-lo[0]);
    int height=trunc32(static_cast<long double>(hi[1])-lo[1]);
    for (;;) {
        const unsigned shift=static_cast<unsigned>(cellShift)&31;
        int extra=static_cast<int>(static_cast<std::uint32_t>(shiftedCell(1,shift))-1u);
        columns=(static_cast<std::int32_t>(static_cast<std::uint32_t>(width)+static_cast<std::uint32_t>(extra)) >> shift)+1;
        rows=(static_cast<std::int32_t>(static_cast<std::uint32_t>(height)+static_cast<std::uint32_t>(extra)) >> shift)+1;
        if (columns<129 && rows<129) break;
        ++cellShift;
    }
    sharedIo->standardOutput->printf(grid_literals::boundsLog,static_cast<double>(lo[0]),static_cast<double>(lo[1]),static_cast<double>(hi[0]),static_cast<double>(hi[1]));
    sharedIo->standardOutput->printf(grid_literals::dimensionsLog,cellShift,columns,rows);
    const int count=rows*columns;
    buildCells=new SceneGridBuildCell[count]();
    if (!buildCells) fatal();
    distributeSceneTriangles(scene);
    cellOffsets=new std::int32_t[count];
    if (!cellOffsets) fatal();
    int words=0;
    for (int i=0;i<count;++i) { cellOffsets[i]=words; words+=1+buildCells[i].count; }
    packedTriangleIds=new std::uint16_t[words];
    if (!packedTriangleIds) fatal();
    std::uint16_t* out=packedTriangleIds;
    for (int i=0;i<count;++i) {
        SceneGridBuildCell& cell=buildCells[i];
        *out++=cell.count;
        for (unsigned j=0;j<cell.count;++j) *out++=cell.triangleIds[j];
        delete[] cell.triangleIds;
        cell.triangleIds=nullptr; cell.count=cell.capacity=0;
    }
    delete[] buildCells; buildCells=nullptr;
}
void SceneGridIndex::releaseSceneIndex() {
    if (cellOffsets) { delete[] cellOffsets; cellOffsets=nullptr; }
    if (packedTriangleIds) { delete[] packedTriangleIds; packedTriangleIds=nullptr; }
}
}
namespace megusta {
// 0052d128: also used by putt selection and parsing. Candidate IDs and the
// temporary deduplication bitmap intentionally share the same 64 KiB storage.
SharedSelectionWorkspace puttSelectionWorkspace{};
void SceneGridIndex::collectSegmentCandidates(SceneObjectCollection& scene, Point3F start, Point3F end) {
    candidateCount=0; candidateIds=nullptr;
    if (!cellOffsets) return;
    float minX=start.x, maxX=end.x, minY=start.y, maxY=end.y;
    if (end.x < start.x) { minX=end.x; maxX=start.x; }
    if (end.y < start.y) { minY=end.y; maxY=start.y; }
    const float* base=bounds.getMin(); const unsigned shift=static_cast<unsigned>(cellShift)&31;
    const auto clampCell=[](int value,int maximum) { if (value < maximum) return value < 1 ? 0 : value; return maximum; };
    const int x0=clampCell(trunc32(static_cast<long double>(minX-1.0f)-base[0]) >> shift,columns-1);
    const int y0=clampCell(trunc32(static_cast<long double>(minY-1.0f)-base[1]) >> shift,rows-1);
    const int x1=clampCell(trunc32(static_cast<long double>(maxX+1.0f)-base[0]) >> shift,columns-1);
    const int y1=clampCell(trunc32(static_cast<long double>(maxY+1.0f)-base[1]) >> shift,rows-1);
    const unsigned bitmapBytes=(static_cast<unsigned>(scene.triangleCount)+7u)>>3;
    const unsigned capacity=(0x10000u-bitmapBytes)>>1;
    auto* bitmap=reinterpret_cast<unsigned char*>(puttSelectionWorkspace.words)+capacity*2;
    std::memset(bitmap,0,bitmapBytes);
    unsigned count=0;
    const auto add=[&](std::uint16_t id) {
        const unsigned mask=1u << (id & 7);
        if (!(bitmap[id >> 3] & mask)) {
            if (capacity <= count) fatal();
            puttSelectionWorkspace.words[count++]=id;
            bitmap[id >> 3] |= static_cast<unsigned char>(mask);
        }
    };
    for (int x=x0;x<=x1;++x) for (int y=y0;y<=y1;++y) {
        const std::uint16_t* cell=packedTriangleIds+cellOffsets[rows*x+y];
        unsigned n=*cell++;
        while (n--) add(*cell++);
    }
    for (int i=1;i<=scene.objectCount;++i) {
        SceneObjectRecord& object=scene.objects[i];
        if ((object.flags & 8) && !(object.flags & 4) && !(object.flags & 1)) {
            sgBox box; box.extend(object.entity->getBSphere());
            const float* low=box.getMin(); const float* high=box.getMax();
            if (low[0]<=high[0] && low[1]<=high[1] && low[2]<=high[2]) {
                const float* m=object.getBaseFrameTransform();
                float a[3], b[3];
                for (int j=0;j<3;++j) {
                    a[j]=low[0]*m[j]+low[1]*m[4+j]+low[2]*m[8+j]+m[12+j];
                    b[j]=high[0]*m[j]+high[1]*m[4+j]+high[2]*m[8+j]+m[12+j];
                }
                if (minX-1.0f<=b[0] && minY-1.0f<=b[1] && a[0]<=maxX+1.0f && a[1]<=maxY+1.0f) {
                    auto index=static_cast<std::uint32_t>(object.triangles-scene.triangleStorage);
                    for (std::uint32_t j=0;j<static_cast<std::uint32_t>(object.triangleCount);++j)
                        add(static_cast<std::uint16_t>(index+j));
                }
            }
        }
    }
    candidateCount=static_cast<int>(count);
    candidateIds=puttSelectionWorkspace.words;
}
void SceneGridIndex::collectObjectTriangles(SceneObjectCollection& scene, int objectIndex) {
    const SceneObjectRecord& object=scene.objects[objectIndex];
    unsigned count=0;
    if (!(object.flags & 1)) {
        auto index=static_cast<std::uint32_t>(object.triangles-scene.triangleStorage);
        for (std::uint32_t i=0;i<static_cast<std::uint32_t>(object.triangleCount);++i)
            puttSelectionWorkspace.words[count++]=static_cast<std::uint16_t>(index+i);
    }
    candidateIds=puttSelectionWorkspace.words;
    candidateCount=static_cast<int>(count);
}
bool SceneGridIndex::resolveSphereTrianglePenetration(SceneObjectCollection& scene, Point3F& position,
                                                       float radius, bool useQueryFilter,
                                                       const GridCollisionServices& services) {
    const float extent=radius*4.0f;
    const Point3F start={static_cast<float>(position.x-extent),static_cast<float>(position.y-extent),position.z};
    const Point3F end={static_cast<float>(position.x+extent),static_cast<float>(position.y+extent),position.z};
    collectSegmentCandidates(scene,start,end);
    bool collided=false;
    const SceneTriangleServices sceneServices=scene.triangleServices();
    for (int i=0;i<candidateCount;++i) {
        SceneTriangleRecord triangle=scene.triangleStorage[candidateIds[i]];
        const SceneObjectRecord& object=scene.objects[triangle.objectIndex];
        if (useQueryFilter && (object.flags & 0x80u) && object.transparencyAux==sceneGrid.queryWord) continue;
        if ((object.flags >> 8) & 1) continue;
        if (!(object.flags & 4)) triangle.refreshPlane(sceneServices);
        float point[3]={position.x,position.y,position.z};
        float distanceSquared=services.pointTriangleDistanceSquared(services.context,triangle,point);
        if (distanceSquared < radius*radius) {
            const long double correction=static_cast<long double>(radius)-std::sqrt(static_cast<double>(distanceSquared));
            collided=true;
            position.x=static_cast<float>(triangle.planeNormal[0]*correction+position.x);
            position.y=static_cast<float>(triangle.planeNormal[1]*correction+position.y);
            position.z=static_cast<float>(triangle.planeNormal[2]*correction+position.z);
        }
    }
    return collided;
}
}
