#include "global_lifetime.h"
#include "task.h"
#include "gui.h"
#include "hole.h"
#include "player.h"
#include "putter.h"
#include "putt_map.h"
#include "game_config_runtime.h"
namespace megusta {
GameProcessLifetime::GameProcessLifetime(GameGlobalState& state,AudioGainServices& audio):state_(state) {
    state_.tasks.scheduledTasks.initializeHead(); state_.gui.initialize();
    config_=std::make_unique<GameConfigRuntime>("/mnt/memcard/minigolf.cfg",audio);
    state_.putter.initialize(state_.fixedTick);
    // Player, PowerUps, Hole and PuttMap native construction already supplies
    // their recovered zero-state initializers. Original GUI/Putter exit bridges
    // call the verified-empty netc service routine, so no fake cleanup is added.
}
GameProcessLifetime::~GameProcessLifetime() {
    state_.puttMap.releaseData(); config_.reset(); state_.players.reset();
    state_.hole.releaseResources(state_.holeServices); state_.tasks.scheduledTasks.clear(state_.tasks);
}
}
