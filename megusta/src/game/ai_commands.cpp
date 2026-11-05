#include "player_ai.h"
#include "player.h"
#include "putt_map_runtime.h"
#include "engine/string_services.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace megusta {
int executeAICommand(const char* command,PlayersManager& players,BallsManager& balls,const PlayerServices& services,
        PuttMapRuntime* scripts,int adjustment,std::uint32_t tick) {
    if (players.currentPlayer) players.currentPlayer->getBall(balls);
    if (!strings::compareInsensitiveBounded(command,"addcustomfile ",14)) {
        if (!scripts) throw std::logic_error("AI custom file requires PuttMapRuntime"); scripts->loadCustomScript(command+14); return 0;
    }
    if (!strings::compareInsensitiveBounded(command,"addcustom ",10)) {
        if (!scripts) throw std::logic_error("AI custom statement requires PuttMapRuntime"); scripts->parseCustomStatement(command+10); return 0;
    }
    if (!strings::compareInsensitiveBounded(command,"learn",5)) {
        int index=-1,result=0; std::sscanf(command+5,"%d %d",&index,&result);
        if (index>=0) players.get(index,services)->ai.learn(result); return 0;
    }
    if (!strings::compareInsensitiveBounded(command,"aiparm",6)) {
        for (unsigned i=0;i<players.playerCount;++i) {
            auto* player=players.get(i,services); char buffer[128]{}; std::strncpy(buffer,command+6,127);
            bool separator=false; char* value=nullptr;
            for (char* cursor=buffer;*cursor;++cursor) {
                if (*cursor<'!') { *cursor='\0'; separator=true; }
                else if (separator && !value) value=cursor;
            }
            if (value) player->ai.applySetting(buffer,value);
        }
    } else if (!strings::compareInsensitiveBounded(command,"holetweak",9)) {
        for (unsigned i=0;i<players.playerCount;++i) {
            auto* player=players.get(i,services); player->ai.applySetting("aiHoleFact",command+9); player->ai.beginHole(services.random,adjustment,tick);
        }
    }
    return 0;
}
}
