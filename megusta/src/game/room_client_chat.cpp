#include "room_client.h"
#include "console.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::handleSay(const char* text) {
    if (text!=latestChat) std::strcpy(latestChat,text);
    int count=chatHistory.count;
    if (count<10) {
        chatHistory.orderedText[count]=chatHistory.textStorage[count];
        std::strcpy(chatHistory.orderedText[count],latestChat);
        chatHistory.timestamps[count]=roomClientRuntime->timeMilliseconds();
        ++chatHistory.count;
    } else {
        char* oldest=chatHistory.orderedText[0];
        for (int i=0;i<9;++i) {
            chatHistory.orderedText[i]=chatHistory.orderedText[i+1];
            chatHistory.timestamps[i]=chatHistory.timestamps[i+1];
        }
        chatHistory.orderedText[9]=oldest;
        std::strcpy(oldest,latestChat);
        chatHistory.timestamps[9]=roomClientRuntime->timeMilliseconds();
    }
    roomClientRuntime->playSound("CHATBEEP",0);
    if (chatCallback) {
        roomClientRuntime->executePython(chatCallback);
        gameConsole.forwardNetworkLine(latestChat);
    }
}
std::uint8_t iRoomClient::getIgnore(const char* name) {
    if (inRoom())
        if (auto* player=findPlayer(name)) return ignoredPlayers[player->getID()];
    return 0;
}
void iRoomClient::setIgnore(const char* name,int ignored) {
    if (!inRoom()) return;
    auto* player=findPlayer(name);
    if (!player) std::sprintf(latestChat,"Player not found: %s",name);
    else if (player==getLocalPlayer()) {
        std::sprintf(latestChat,"Ignore yourself, lol.");
        handleSay(latestChat); return;
    } else {
        ignoredPlayers[player->getID()]=ignored!=0;
        std::sprintf(latestChat,ignored ? "%s added to ignore list" : "%s removed from ignore list",player->name);
    }
    handleSay(latestChat);
}
void iRoomClient::whenSaid(const char* callback) {
    if (chatCallback) { std::free(chatCallback); chatCallback=nullptr; }
    if (callback) chatCallback=strings::duplicate(callback);
}
char* iRoomClient::lastSaid() { return latestChat; }
int iRoomClient::getNumSaid() { return chatHistory.count; }
const char* iRoomClient::getSaid(int index) {
    return index>=0 && index<chatHistory.count ? chatHistory.orderedText[index] : "";
}
std::uint32_t iRoomClient::getSaidMetadata(int index) {
    return index>=0 && index<chatHistory.count ? chatHistory.timestamps[index] : 0;
}
void iRoomClient::clearSaid() { chatHistory.count=0; }
char* formatChatLocalTime(const std::time_t* time) {
    char* result=std::asctime(std::localtime(time));
    std::size_t length=std::strlen(result);
    if (result[length-1]=='\n') result[length-1]=0;
    return result;
}
char* roomClientDate() {
    auto time=std::time(nullptr);
    std::tm local=*std::localtime(&time);
    std::sprintf(roomClientRuntime->dateText,"%.2d%.2d%.4d",local.tm_mon+1,local.tm_mday,local.tm_year+1900);
    return roomClientRuntime->dateText;
}
char* inviteGameName() { return roomClientRuntime->inviteGameName; }
char* inviteGamePass() { return roomClientRuntime->inviteGamePass; }
void whenInvited(const char* callback) {
    auto& current=roomClientRuntime->invitedCallback;
    if (current) { std::free(current); current=nullptr; }
    if (callback) current=strings::duplicate(callback);
}
}
