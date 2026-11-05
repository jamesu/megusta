#pragma once
#include <cstdint>
class fntFont;
class fntRenderer;
class netMonitorServer;
namespace megusta {
class GameConfigRuntime;
class iRoomClient;
class OpenGLAdapter;
using ConsoleCommandHandler=void (*)(const char*);
enum class ConsoleSpecialKey : int { HistoryOlder=101,HistoryNewer=103,ScrollOlder=104,ScrollNewer=105 };
enum ConsoleButton : std::uint32_t {
    ConsoleScrollNewer=1,ConsoleScrollOlder=4,ConsoleClose=0x200,ConsoleOpen=0x400,
    ConsoleHistoryOlder=0x1000,ConsoleHistoryNewer=0x4000
};
class Console {
public:
    static constexpr int OutputLineCount=60,TextCapacity=80,HistoryLineCount=30,PageStep=8;
    const char *title,*prompt;
    char outputLines[OutputLineCount][TextCapacity];
    float outputColors[OutputLineCount][4];
    char historyLines[HistoryLineCount][TextCapacity];
    int historyWriteIndex,historyCount,historySelection;
    ConsoleCommandHandler commandHandler;
    int outputWriteIndex,outputViewIndex;
    fntFont* font;
    fntRenderer* renderer;
    int visible,opening;
    std::uint32_t toggleTime;
    bool forwardOutput;
    netMonitorServer *outputMonitor,*chatMonitor;
    void clearOutput();
    void clearHistory();
    void selectHistory(int);
    void echo(const char*);
    void forwardNetworkLine(const char*);
    void initialize(const char* title,const char* prompt,fntFont*);
    void releaseFontRenderer();
    void setCommandHandler(ConsoleCommandHandler);
    void drawTextAtCell(int row,int column,const char*);
    void advanceOutputLine();
    void handleSpecialKey(int);
    void handleCharacter(char);
    void handleButtonMask(std::uint32_t);
    void drawOverlay();
private:
    void scrollOlder(int comparisonView);
    void scrollNewer(int comparisonView);
};
struct ConsoleServices {
    GameConfigRuntime& config;
    iRoomClient*& link; // Shared client is constructed after Console initialization.
    OpenGLAdapter& gl;
    int &displayWidth,&displayHeight,&columns,&rows;
    bool& dimensionsScaled;
    std::uint32_t (*timeMilliseconds)();
    int (*queryWindowDimension)(int);
};
extern Console gameConsole;
void bindConsoleServices(ConsoleServices&);
void consoleMonitorCommand(const char*);
void consoleMonitorChat(const char*);
}
