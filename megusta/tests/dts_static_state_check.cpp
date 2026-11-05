#include "../src/dts/static_state.h"
#include <cassert>

int main() {
    DTS_registeredStartup_004d5db0();
    for (int i=0;i<8;++i) {
        const std::uint32_t expected=i>=1 && i<=4 ? 0x3f800000u : 0u;
        assert(megusta::dts::materialRenderStateWords[i]==expected);
    }
    megusta::dts::materialRenderStateWords[1]=0;
    DTS_staticInitializer_004d5dc0();
    assert(megusta::dts::materialRenderStateWords[1]==0x3f800000u);

    DTS_registeredStartup_004d5e20();
    const std::uint32_t expectedBasis[12]={0,0xbf800000,0, 0,0xbf800000,0,
                                            0,0xbf800000,0, 0,0xbf800000,0};
    for (int i=0;i<12;++i) assert(megusta::dts::billboardBasisStateWords[i]==expectedBasis[i]);
    megusta::dts::billboardBasisStateWords[1]=0;
    DTS_staticInitializer_004d5e30();
    assert(megusta::dts::billboardBasisStateWords[1]==0xbf800000u);
    DTS_registeredStartup_004d4700();
    DTS_staticInitializer_004d4710();
}
