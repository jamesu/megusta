#include "console.h"
#include "game_config_runtime.h"
#include "room_client.h"
#include "fnt.h"
#include "netMonitor.h"
#include <cstdio>
#include <cstring>
namespace megusta {
Console gameConsole{};
ConsoleServices* consoleServices=nullptr;
namespace {
// Exact short literals recovered from the canonical Ghidra data listing.
constexpr char MonitorLineEnding[]="\r\n"; // 004f2ad0
constexpr char InitialConsoleLineFormat[]="%s_"; // 004f2b1c
}
void bindConsoleServices(ConsoleServices& services) { consoleServices=&services; }
void Console::clearOutput() {
    for (int i=0;i<OutputLineCount;++i) {
        std::memset(outputLines[i],0,TextCapacity);
        for (auto& component:outputColors[i]) component=1;
    }
    outputWriteIndex=0; outputViewIndex=0;
}
void Console::clearHistory() {
    std::memset(historyLines,0,sizeof historyLines);
    historyCount=0; historyWriteIndex=0; historySelection=0;
}
void Console::selectHistory(int selection) {
    historySelection=selection;
    if (selection<1) historySelection=0;
    if (historyCount<historySelection) historySelection=historyCount;
    int index=historyWriteIndex-historySelection;
    if (index<0) index+=HistoryLineCount;
    std::sprintf(outputLines[outputWriteIndex],"%s%s_",prompt,historyLines[index]);
}
void Console::advanceOutputLine() {
    auto* previous=outputColors[outputWriteIndex];
    previous[0]=previous[1]=previous[2]=0.7f; previous[3]=1;
    --outputWriteIndex;
    if (outputWriteIndex<0) outputWriteIndex+=OutputLineCount;
    std::memset(outputLines[outputWriteIndex],0,TextCapacity);
    for (auto& component:outputColors[outputWriteIndex]) component=1;
    outputViewIndex=outputWriteIndex;
}
void Console::echo(const char* text) {
    if (!text) return;
    auto& services=*consoleServices;
    if (outputMonitor && forwardOutput) {
        char line[80]; std::strncpy(line,text,79); line[77]=0;
        std::strcat(line,MonitorLineEnding); outputMonitor->push(line);
    }
    char current[TextCapacity]; std::strcpy(current,outputLines[outputWriteIndex]);
    while (*text) {
        char* line=outputLines[outputWriteIndex]; std::memset(line,0,TextCapacity);
        int used=0;
        while (*text && used<TextCapacity-1) {
            // Equivalent to the recovered stack puFont at size16/slant0,
            // using its actual fntFont virtual interface without narrowing it.
            int width=0;
            if (font) {
                float left,right; font->getBBox(line,16,0,&left,&right,nullptr,nullptr);
                width=static_cast<int>(static_cast<long double>(right)-left);
            }
            if (width>=services.displayWidth-40) break;
            line[used++]=*text++;
        }
        advanceOutputLine();
    }
    std::strcpy(outputLines[outputWriteIndex],current);
}
void Console::forwardNetworkLine(const char* text) {
    if (chatMonitor) {
        char line[80]; std::strncpy(line,text,79); line[77]=0;
        std::strcat(line,MonitorLineEnding); chatMonitor->push(line);
    }
}
void Console::initialize(const char* name,const char* commandPrompt,fntFont* consoleFont) {
    auto& services=*consoleServices;
    title=name; prompt=commandPrompt; font=consoleFont; opening=1;
    toggleTime=services.timeMilliseconds();
    if (!services.dimensionsScaled) {
        services.dimensionsScaled=true;
        services.columns=(services.displayWidth*services.columns)/640;
        services.rows=(services.displayHeight*services.rows)/480;
    }
    visible=0; renderer=new fntRenderer; renderer->setFont(font); renderer->setPointSize(16);
    clearOutput(); clearHistory(); advanceOutputLine();
    // Inferred short format 004f2b1c: input handling explicitly maintains '_'.
    std::sprintf(outputLines[outputWriteIndex],InitialConsoleLineFormat,prompt); commandHandler=nullptr;
    int port=services.config.readInt("Console","port",-1);
    outputMonitor=port==-1 ? nullptr : new netMonitorServer(title,port);
    if (outputMonitor) { outputMonitor->setPrompt(prompt); outputMonitor->setCommandFunc(consoleMonitorCommand); }
    forwardOutput=false;
    int chatPort=services.config.readInt("Console","chatport",-1);
    chatMonitor=chatPort==-1 ? nullptr : new netMonitorServer(title,chatPort);
    if (chatMonitor) { chatMonitor->setPrompt(""); chatMonitor->setCommandFunc(consoleMonitorChat); }
}
void Console::releaseFontRenderer() { delete renderer; font=nullptr; }
void Console::setCommandHandler(ConsoleCommandHandler handler) { commandHandler=handler; }
void consoleMonitorCommand(const char* command) {
    if (consoleServices->config.debugKeys() && (!consoleServices->link || !consoleServices->link->inGame())) {
        gameConsole.forwardOutput=true;
        if (gameConsole.commandHandler) gameConsole.commandHandler(command);
        gameConsole.forwardOutput=false;
    }
}
void consoleMonitorChat(const char* text) { if(consoleServices->link)consoleServices->link->say(const_cast<char*>(text)); }
}
