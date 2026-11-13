#include "gui_shell.h"
#include "gui.h"
#include "gui_tooltip.h"
#include "gui_runtime.h"
#include "gui_render.h"
#include "driver.h"
#include "native_game_helpers.h"
#include "engine/opengl_adapter.h"
#include "engine/string_services.h"
#include "engine/runtime_adapters.h"
#include "engine/movie_playback.h"
#include "ssg.h"
#include "fnt.h"
#include "pu.h"
#include "ul.h"
#include "../engine/platform_gl.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta {
GuiShellState* guiShellState=nullptr;
GuiFontRegistry guiFonts;
void destroyGuiTextures(GuiTextureAtlas* textures) {
    if (textures) {
        for (std::uint32_t i=0;i<textures->textureCount;++i) guiRuntime->renderer->gl.deleteTextures(1,&textures->textureNames[i]);
        delete textures;
    }
}
void openGuiShell() {
    ssgSetFOV(60,0); driverViewport.set(640,480,false);
    if (!guiShellState) {
        guiShellState=new GuiShellState; guiShellState->slideRemaining=1;
        auto& registry=guiRuntime->renderer->atlases;
        auto load=[&](const char* name) { return pix::loadMultiTexture(registry.resources,name,true,0,GL_CLAMP,GL_NEAREST,GL_NEAREST); };
        guiShellState->backgroundAtlas=load("data/textures/gui/SHLBG.mpx");
        if (!guiShellState->backgroundAtlas || guiShellState->backgroundAtlas->textureCount!=9) std::abort();
        guiShellState->generalAtlas=load("data/textures/gui/GENER.mpx");
        if (!guiShellState->generalAtlas) std::abort();
        registry.shellOpen=true; registry.shellGeneralAtlas=guiShellState->generalAtlas;
        initializeGuiTooltip();
    }
}
void closeGuiShell() {
    if (guiShellState) {
        destroyGuiTextures(guiShellState->backgroundAtlas); guiShellState->backgroundAtlas=nullptr;
        for (auto*& font:guiShellState->fonts) { delete font; font=nullptr; }
        destroyGuiTextures(guiShellState->generalAtlas); guiShellState->generalAtlas=nullptr;
        delete guiShellState; guiShellState=nullptr;
        auto& registry=guiRuntime->renderer->atlases; registry.shellOpen=false; registry.shellGeneralAtlas=nullptr;
    }
}
bool isGuiShellOpen() { return guiShellState!=nullptr; }
void releaseGuiSharedAssets() {
    for (auto*& font:guiFonts.slots) { delete font; font=nullptr; }
    for (auto*& atlas:guiRuntime->renderer->atlases.atlases) { destroyGuiTextures(atlas); atlas=nullptr; }
}
fntTexFont* resolveGuiFont(int index) {
    if (index<200) {
        if (index>99) {
            index-=100;
            if (index<0 || index>9 || !guiShellState || !guiShellState->fonts[index]) std::abort();
            return guiShellState->fonts[index];
        }
        if (index<0 || index>9 || !guiFonts.slots[index]) std::abort();
        return guiFonts.slots[index];
    }
    if (index==201) return guiFonts.primary;
    if (index==202) return guiFonts.console;
    if (index!=203) std::abort();
    return guiFonts.secondary;
}
void guiSystem::advanceShellBackground() {
    if (guiShellState) { guiShellState->animationTime+=frameDelta; guiShellState->slideRemaining-=frameDelta; }
}
void guiSystem::drawShellBackground() {
    auto& renderer=*guiRuntime->renderer;
    if (backgroundTexture) {
        renderer.beginTexturedAlpha(1);
        renderer.drawTextureRect(backgroundTexture,0,guiRuntime->screenHeight,guiRuntime->screenWidth,guiRuntime->screenHeight,false);
        renderer.endTexturedAlpha(); renderer.gl.flush(); return;
    }
    auto* textures=static_cast<GuiTextureAtlas*>(backgroundTextures);
    if (!textures && guiShellState) textures=guiShellState->backgroundAtlas;
    if (!textures) return;
    renderer.beginTexturedAlpha(1);
    for (int row=0;row<2;++row) for (int column=0;column<3;++column)
        renderer.drawTextureRect(pix::getMultiTexture(textures,row*3+column),column*256,480-row*256,column==2?128:256,256,false);
    renderer.endTexturedAlpha();
    if (backgroundTextures) { renderer.gl.flush(); return; }
    float slide=guiShellState->slideRemaining; if (slide<0) slide=0;
    const int left=static_cast<int>(static_cast<long double>(slide)*-60);
    const int topOffset=static_cast<int>(-(static_cast<long double>(slide)*-100));
    renderer.beginTexturedAlpha(1);
    renderer.drawTextureRect(pix::getMultiTexture(textures,6),left,480,128,256,false);
    renderer.drawTextureRect(pix::getMultiTexture(textures,7),left,224,128,256,false);
    renderer.drawTextureRect(pix::getMultiTexture(textures,8),0,476+topOffset,256,128,false);
    const int frame=static_cast<int>(static_cast<long double>(guiShellState->animationTime)*10)%6;
    renderer.drawTextureRect(renderer.atlases.resolve(100,frame),24,460+topOffset,64,64,false);
    renderer.endTexturedAlpha(); drawShapePreviewBackgrounds();
}
void guiSystem::setBackground(const char* path) {
    auto& renderer=*guiRuntime->renderer;
    if (backgroundTexture) { renderer.gl.deleteTextures(1,&backgroundTexture); backgroundTexture=0; }
    if (backgroundTextures) { destroyGuiTextures(static_cast<GuiTextureAtlas*>(backgroundTextures)); backgroundTextures=nullptr; }
    if (path) {
        const auto length=std::strlen(path);
        if (!strings::compareInsensitive(path+length-4,".mpx")) {
            backgroundTextures=pix::loadMultiTexture(renderer.atlases.resources,path,true,0,GL_CLAMP,GL_NEAREST,GL_NEAREST);
        } else backgroundTexture=pix::bindPngTexture(renderer.atlases.resources,path,0,GL_CLAMP,GL_NEAREST,GL_NEAREST);
    }
}
void guiSystem::clearScreen() {
    setBackground(nullptr);
    for (int i=0;i<elementCount;++i) if (elements[i]) { elements[i]->release(); delete elements[i]; }
    elementCount=0; currentElement=nullptr; tabElement=nullptr; hasTabElements=false;
    guiRuntime->messageActive=false; bindings.release(); flushShapePreviews(); resetGuiTooltipDefaults();
    releaseGuiSharedAssets(); releaseZoomCache();
    if (previewRoot) { delete previewRoot; previewRoot=nullptr; }
}
int guiSystem::shellCmd(const char* command,int a,int b,int c,int d,int e) {
    const auto is=[&](const char* value){return !strings::compareInsensitive(command,value);};
    const auto prefix=[&](const char* value,int size){return !strings::compareInsensitiveBounded(command,value,size);};
    if (is("ShellOpen")) openGuiShell();
    else if (is("ShellClose")) closeGuiShell();
    else if (prefix("TooltipArea",11)) {
        const unsigned char digit=command[11]; int area=std::isdigit(digit)?digit-'0':0;
        if (area<0 || area>3) std::abort();
        tooltipAreas[area][0]=a; tooltipAreas[area][1]=b; tooltipAreas[area][2]=c; tooltipAreas[area][3]=d;
    } else if (is("TooltipDelay")) tooltipDelay=a;
    else if (is("TooltipAlpha")) { const int values[4]={a,b,c,d}; for (int i=0;i<4;++i) tooltipColor[i]=values[i]*0.00390625f; }
    else if (is("TooltipFlush")) {
        guiTooltipState.hoveredElement=nullptr; guiTooltipState.hoveredRow=-1;
        guiTooltipState.fadingOut=false; guiTooltipState.text=nullptr;
    } else if (prefix("LoadMPX:",8)) {
        if (a<0 || a>14) std::abort();
        auto& registry=guiRuntime->renderer->atlases; destroyGuiTextures(registry.atlases[a]);
        char path[128]; std::sprintf(path,"data/textures/gui/%s",command+8);
        registry.atlases[a]=pix::loadMultiTexture(registry.resources,path,true,0,GL_CLAMP,GL_NEAREST,GL_NEAREST);
        if (!registry.atlases[a] || (b>0 && registry.atlases[a]->textureCount!=static_cast<unsigned>(b))) std::abort();
    } else if (prefix("LoadFNT:",8)) {
        if (a<0 || a>9) std::abort();
        delete guiFonts.slots[a]; guiFonts.slots[a]=new fntTexFont;
        char path[128]; std::sprintf(path,"data/fonts/%s",command+8);
        if (!guiFonts.slots[a]->load(path,b?GL_LINEAR:GL_NEAREST,b?GL_LINEAR:GL_LINEAR_MIPMAP_LINEAR)) std::abort();
    } else if (prefix("PlayIntro:",10)) engine_movie::playMovie(command+10,0,0,0,0,0);
    else if (prefix("PlayMovie:",10)) {
        char path[128]; std::sprintf(path,"movies/%s",command+10);
        engine_movie::playMovie(path,static_cast<unsigned>(a),b,c,d,e);
    } else if (is("Credits") || prefix("MountRBX:",9)) { /* These two original branches have no operation. */ }
    else if (is("FinishDlg")) {
        for (int i=0;i<elementCount;++i) if (elements[i]->type==0) {
            auto* group=static_cast<puGroup*>(elements[i]->widget); group->close(); group->reveal(); return 1;
        }
    } else if (is("AutoPlay"))
        guiRuntime->scriptServices->autorun.configure(a,b,c!=0,guiRuntime->scriptServices->autorunServices);
    else if (is("MinigolfConfig.exe")) {
        const char* arguments[3]={nullptr,"-computedefaults","-passthru"};
        launchConfigurationTool(arguments[a]);
    } else if (prefix("http:",5)) {
        const char* arguments[2]={command,nullptr}; engine_runtime::engineRuntime_openUrlOrExec_impl(command,arguments);
    } else if (is("setViewport")) driverViewport.set(a,b,c!=0);
    else if (is("crash")) engine_runtime::engineRuntime_fatalExit_impl();
    else ulSetError(UL_WARNING,"shellCmd(%s,%d,%d,%d,%d): Unknown command",command,a,b,c,d);
    return 0;
}
}
