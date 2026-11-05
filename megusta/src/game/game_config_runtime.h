#pragma once
#include "game_config.h"
namespace megusta {
// Shared with the sound subsystem; values are the recovered category gains.
struct AudioCategoryGains {
    float master;
    float music;
    float character;
    float sound;
    float announcer;
};
struct AudioGainServices {
    AudioCategoryGains& gains;
    void (*refreshPlayingGains)(void* context);
    void* context;
};
class GameConfigRuntime : public GameConfig {
public:
    GameConfigRuntime(const char* filename, AudioGainServices& audio) noexcept;
    void changed(const char* section, const char* key, const char* value) override;
    float viewGamma() const { return viewGamma_; }
    bool ghostShadows() const { return ghostShadows_; }
    bool mouseJump() const { return mouseJump_; }
    bool debugKeys() const { return debugKeys_; }
    bool nameTags() const { return nameTags_; }
private:
    AudioGainServices& audio_;
    float viewGamma_;
    bool ghostShadows_, mouseJump_, debugKeys_, nameTags_;
};
}
