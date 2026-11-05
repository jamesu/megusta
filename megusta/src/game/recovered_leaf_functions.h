#pragma once
#include "room_client.h"

namespace megusta {
int GuiWidget_sharedReturnZero_impl();
void binding_shared_noop();
void game_netRecord_copyName_shared(NetNamedRecord* self,const NetNamedRecord* source);
}
