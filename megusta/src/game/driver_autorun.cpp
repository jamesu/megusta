#include "driver.h"
#include "room_client.h"
#include "../engine/string_services.h"
#include <cstdio>
namespace megusta {
DriverAutorun driverAutorun{};
namespace {
bool startsWith(const char* value,const char* prefix,std::size_t length) {
    return strings::compareInsensitiveBounded(value,prefix,length)==0;
}
void playerSections(int i,char* player,char* backup) {
    std::sprintf(player,"Player%d",i); std::sprintf(backup,"AutorunBk_Player%d",i);
}
std::uint32_t addSeconds(std::uint32_t time,int seconds) {
    return time+static_cast<std::uint32_t>(seconds)*1000u;
}
}
void DriverAutorun::configure(int menu,int game,bool startNow,const DriverAutorunServices& services) {
    enabled=menu>=1 && game>=1; menuSeconds=menu; gameSeconds=game;
    if (startNow) deadline=addSeconds(services.timeMilliseconds(services.context),menuSeconds);
}
bool DriverAutorun::inputGate(Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services) {
    if (!enabled || !deadline || !services.hole.transitionState) return false;
    deadline=services.timeMilliseconds(services.context); update(true,driver,scripts,services); return true;
}
void DriverAutorun::update(bool force,Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services) {
    if (!enabled || !deadline) return;
    if (!force && (updateCounter++%30u)!=0) return;
    bool menu=startsWith(driver.activeScript,"mainmenu",8);
    std::uint32_t now=services.timeMilliseconds(services.context);
    if (static_cast<std::int32_t>(deadline)<=static_cast<std::int32_t>(now)) {
        int seconds;
        if (!menu) {
            if (services.players.currentPlayer) networkServiceEnd();
            driver.changeScript("mainmenu",scripts); seconds=menuSeconds;
        } else {
            auto& config=services.config;
            config.copySection("Game",config,"AutorunBk_Game"); config.writeBool("Game","isAutorun",true);
            for (int i=0;i<4;++i) { char player[32],backup[32]; playerSections(i,player,backup); config.copySection(player,config,backup); }
            driver.changeScript("startaut",scripts); seconds=gameSeconds;
        }
        deadline=addSeconds(now,seconds);
    }
    int dx=services.input.frame.mouse.x-lastMouseX,dy=services.input.frame.mouse.y-lastMouseY;
    if (dx<0) dx=-dx; if (dy<0) dy=-dy;
    if (dx<5 && dy<5) return;
    if (menu) deadline=addSeconds(now,menuSeconds);
    lastMouseX=services.input.frame.mouse.x; lastMouseY=services.input.frame.mouse.y;
}
void DriverAutorun::restoreConfig(const char* name,Driver& driver,DriverScriptServices& scripts,DriverAutorunServices& services) {
    auto& config=services.config;
    if (startsWith(name,"mainmenu",8) && config.readBool("Game","isAutorun",false)) {
        config.copySection("AutorunBk_Game",config,"Game"); config.removeSection("AutorunBk_Game");
        config.writeBool("GAME","isAutorun",false);
        for (int i=0;i<4;++i) { char player[32],backup[32]; playerSections(i,player,backup); config.copySection(backup,config,player); config.removeSection(backup); }
    }
    if (!enabled) return;
    if (startsWith(name,"mainmenu",8)) { deadline=addSeconds(services.timeMilliseconds(services.context),menuSeconds); return; }
    if (startsWith(name,"result",6)) {
        if (deadline) driver.changeScript("mainmenu",scripts);
    } else if (!startsWith(name,"startaut",8)) {
        if (startsWith(name,"startgme",8)) { if (startsWith(driver.activeScript,"mainmenu",8)) deadline=0; }
        else if (strings::length(name)!=3) deadline=0;
    }
}
}
