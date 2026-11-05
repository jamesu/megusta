#include "gameplay_messages.h"
#include "game_shape_adapter.h"
#include "gui_runtime.h"
#include "player.h"
#include <cstring>
namespace megusta {
void setHoleAmbientColour(float r,float g,float b) { setGameShapeMeshColour(r,g,b); }
void showHudStringMessage(const char* text) {
    gamePlayers.showMessage(text,static_cast<std::uint32_t>(std::strlen(text)),
        guiRuntime->fixedTick(guiRuntime->context));
}
void showSpatialEditorMessage(const char* text) {
    gamePlayers.showMessage(text,static_cast<std::uint32_t>(std::strlen(text))<<1,
        guiRuntime->fixedTick(guiRuntime->context));
}
}
