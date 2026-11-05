#include "random.h"
#include "platform/game_bridge.h"
namespace megusta {
Random ballCollisionRandom{};
void Random::seedFromClock() { setSeed(platform::gameMilliseconds()); }
void Random::setSeed(std::uint32_t value) { state=value ? value : 0x11647u; }
std::uint32_t Random::generate() {
    std::uint32_t value=state;
    state=(((((value&0xfffffffeu)^(value*2u))<<1 ^ (value&0xfffffffcu))<<2 ^
            (value&0xfffffff0u))<<2 ^ (value&0xffffffc0u))<<25 ^ (value&0x80000000u) | value>>1;
    return state;
}
}
