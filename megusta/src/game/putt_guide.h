#pragma once
#include <cstdint>
class ssgEntity;
class ssgBranch;
class ssgVertexArray;
class ssgColourArray;
namespace megusta {
struct Point3F;
struct SceneObjectCollection;
class SceneGridIndex;
class BallCollisionWorkspace;
class PuttGuide {
public:
    ssgEntity* lineEntity=nullptr;
    ssgEntity* quadEntity=nullptr;
    ssgVertexArray* linePositions=nullptr;
    ssgVertexArray* quadPositions=nullptr;
    ssgColourArray* lineColors=nullptr;
    ssgColourArray* quadColors=nullptr;
    int pointCount=0;
    float tint[3]={},alpha=0;
    int alphaRising=0;
    std::uint32_t lastAnimationTick=0,animationPeriod=0;
    void buildSceneGeometry(int points,std::uint32_t tick);
    void attachSceneRoots(ssgBranch* root,ssgBranch* secondaryRoot);
    void hideGeometry();
    void animateColors(std::uint32_t tick);
    void updateTrajectory(const Point3F& origin,const Point3F& travel,SceneObjectCollection& objects,
                          SceneGridIndex& grid,BallCollisionWorkspace& collision);
};
}
