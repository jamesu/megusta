#include "network_sync.h"
#include "room_client.h"
#include <cstdio>
#include <cstring>
namespace megusta {
GameSyncRecord *gameSyncSnapshots=nullptr,*gameSyncActions=nullptr,*gameSyncActionsTail=nullptr;
namespace {
constexpr const char* clearSyncText="Net: clear sync ";
constexpr const char* savedActionTexts[9]={
    nullptr,
    "CLEARSYNC: saving SYNC_PUTT frame ",
    "CLEARSYNC: saving SYNC_TURN_OVER frame ",
    "CLEARSYNC: saving SYNC_POWERUPACTIVATE frame ",
    "CLEARSYNC: saving SYNC_POWERUPNEXT frame ",
    "CLEARSYNC: saving SYNC_POWERUPPREV frame ",
    "CLEARSYNC: saving SYNC_POWERUPPICKUP frame ",
    nullptr,
    "CLEARSYNC: saving SYNC_PICKUPBALL frame "};
}
GameSyncRecord* prependSyncRecord(GameSyncKind kind) {
    auto* record=new GameSyncRecord;
    record->next=gameSyncSnapshots; gameSyncSnapshots=record; record->kind=kind;
    return record;
}
GameSyncRecord* appendSyncRecord(GameSyncKind kind) {
    auto* record=new GameSyncRecord;
    if (gameSyncActionsTail) gameSyncActionsTail->next=record;
    if (!gameSyncActions) gameSyncActions=record;
    gameSyncActionsTail=record; record->next=nullptr; record->kind=kind;
    return record;
}
void clearSyncPreservingFutureActions(std::uint32_t firstFrame) {
    std::printf("%s",clearSyncText);
    auto* action=gameSyncActions;
    gameSyncActions=nullptr; gameSyncActionsTail=nullptr;
    auto* snapshot=gameSyncSnapshots;
    while (action) {
        gameSyncSnapshots=snapshot;
        if (action->frame && firstFrame<=action->frame) {
            auto* copy=appendSyncRecord(action->kind);
            // Copy bytes as in the original; unused fields may be indeterminate.
            // sizeof also follows the host pointer width instead of fixing 0xe4.
            std::memcpy(copy,action,sizeof *copy); copy->next=nullptr;
            switch (copy->kind) {
            case GameSyncKind::Putt: case GameSyncKind::TurnOver: case GameSyncKind::ActivatePowerUp:
            case GameSyncKind::NextPowerUp: case GameSyncKind::PreviousPowerUp:
            case GameSyncKind::PickupPowerUp: case GameSyncKind::PickupBall:
                std::printf("%s",savedActionTexts[static_cast<unsigned>(copy->kind)]); break;
            default: break;
            }
        }
        auto* next=action->next; delete action; action=next;
    }
    gameSyncSnapshots=nullptr;
    while (snapshot) { auto* next=snapshot->next; delete snapshot; snapshot=next; }
}
}
