#pragma once
#include <memory>
#include <cstdint>
namespace megusta {
struct TaskRuntime;
class guiSystem;
class Hole;
struct HoleServices;
class PlayersManager;
class Putter;
class PuttMap;
struct AudioGainServices;
class GameConfigRuntime;
struct GameGlobalState {
    TaskRuntime& tasks;
    guiSystem& gui;
    Hole& hole;
    HoleServices& holeServices;
    PlayersManager& players;
    Putter& putter;
    PuttMap& puttMap;
    std::uint32_t& fixedTick;
};
// Replaces the original translation-unit initializer/atexit chain. The owner
// supplies the same objects to DriverInitialization; no second game singleton
// or copied configuration is created by a binding shim.
class GameProcessLifetime {
public:
    GameProcessLifetime(GameGlobalState&,AudioGainServices&);
    ~GameProcessLifetime();
    GameProcessLifetime(const GameProcessLifetime&)=delete;
    GameProcessLifetime& operator=(const GameProcessLifetime&)=delete;
    GameConfigRuntime& config() { return *config_; }
private:
    GameGlobalState& state_;
    std::unique_ptr<GameConfigRuntime> config_;
};
}
