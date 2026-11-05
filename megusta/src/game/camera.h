#pragma once
#include "scene_triangle.h"
#include <cstdint>
namespace megusta {
class Player;
class PlayersManager;
class Ball;
class BallsManager;
class Putter;
class SpatialObjects;
class SceneGridIndex;
class OpenGLAdapter;
struct GameInputFrame;
struct SceneObjectRecord;
struct SceneObjectCollection;
struct SceneObjectAnimationServices;
class SpatialRegion;
class SpatialCamera;
enum class CameraMode : std::int32_t {
    NoAutomatic=0, Automatic=1, Sky=2, Ball=3, Secure=4, Player=5,
    Frozen=6, Cinema=7, Control=8, Out=9, Zoom=10, Replay=11, MatrixMovie=12,
    Internal13=13,Internal14=14,Internal15=15,Internal16=16,Internal17=17
};
struct CameraServices {
    void (*fatal)();
    void (*setFov)(float horizontal,float vertical);
    void (*preloadSound)(const char* name,int group);
    void (*setMouseCapture)(int mode);
};
// Shared process state, supplied explicitly like the existing game runtime
// records. This is not per-camera storage and has no invented initial values.
struct CameraRuntime {
    Point3F controlPoints[32];
    std::int32_t controlPointCount;
    std::int32_t controlPathStep,controlPathFrame;
    SceneObjectRecord* controlPathObject;
    Point3F ballPosition;
    std::uint32_t ballTargetOffset;
    std::uint32_t jitterStartTick;
    std::int32_t jitterDuration;
    bool headingRestoreRequested,headingAvailable;
    Point3F transitionVector;
    float spatialBlend;
    std::uint32_t overrideStartTick,overrideDuration;
    std::uint32_t orbitBlendFrame,followWordCe58;
    float headingAcceleration,pitchAcceleration,headingVelocity,pitchVelocity;
    int previousMouseX,previousMouseY;
    bool zoomDragMoved;
    Point3F spatialEye,regionCenter,previousSpatialEye,regionBallPosition;
    SpatialRegion* activeRegion;
    SpatialCamera* activeSpatialCamera;
    ssgEntity* followEntity;
    Ball* followMotionBall=nullptr;
    Point3F previousFollowMotionPosition{};
};
struct CameraScreenPoint { std::int32_t x,y,depth; };
struct CameraFrameServices {
    CameraRuntime& runtime;
    PlayersManager& players;
    BallsManager& balls;
    Putter& putter;
    SpatialObjects& spatial;
    SceneObjectCollection& objects;
    SceneGridIndex& grid;
    OpenGLAdapter& gl;
    GameInputFrame& input;
    const CameraServices& platform;
    const SceneObjectAnimationServices& animation;
    std::uint32_t& fixedTick;
    int& screenWidth;
    int& screenHeight;
    int& typeFilter;
    bool& hasTabElements;
    bool captureOption;
    void (*warpCursor)(int,int);
    void (*playSound)(const char*,int);
};
// Partial class declaration: only fields consumed by implemented methods are
// represented. Original offsets live in the evidence registry.
class Camera {
public:
    static bool hasFollowBallMotion(CameraRuntime&,Ball&);
    static bool canFollowMovingBall(CameraRuntime&,bool,const Player*,BallsManager&,const Putter&);
    CameraMode state;
    float viewBasis[9],screenProjection[4];
    sgMat4 viewMatrix;
    Point3F controlDirection,controlEye,controlFramePosition,controlLookAt;
    float fieldOfView,headingDegrees,pitchDegrees,followDistance,savedHeadingDegrees;
    float controlCamArgument,resolvedHeadingDegrees,resolvedPitchDegrees,resolvedFollowDistance;
    std::int32_t mouseCaptured;
    static void setControlPathStep(CameraRuntime&,std::int32_t step);
    static bool loadControlPath(CameraRuntime&,SceneObjectCollection&,const char* name,
                                const SceneObjectAnimationServices&);
    static void addControlPoint(CameraRuntime&,float x,float y,float z);
    static void addControlPoint(CameraRuntime&,const Point3F&);
    bool checkControlCam() const;
    void targetCharacterHeight(CameraRuntime&,const Player&);
    void targetBallPosition(CameraRuntime&);
    void targetBallWithOffset(CameraRuntime&);
    Point3F getFramingTarget(bool editing,bool offsetEnabled) const;
    void worldToViewVector(const Point3F& input,Point3F& output) const;
    bool projectPointToScreen(const Point3F&,CameraScreenPoint&) const;
    void smoothHeading(float target);
    bool restoreSavedHeadingAtRest(const Ball&);
    static float headingFromOrbitVector(const Point3F& direction);
    static void restorePlayerHeading(CameraRuntime&,Player*);
    void saveHeadingToPlayer(Player*) const;
    void resetFovDefault();
    static void jitter(CameraRuntime&,std::uint32_t fixedTick,float seconds);
    static bool collisionEnabled();
    void clampEditorOrbit(bool (*isEditing)());
    void captureOrbitHeading();
    bool turnTowardBall(CameraRuntime&,const Point3F&);
    bool headingFromPlayerTransform(const Player&);
    bool resetOrbitForPlayer(const Player&);
    bool turnAlongBallVelocity(CameraFrameServices&);
    void handleInput(CameraFrameServices&);
    void applyInputMotion(CameraRuntime&);
    void selectSpatialRegionCamera(CameraFrameServices&);
    void resolveOrbitAndRepulsion(CameraFrameServices&,const Point3F&,unsigned flags);
    void updateState(CameraFrameServices&);
    void updateFollowTarget(CameraFrameServices&);
    void resetFollowState(CameraFrameServices&);
    void buildViewMatrices(CameraFrameServices&);
    void drawEditorTargetMarker(CameraFrameServices&) const;
    void dump(CameraFrameServices&) const;
    void setState(CameraRuntime&,CameraMode);
    void startControlCam(CameraRuntime&,float argument);
    void returnToAutomatic(CameraRuntime&);
    bool stopControlCam(CameraRuntime&);
    void advanceControlPath(CameraRuntime&,const SceneObjectAnimationServices&,const CameraServices&);
    void restartFollow(CameraRuntime&);
    void setPosition(CameraRuntime&,std::uint32_t tick,float x,float y,float z);
    void setLookat(CameraRuntime&,std::uint32_t tick,float x,float y,float z,float seconds);
    void setLookatPlayer(CameraRuntime&);
    void setLookatBall(CameraRuntime&);
    void enterPostShotFollow(CameraRuntime&,Player&,const Ball&);
    void initializeHole(CameraRuntime&,const CameraServices&);
    void endHole(CameraRuntime&,bool captureOption,const CameraServices&);
    static void allowCollision(std::int32_t& typeFilter,void (*showMessage)(const char*));
    static void disallowCollision(std::int32_t& typeFilter);
};
void transformPackedCameraVector(const Point3F&,const float* basis,Point3F&);
}
