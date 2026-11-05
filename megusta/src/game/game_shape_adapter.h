#pragma once
#include <cstdint>
namespace megusta {
class ShapeShadow;
class Camera;
class BallsManager;
class Putter;
class PlayersManager;
struct CameraRuntime;
struct SceneObjectCollection;
struct GameShapeRuntime {
    const char* const* sequenceNames;
    Camera& camera;
    CameraRuntime& cameraState;
    BallsManager& balls;
    Putter& putter;
    PlayersManager& players;
    SceneObjectCollection& objects;
    // Two _CIasin arguments are missing from the retained callback body.
    void (*nodeAimAngles)(const float* direction,float& yaw,float& pitch);
};
void bindGameShapeRuntime(GameShapeRuntime&);
struct ShapeAnimationServices {
    void (*setSequence)(void*,void*,int,float);
    void (*transitionToSequence)(void*,void*,int,float,float,bool);
    long double (*duration)(void*);
    long double (*time)(void*);
    void (*setPosition)(void*,float);
    bool (*isTransitioning)(void*);
    void (*advanceAll)(void*,float);
};
const ShapeAnimationServices& nativeShapeAnimationServices();
// The underlying shape loader and instance implementation are recovered separately.
class GameShapeAdapter {
public:
    std::uint32_t playbackFlags=0;
    char* shapeName=nullptr;
    void* shape=nullptr;
    void* instance=nullptr;
    void* thread=nullptr;
    int importedSequenceCount=0,currentSequence=0;
    ShapeShadow* optionalShadowObject=nullptr;
    bool borrowedShape=false,wasTransitioning=false;
    int baseSkinMaterial=0,activeSkinMaterial=0,remainingPlaybackFrames=0;
    float shadowScale;
    void release();
    bool load(const char* name,unsigned sequenceMask,bool shadow);
    void setSkin(int index);
    void configureNodeAim(bool mirrored);
    void drawPreview(const float* view,const float* transform,bool mirrored);
    void drawWithAlpha(const float* view,const float* transform,bool mirrored,float alpha);
    void drawShadow(const float* view,const float* transform,bool mirrored);
    void initialize();
    void setSequence(int sequence,const ShapeAnimationServices& services);
    long double getDuration(const ShapeAnimationServices& services) const;
    long double getTime(const ShapeAnimationServices& services) const;
    void setTime(float time,const ShapeAnimationServices& services);
    void setPosition(float position,const ShapeAnimationServices& services);
    bool isTransitioning(const ShapeAnimationServices& services) const;
    void startPlayback(const ShapeAnimationServices& services);
    void advanceTime(float time,const ShapeAnimationServices& services);
};
extern float gameShapeMeshColour[3];
void setGameShapeMeshColour(float,float,float);
const float* gameShapeNodeAimCallback(int,void* instance,int node);
}
