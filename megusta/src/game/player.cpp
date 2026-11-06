#include "player.h"
#include "balls.h"
#include "hole.h"
#include "putter.h"
#include "camera.h"
#include "powerups.h"
#include "room_client.h"
#include <cstring>
#include <new>
#include <cmath>
namespace megusta {
PlayersManager gamePlayers{};
void Player::setSkin(int index,const PlayerServices& services) {
    skinIndex=index; services.setPlayerSkin(services.context,*this,index);
}
Ball* Player::getBall(BallsManager& balls) const { return balls.balls+playerIndex; }
bool Player::isDone(BallsManager& balls) const { return (getBall(balls)->flags&0x10u)!=0; }
void Player::takeMulligan(PlayersManager& players,BallsManager& balls,const PlayerServices& services) {
    Ball* ball=getBall(balls); auto ballFlags=static_cast<std::uint8_t>(ball->flags);
    if ((ballFlags&1u) && players.turnState==1) {
        if (this==players.currentPlayer && !(ballFlags&0x10u) && ball->restHistoryIndex && ball->restoreRestHistory()) {
            flags|=8u; ball->flags &= ~2u;
            services.putterTurn->camera->restartFollow(services.putterTurn->cameraFrame->runtime);
        }
        if (strokeCount) --strokeCount;
        if (players.currentPlayer->lastStrokePenaltyFlag && penaltyCount) --penaltyCount;
    }
}
void Player::addPowerUpUnchecked(std::uint32_t id,const PlayerServices& services) {
    for (auto& slot:powerUps) if (!slot.definitionId) { slot.definitionId=id; *services.powerUpDirty=1; return; }
}
std::uint32_t Player::addPowerUp(std::uint32_t id,const PlayerServices& services) {
    for (auto& slot:powerUps) if (!slot.definitionId) {
        if (services.sendPickupPowerUp(services.context,id)) { slot.definitionId=id; *services.powerUpDirty=1; }
        return id;
    }
    return 0xffffffffu;
}
void Player::usedPowerUp(const PlayerServices& services) {
    if (powerUpActivationPending) return;
    int next=selectedPowerUpSlot;
    for (int i=0;i<3;++i) {
        next=next<2 ? next+1 : 0;
        if (powerUps[next].definitionId) {
            if (selectedPowerUpSlot!=next) services.playButtonSound(services.context);
            selectedPowerUpSlot=next; return;
        }
    }
    selectedPowerUpSlot=0;
}
void Player::selectPreviousPowerUp(const PlayerServices& services) {
    if (powerUpActivationPending) return;
    int next=selectedPowerUpSlot;
    for (int i=0;i<3;++i) {
        next=next<1 ? 2 : next-1;
        if (powerUps[next].definitionId) {
            if (selectedPowerUpSlot!=next) services.playButtonSound(services.context);
            selectedPowerUpSlot=next; return;
        }
    }
    selectedPowerUpSlot=0;
}
void PlayersManager::reset() {
    delete[] playerStorage; playerStorage=nullptr; playerCount=0; currentPlayer=nullptr; showMessage(nullptr,0,0);
}
Player* PlayersManager::add(const char* nameValue,const char* modelValue,const PlayerServices& services) {
    if (playerCount>3) return nullptr;
    if (!playerStorage) { playerStorage=new(std::nothrow) Player[4]{}; if (!playerStorage) services.fatal(services.context); }
    Player* player=playerStorage+playerCount++; *player=Player{};
    std::strncpy(player->name,nameValue,31); std::strncpy(player->modelTag,modelValue,7);
    player->useMouse=player->controlFlag8d=player->controlFlag8e=player->leftHanded=true;
    player->controlMode=1; player->isHuman=true; player->playerIndex=static_cast<int>(player-playerStorage);
    player->accuracy=player->controlAttribute=player->luck=1.0f;
    player->ai.loadDifficultySettings(services.config,services.networkInGame(services.context)); return player;
}
Player* PlayersManager::get(std::uint32_t index,const PlayerServices& services) {
    if (!playerStorage || index>3) services.fatal(services.context);
    return playerStorage+index;
}
void PlayersManager::showMessage(const char* text,std::uint32_t duration,std::uint32_t tick) {
    message[0]=0;
    if (text && *text) { std::strncpy(message,text,0xfff); message[0xfff]=0; messageStartTick=tick; messageDuration=duration; }
}
void PlayersManager::setCurrent(Player* player,BallsManager& balls,const PlayerServices& services) {
    if (!player) services.fatal(services.context);
    services.finishPutt(services.context); services.logSetCurrent(services.context,player->name);
    if (player->isDone(balls)) { gameHole.beginTransition(gameHoleClock); return; }
    currentPlayer=player; turnState=0;
}
Player* PlayersManager::findNext(Player* player) {
    if (player && static_cast<std::uint32_t>(player->playerIndex)<playerCount-1u) return player+1;
    return playerStorage;
}
Player* PlayersManager::findNextOrFirst(Player* player) {
    if (!player) return playerStorage;
    return playerCount-1u<=static_cast<std::uint32_t>(player->playerIndex) ? nullptr : player+1;
}
void PlayersManager::beginTurn(BallsManager& balls,const PlayerServices& services) {
    for (unsigned i=0;i<gamePlayers.playerCount;++i) {
        Ball* ball=gamePlayers.playerStorage[i].getBall(balls);
        if ((ball->flags&0x10u) || !ball->restHistoryIndex) {
            ball->flags |= 2u;
            for (ssgEntity* model:ball->specialModel) model->clrTraversalMaskBits(1);
        }
    }
    Ball* ball=gamePlayers.currentPlayer->getBall(balls); ball->turnFlag=0;
    services.powerUps->slowMotion=false; services.powerUps->lastUsedPowerUpId=0; *services.powerUpDirty=1;
    gamePlayers.currentPlayer->getBall(balls)->flags &= ~2u;
    turnState=1;
    if (!services.putterTurn || !services.putterTurn->putter || !services.putterTurn->animation ||
        !services.putterTurn->camera || !services.putterTurn->cameraFrame || !services.putterTurn->input ||
        !services.putterTurn->client || !services.putterTurn->fixedTick || !services.putterTurn->taskTick ||
        !services.putterTurn->editorMode) {
        if (services.fatal) services.fatal(services.context);
        return;
    }
    PutterTurnRuntime& putterTurn=*services.putterTurn;
    PutterAnimationServices& animation=*putterTurn.animation;
    animation.player=currentPlayer; animation.tick=*putterTurn.fixedTick; animation.editing=*putterTurn.editorMode!=0;
    putterTurn.putter->beginTurn(*currentPlayer,services.random,*putterTurn.fixedTick,*putterTurn.taskTick,
        animation,*putterTurn.input,*putterTurn.client,*putterTurn.camera,*putterTurn.cameraFrame);
    if (*services.client) (*services.client)->forceDeadPlayerPickup();
}
bool PlayersManager::isActive() const { return turnState==1 || turnState==2; }
void PlayersManager::recordStroke(BallsManager& balls,const BallRestServices& rest) {
    gamePlayers.currentPlayer->lastStrokePenaltyFlag=false; ++gamePlayers.currentPlayer->strokeCount;
    gamePlayers.currentPlayer->getBall(balls)->advanceRestHistory(rest); turnState=2;
}
void PlayersManager::markReadyToAdvance() { if (turnState>=0 && turnState<=3) turnState=4; }
bool PlayersManager::allBallsReady(BallsManager& balls,const PlayerServices& services) {
    for (unsigned i=0;i<gamePlayers.playerCount;++i) {
        Player& player=gamePlayers.playerStorage[i];
        if (!player.isDone(balls) && !(player.getBall(balls)->flags&1u)) return false;
    }
    return services.putterTurn->putter->isTurnSettled(*currentPlayer,balls);
}
bool isBallPositionValid(float x,float y,const float* tee) {
    float lowX=tee[0]-2.5, highX=tee[0]+2.5, highY=tee[1]+2.5, lowY=tee[1]-2.5;
    return x>lowX && x<highX && y<highY && y>lowY;
}
void PlayersManager::handleInput(PlayerInputContext& c) {
    const auto buttons=c.input.buttons;
    switch (turnState) {
    case 2:
        if (static_cast<std::int32_t>(currentPlayer->puttDeadlineMs)<static_cast<std::int32_t>(c.services.timeMilliseconds(c.services.context)))
            c.services.serviceNetworkEnd(c.services.context);
        if (!allBallsReady(c.balls,c.services)) return;
        c.services.serviceNetworkEnd(c.services.context); markReadyToAdvance(); return;
    case 4:
        startNextPlayerTask(c.balls,c.tasks,c.services);
        if (turnState!=0) return;
        [[fallthrough]];
    case 0:
        beginTurn(c.balls,c.services);
        if (turnState!=1) return;
        [[fallthrough]];
    case 1: break;
    default: return;
    }
    if (allBallsReady(c.balls,c.services)) { c.services.serviceNetworkEnd(c.services.context); markReadyToAdvance(); }
    Ball* ball=currentPlayer->getBall(c.balls);
    if (ball->restHistoryIndex || c.services.isEditing(c.services.context)) return;
    if (((c.input.buttons&0x200u) && !(c.input.previousButtons&0x200u)) ||
        ((c.input.modifiers&4u) && c.input.keyCode==0x16bu)) {
        ++teeIndex; if (teeIndex>2) teeIndex=0;
        ball=currentPlayer->getBall(c.balls); ball->snapToSurface(c.teePositions[teeIndex],c.rest);
    }
    if (!(buttons&0x80u) && !(c.input.modifiers&4u)) return;
    ball=currentPlayer->getBall(c.balls);
    float moved[3]={ball->position[0],ball->position[1],ball->position[2]};
    float angle=static_cast<float>(std::atan2(static_cast<long double>(c.smoothedAim[1]),static_cast<long double>(c.smoothedAim[0])));
    long double cosine=std::cos(static_cast<long double>(angle)),sine=std::sin(static_cast<long double>(angle));
    float dx; long double dy;
    const long double step=static_cast<long double>(0.1);
    if (buttons&0x408000u) { dx=static_cast<float>(sine*step); dy=cosine*-step; }
    else if (buttons&0x802000u) { dx=static_cast<float>(sine*-step); dy=cosine*step; }
    else if (buttons&0x101000u) { dx=static_cast<float>(cosine*-step); dy=sine*-step; }
    else if (buttons&0x204000u) { dx=static_cast<float>(cosine*step); dy=sine*step; }
    else return;
    moved[0]+=dx; moved[1]=static_cast<float>(dy+moved[1]); moved[2]+=c.smoothedAim[2];
    if (isBallPositionValid(moved[0],moved[1],c.teePositions[0])) currentPlayer->getBall(c.balls)->snapToSurface(moved,c.rest);
    c.input.buttons=0;
}
void PlayersManager::release(const PlayerServices& services) {
    for (std::uint32_t i=0;i<playerCount;++i) {
        services.releasePlayerShape(services.context,playerStorage[i]); services.unloadSoundGroup(services.context,i+1000);
    }
    for (int i=0;i<4;++i) setEventHandler(i,nullptr);
}
void PlayersManager::initializeHole(std::uint32_t tick,BallsManager& balls,TaskRuntime& tasks,const PlayerServices& services) {
    gamePlayers.lastProgressTick=tick; gamePlayers.progressInterval=0; displayedStrokeCount=0; holeFlag=false;
    for (std::uint32_t i=0;i<playerCount;++i) {
        Player& player=playerStorage[i]; player.strokeCount=0; player.penaltyCount=0; player.playClock=0;
        player.getBall(balls); services.loadPlayerShape(services.context,player,0x3ff,true);
        for (int voice=0;voice<10;++voice) if (services.voiceNames[voice])
            services.preloadVoice(services.context,player.modelTag,services.voiceNames[voice],i+1000);
        player.ai.beginHole(services.random,gameHole.initIntegerArgument,tasks.scheduledTasks.tickCount);
        if (services.inputDeviceEnabled(services.context)>0) player.inputDeviceId=i%services.inputDeviceCount(services.context)+1;
    }
    services.preloadButtonSound(services.context,2); currentPlayer=nullptr; startNextPlayerTask(balls,tasks,services);
}
void PlayersManager::advanceTurnIfReady(std::uint32_t tick,BallsManager& balls,const PlayerServices& services) {
    if (gamePlayers.progressInterval<=tick-gamePlayers.lastProgressTick) {
        if (gamePlayers.currentPlayer && gamePlayers.currentPlayer->strokeCount && !gamePlayers.currentPlayer->isDone(balls) &&
            services.canProgressActivePutt(services.context)) ++gamePlayers.currentPlayer->playClock;
        gamePlayers.progressInterval=30; gamePlayers.lastProgressTick=tick;
    }
}
void PlayersManager::setEventHandler(int slot,Task* task) { if (slot>=0 && slot<4) eventTasks[slot]=task; }
void PlayersManager::dispatchTerminalEvent(BallsManager& balls,TaskRuntime& tasks,const BallGameplayServices& gameplay) {
    if (eventTasks[1]) eventTasks[1]->start(tasks);
    else if (balls.currentBall) balls.currentBall->lost(gameplay);
}
void PlayersManager::startNextPlayerTask(BallsManager& balls,TaskRuntime& tasks,const PlayerServices& services) {
    if (currentPlayer && !allBallsReady(balls,services)) return;
    if (!eventTasks[2]) services.reportMissingEvent(services.context,0x315);
    eventTasks[2]->start(tasks);
    if (!currentPlayer) services.reportMissingEvent(services.context,0x317);
    currentPlayer->ai.resetDecision(tasks.scheduledTasks.tickCount);
}
void PlayersManager::startNextHoleTask(TaskRuntime& tasks,const PlayerServices& services) {
    if (services.acknowledgeFrameGate(services.context)) {
        if (!eventTasks[3]) services.reportMissingEvent(services.context,800);
        eventTasks[3]->start(tasks);
    }
}
}
