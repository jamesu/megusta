#pragma once
#include <cstdint>
namespace megusta {
class GameConfig;
class Random;
class Player;
class PlayersManager;
class BallsManager;
class PuttMapRuntime;
struct PlayerServices;
int executeAICommand(const char*,PlayersManager&,BallsManager&,const PlayerServices&,PuttMapRuntime*,int holeAdjustment,std::uint32_t tick);
struct PuttCondition {
    char sourceName[1];
    std::uint8_t unresolvedNameStorage[67]; // Capacity and internal meaning not established.
    int minimum,maximum,invert,modulus;
};
struct PuttShotChoice {
    float heading,power;
    int kind;
    float estimatedStrokeCost,fallbackHeading,fallbackPower,distance;
    bool hasCondition;
    PuttCondition condition;
};
struct PlayerAIUpdateServices {
    void* context;
    bool cameraControlMode;
    float& cameraHeading;
    std::uint32_t taskTick;
    Random& random;
    void (*selectPutt)(void*,int,const float*,int,PuttShotChoice&);
};
class PlayerAI {
public:
    int decisionState,targetStrokeCount;
    float aiBase,aiRandom,aiLearnUp,aiLearnDn,aiHazard,aiBallHit,aiLuck,aiDumbAng,aiDumbPow,aiHoleFact;
    int previousChoice;
    PuttShotChoice choice;
    float smoothedHeading;
    std::uint32_t decisionStartTick;
    void applySetting(const char* key,const char* value);
    void loadDifficultySettings(GameConfig& config,bool networkInGame);
    void beginHole(Random& random,int holeAdjustment,std::uint32_t tick);
    void resetDecision(std::uint32_t tick);
    void learn(int result);
    void setTargetStrokeCount();
    bool updateShotDecision(Player& player,PlayerAIUpdateServices& services);
};
long double shortestAngleDistance(float first,float second);
}
