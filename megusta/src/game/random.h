#pragma once
#include <cstdint>
namespace megusta {
class Random {
public:
    std::uint32_t state;
    void setSeed(std::uint32_t value);
    void seedFromClock();
    std::uint32_t generate();
};
extern Random ballCollisionRandom;
}
