#include "room_client.h"
#include "driver.h"
#include "random.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::handleJoinGame(int id) {
    auto& runtime=*roomClientRuntime;
    auto* player=getPlayerBySlot(id);
    if (!player || !player->game_id) runtime.fatal();
    if (id==getLocalID()) {
        runtime.config.push(); playerStates.clear();
        auto* slot=playerStates.addPlayer(player);
        if (player->watching || !slot) runtime.fatal();
        for (auto* other=getFirstPlayer();other;other=getNextPlayer())
            if (other!=player && other->game_id==player->game_id && !other->watching)
                playerStates.addPlayer(other);
        slot->config.copySection("Player0",runtime.config);
        auto* game=player->getGame();
        if (!isGameMaster()) {
            gameConfig.removeSection("Game"); gameConfig.removeSection("Custom");
            if (game->in_progress && !player->watching) runtime.fatal();
            setPlayerGameState(0);
            netMessage message(1,0);
            slot->config.writeSectionMessage("Player0",message); sendMessage(message);
            isHost=0;
        } else {
            gameConfig.copySection("Game",runtime.config); gameConfig.removeSection("Custom");
            const char* course=runtime.config.readString("Game","course",nullptr);
            for (const char* found=std::strstr(course,"SLOT");found;found=std::strstr(found+1,"SLOT")) {
                int index=std::atoi(found+4);
                char par[10],name[10],hole[12];
                std::sprintf(par,"%dpar",index); std::sprintf(name,"%dname",index); std::sprintf(hole,"%dhole",index);
                const char* keys[]={par,name,hole};
                for (auto* key:keys) {
                    const char* value=runtime.config.readString("Custom",key,nullptr);
                    if (std::strlen(value)) gameConfig.writeString("Custom",key,value);
                }
            }
            isHost=1;
            if (game->in_progress || game->getNumPlayers()!=1) runtime.fatal();
            setPlayerGameState(1);
        }
    } else {
        if (!player->watching) playerStates.addPlayer(player);
        char text[256]; std::sprintf(text,"%s joined.",player->name); handleSay(text);
    }
    if (isHost) broadcastGameConfig();
}
void iRoomClient::handleLeaveGame(int id) {
    auto& runtime=*roomClientRuntime;
    auto* player=getPlayerBySlot(id);
    if (!player || player->game_id) return;
    if (id!=getLocalID()) {
        if (player==findCurrentGameStatePlayer() && gameHole.hasActiveScene(runtime.objects)) {
            pickupBroadcastEnabled=false; runtime.executePython("hud.pickUpBall()"); pickupBroadcastEnabled=true;
        } else if (!strings::compareInsensitive(gameDriver.activeScript,"SELCTTME"))
            runtime.executePython("menu.goto(\"net_setp\")");
        char text[256]; std::sprintf(text,"%s left.",player->name); handleSay(text);
        auto* local=getLocalPlayer(); auto* game=getGameBySlot(local->game_id);
        if (!player->watching && game && game->in_progress) {
            auto* slot=playerStates.findPlayer(player);
            char section[32]; std::sprintf(section,"Player%d",slot->gamePlayerIndex);
            runtime.config.writeInt(section,"dead",1);
        }
        playerStates.removePlayer(player);
        if (isHost) broadcastGameConfig();
    } else {
        exitGame(); runtime.config.pop(); playerStates.removePlayer(player);
    }
}
void iRoomClient::initSettings() {
    if (!inGame()) return;
    auto& config=roomClientRuntime->config;
    config.copySection("Game",gameConfig); config.copySection("Custom",gameConfig);
    auto* local=getLocalPlayer();
    int putting=config.readInt("Game","PuttOnline",0);
    switch (putting) { case 0: putting=-1; break; case 1: putting=1; break; case 2: putting=0; break; case 3: putting=2; break; }
    int count=0;
    for (auto* player=getFirstPlayer();player;player=getNextPlayer()) if (player->game_id==local->game_id) {
        ++count;
        if (auto* state=playerStates.findPlayer(player)) {
            state->gamePlayerIndex=player->protocolValue;
            char section[20]; std::sprintf(section,"Player%d",player->protocolValue);
            config.copySection("Player0",state->config,section);
            config.writeString(section,"name",player->name);
            if (putting>=0) config.writeInt(section,"putting",putting);
        }
    }
    char number[12]; std::sprintf(number,"%d",count); config.writeString("Game","numPlayers",number);
}
void iRoomClient::requestGameConfig(const char* name) {
    if (inRoom() && !inGame()) { netMessage message(0xd0,0); message.puts(name); sendMessage(message); }
}
void iRoomClient::broadcastGameConfig() {
    if (!inGame() || !isHost) return;
    auto* local=getLocalPlayer(); auto* game=getGameBySlot(local->game_id);
    char payload[256],scratch[32];
    std::strncpy(payload,game->name,31); payload[30]=0;
    std::strcat(payload,"\t");
    std::strncpy(scratch,gameConfig.readString("Game","Course",""),3); scratch[3]=0;
    std::strcat(payload,scratch);
    int count=0;
    for (auto* player=getFirstPlayer();player;player=getNextPlayer()) if (player->game_id==game->getID()) {
        std::strncpy(scratch,player->name,31); scratch[30]=0;
        std::strcat(payload,"\t"); std::strcat(payload,scratch); ++count;
    }
    for (;count<4;++count) std::strcat(payload,"\t");
    const char* keys[]={"puttOnline","hazards","powerups","bonusstrokes","mode"};
    for (auto* key:keys) {
        std::strcat(payload,"\t");
        // 004f3788 is the literal "0" used as the default for all five options.
        std::strcat(payload,gameConfig.readString("Game",key,"0"));
    }
    // The original begins this message with source byte 0x1c before the
    // common send operation replaces it with the local ID.
    netMessage message(0xdb,0,0x1c);
    // 004f3730: "Net: updating game_config : %s\n".
    std::printf("Net: updating game_config : %s\n",payload);
    message.puts(game->name); message.puts(payload); sendMessage(message);
}
void iRoomClient::startGame() {
    if (inGame() && isGameMaster() && allPlayersInState(1)) {
        netMessage message(15,0);
        // 004f3820 contains the exact default string "-1".
        message.puts(roomClientRuntime->config.readString("Game","Teams","-1"));
        message.puti(roomClientRuntime->random.state); sendMessage(message);
        netRoomClient::startGame();
    }
}
void iRoomClient::toggleInvites() {
    if (inviteFlags&2) { inviteFlags&=~2u; handleSay("Invites excepted"); }
    else { inviteFlags|=2; handleSay("Invites ignored"); }
}
void iRoomClient::pollDisconnectAndTurnState() {
    if (!poll(0)) {
        if (inGame() || inRoom()) {
            if (gameHole.isTransitioning(roomClientRuntime->objects)) {
                exitGame(); roomClientRuntime->config.pop();
            }
            players.removeAll(); games.removeAll();
            gameDriver.changeScript("mainmenu",roomClientRuntime->scripts);
        }
    } else if (localNetworkState()==5) sendTurnOver();
}
}
