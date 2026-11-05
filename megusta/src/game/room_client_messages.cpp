#include "netMessage.h"
#include "room_client.h"
#include "hole.h"
#include "scene_object.h"
#include "random.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::receiveInvite(const netMessage& message) {
    if (!(inviteFlags&2)) {
        message.gets(roomClientRuntime->inviteGameName,32);
        message.gets(roomClientRuntime->inviteGamePass,32);
        if (roomClientRuntime->invitedCallback)
            roomClientRuntime->executePython(roomClientRuntime->invitedCallback);
    }
}
void iRoomClient::receiveChat(const netMessage& message) {
    char text[256];
    message.gets(text,256);
    auto* sender=getPlayerBySlot(message.getFromID());
    if (!sender) std::sprintf(latestChat,"sys> %s",text);
    else {
        if (ignoredPlayers[sender->getID()]) return;
        if (!message.getToID()) std::sprintf(latestChat,"%s> %s",sender->name,text);
        else if (sender==getLocalPlayer()) {
            auto* recipient=getPlayerBySlot(message.getToID());
            std::sprintf(latestChat,"Whisper to %s> %s",recipient->name,text);
        } else {
            std::strcpy(whisperReplyTarget,sender->name);
            std::sprintf(latestChat,"Whisper from %s> %s",sender->name,text);
        }
    }
    handleSay(latestChat);
}
void iRoomClient::receivePlayerState(const netMessage& message) {
    // Malformed-frame behavior remains unresolved in the saved pointer alias.
    std::uint32_t state=message.geti();
    if (auto* player=getPlayerBySlot(message.getFromID())) setPlayerGameState(state,player);
}
void iRoomClient::sendInvite(const char* name,const char* password) {
    auto& runtime=*roomClientRuntime;
    if (!inGame() || !isGameMaster()) return;
    auto* local=getLocalPlayer();
    auto* localGame=local->getGame();
    if (localGame->in_progress) return;
    auto* recipient=findPlayer(name);
    if (!recipient || recipient->getGame()) return;
    netMessage message(10,recipient->getID());
    message.puts(localGame->name);
    message.puts(password);
    sendMessage(message);
    std::sprintf(latestChat,"%s invited",recipient->name);
    handleSay(latestChat);
}
void iRoomClient::setPlayerGameState(std::uint32_t state,iPlayer* player) {
    bool local=player==nullptr;
    if (local) { player=getLocalPlayer(); if (!player) return; }
    auto* slot=player->game_id ? playerStates.findPlayer(player) : nullptr;
    if (!slot || slot->state==state) return;
    std::printf("Net: %s (%s) %s -> %s\n",local ? "LOCAL" : "REMOTE",player->name,
        networkStateName(slot->state),networkStateName(state));
    slot->state=state;
    if (local) {
        netMessage message(9,0);
        message.puti(state); sendMessage(message);
    }
}
void iRoomClient::listGameStates() {
    if (!inGame()) return;
    std::printf("Net: game states:\n");
    for (int i=0;i<4;++i) if (auto* slot=playerStates.getSlot(i)) {
        auto* player=slot->networkPlayer;
        std::printf("Net: %d: %s, state=%s",player->getID(),player->name,slot->getName());
        if (player==getLocalPlayer()) std::printf("<-- local");
        std::printf("\n");
    }
}
void iRoomClient::receiveTeamSeed(const netMessage& message) {
    if (inGame() && !isGameMaster()) {
        char teams[32];
        message.gets(teams,32);
        gameConfig.writeString("Game","Teams",teams);
        roomClientRuntime->config.writeString("Game","Teams",teams);
        roomClientRuntime->random.setSeed(message.geti());
    }
}
void iRoomClient::receivePuttOver(const netMessage& message) {
    if (inGame() && !isGameMaster()) {
        roomClientRuntime->puttOverFrame=message.geti();
        std::printf(roomClientRuntime->puttOverFormat,roomClientRuntime->puttOverFrame);
    }
}
void iRoomClient::receiveFrameGate(const netMessage& message) {
    if (inGame() && !isGameMaster()) roomClientRuntime->frameGate=message.geti();
}
void iRoomClient::receiveResetTurn() {
    setPlayerGameState(5);
    gameHoleClock.resetFrameCounters(roomClientRuntime->timeMilliseconds());
    roomClientRuntime->objects.resetAnimationFrames(roomClientRuntime->animation);
}
}
