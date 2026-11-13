// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "scene_triangle.h"
namespace megusta {
enum class GameSyncKind : std::uint32_t {
    Snapshot=0,Putt=1,TurnOver=2,ActivatePowerUp=3,NextPowerUp=4,
    PreviousPowerUp=5,PickupPowerUp=6,AimVector=7,PickupBall=8
};
struct GameSyncRecord {
    GameSyncRecord* next;
    GameSyncKind kind;
    std::uint32_t frame,randomSeed,extraWord;
    int selectionOrPowerUp;
    std::uint32_t tickFlag;
    int playClock;
    float puttPower;
    int playerCount,playerIds[4];
    std::uint32_t ballFlags[4];
    Point3F actionVector,positions[4],velocities[4];
    std::uint32_t powerUpIds[4][3];
};
extern GameSyncRecord *gameSyncSnapshots,*gameSyncActions,*gameSyncActionsTail;
GameSyncRecord* prependSyncRecord(GameSyncKind);
GameSyncRecord* appendSyncRecord(GameSyncKind);
void clearSyncPreservingFutureActions(std::uint32_t firstFrame);
}
