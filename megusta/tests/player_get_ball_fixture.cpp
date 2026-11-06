// Camera tests need this small Player accessor but deliberately do not link
// the rest of player.cpp and its unrelated runtime services.
#include "game/player.h"
#include "game/balls.h"
#include "game/sounds.h"
namespace megusta {
Ball* Player::getBall(BallsManager& balls) const { return balls.balls+playerIndex; }
bool Sounds::isPlaying(const char*,int) { return false; }
bool Sounds::play3d(const char*,float,float,float,int,int) { return false; }
}
