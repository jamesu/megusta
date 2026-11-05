#pragma once
#include <cstdint>
namespace megusta {
// The first 128 indices use the existing recovered Win32 table snapshot.
// Other entries use documented call-site mappings or remain unmapped.
class Win32GLTokenTable {
public:
    static constexpr std::uint32_t unmapped=0xffffffffu;
    Win32GLTokenTable();
    bool isMapped(unsigned token) const { return token<0xa9u && values[token]!=unmapped; }
    std::uint32_t values[0xa9];
};
}
