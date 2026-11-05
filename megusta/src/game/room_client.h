#pragma once
#include "game_config.h"
#include "netRoom.h"
#include <cstdint>
#include <ctime>
class netMessage;
namespace megusta {
class Ball;
class BallsManager;
class Putter;
class Effects;
struct BallGameplayServices;
struct SceneObjectCollection;
struct SceneObjectAnimationServices;
class Random;
struct Point3F;
struct PlayerServices;
struct DriverScriptServices;
class iRoomClient;
class iPlayer;
class iGame;
// The game extensions use the actual vendored PLIB record storage and vtables.
using NetNamedRecord=netThing;
using iRoom=netRoom;
class iPlayer : public netRoomPlayer {
public:
    iGame* getGame();
    bool isMaster();
};
class iGame : public netRoomGame {
public:
    iPlayer* getMaster();
    int getNumPlayers();
};
class GameNetPlayerState {
public:
    GameNetPlayerState();
    ~GameNetPlayerState();
    iPlayer* networkPlayer;
    int gamePlayerIndex,state;
    GameConfig config;
    Ball* getBall();
    std::uint32_t getPowerUpSlot(int);
    const char* getName() const;
};
class GameNetPlayerStates {
public:
    GameNetPlayerState slots[4];
    GameNetPlayerState* getSlot(int);
    int count() const;
    GameNetPlayerState* findPlayer(iPlayer*);
    void removePlayer(iPlayer*);
    void clear();
    GameNetPlayerState* addPlayer(iPlayer*);
};
struct ChatHistoryStorage {
    char textStorage[10][256];
    char* orderedText[10];
    std::uint32_t timestamps[10];
    int count;
};
class iRoomClient : public netRoomClient {
public:
    iRoomClient();
    ~iRoomClient() override;
    std::uint8_t nicknameValid,isHost;
    char nickname[32];
    int chatUp,nextHoleGo;
    netRoomList rooms;
    GameNetPlayerStates playerStates;
    GameConfig gameConfig{nullptr};
    bool serverListLoaded,hasGameConfig,pickupBroadcastEnabled;
    char gameConfigText[256];
    ChatHistoryStorage chatHistory;
    char latestChat[256];
    char* chatCallback;
    std::uint8_t ignoredPlayers[257];
    char whisperReplyTarget[31];
    std::uint32_t inviteFlags;
    iRoom* getRoomBySlot(int);
    iRoom* getFirstRoom();
    iRoom* getPrevRoom();
    iRoom* getNextRoom();
    iRoom* findRoom(const char*);
    iPlayer* getPlayerBySlot(int);
    iPlayer* getFirstPlayer();
    iPlayer* getPrevPlayer();
    iPlayer* getNextPlayer();
    iPlayer* findPlayer(const char*);
    iPlayer* getLocalPlayer();
    iPlayer* findCurrentGameStatePlayer();
    iGame* getGameBySlot(int);
    iGame* getFirstGame();
    iGame* getPrevGame();
    iGame* getNextGame();
    iGame* findGame(const char*);
    const char* getError();
    std::uint8_t getIgnore(const char*);
    void setIgnore(const char*,int);
    void whenSaid(const char*);
    char* lastSaid();
    int getNumSaid();
    const char* getSaid(int);
    std::uint32_t getSaidMetadata(int);
    void clearSaid();
    bool allPlayersInState(int);
    void say(char*);
    void sayTo(const char* text,const char* recipient);
    void loadServerList();
    void saveServerList();
    iRoom* recountRoomState();
    void setPlayerGameState(std::uint32_t,iPlayer* player=nullptr);
    void listGameStates();
    void setNick(const char*);
    std::uint8_t isNickValid() const;
    void clearNickname();
    void clearNextHoleGo();
    bool isNextHoleGo() const;
    bool hasPendingStartState() const;
    void clearUiCallbacks();
    std::uint8_t getIsHost() const;
    bool isVolatileSafe();
    bool canProgressActivePutt();
    bool okToPickupBall();
    bool isTurnStateReady();
    bool canAdvanceSimulation();
    bool shouldWaitForRemoteTurn();
    void receiveChat(const netMessage&);
    void handleSay(const char*) override;
    void receivePlayerState(const netMessage&);
    void receiveInvite(const netMessage&);
    void sendInvite(const char* player,const char* password);
    iRoom* handleNewRoom() override;
    iPlayer* handleNewPlayer() override;
    iGame* handleNewGame() override;
    void initializeNetwork();
    void browseRooms();
    void enterRoom(const char* player,const char* room,const char* password);
    void leaveRoom();
    void createGame(const char* name,const char* password,int maxPlayers);
    void sendNextHoleGo();
    void exitGame();
    bool isDead(int playerIndex);
    void forceDeadPlayerPickup();
    void receiveTeamSeed(const netMessage&);
    void receivePuttOver(const netMessage&);
    void receiveFrameGate(const netMessage&);
    void receiveResetTurn();
    void handleMessage(const netMessage&) override;
    void handleJoinGame(int) override;
    void handleLeaveGame(int) override;
    void initSettings();
    void requestGameConfig(const char*);
    void broadcastGameConfig();
    void sendActivatePowerUp();
    void sendNextPowerUp();
    void sendPreviousPowerUp();
    bool sendPickupPowerUp(std::uint32_t);
    void pickUpBall();
    bool acknowledgeFrameGate();
    void finishPutt();
    void sendTurnOver();
    void sendPeriodicSyncSnapshot();
    void sendAimVector(const Point3F&);
    void sendPutt(float,const Point3F&);
    void startGame();
    void toggleInvites();
    void pollDisconnectAndTurnState();
    void processFrameSync();
    void processTurnSync();
    bool coordinateTurnReadiness();
    void handleEnterRoom() override;
    // Adapt the vendored PLIB callback to the original slot-13 dispatcher.
    void handleRoomInfo(const netMessage&,const netAddress&) override;
    void receivePlayerConfig(const netMessage&);
    void receiveWelcome(const netMessage&);
    void receiveAimVector(const netMessage&);
    void receivePutt(const netMessage&);
    void receiveTurnOver(const netMessage&);
    void receiveSnapshot(const netMessage&);
    void receivePowerUp(const netMessage&);
    void receivePickupBall(const netMessage&);
    void receiveServerAddresses(const netMessage&);
    void receiveRoomInfo(const netMessage&);
    void receiveGameConfigText(const netMessage&);
};
struct RoomClientRuntime {
    iRoomClient& client;
    BallsManager& balls;
    Putter& putter;
    Effects& effects;
    const BallGameplayServices& ballGameplay;
    const PlayerServices& playerServices;
    DriverScriptServices& scripts;
    GameConfig& config;
    SceneObjectCollection& objects;
    const SceneObjectAnimationServices& animation;
    Random& random;
    std::uint32_t& puttOverFrame;
    std::uint32_t& frameGate;
    std::uint32_t& lastAimFrame;
    std::uint32_t& aimSendInterval;
    int roomCursor,gameCursor,playerCursor;
    char dateText[16],inviteGameName[32],inviteGamePass[32];
    char* invitedCallback;
    void (*executePython)(const char*);
    std::uint32_t (*timeMilliseconds)();
    void (*playSound)(const char*,int);
    void (*fatal)();
    const char *puttOverFormat;
    const char *sentWelcomeFormat,*gotWelcomeFormat,*gotPuttFormat,*unexpectedPuttFormat;
    const char *receivedTurnFormat,*roomNameSuffixFormat,*receivedConfigTextFormat;
    const char* powerUpMessages[4];
    const char* sentPowerUpMessages[4];
    const char *sentPickupText,*pickupTauntText,*gameStateFormat,*sentPuttOverFormat;
    const char *sentTurnPlayerFormat,*sentTurnFrameFormat,*sentPuttFormat;
    void* cameraContext;
    void (*enterCameraFollow)(void*);
    const char *unhandledSyncFormat,*selectionMismatchFormat,*activateFrameFormat;
    const char *nextPowerFrameFormat,*previousPowerFrameFormat,*receivedPickupText;
    const char *randomMismatchFormat,*tickMismatchFormat,*processingTurnFormat,*makingMasterFormat;
};
void bindRoomClientRuntime(RoomClientRuntime&);
// Bind retained original 00529e80 GUID data before constructing a client.
char* roomClientDate();
char* inviteGameName();
char* inviteGamePass();
void whenInvited(const char*);
char* formatChatLocalTime(const std::time_t*);
const char* networkStateName(int);
int localNetworkState();
// These were already no-op service functions in the executable.
int networkServiceStart();
void networkServiceEnd();
}
