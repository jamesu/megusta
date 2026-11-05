#include "room_client.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
namespace { constexpr const char* roomNameCollisionFormat=".%d"; }
void iRoomClient::receiveServerAddresses(const netMessage& message) {
    for (;;) {
        char host[32];
        message.gets(host,31);
        if (!*host) return;
        int port=message.geti();
        bool found=false;
        for (int id=1;id<256;++id) if (auto* room=rooms.get(id))
            if (!strings::compareInsensitive(room->host,host) && room->port==port) { found=true; break; }
        if (found) continue;
        if (!rooms.add(new iRoom(host,port,1))) return;
    }
}
void iRoomClient::receiveRoomInfo(const netMessage& message) {
    netRoom incoming;
    if (message.geti()!=0x22) return;
    incoming.get(message);
    int collisions=0;
    for (auto* room=getFirstRoom();room;room=getNextRoom()) {
        if (std::strncmp(room->name,incoming.name,std::strlen(incoming.name))!=0) continue;
        bool existingAddress=false;
        for (int id=1;id<256;++id) if (auto* candidate=rooms.get(id))
            if (!strings::compareInsensitive(candidate->host,incoming.host) && candidate->port==incoming.port)
                { existingAddress=true; break; }
        if (existingAddress) continue;
        auto oldLength=std::strlen(room->name),newLength=std::strlen(incoming.name);
        if (oldLength==newLength) ++collisions;
        if (oldLength>newLength && room->name[newLength]=='.') ++collisions;
    }
    if (collisions>0) {
        char suffix[16],name[32];
        std::sprintf(suffix,roomNameCollisionFormat,collisions);
        auto limit=30-std::strlen(suffix);
        std::strncpy(name,incoming.name,limit); name[limit]=0;
        // Compatibility: terminate the native strncpy result before strcat.
        // Saved terminator behavior remains unresolved, so this stays partial.
        std::strcat(name,suffix); std::strcpy(incoming.name,name);
    }
    netRoom* destination=nullptr;
    for (int id=1;id<256;++id) if (auto* room=rooms.get(id))
        if (!strings::compareInsensitive(room->host,incoming.host) && room->port==incoming.port)
            { destination=room; break; }
    if (!destination) destination=rooms.add(handleNewRoom());
    if (destination) destination->copy(&incoming);
}
void iRoomClient::receiveGameConfigText(const netMessage& message) {
    if (!inGame()) {
        message.gets(gameConfigText,255);
        std::printf(roomClientRuntime->receivedConfigTextFormat,gameConfigText);
        hasGameConfig=true;
    }
}
}
