#include "room_client.h"
#include <cstdio>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::receivePlayerConfig(const netMessage& message) {
    if (!inGame()) return;
    auto* player=getPlayerBySlot(message.getFromID());
    if (!player || player==getLocalPlayer()) return;
    if (auto* state=playerStates.findPlayer(player)) state->config.readSectionMessage("Player0",message);
    if (!isGameMaster()) return;
    auto* section="Game";
    int capacity=gameConfig.measureSectionText("Custom")+gameConfig.measureSectionText(section)+0x800;
    netMessage reply(2,player->getID(),0,capacity);
    reply.puti(playerStates.count());
    for (int i=0;i<4;++i) if (auto* state=playerStates.getSlot(i)) {
        reply.puti(state->networkPlayer->getID()); reply.puti(state->state);
        state->config.measureSectionText("Player0");
        state->config.writeSectionMessage("Player0",reply);
    }
    gameConfig.writeSectionMessage(section,reply);
    gameConfig.writeSectionMessage("Custom",reply);
    sendMessage(reply);
    std::printf(roomClientRuntime->sentWelcomeFormat,reply.getLength());
}
void iRoomClient::receiveWelcome(const netMessage& message) {
    if (!inGame() || isGameMaster() || localNetworkState()!=0) return;
    std::printf(roomClientRuntime->gotWelcomeFormat,message.getLength());
    int count=message.geti();
    for (int i=0;i<count;++i) {
        int id=message.geti(); std::uint32_t state=message.geti();
        auto* player=getPlayerBySlot(id); auto* slot=playerStates.findPlayer(player);
        if (!player || !slot) roomClientRuntime->fatal();
        setPlayerGameState(state,player); slot->config.readSectionMessage("Player0",message);
    }
    const char* section="Game";
    gameConfig.removeSection(section); gameConfig.removeSection("Custom");
    gameConfig.readSectionMessage(section,message); gameConfig.readSectionMessage("Custom",message);
    roomClientRuntime->config.copySection(section,gameConfig);
    roomClientRuntime->config.copySection("Custom",gameConfig);
    if (localNetworkState()!=0) roomClientRuntime->fatal();
    setPlayerGameState(1);
}
}
