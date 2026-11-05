#include "room_client.h"
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::setNick(const char* value) {
    if (value) { std::strncpy(nickname,value,31); nickname[31]=0; }
    else nickname[0]=0;
    nicknameValid=1;
}
std::uint8_t iRoomClient::isNickValid() const { return nicknameValid; }
void iRoomClient::clearNickname() { nickname[0]=0; nicknameValid=0; }
void iRoomClient::clearNextHoleGo() { nextHoleGo=0; }
bool iRoomClient::isNextHoleGo() const { return nextHoleGo==1; }
bool iRoomClient::hasPendingStartState() const { return inRoom() && chatUp!=0; }
void iRoomClient::clearUiCallbacks() { whenSaid(nullptr); whenInvited(nullptr); }
std::uint8_t iRoomClient::getIsHost() const { return isHost; }
int localNetworkState() {
    auto& client=roomClientRuntime->client;
    auto* player=client.getLocalPlayer();
    if (player && player->game_id) if (auto* slot=client.playerStates.findPlayer(player)) return slot->state;
    return 0;
}
// Names here describe predicate roles, not a claim about the original enum.
namespace { constexpr int readyState=3,playbackState=4,waitingState=5; }
bool iRoomClient::isVolatileSafe() {
    if (inGame())
        if (localNetworkState()!=readyState && localNetworkState()!=playbackState) return false;
    return true;
}
bool iRoomClient::canProgressActivePutt() {
    if (inGame())
        if (localNetworkState()!=playbackState && !allPlayersInState(readyState)) return false;
    return true;
}
bool iRoomClient::okToPickupBall() {
    if (!inGame()) return true;
    return isGameMaster() && localNetworkState()==readyState;
}
bool iRoomClient::isTurnStateReady() {
    if (!inGame()) return true;
    if (isGameMaster()) return allPlayersInState(readyState);
    return localNetworkState()==readyState;
}
bool iRoomClient::canAdvanceSimulation() {
    if (inGame()) if (localNetworkState()!=playbackState) {
        if (localNetworkState()!=waitingState && isTurnStateReady()) return true;
        return false;
    }
    return true;
}
bool iRoomClient::shouldWaitForRemoteTurn() {
    if (!inGame()) return false;
    if (isGameMaster() && localNetworkState()!=waitingState) return false;
    return true;
}
iRoom* iRoomClient::handleNewRoom() { return new iRoom; }
iPlayer* iRoomClient::handleNewPlayer() { return new iPlayer; }
iGame* iRoomClient::handleNewGame() { return new iGame; }
}
