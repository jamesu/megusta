#include "room_client.h"
#include "../engine/string_services.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern RoomClientRuntime* roomClientRuntime;
void iRoomClient::loadServerList() {
    rooms.removeAll();
    auto& config=roomClientRuntime->config;
    const char* sections[]={"Server","MyServer"};
    for (const char* section:sections) {
        char** keys=config.readKeys(section);
        if (!keys) continue;
        for (;*keys;++keys) {
            char* value=config.readString(section,*keys,nullptr);
            if (!value || !*value) continue;
            char address[32];
            std::strncpy(address,value,31); address[31]=0;
            if (char* colon=std::strchr(address,':')) {
                *colon=0;
                rooms.add(new iRoom(address,strings::toInt(colon+1),1));
            }
        }
    }
}
void iRoomClient::saveServerList() {
    auto& config=roomClientRuntime->config;
    config.removeSection("Server");
    for (int id=1;id<256;++id) if (auto* room=rooms.get(id)) {
        char address[64];
        // The decompiler loses the original variadic host/port arguments.
        // The surviving "%s:%d" literal and room layout establish intent.
        std::sprintf(address,"%s:%d",room->host,room->port);
        config.writeString("Server",room->name,address);
    }
}
iRoom* iRoomClient::recountRoomState() {
    // 004482e0 is an alias of the vendored/evidenced netRoomClient::getInfo.
    return const_cast<iRoom*>(getInfo());
}
}
