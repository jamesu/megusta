#include "console.h"
#include "game_config_runtime.h"
#include "room_client.h"
#include <cstdio>
#include <cstring>
namespace megusta {
extern ConsoleServices* consoleServices;
void Console::scrollOlder(int comparisonView) {
    int previous=outputViewIndex,next=previous+PageStep;
    outputViewIndex=next;
    if (comparisonView<outputWriteIndex && outputWriteIndex<next) { outputViewIndex=outputWriteIndex; return; }
    if (next>OutputLineCount-1) outputViewIndex=previous-(OutputLineCount-PageStep);
    if (outputViewIndex==outputWriteIndex) --outputViewIndex;
}
void Console::scrollNewer(int comparisonView) {
    int previous=outputViewIndex,next=previous-PageStep;
    outputViewIndex=next;
    if (comparisonView<=outputWriteIndex || outputWriteIndex<=next) {
        if (next<0) outputViewIndex=previous+(OutputLineCount-PageStep);
        if (outputViewIndex!=outputWriteIndex-1) return;
    }
    outputViewIndex=outputWriteIndex;
}
void Console::handleSpecialKey(int value) {
    if (!visible) return;
    int previous=outputViewIndex;
    switch (static_cast<ConsoleSpecialKey>(value)) {
    case ConsoleSpecialKey::HistoryOlder: selectHistory(historySelection+1); break;
    case ConsoleSpecialKey::HistoryNewer: selectHistory(historySelection-1); break;
    case ConsoleSpecialKey::ScrollOlder: scrollOlder(previous); break;
    case ConsoleSpecialKey::ScrollNewer: scrollNewer(previous); break;
    }
}
void Console::handleCharacter(char value) {
    if (value=='`' || value=='~') {
        if (!visible) { visible=1; opening=1; toggleTime=consoleServices->timeMilliseconds(); return; }
        if (!opening) visible=0;
    } else if (visible) {
        char* line=outputLines[outputWriteIndex];
        int length=static_cast<int>(std::strlen(line)),prefix=static_cast<int>(std::strlen(prompt));
        if (value=='\b') {
            if (prefix<length-1) { line[length-1]=0; line[length-2]='_'; outputViewIndex=outputWriteIndex; }
        } else if (value=='\n' || value=='\r') {
            line[length-1]=0; const char* command=line+prefix;
            std::strcpy(historyLines[historyWriteIndex],command); historySelection=0;
            ++historyCount; historyWriteIndex=(historyWriteIndex+1)%HistoryLineCount;
            if (historyCount>HistoryLineCount) historyCount=HistoryLineCount;
            advanceOutputLine(); std::sprintf(outputLines[outputWriteIndex],"%s_",prompt);
            if (consoleServices->config.debugKeys() && (!consoleServices->link || !consoleServices->link->inGame()) && commandHandler) commandHandler(command);
        } else if (length-1<TextCapacity-2) {
            line[length-1]=value; line[length]='_'; line[length+1]=0; outputViewIndex=outputWriteIndex;
        }
    }
}
void Console::handleButtonMask(std::uint32_t buttons) {
    if (buttons&ConsoleOpen) {
        if (!visible) { visible=1; opening=1; toggleTime=consoleServices->timeMilliseconds(); }
        return;
    }
    if (buttons&ConsoleClose) { if (!opening) visible=0; return; }
    if (!visible) return;
    int previous=outputViewIndex;
    if (buttons&ConsoleHistoryOlder) selectHistory(historySelection+1);
    if (buttons&ConsoleHistoryNewer) selectHistory(historySelection-1);
    if (buttons&ConsoleScrollOlder) scrollOlder(previous);
    if (buttons&ConsoleScrollNewer) scrollNewer(previous);
}
}
