#include "room_client.h"
#include "player.h"
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace {
// Pointer-table order recovered from 005044ac in recomp/out/room_client_session_evidence.tsv.
const char* const recoveredNetworkStateNames[7]={
    "WAIT_FOR_WELCOME","WAIT_FOR_START","WAIT_FOR_START_SYNC","WAIT_FOR_PUTT",
    "WATCH_PUTT","WAIT_FOR_TURN","WAIT_FOR_NEW_MASTER"};
}
GameNetPlayerState::GameNetPlayerState():config(nullptr) {}
GameNetPlayerState::~GameNetPlayerState() {}
const char* networkStateName(int index) {
    return index>=0 && index<7 ? recoveredNetworkStateNames[index] : "(null)";
}
const char* GameNetPlayerState::getName() const { return networkStateName(state); }
GameNetPlayerState* GameNetPlayerStates::getSlot(int index) {
    return index>=0 && index<4 && slots[index].networkPlayer ? &slots[index] : nullptr;
}
int GameNetPlayerStates::count() const {
    int count=0;
    for (const auto& slot:slots) if (slot.networkPlayer) ++count;
    return count;
}
GameNetPlayerState* GameNetPlayerStates::findPlayer(iPlayer* player) {
    if (player) for (auto& slot:slots) if (slot.networkPlayer==player) return &slot;
    return nullptr;
}
void GameNetPlayerStates::removePlayer(iPlayer* player) {
    if (auto* slot=findPlayer(player)) slot->networkPlayer=nullptr;
}
void GameNetPlayerStates::clear() { for (auto& slot:slots) slot.networkPlayer=nullptr; }
GameNetPlayerState* GameNetPlayerStates::addPlayer(iPlayer* player) {
    if (auto* value=findPlayer(player)) return value;
    for (auto& slot:slots) if (!slot.networkPlayer) {
        slot.networkPlayer=player; slot.gamePlayerIndex=-1; slot.state=0; slot.config.clear();
        return &slot;
    }
    return nullptr;
}
Ball* GameNetPlayerState::getBall() {
    if (gamePlayerIndex==-1) roomClientRuntime->fatal();
    return gamePlayers.get(gamePlayerIndex,roomClientRuntime->playerServices)->getBall(roomClientRuntime->balls);
}
std::uint32_t GameNetPlayerState::getPowerUpSlot(int index) {
    if (gamePlayerIndex==-1) roomClientRuntime->fatal();
    return gamePlayers.get(gamePlayerIndex,roomClientRuntime->playerServices)->powerUps[index].definitionId;
}
}
