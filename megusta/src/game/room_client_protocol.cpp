#include "room_client.h"
#include "network_sync.h"
#include "hole.h"
#include <cstdio>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::handleMessage(const netMessage& message) {
    netRoomClient::handleMessage(message);
    switch (message.getType()) {
    case 0: receiveChat(message); break;
    case 1: receivePlayerConfig(message); break;
    case 2: receiveWelcome(message); break;
    case 3: receiveAimVector(message); break;
    case 4: receivePutt(message); break;
    case 5: receivePuttOver(message); break;
    case 6: receiveTurnOver(message); break;
    case 7: receiveFrameGate(message); break;
    case 8: receiveSnapshot(message); break;
    case 9: receivePlayerState(message); break;
    case 10: receiveInvite(message); break;
    case 11: receiveResetTurn(); break;
    case 12: receivePowerUp(message); break;
    case 13: nextHoleGo=1; break;
    case 14: receivePickupBall(message); break;
    case 15: receiveTeamSeed(message); break;
    case 0xd1: receiveServerAddresses(message); break;
    case 0xd6: isHost=1; handleSay("You are the host now."); break;
    case 0xda: receiveRoomInfo(message); break;
    case 0xdb: receiveGameConfigText(message); break;
    }
}
void iRoomClient::receiveAimVector(const netMessage& message) {
    if (inGame() && !isGameMaster() && localNetworkState()==3) {
        auto* record=appendSyncRecord(GameSyncKind::AimVector);
        record->frame=message.geti();
        message.getfv(&record->actionVector.x,3);
    }
}
void iRoomClient::receivePutt(const netMessage& message) {
    if (inGame() && !isGameMaster() && localNetworkState()==3) {
        auto* record=appendSyncRecord(GameSyncKind::Putt);
        record->frame=message.geti(); record->randomSeed=message.geti();
        record->tickFlag=message.geti(); record->playClock=message.geti();
        record->puttPower=message.getf(); message.getfv(&record->actionVector.x,3);
        std::printf(roomClientRuntime->gotPuttFormat,static_cast<double>(record->actionVector.x),
            static_cast<double>(record->actionVector.y),static_cast<double>(record->actionVector.z));
        return;
    }
    std::printf(roomClientRuntime->unexpectedPuttFormat,networkStateName(localNetworkState()));
    roomClientRuntime->fatal();
}
void iRoomClient::receiveTurnOver(const netMessage& message) {
    if (!inGame() || localNetworkState()!=5) return;
    auto* record=appendSyncRecord(GameSyncKind::TurnOver);
    record->frame=message.geti(); record->randomSeed=message.geti();
    record->tickFlag=message.geti(); record->playClock=message.geti();
    record->playerCount=message.geti();
    for (int i=0;i<record->playerCount;++i) {
        record->playerIds[i]=message.geti(); record->ballFlags[i]=message.geti();
        message.getfv(&record->positions[i].x,3); message.getfv(&record->velocities[i].x,3);
    }
    record->extraWord=message.geti();
    std::printf(roomClientRuntime->receivedTurnFormat,gameHoleClock.totalFixedTickCount);
}
void iRoomClient::receiveSnapshot(const netMessage& message) {
    if (!inGame() || isGameMaster()) return;
    auto* record=prependSyncRecord(GameSyncKind::Snapshot);
    record->frame=message.geti(); record->randomSeed=message.geti();
    record->tickFlag=message.geti(); record->playClock=message.geti();
    record->playerCount=message.geti();
    for (int i=0;i<record->playerCount;++i) {
        record->playerIds[i]=message.geti(); record->ballFlags[i]=message.geti();
        message.getfv(&record->positions[i].x,3); message.getfv(&record->velocities[i].x,3);
        for (int slot=0;slot<3;++slot) record->powerUpIds[i][slot]=message.geti();
    }
}
void iRoomClient::receivePowerUp(const netMessage& message) {
    if (!inGame() || isGameMaster()) return;
    auto* player=getPlayerBySlot(message.getFromID());
    if (!player) return;
    auto* state=playerStates.findPlayer(player);
    auto* record=appendSyncRecord(GameSyncKind::ActivatePowerUp);
    record->extraWord=state->gamePlayerIndex;
    record->frame=message.geti();
    int action=message.geti();
    switch (action) {
    case 0: record->selectionOrPowerUp=message.geti(); break;
    case 1: record->kind=GameSyncKind::NextPowerUp; break;
    case 2: record->kind=GameSyncKind::PreviousPowerUp; break;
    case 3: record->kind=GameSyncKind::PickupPowerUp; record->selectionOrPowerUp=message.geti(); break;
    default: return;
    }
    std::printf("%s",roomClientRuntime->powerUpMessages[action]);
}
void iRoomClient::receivePickupBall(const netMessage& message) {
    if (!inGame() || isGameMaster()) return;
    auto* player=getPlayerBySlot(message.getFromID());
    if (!player) return;
    auto* state=playerStates.findPlayer(player);
    auto* record=appendSyncRecord(GameSyncKind::PickupBall);
    record->extraWord=state->gamePlayerIndex; record->frame=message.geti();
}
}
