#include "driver.h"
#include "gui.h"
namespace megusta {
void Driver::joystickCallback(std::uint32_t buttons,int count,const float* axes,DriverInputServices& s) {
    if (buttons && s.scripts.autorun.inputGate(*this,s.scripts,s.scripts.autorunServices)) return;
    Player* player=s.players.currentPlayer;
    if (s.hole.transitionState==1) {
        auto id=static_cast<std::uint32_t>(s.joystickDeviceId(s.context));
        if (player->inputDeviceId==id || s.joystickDeviceId(s.context)==0) {
            s.consoleButtons(s.context,buttons);
            if (!s.consoleVisible(s.context) || s.consoleOpening(s.context)) s.input.setJoystick(buttons,count,axes,s.inputServices);
        }
        return;
    }
    int id=s.joystickDeviceId(s.context);
    bool first=id==1 || s.joystickDeviceId(s.context)==0;
    if (first) joystickAggregateButtons=0;
    if (first && count) for (int i=0;i<count;++i) joystickAggregateAxes[i]=0;
    joystickAggregateButtons |= buttons;
    for (int i=0;i<count;++i) if (axes[i]>0.4 || axes[i]<-0.4) joystickAggregateAxes[i]+=axes[i];
    s.consoleButtons(s.context,joystickAggregateButtons);
    if (!s.consoleVisible(s.context) || s.consoleOpening(s.context))
        s.input.setJoystick(joystickAggregateButtons,count,joystickAggregateAxes,s.inputServices);
}
void Driver::mouseButtonCallback(int button,int state,int x,int y,DriverInputServices& s) {
    if (state!=0) {
        if (!s.config.mouseJump()) s.disableWarpCursorOnTab(s.context);
        if (s.scripts.autorun.inputGate(*this,s.scripts,s.scripts.autorunServices)) return;
    }
    s.viewport.mapMouse(x,y);
    const int canvasY=y-s.viewport.displayHeight+s.viewport.viewportHeight;
    if(s.viewport.surface.enabled() && (!s.viewport.worldExpanded || s.scripts.gui.hasTabElements) && state==0 &&
       (x<0 || x>=s.viewport.viewportWidth || canvasY<0 || canvasY>=s.viewport.viewportHeight))return;
    s.puiMouseButton(s.context,button,state,x,y);
    s.input.setMouseButton(button,state,x,y,s.inputServices);
}
void Driver::mouseMotionCallback(int x,int y,DriverInputServices& s) {
    s.viewport.mapMouse(x,y); s.puiMouseMotion(s.context,x,y); s.input.setMousePosition(x,y);
}
void Driver::specialKeyDownCallback(int key,DriverInputServices& s) {
    if (key && s.scripts.autorun.inputGate(*this,s.scripts,s.scripts.autorunServices)) return;
    s.consoleSpecialKey(s.context,key); auto code=static_cast<std::uint32_t>(key)+0x100u;
    s.puiKeyboard(s.context,code,0); s.input.pressKey(code,s.inputServices);
}
void Driver::specialKeyUpCallback(int key,DriverInputServices& s) {
    s.input.clearKey(static_cast<std::uint32_t>(key)+0x100u);
}
void Driver::keyDownCallback(std::uint8_t key,DriverInputServices& s) {
    if (key && s.scripts.autorun.inputGate(*this,s.scripts,s.scripts.autorunServices)) return;
    s.consoleCharacter(s.context,key); s.puiKeyboard(s.context,key,0);
    if (!s.consoleVisible(s.context) || s.consoleOpening(s.context)) s.input.pressKey(key,s.inputServices);
}
}
