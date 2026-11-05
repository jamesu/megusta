#pragma once
#include "scene_object.h"
namespace megusta {
struct SceneGridBuildCell { std::uint16_t capacity, count; std::uint16_t* triangleIds; };
struct PuttSelectionEntry { std::uint32_t weight; int index; };
struct PuttSelectionScratch {
    int count;
    std::uint32_t totalWeight;
    PuttSelectionEntry entries[1024];
};
static_assert(sizeof(PuttSelectionScratch)==0x2008,"saved putt-selection scratch size");
struct SharedSelectionWorkspace { alignas(4) std::uint16_t words[32768]; };
extern SharedSelectionWorkspace puttSelectionWorkspace;
struct GridCollisionServices {
    void* context;
    float (*pointTriangleDistanceSquared)(void*, const SceneTriangleRecord&, const float* point);
};
float pointTriangleDistanceSquared(const SceneTriangleRecord& triangle, const float* point,
                                   float* outS, float* outT);
GridCollisionServices defaultGridCollisionServices();
class SceneGridIndex {
public:
    SceneGridIndex();
    void appendCellTriangle(int column, int row, int triangle);
    void distributeSceneTriangles(SceneObjectCollection& scene);
    static bool triangleBoundsOverlap(const Point3F* vertices, int count, float x, float y, float width, float height);
    void buildSceneIndex(SceneObjectCollection& scene);
    void releaseSceneIndex();
    void collectSegmentCandidates(SceneObjectCollection& scene, Point3F start, Point3F end);
    void collectObjectTriangles(SceneObjectCollection& scene, int objectIndex);
    bool resolveSphereTrianglePenetration(SceneObjectCollection& scene, Point3F& position,
                                          float radius, bool useQueryFilter,
                                          const GridCollisionServices& services);
    // Later query methods use the same shared backing arrays.
    int cellShift, columns, rows;
    std::int32_t* cellOffsets;
    std::uint16_t* packedTriangleIds;
    std::uint32_t queryWord;
    sgBox bounds;
    std::uint16_t* candidateIds;
    int candidateCount;
};
extern SceneGridIndex sceneGrid;
SceneGridIndex* Grid_ctorTrivial_impl(SceneGridIndex* grid);
void Grid_initializeGlobal_impl();
void Grid_registerGlobalCleanup_impl();
void Grid_cleanupGlobal_stub();
}
