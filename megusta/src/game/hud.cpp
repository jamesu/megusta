#include "hud.h"
#include "player.h"
#include "balls.h"
#include "camera.h"
#include "hole.h"
#include "putter.h"
#include "effect.h"
#include "fnt.h"
#include "ssg.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace megusta {
fntRenderer editorFontRenderer{};
void drawOutlinedPlayerText(const char* text,int x,int y,fntRenderer& renderer,const float* rgb) {
    renderer.start2f(static_cast<float>(x),static_cast<float>(y)); glColor3f(0,0,0); renderer.puts(text);
    renderer.start2f(static_cast<float>(x-1),static_cast<float>(y+1)); glColor3fv(rgb); renderer.puts(text);
}
void PlayersManager::drawOverlay(PlayerOverlayServices& services) {
    auto& renderer=editorFontRenderer; renderer.setFont(services.primaryFont); renderer.setPointSize(32); renderer.begin();
    if (*message) {
        if (services.fixedTick-messageStartTick<messageDuration) {
            if (!services.messageColor) throw std::logic_error("Player message foreground color not bound");
            int y=services.screenHeight/2,size=32; if (std::strlen(message)>80) { y=460; size=14; }
            char* line=message;
            while (*line) {
                renderer.setPointSize(static_cast<float>(size)); char* end=line;
                while (end-message<4090 && *end!='\n') { if (*end=='\r' || !*++end) break; }
                const char saved=*end; *end='\0'; float left,right,bottom,top;
                renderer.getFont()->getBBox(line,renderer.getPointSize(),renderer.getSlant(),&left,&right,&bottom,&top);
                const int width=static_cast<int>(static_cast<long double>(right)-left);
                drawOutlinedPlayerText(line,(services.screenWidth-width)/2,y,renderer,services.messageColor);
                renderer.setPointSize(32); *end=saved;
                if (!saved || end-message>4089) break;
                do { ++end; } while ((*end=='\n' || *end=='\r') && end-message<4090);
                line=end; y-=size;
            }
        } else showMessage(nullptr,0,services.fixedTick);
    }
    renderer.end();
    if (!currentPlayer) return;
    displayedStrokeCount=currentPlayer->penaltyCount+currentPlayer->strokeCount;
    if (currentPlayer->isDone(services.balls) || turnState!=1) return;
    ++displayedStrokeCount;
    if (playerCount<=1 || !services.nameTags) return;
    const float colors[4][3]={{1,1,0},{0,0,1},{1,0,1},{1,0,0}}; renderer.setPointSize(14);
    for (unsigned i=0;i<playerCount;++i) {
        auto& player=playerStorage[i]; auto* ball=player.getBall(services.balls); CameraScreenPoint screen;
        if (&player!=currentPlayer && !(ball->flags&10) && !(ball->flags&0x10) && ball->restHistoryIndex &&
            services.camera.projectPointToScreen({ball->position[0],ball->position[1],ball->position[2]},screen))
            drawOutlinedPlayerText(player.name,screen.x+10,services.screenHeight-screen.y-14,renderer,colors[i]);
    }
    renderer.setPointSize(32);
}
void drawPerformanceOverlay(fntFont* font,const HoleFrameClock& clock,int width,int height,bool network) {
    if (!(clock.fixedTicksPerSecond>0)) return;
    fntRenderer renderer; renderer.setFont(font); renderer.setPointSize(14); renderer.begin();
    renderer.start2f(static_cast<float>(width/2-150),static_cast<float>(height-16)); glColor4f(0,0,0,1);
    const char* stats=ssgShowStats();
    for (int pass=0;pass<2;++pass) {
        char text[1024]; std::sprintf(text,"%s, FPS=%.02f ",stats,static_cast<double>(clock.framesPerSecond)); renderer.puts(text);
        if (network) { std::sprintf(text,"tick_count : %d ",clock.totalFixedTickCount); renderer.puts(text); }
        renderer.start2f(static_cast<float>(width/2-151),static_cast<float>(height-15)); glColor4f(1,1,1,1);
    }
    renderer.end();
}
namespace {
void hudQuad(std::uint32_t texture,int left,int right,int bottom,int top,float u0,float u1) {
    glBindTexture(GL_TEXTURE_2D,texture); glBegin(GL_QUADS);
    glTexCoord2f(u0,0); glVertex2i(left,top); glTexCoord2f(u1,0); glVertex2i(right,top);
    glTexCoord2f(u1,1); glVertex2i(right,bottom); glTexCoord2f(u0,1); glVertex2i(left,bottom); glEnd();
}
int hudPowerPixel(float power) {
    int pixel=static_cast<int>(static_cast<long double>(power)*356);
    if (pixel<356) { if (pixel<1) pixel=0; } else pixel=356; return pixel;
}
}
void Putter::drawPowerHud(const Player& player,bool ready,bool waiting,const PutterHudTextures& textures) {
    if (!powerBarVisible || !ready || waiting || state<1 || (state>6 && state!=9)) return;
    const int mode=player.controlMode;
    if (mode<0 || mode>2) return;
    glColor4f(1,1,1,mode?1.0f:0.75f); glEnable(GL_TEXTURE_2D);
    hudQuad(textures.leftBar,hudCenterX-96,hudCenterX+141,36,68,0.07f,1);
    const auto right=(!mode && (effectivePower>=0.28f || power>=0.28f))?textures.highPowerBar:textures.rightBar;
    hudQuad(right,hudCenterX+141,hudCenterX+260,36,68,0,0.9f);
    if (mode) {
        const int offset=hudPowerPixel(power)-348; hudQuad(textures.simpleNeedle,hudCenterX-offset-96,hudCenterX-offset-80,36,68,0,1);
        glDisable(GL_TEXTURE_2D); return;
    }
    const int effective=hudPowerPixel(effectivePower)-348;
    if (power!=0) {
        const int offset=hudPowerPixel(power)-356; glColor4f(1,0,0,1);
        hudQuad(textures.powerNeedle,hudCenterX-offset-104,hudCenterX-offset-88,36,68,0,1);
    }
    glColor4f(1,1,1,1); hudQuad(textures.powerNeedle,hudCenterX-effective-96,hudCenterX-effective-80,36,68,0,1);
    static_cast<Effect*>(effect)->drawOverlayStyles(); glDisable(GL_TEXTURE_2D);
}
}
