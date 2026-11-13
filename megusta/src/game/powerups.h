// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "player.h"
#include "power_up.h"
#include "scene_triangle.h"
class fntRenderer;
class fntFont;
class ssgTransform;
namespace megusta {
struct PowerUpDefinition { int id; unsigned kind; char name[80]; Task* task; };
struct PowerUpsRuntime;
class ResourceFile;
class ResourceMountTable;
using PowerUpInstance=PowerUp;
class PowerUps {
public:
    PowerUps() { initialize(); }
    void initialize();
    unsigned getDefinitionKind(int id) const;
    void hideCoinTransform(int index);
    void releaseHole();
    void loadHoleModels();
    void processPickups();
    void handleInput();
    void advanceAnimations();
    void attachPowerUpRecord(PowerUp*);
    void drawCoinGroundMarkers();
    void generateCoinTableStep();
    void drawEditorBallPosition(fntRenderer&,fntFont*);
    int powerDefinitionCount=0,powerInstanceCount=0,coinDefinitionCount=0,coinInstanceCount=0;
    unsigned lastUsedPowerUpId=0;
    PowerUpDefinition* powerDefinitions[10]{};
    PowerUpDefinition* coinDefinitions[3]{};
    PowerUpInstance* powerInstances[10]{};
    PowerUp* coinInstances[300]{};
    ssgEntity *powerBallModel=nullptr,*glowModel=nullptr,*coinModels[3]{};
    std::uint32_t groundMarkerTexture=0;
    ResourceFile* coinTableFile=nullptr;
    int currentInstanceIndex=0;
    bool powersEnabled=false,coinsEnabled=false,slowMotion=false;
    int slowMotionTickDivider=0;
    float generationRadius=0,generationOffsetX=0,generationOffsetY=0;
    int generationTeeIndex=1,generatedCoinCount=0;
    bool generatingCoinTable=false;
    char generationSurfaceName[80]="GREEN";
    Task *defaultPickupTask=nullptr,*coinPickupTask=nullptr;
    int createPower(unsigned,const char*,Task*);
    void createCoin(int,unsigned,const char*);
    const char* getName(int) const;
    const char* getCoinName(int) const;
    int getCurrentPowerUp() const;
    int getCurrentCoin() const;
    void addHoleCoin(int,float,float,float,float);
    void deployCoins(int,int,int,int);
    void generateCoinTable(float,int,const char*);
    void removeCurrentPowerUp(); void removeCurrentCoin(); void reset();
};
class iRoomClient;
class SceneGridIndex;
class BallCollisionWorkspace;
struct SceneObjectCollection;
struct TaskRuntime;
struct PlayerServices;
class OpenGLAdapter;
struct PowerUpsRuntime {
    PowerUps& powers;
    Random& random;
    PlayersManager& players;
    BallsManager& balls;
    SceneObjectCollection& objects;
    SpatialObjects& spatialObjects;
    SceneGridIndex& grid;
    BallCollisionWorkspace& collision;
    TaskRuntime& tasks;
    iRoomClient*& network; // Driver creates the shared client after binding.
    const PlayerServices& playerServices;
    GameInputFrame& input;
    OpenGLAdapter& gl;
    ResourceMountTable& resources;
    const float* cameraMatrix;
    const char* mapName;
    std::uint32_t& fixedTick;
    int powerUpDirty=0; // Original DAT_0052a510: set when a player's power-up slots change.
    int generationPreviousHit=0; // Original process global 0052a514.
};
void bindPowerUpsRuntime(PowerUpsRuntime&);
PowerUpsRuntime& powerUpsRuntime();
}
