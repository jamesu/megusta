#include "room_client.h"
#include "hole.h"
#include "scene_object.h"
#include "player.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace {
// The 16 bytes at saved global 00529e80 are zero-initialized in the PE image.
const netGuid savedNetworkGuid{};
}
iRoomClient::iRoomClient():netRoomClient(savedNetworkGuid,0x21fc) {
    clearNickname(); chatHistory.count=0; chatCallback=nullptr;
    chatUp=0; nextHoleGo=0; isHost=0; serverListLoaded=false;
    inviteFlags=0; hasGameConfig=false; gameConfigText[0]=0;
    whisperReplyTarget[0]=0; pickupBroadcastEnabled=true;
}
iRoomClient::~iRoomClient() { leaveRoom(); }
void iRoomClient::initializeNetwork() {
    if (netInit(nullptr,nullptr)<0) {
        // These are the two diagnostic strings referenced by the saved
        // 004107e0 body. The first contains the original line break after
        // "Net: "; both were formerly left to host wiring.
        std::printf("Net: \nInit() failed. ");
        std::printf("Failed to initialized network layer ");
        return;
    }
    if (!serverListLoaded) { loadServerList(); serverListLoaded=true; }
    leaveRoom();
}
void iRoomClient::browseRooms() {
    initializeNetwork();
    if (!openDiscoverySocket()) {
        std::printf("Net: Failed to open udp socket to listen to game servers ");
        return;
    }
    for (auto* room=getFirstRoom();room;room=getNextRoom()) queryRoomInfo(*room);
    rooms.removeAll();
}
void iRoomClient::enterRoom(const char* player,const char* name,const char* password) {
    leaveRoom();
    for (auto* room=getFirstRoom();room;room=getNextRoom()) if (std::strcmp(room->name,name)==0) {
        std::printf("Net: Entering room %s (%s,%d)... ",room->name,room->host,room->port);
        netRoomClient::enterRoom(player,*room,password); return;
    }
}
void iRoomClient::leaveRoom() {
    if (inRoom()) { saveServerList(); netRoomClient::leaveRoom(); }
    clearSaid(); clearNickname();
    std::memset(ignoredPlayers,0,sizeof ignoredPlayers);
}
void iRoomClient::createGame(const char* name,const char* password,int maximum) {
    netRoomClient::createGame(name,password,maximum); isHost=1;
}
void iRoomClient::sendNextHoleGo() { netMessage message(13,0); sendMessage(message); }
void iRoomClient::exitGame() {
    if (gameHole.isTransitioning(roomClientRuntime->objects)) {
        clearSaid(); gameHole.beginTransition(gameHoleClock);
    }
}
bool iRoomClient::isDead(int playerIndex) {
    if (!inGame()) return false;
    char section[32];
    std::sprintf(section,"Player%d",playerIndex);
    return roomClientRuntime->config.readInt(section,"dead",0)!=0;
}
void iRoomClient::forceDeadPlayerPickup() {
    if (inGame() && gamePlayers.currentPlayer && isDead(gamePlayers.currentPlayer->playerIndex)) {
        pickupBroadcastEnabled=false;
        roomClientRuntime->executePython("hud.pickUpBall()");
        pickupBroadcastEnabled=true;
    }
}
}
