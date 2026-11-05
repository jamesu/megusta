#include "player_ai.h"
#include "player.h"
#include "balls.h"
#include "random.h"
#include "game_config.h"
#include "../engine/string_services.h"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace megusta {
void PlayerAI::applySetting(const char* key,const char* value) {
    strings::toInt(value); float number=static_cast<float>(strings::toDouble(value));
    struct Setting { const char* name; float PlayerAI::*field; };
    static const Setting settings[]={{"aiBase",&PlayerAI::aiBase},{"aiRandom",&PlayerAI::aiRandom},
        {"aiLearnUp",&PlayerAI::aiLearnUp},{"aiLearnDn",&PlayerAI::aiLearnDn},{"aiHazard",&PlayerAI::aiHazard},
        {"aiBallHit",&PlayerAI::aiBallHit},{"aiLuck",&PlayerAI::aiLuck},{"aiDumbAng",&PlayerAI::aiDumbAng},
        {"aiDumbPow",&PlayerAI::aiDumbPow},{"aiHoleFact",&PlayerAI::aiHoleFact}};
    for (const auto& setting:settings) if (!strings::compareInsensitive(key,setting.name)) { this->*setting.field=number; return; }
}
void PlayerAI::loadDifficultySettings(GameConfig& config,bool networkInGame) {
    *this=PlayerAI{};
    int difficulty=config.readInt("Game","Difficulty",0); if (networkInGame) difficulty=2;
    char section[16]; std::sprintf(section,"AI_%d",difficulty);
    int count=0; char** keys=config.readKeys(section,&count);
    if (keys && count>0) for (int i=0;i<count;++i) {
        char* value=config.readString(section,keys[i],nullptr); if (value) applySetting(keys[i],value);
    }
}
void PlayerAI::beginHole(Random& random,int adjustment,std::uint32_t tick) {
    std::uint32_t sample=random.generate();
    float value=aiHoleFact+aiBase+(aiRandom*0.5-static_cast<float>(sample%1000u)*aiRandom*0.001)+0.5;
    targetStrokeCount=static_cast<int>(std::floor(static_cast<long double>(value)))+adjustment;
    if (targetStrokeCount<7) { if (targetStrokeCount<2) targetStrokeCount=1; } else targetStrokeCount=7;
    previousChoice=-1; resetDecision(tick);
}
void PlayerAI::resetDecision(std::uint32_t tick) { std::memset(&choice,0,sizeof(choice)); decisionState=0; decisionStartTick=tick; }
void PlayerAI::learn(int result) { if (result<0) aiBase=aiLearnDn+aiBase; if (result>0) aiBase-=aiLearnUp; }
void PlayerAI::setTargetStrokeCount() { targetStrokeCount=static_cast<int>(static_cast<long double>(targetStrokeCount)+aiBallHit); }
void Player::tryActivatePowerUp(Random& random,const PlayerServices& services) {
    if (powerUpActivationPending) return;
    unsigned count=0; for (const auto& slot:powerUps) if (slot.definitionId) ++count;
    if (!count) return;
    unsigned threshold=count==1 ? 31u : (count==2 ? 61u : 91u);
    if (random.generate()%100u>=threshold) return;
    int slot=0;
    for (int i=0;i<100;++i) {
        if (powerUps[slot].definitionId && random.generate()%100u<20u) {
            if (slot>=0 && slot<3) selectedPowerUpSlot=slot;
            powerUpActivationPending=true; services.sendActivatePowerUp(services.context); return;
        }
        slot=slot==2 ? 0 : slot+1;
    }
}
long double shortestAngleDistance(float first,float second) {
    long double distance=first<=second ? static_cast<long double>(second)-first : static_cast<long double>(first)-second;
    while (distance>360.0L) distance-=360.0L;
    if (distance>180.0L) distance=360.0L-distance;
    return distance;
}
bool PlayerAI::updateShotDecision(Player& player,PlayerAIUpdateServices& services) {
    float camera=services.cameraHeading;
    if (services.cameraControlMode) return false;
    if (decisionState==0) { decisionState=1; return false; }
    if (decisionState==1) { decisionState=2; return false; }
    if (decisionState==5) return false;
    if (decisionState==3) {
        if (services.taskTick-decisionStartTick<40u) {
            float heading=choice.heading; smoothedHeading=camera;
            long double distance=shortestAngleDistance(camera,heading);
            if (distance>1.0L) {
                float step=static_cast<float>(distance*static_cast<long double>(0.083333336f)+1.0L);
                long double forward=shortestAngleDistance(smoothedHeading+1.0,heading);
                long double backward=shortestAngleDistance(smoothedHeading-1.0,heading);
                smoothedHeading=backward<static_cast<float>(forward) ? smoothedHeading-step : smoothedHeading+step;
                services.cameraHeading=smoothedHeading;
            }
            return false;
        }
        decisionState=4;
    }
    if (decisionState==4) { smoothedHeading=choice.heading; return true; }
    decisionState=3;
    Ball* ball=player.getBall(gameBalls); float position[3]={ball->position[0],ball->position[1],ball->position[2]};
    int budget=targetStrokeCount-player.penaltyCount-player.strokeCount;
    if (budget<10) { if (budget<2) budget=1; } else budget=10;
    services.selectPutt(services.context,budget,position,0,choice);
    if (choice.estimatedStrokeCost<static_cast<float>(budget) && choice.kind==2) {
        float difference=static_cast<float>(budget)-choice.estimatedStrokeCost;
        std::uint32_t a=services.random.generate(),b=services.random.generate();
        // The saved expression narrows the unsigned subtraction back to a
        // signed 32-bit value before converting to float. Keep the 0..999
        // branch negative rather than converting its unsigned wrap to float.
        const int headingJitter=static_cast<int>(a%2000u)-1000;
        const int powerJitter=static_cast<int>(b%2000u)-1000;
        choice.heading=static_cast<float>(headingJitter)*aiDumbAng*difference*0.001+choice.heading;
        float power=static_cast<float>(powerJitter)*aiDumbPow*difference*0.001+choice.power;
        if (power<1.0) { if (power<=0.05) power=0.05; } else power=1.0;
        choice.power=power;
    }
    return false;
}
}
