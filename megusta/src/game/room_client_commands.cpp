#include "room_client.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace {
constexpr char ReplyCommand[]="/r "; // Exact bytes at 005032b8.
constexpr char IgnoreCommand[]="/i "; // Exact bytes at 005032bc.
constexpr char WhisperCommand[]="/w "; // Exact bytes at 005032c0.
constexpr char TimeOutputFormat[]="%s\n"; // Exact bytes at 005032c4.
}
void iRoomClient::say(char* text) {
    if (!inRoom()) {
        if (*text && std::strncmp(text,"/addserver",10)==0) {
            if (char* colon=std::strchr(text+11,':')) {
                *colon=0;
                rooms.add(new iRoom(text+11,strings::toInt(colon+1),1));
            }
        }
        return;
    }
    if (!*text) return;
    if (std::strncmp(text,"/time",5)==0) {
        auto time=std::time(nullptr);
        char output[32];
        std::sprintf(output,TimeOutputFormat,formatChatLocalTime(&time));
        handleSay(output); return;
    }
    char name[32];
    if (std::strncmp(text,WhisperCommand,3)==0) {
        char* start=text+(text[3]=='\'' ? 4 : 3);
        std::strncpy(name,start,30); name[30]=0;
        for (int i=0;i<31;++i) if (name[i]==(text[3]=='\'' ? '\'' : ' ')) {
            name[30]=0; name[i]=0; break;
        }
        sayTo(start+std::strlen(name)+1,name); return;
    }
    if (std::strncmp(text,IgnoreCommand,3)==0) {
        if (!text[3]) std::strcpy(name,whisperReplyTarget);
        else {
            std::strncpy(name,text+(text[3]=='\'' ? 4 : 3),30); name[30]=0;
            if (text[3]=='\'') for (int i=0;i<31;++i) if (name[i]=='\'') { name[i]=0; break; }
        }
        setIgnore(name,getIgnore(name)==0); return;
    }
    if (std::strncmp(text,ReplyCommand,3)==0) {
        std::strcpy(name,whisperReplyTarget); sayTo(text+3,name); return;
    }
    netMessage message(netRoom::MSG_SAY,0);
    message.puts(text); sendMessage(message);
}
void iRoomClient::sayTo(const char* text,const char* name) {
    if (!inRoom() || !*text) return;
    auto* recipient=findPlayer(name);
    if (!recipient) {
        std::sprintf(latestChat,"Player not found: %s",name);
        handleSay(latestChat);
    } else if (recipient==getLocalPlayer()) {
        std::sprintf(latestChat,"%s, stop talking to yourself.",recipient->name);
        handleSay(latestChat);
    } else {
        netMessage message(netRoom::MSG_SAY,recipient->getID());
        message.puts(text); sendMessage(message);
    }
}
}
