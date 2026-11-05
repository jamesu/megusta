#include "room_client.h"
#include "network_sync.h"
#include "hole.h"
#include "player.h"
#include "random.h"
#include <cstdio>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace {
// Shared serialization mechanism for the four independently read send bodies.
void powerUpMessage(iRoomClient& client,int action,bool withSelection,std::uint32_t selection) {
    netMessage message(12,0);
    message.puti(gameHoleClock.totalFixedTickCount); message.puti(action);
    if (withSelection) message.puti(selection);
    client.sendMessage(message);
    std::printf("%s",roomClientRuntime->sentPowerUpMessages[action]);
}
void syncHeader(netMessage& message,iRoomClient& client) {
    message.puti(gameHoleClock.totalFixedTickCount);
    message.puti(roomClientRuntime->random.state);
    message.puti(gameTasks.frameMask);
    message.puti(gamePlayers.currentPlayer ? gamePlayers.currentPlayer->playClock : 0);
    message.puti(client.playerStates.count());
}
}
void iRoomClient::sendActivatePowerUp() {
    if (inGame() && isGameMaster())
        powerUpMessage(*this,0,true,gamePlayers.currentPlayer->selectedPowerUpSlot);
}
void iRoomClient::sendNextPowerUp() { if (inGame() && isGameMaster()) powerUpMessage(*this,1,false,0); }
void iRoomClient::sendPreviousPowerUp() { if (inGame() && isGameMaster()) powerUpMessage(*this,2,false,0); }
bool iRoomClient::sendPickupPowerUp(std::uint32_t id) {
    if (inGame() && gamePlayers.currentPlayer) {
        if (!isGameMaster()) return false;
        powerUpMessage(*this,3,true,id);
    }
    return true;
}
void iRoomClient::pickUpBall() {
    if (inGame() && isGameMaster() && pickupBroadcastEnabled) {
        netMessage message(14,0); message.puti(gameHoleClock.totalFixedTickCount); sendMessage(message);
        std::printf("%s",roomClientRuntime->sentPickupText);
        std::printf("%s",roomClientRuntime->pickupTauntText);
    }
}
bool iRoomClient::acknowledgeFrameGate() {
    if (inGame()) {
        if (!isGameMaster()) { if (!roomClientRuntime->frameGate) return false; }
        else { netMessage message(7,0); message.puti(gameHoleClock.totalFixedTickCount); sendMessage(message); }
    }
    roomClientRuntime->frameGate=0;
    return true;
}
void iRoomClient::finishPutt() {
    if (!inGame()) return;
    auto& runtime=*roomClientRuntime;
    if (!gamePlayers.currentPlayer) {
        std::printf(runtime.gameStateFormat,networkStateName(localNetworkState()));
        setPlayerGameState(2); return;
    }
    if (localNetworkState()!=5) {
        if (isGameMaster()) {
            netMessage message(5,0); message.puti(gameHoleClock.totalFixedTickCount); sendMessage(message);
            std::printf(runtime.sentPuttOverFormat,gameHoleClock.totalFixedTickCount);
            setPlayerGameState(5);
        } else {
            setPlayerGameState(5); runtime.puttOverFrame=0;
            runtime.executePython("hud.stopMenu()");
        }
    }
}
void iRoomClient::sendTurnOver() {
    if (!inGame() || !isGameMaster() || !allPlayersInState(5)) return;
    auto* next=findCurrentGameStatePlayer();
    // The saved body emits no message and changes no room state when its
    // current-state player lookup returns null.
    if (!next) return;
    netMessage message(6,0); syncHeader(message,*this);
    for (int i=0;i<4;++i) if (auto* state=playerStates.getSlot(i)) {
        auto* ball=state->getBall();
        message.puti(state->networkPlayer->getID()); message.puti(ball->flags);
        message.putfv(ball->position,3); message.putfv(ball->velocity,3);
    }
    message.puti(next->getID()); sendMessage(message);
    auto& runtime=*roomClientRuntime;
    std::printf(runtime.sentTurnPlayerFormat,next->name);
    if (next==getLocalPlayer()) {
        setPlayerGameState(3); runtime.aimSendInterval=0; runtime.lastAimFrame=gameHoleClock.totalFixedTickCount;
    } else { setGameMaster(next->getID()); setPlayerGameState(6); }
    std::printf(runtime.sentTurnFrameFormat,gameHoleClock.totalFixedTickCount);
}
void iRoomClient::sendPeriodicSyncSnapshot() {
    int state=localNetworkState();
    if (state!=4 && !gameHole.hasActiveScene(roomClientRuntime->objects)) return;
    auto frame=gameHoleClock.totalFixedTickCount;
    if (!frame || (frame&15)) return;
    netMessage message(8,0); syncHeader(message,*this);
    for (int i=0;i<4;++i) if (auto* slot=playerStates.getSlot(i)) {
        auto* ball=slot->getBall(); message.puti(slot->networkPlayer->getID());
        if (ball) { message.puti(ball->flags); message.putfv(ball->position,3); message.putfv(ball->velocity,3); }
        for (int power=0;power<3;++power) message.puti(slot->getPowerUpSlot(power));
    }
    sendMessage(message);
}
void iRoomClient::sendAimVector(const Point3F& aim) {
    auto& runtime=*roomClientRuntime;
    auto frame=gameHoleClock.totalFixedTickCount;
    if (inGame() && isGameMaster() && localNetworkState()==3 && runtime.aimSendInterval<=frame-runtime.lastAimFrame) {
        runtime.lastAimFrame=frame; runtime.aimSendInterval=5;
        netMessage message(3,0); message.puti(frame); message.putfv(&aim.x,3); sendMessage(message);
    }
}
void iRoomClient::sendPutt(float power,const Point3F& vector) {
    if (!inGame() || !isGameMaster()) return;
    netMessage message(4,0);
    message.puti(gameHoleClock.totalFixedTickCount); message.puti(roomClientRuntime->random.state);
    // The saved sender writes this shared sync-frame mask. Its field meaning
    // is confirmed by receive action 04 and the sync consumer restoring it.
    message.puti(gameTasks.frameMask);
    message.puti(gamePlayers.currentPlayer ? gamePlayers.currentPlayer->playClock : 0);
    message.putf(power); message.putfv(&vector.x,3); sendMessage(message);
    std::printf(roomClientRuntime->sentPuttFormat,static_cast<double>(vector.x),
        static_cast<double>(vector.y),static_cast<double>(vector.z));
    if (localNetworkState()!=3) roomClientRuntime->fatal();
    setPlayerGameState(4);
}
}
