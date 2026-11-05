#include "game_config_runtime.h"
#include "../engine/string_services.h"
namespace megusta {
// 00410140: preserve the recovered initial preferences.
GameConfigRuntime::GameConfigRuntime(const char* filename, AudioGainServices& audio) noexcept
    : GameConfig(filename), audio_(audio), viewGamma_(0.5f), ghostShadows_(false),
      mouseJump_(true), debugKeys_(false), nameTags_(true) {}
// 00410170: section/key matching is insensitive; each volume write refreshes
// currently playing gains, while display/input preferences only update state.
void GameConfigRuntime::changed(const char* section, const char* key, const char*) {
    if (strings::compareInsensitive("Prefs", section)) return;
    ConfigKey* found = findKey(section, key);
    if (!found) return;
    float* gain = nullptr;
    if (!strings::compareInsensitive("MasterVolume", key)) gain = &audio_.gains.master;
    else if (!strings::compareInsensitive("MusicVolume", key)) gain = &audio_.gains.music;
    else if (!strings::compareInsensitive("SoundVolume", key)) gain = &audio_.gains.sound;
    else if (!strings::compareInsensitive("CharacterVolume", key)) gain = &audio_.gains.character;
    else if (!strings::compareInsensitive("AnnouncerVolume", key)) gain = &audio_.gains.announcer;
    if (gain) {
        int value = strings::toInt(found->value);
        *gain = static_cast<float>(static_cast<float>(value) * 0.1);
        audio_.refreshPlayingGains(audio_.context);
    } else if (!strings::compareInsensitive("Gamma", key)) viewGamma_ = static_cast<float>(static_cast<float>(strings::toInt(found->value)) * 0.1);
    else if (!strings::compareInsensitive("GhostShadows", key)) ghostShadows_ = strings::toInt(found->value) != 0;
    else if (!strings::compareInsensitive("MouseJump", key)) mouseJump_ = strings::toInt(found->value) != 0;
    else if (!strings::compareInsensitive("DebugKeys", key)) debugKeys_ = strings::toInt(found->value) != 0;
    else if (!strings::compareInsensitive("NameTags", key)) nameTags_ = strings::toInt(found->value) != 0;
}
}
