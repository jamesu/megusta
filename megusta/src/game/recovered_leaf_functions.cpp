#include "recovered_leaf_functions.h"
#include "../engine/string_services.h"

namespace megusta {

int GuiWidget_sharedReturnZero_impl() { return 0; }
void binding_shared_noop() {}

void game_netRecord_copyName_shared(NetNamedRecord* self,const NetNamedRecord* source) {
    // netThing stores name inline in the vendored PLIB declaration, so its
    // address is always non-null; the decompiler's pointer test folds away.
    strings::copyBounded(self->name,source->name,0x1f);
    self->name[0x1f]='\0';
}

} // namespace megusta
