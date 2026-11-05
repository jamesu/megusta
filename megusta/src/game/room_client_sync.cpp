#include "room_client.h"
#include "network_sync.h"
#include "putter.h"
#include "effect.h"
#include "hole.h"
#include "player.h"
#include "random.h"
#include <cstdio>
#include <limits>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace {
void restoreBalls(iRoomClient& client,const GameSyncRecord& record) {
    for (int i=0;i<record.playerCount;++i) {
        auto* player=client.getPlayerBySlot(record.playerIds[i]);
        auto* state=client.playerStates.findPlayer(player);
        if (player && state) {
            auto* ball=state->getBall(); ball->flags=record.ballFlags[i];
            ball->position[0]=record.positions[i].x; ball->position[1]=record.positions[i].y; ball->position[2]=record.positions[i].z;
            ball->velocity[0]=record.velocities[i].x; ball->velocity[1]=record.velocities[i].y; ball->velocity[2]=record.velocities[i].z;
        }
    }
}
}
void iRoomClient::processFrameSync() {
    auto& runtime=*roomClientRuntime;
    const auto frame=gameHoleClock.totalFixedTickCount;
    if (localNetworkState()!=4 && localNetworkState()!=3) return;
    for (auto* record=gameSyncActions;record;record=record->next) {
        if (!record->frame || frame<record->frame) continue;
        switch (record->kind) {
        case GameSyncKind::Putt: {
            auto* ball=gamePlayers.currentPlayer->getBall(runtime.balls);
            runtime.random.setSeed(record->randomSeed); gameTasks.frameMask=record->tickFlag;
            gamePlayers.currentPlayer->playClock=record->playClock; setPlayerGameState(4);
            ball->strike(record->actionVector,runtime.ballGameplay);
            runtime.putter.enterFollowThrough(record->puttPower,runtime.cameraContext,runtime.enterCameraFollow);
            break;
        }
        case GameSyncKind::ActivatePowerUp: {
            int selected=gamePlayers.currentPlayer->selectedPowerUpSlot;
            int expected=record->selectionOrPowerUp;
            if (selected!=expected) {
                std::printf(runtime.selectionMismatchFormat,selected,expected);
                auto* player=gamePlayers.get(record->extraWord,runtime.playerServices);
                if (expected>=0 && expected<3) player->selectedPowerUpSlot=expected;
            }
            gamePlayers.get(record->extraWord,runtime.playerServices)->powerUpActivationPending=true;
            std::printf(runtime.activateFrameFormat,record->frame); break;
        }
        case GameSyncKind::NextPowerUp:
            gamePlayers.get(record->extraWord,runtime.playerServices)->usedPowerUp(runtime.playerServices);
            std::printf(runtime.nextPowerFrameFormat,record->frame); break;
        case GameSyncKind::PreviousPowerUp:
            gamePlayers.get(record->extraWord,runtime.playerServices)->selectPreviousPowerUp(runtime.playerServices);
            std::printf(runtime.previousPowerFrameFormat,record->frame); break;
        case GameSyncKind::PickupPowerUp:
            gamePlayers.get(record->extraWord,runtime.playerServices)->addPowerUpUnchecked(record->selectionOrPowerUp,runtime.playerServices);
            // The saved body reuses the previous-powerup diagnostic here.
            std::printf(runtime.previousPowerFrameFormat,record->frame); break;
        case GameSyncKind::AimVector: runtime.putter.setAimTargetWhileReady(&record->actionVector.x); break;
        case GameSyncKind::PickupBall:
            runtime.executePython("hud.pickUpBall()"); std::printf("%s",runtime.receivedPickupText); break;
        default: std::printf(runtime.unhandledSyncFormat,static_cast<unsigned>(record->kind)); break;
        }
        record->frame=0;
    }
    if (!frame || (frame&15)) return;
    GameSyncRecord** link=&gameSyncSnapshots;
    while (*link && ((*link)->kind!=GameSyncKind::Snapshot || (*link)->frame!=frame)) link=&(*link)->next;
    auto* snapshot=*link;
    if (!snapshot) return;
    *link=snapshot->next;
    if (snapshot->randomSeed!=runtime.random.state) {
        std::printf(runtime.randomMismatchFormat,snapshot->randomSeed,runtime.random.state,frame);
        runtime.random.setSeed(snapshot->randomSeed);
    }
    if (snapshot->tickFlag!=gameTasks.frameMask) {
        std::printf(runtime.tickMismatchFormat,snapshot->tickFlag,gameTasks.frameMask,frame);
        gameTasks.frameMask=snapshot->tickFlag;
    }
    if (gamePlayers.currentPlayer && snapshot->playClock) gamePlayers.currentPlayer->playClock=snapshot->playClock;
    restoreBalls(*this,*snapshot);
    // Power-up IDs are carried by snapshots but not applied by this saved body.
    delete snapshot;
}
void iRoomClient::processTurnSync() {
    if (!inGame()) return;
    auto& runtime=*roomClientRuntime;
    auto frame=gameHoleClock.totalFixedTickCount;
    if (localNetworkState()==5) for (auto* record=gameSyncActions;record;record=record->next) {
        if (!record->frame || record->frame>frame) continue;
        if (record->kind==GameSyncKind::TurnOver) {
            std::printf(runtime.processingTurnFormat,frame);
            runtime.random.setSeed(record->randomSeed); gameTasks.frameMask=record->tickFlag;
            if (record->playClock) gamePlayers.currentPlayer->playClock=record->playClock;
            restoreBalls(*this,*record);
            auto* current=findCurrentGameStatePlayer(); auto* local=getLocalPlayer();
            setPlayerGameState(current==local ? 3 : 6);
            runtime.aimSendInterval=0; runtime.lastAimFrame=gameHoleClock.totalFixedTickCount;
            auto completedFrame=record->frame; record->frame=0;
            clearSyncPreservingFutureActions(completedFrame); return;
        }
        std::printf(runtime.unhandledSyncFormat,static_cast<unsigned>(record->kind)); record->frame=0;
    }
    if (isGameMaster()) sendPeriodicSyncSnapshot(); else processFrameSync();
}
bool iRoomClient::coordinateTurnReadiness() {
    auto& runtime=*roomClientRuntime;
    if (inGame()) {
        if (localNetworkState()==2) {
            if (isGameMaster() && allPlayersInState(2)) {
                netMessage message(11,0); sendMessage(message);
                receiveResetTurn(); clearSyncPreservingFutureActions(std::numeric_limits<std::uint32_t>::max());
            }
            processTurnSync(); runtime.effects.updateAll(); return false;
        }
        if (!isGameMaster() && gameHole.isTransitioning(runtime.objects)) {
            auto* local=getLocalPlayer(); auto* game=getGameBySlot(local->game_id);
            if (!game->master_id) {
                auto* current=findCurrentGameStatePlayer();
                if (getLocalPlayer()==current) {
                    current=findCurrentGameStatePlayer(); setGameMaster(current->getID());
                    std::printf(runtime.makingMasterFormat,current->name); return true;
                }
            }
            for (auto* snapshot=gameSyncSnapshots;snapshot;snapshot=snapshot->next) {
                if (gameHoleClock.totalFixedTickCount+30u<=snapshot->frame) {
                    if (localNetworkState()==6) {
                        setPlayerGameState(3); runtime.aimSendInterval=0;
                        runtime.lastAimFrame=gameHoleClock.totalFixedTickCount;
                    }
                    return true;
                }
            }
            gameHoleClock.resetFrameDeadline(runtime.timeMilliseconds());
            processTurnSync(); runtime.effects.updateAll(); return false;
        }
    }
    return true;
}
void iRoomClient::handleEnterRoom() {
    netMessage message(0xd1,0,0,0x1450);
    for (auto* room=getFirstRoom();room;room=getNextRoom()) {
        message.puts(room->host); message.puti(room->port);
    }
    sendMessage(message);
}
void iRoomClient::handleRoomInfo(const netMessage& message,const netAddress&) {
    // Saved 00468410 invokes vtable +0x34, the actual game handleMessage slot.
    // Current PLIB exposes that path through handleRoomInfo instead.
    handleMessage(message);
}
int networkServiceStart() { return 0; } // Original 004876a0, not an invented stub.
void networkServiceEnd() {} // Original 00401130, not an invented stub.
}
