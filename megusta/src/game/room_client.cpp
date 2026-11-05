#include "room_client.h"
#include "player.h"
#include "../engine/string_services.h"
namespace megusta {
RoomClientRuntime* roomClientRuntime=nullptr;
void bindRoomClientRuntime(RoomClientRuntime& runtime) { roomClientRuntime=&runtime; }
namespace {
template<class T> T* lookup(netList& store,int id,int& cursor) {
    cursor=id;
    return static_cast<T*>(store.get(id));
}
template<class T> T* previous(netList& store,int& cursor) {
    if (cursor>1) for (--cursor;cursor>0;--cursor)
        if (auto* value=lookup<T>(store,cursor,cursor)) return value;
    return nullptr;
}
template<class T> T* next(netList& store,int& cursor) {
    if (cursor<255) for (++cursor;cursor<256;++cursor)
        if (auto* value=lookup<T>(store,cursor,cursor)) return value;
    return nullptr;
}
template<class T> T* find(netList& store,const char* name,int& cursor) {
    for (int id=1;id<256;++id) if (auto* value=static_cast<T*>(store.get(id))) if (strings::compareInsensitive(value->name,name)==0) {
        cursor=value->getID();
        return value;
    }
    return nullptr;
}
}
iRoom* iRoomClient::getRoomBySlot(int id) { return lookup<iRoom>(rooms,id,roomClientRuntime->roomCursor); }
iRoom* iRoomClient::getFirstRoom() { roomClientRuntime->roomCursor=0; return getNextRoom(); }
iRoom* iRoomClient::getPrevRoom() { return previous<iRoom>(rooms,roomClientRuntime->roomCursor); }
iRoom* iRoomClient::getNextRoom() { return next<iRoom>(rooms,roomClientRuntime->roomCursor); }
iRoom* iRoomClient::findRoom(const char* name) { return find<iRoom>(rooms,name,roomClientRuntime->roomCursor); }
iPlayer* iRoomClient::getPlayerBySlot(int id) { return lookup<iPlayer>(players,id,roomClientRuntime->playerCursor); }
iPlayer* iRoomClient::getFirstPlayer() { roomClientRuntime->playerCursor=0; return getNextPlayer(); }
iPlayer* iRoomClient::getPrevPlayer() { return previous<iPlayer>(players,roomClientRuntime->playerCursor); }
iPlayer* iRoomClient::getNextPlayer() { return next<iPlayer>(players,roomClientRuntime->playerCursor); }
iPlayer* iRoomClient::findPlayer(const char* name) { return find<iPlayer>(players,name,roomClientRuntime->playerCursor); }
iPlayer* iRoomClient::getLocalPlayer() { return getPlayerBySlot(getLocalID()); }
iGame* iRoomClient::getGameBySlot(int id) { return lookup<iGame>(games,id,roomClientRuntime->gameCursor); }
// 0041abd0 consumes and encodes the result of 00448490.
iGame* iRoomClient::getFirstGame() { roomClientRuntime->gameCursor=0; return getNextGame(); }
iGame* iRoomClient::getPrevGame() { return previous<iGame>(games,roomClientRuntime->gameCursor); }
iGame* iRoomClient::getNextGame() { return next<iGame>(games,roomClientRuntime->gameCursor); }
iGame* iRoomClient::findGame(const char* name) { return find<iGame>(games,name,roomClientRuntime->gameCursor); }
iGame* iPlayer::getGame() { return roomClientRuntime->client.getGameBySlot(game_id); }
bool iPlayer::isMaster() { auto* value=getGame(); return value && value->master_id==getID(); }
iPlayer* iGame::getMaster() { return roomClientRuntime->client.getPlayerBySlot(master_id); }
int iGame::getNumPlayers() {
    int count=0;
    for (auto* value=roomClientRuntime->client.getFirstPlayer();value;
         value=roomClientRuntime->client.getNextPlayer()) if (value->game_id==getID()) ++count;
    return count;
}
iPlayer* iRoomClient::findCurrentGameStatePlayer() {
    if (inGame() && gamePlayers.currentPlayer)
        for (int i=0;i<4;++i) {
            auto* value=playerStates.getSlot(i);
            if (value && value->gamePlayerIndex==gamePlayers.currentPlayer->playerIndex) return value->networkPlayer;
        }
    return nullptr;
}
const char* iRoomClient::getError() {
    return inRoom() ? netRoomClient::getErrorString() : "";
}
bool iRoomClient::allPlayersInState(int state) {
    if (inGame()) for (int i=0;i<4;++i) {
        auto* value=playerStates.getSlot(i);
        if (value && value->state!=state) return false;
    }
    return true;
}
}
