#include "../src/dts/graphics_state.h"
#include <cassert>

namespace {
const int* displayDimensions() {
    static const int dimensions[2]={1024,768};
    return dimensions;
}
}

int main() {
    auto* size=FUN_004d4860();
    assert(size->width==640 && size->height==480);
    megusta::dts::bindDisplaySizeProvider(displayDimensions);
    size=FUN_004d4860();
    assert(size->width==1024 && size->height==768);
    FUN_004d48c0();
}
