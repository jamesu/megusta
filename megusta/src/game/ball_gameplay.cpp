#include "ball.h"
#include "player.h"
#include <cmath>
namespace megusta {
Player* Ball::getPlayer(const PlayerServices& services) const {
    return gamePlayers.get(static_cast<std::uint32_t>(playerIndex),services);
}
void Ball::setColour(int colour,void (*fatal)()) {
    colorModelIndex=colour;
    if (colour<0 || colour>17) fatal();
}
bool Ball::lost(const BallGameplayServices& services) {
    if ((flags&1u) && restHistoryIndex) --restHistoryIndex;
    BallRestHistoryEntry& entry=restHistory[restHistoryIndex];
    flags=entry.flags; lastContactObjectIndex=0;
    position[0]=entry.position[0]; position[1]=entry.position[1]; position[2]=entry.position[2]; flags |= 4u;
    velocity[0]=gameZeroVector[0]; velocity[1]=gameZeroVector[1]; velocity[2]=gameZeroVector[2];
    Player* player=services.playerByIndex(services.context,playerIndex);
    bool penalty=static_cast<std::uint32_t>(player->strokeCount+player->penaltyCount)<=
                 static_cast<std::uint32_t>(entry.strokeAndPenaltyCount)+1u;
    if (penalty) { player->lastStrokePenaltyFlag=true; ++player->penaltyCount; }
    restHistory[restHistoryIndex].strokeAndPenaltyCount=player->penaltyCount+player->strokeCount;
    if (!restHistory[restHistoryIndex].supportObjectIndex) {
        if (restHistoryIndex) --restHistoryIndex;
        resetMotionState();
    }
    services.restorePlayerHeading(services.context); return penalty;
}
void Ball::strike(const Point3F& direction,const BallGameplayServices& services) {
    services.recordStroke(services.context);
    velocity[0]=direction.x; velocity[1]=direction.y; velocity[2]=direction.z; resetMotionState();
    services.playStrikeSound(services.context,position);
    Player* player=services.currentPlayer(services.context);
    if (player && player->isHuman) {
        float squared=direction.z*direction.z+direction.y*direction.y+direction.x*direction.x;
        long double speed=std::sqrt(static_cast<long double>(squared));
        int delay=static_cast<int>(speed*10.0L+150.0L);
        if (delay>255) delay=255;
        player->puttDeadlineMs=static_cast<std::uint32_t>((delay*1000)/1000)+services.timeMilliseconds(services.context);
        services.serviceNetworkEnd(services.context);
    }
}
}
