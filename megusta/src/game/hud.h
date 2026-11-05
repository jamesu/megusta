#pragma once
#include <cstdint>
class fntRenderer;
class fntFont;
namespace megusta {
// The saved game exposes this same process-global renderer through gamec.cvar.
extern fntRenderer editorFontRenderer;
class Camera;
class BallsManager;
struct HoleFrameClock;
enum class PutterHudTextureSlot : int {
    powerNeedle = 3,
    simpleNeedle = 4,
    leftBar = 5,
    rightBar = 6,
    highPowerBar = 7
};
struct PlayerOverlayServices {
    fntRenderer& renderer;
    fntFont* primaryFont;
    Camera& camera;
    BallsManager& balls;
    int screenWidth,screenHeight;
    std::uint32_t fixedTick;
    bool nameTags;
    const float* messageColor;
};
struct PutterHudTextures { std::uint32_t leftBar,rightBar,highPowerBar,simpleNeedle,powerNeedle; };
void drawOutlinedPlayerText(const char*,int,int,fntRenderer&,const float* rgb);
void drawPerformanceOverlay(fntFont*,const HoleFrameClock&,int width,int height,bool networkInGame);
}
