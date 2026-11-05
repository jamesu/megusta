#include "game_config.h"
#include "config_literals.h"
#include "../engine/resource_mount.h"
#include "../engine/runtime_adapters.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
namespace megusta {
namespace {
// Recovered global scratch lists, shared by all instances; final slot is sentinel.
char* sectionNames[33];
char* keyNames[257];
char* duplicate(const char* value) {
    char* result = static_cast<char*>(std::malloc(std::strlen(value) + 1));
    std::strcpy(result, value);
    return result;
}
}
GameConfig::GameConfig(const char* filename) noexcept
    : filename_(filename), sectionsByLevel_{nullptr, nullptr}, activeLevel_(0) {}
GameConfig::~GameConfig() { while (activeLevel_ > 0) pop(); clear(); }
void GameConfig::changed(const char*, const char*, const char*) {}
ConfigSection* GameConfig::findSection(const char* name) const {
    for (auto* section = sectionsByLevel_[activeLevel_]; section; section = section->next)
        if (!_stricmp(section->name, name)) return section;
    return nullptr;
}
ConfigKey* GameConfig::findKey(const char* section, const char* name) const {
    auto* found = findSection(section);
    if (found) for (auto* key = found->keys; key; key = key->next)
        if (!_stricmp(key->name, name)) return key;
    return nullptr;
}
char** GameConfig::readSections() const {
    int count = 0;
    for (auto* section = sectionsByLevel_[activeLevel_]; section; section = section->next)
        if (count < 32) sectionNames[count++] = section->name;
    sectionNames[count] = nullptr;
    return sectionNames;
}
char** GameConfig::readKeys(const char* name, int* outCount) const {
    int count = 0;
    auto* section = findSection(name);
    if (section) for (auto* key = section->keys; key; key = key->next)
        if (count < 256) keyNames[count++] = key->name;
    if (outCount) *outCount = count;
    keyNames[count] = nullptr;
    return keyNames;
}
char* GameConfig::readString(const char* section, const char* name, const char* fallback) {
    auto* key = findKey(section, name);
    if (!key) {
        writeString(section, name, fallback);
        key = findKey(section, name);
        if (!key) engine_runtime::engineRuntime_fatalExit_impl();
    }
    return key->value;
}
void GameConfig::writeString(const char* sectionName, const char* name, const char* value) {
    auto* section = findSection(sectionName);
    if (!section) {
        section = static_cast<ConfigSection*>(std::calloc(1, sizeof(ConfigSection)));
        section->name = duplicate(sectionName);
        ConfigSection** tail = &sectionsByLevel_[activeLevel_];
        while (*tail) tail = &(*tail)->next;
        *tail = section;
    }
    auto* key = findKey(sectionName, name);
    if (!key) {
        key = static_cast<ConfigKey*>(std::calloc(1, sizeof(ConfigKey)));
        key->name = duplicate(name);
        ConfigKey** tail = &section->keys;
        while (*tail) tail = &(*tail)->next;
        *tail = key;
    }
    if (!value) value = config_literals::emptyValue;
    int length = static_cast<int>(std::strlen(value));
    if (key->value && !std::strcmp(key->value, value)) return;
    if (!key->value || length >= key->valueCapacity) {
        std::free(key->value);
        key->valueCapacity = length + 3;
        key->value = static_cast<char*>(std::malloc(key->valueCapacity));
    }
    std::strcpy(key->value, value);
    changed(section->name, key->name, key->value);
}
int GameConfig::readInt(const char* section, const char* key, int fallback) {
    char text[20]; std::sprintf(text, config_literals::integerFormat, fallback);
    char* end;
    return static_cast<int>(std::strtol(readString(section, key, text), &end, 10));
}
void GameConfig::writeInt(const char* section, const char* key, int value) {
    char text[20]; std::sprintf(text, config_literals::integerFormat, value); writeString(section, key, text);
}
bool GameConfig::readBool(const char* section, const char* key, bool fallback) {
    return readInt(section, key, fallback ? 1 : 0) != 0;
}
void GameConfig::writeBool(const char* section, const char* key, bool value) {
    writeInt(section, key, value ? 1 : 0);
}
void GameConfig::removeSection(const char* name) {
    ConfigSection** link = &sectionsByLevel_[activeLevel_];
    while (*link && _stricmp((*link)->name, name)) link = &(*link)->next;
    if (!*link) return;
    ConfigSection* section = *link;
    *link = section->next;
    while (section->keys) {
        ConfigKey* key = section->keys;
        section->keys = key->next;
        std::free(key->name); std::free(key->value); std::free(key);
    }
    std::free(section->name); std::free(section);
}
void GameConfig::clear() {
    while (sectionsByLevel_[activeLevel_]) removeSection(sectionsByLevel_[activeLevel_]->name);
}
void GameConfig::mergeSectionList(const ConfigSection* sections) {
    for (; sections; sections = sections->next)
        for (auto* key = sections->keys; key; key = key->next)
            writeString(sections->name, key->name, key->value);
}
void GameConfig::push() {
    if (activeLevel_ < 1) {
        ++activeLevel_; sectionsByLevel_[activeLevel_] = nullptr;
        mergeSectionList(sectionsByLevel_[activeLevel_ - 1]);
    }
}
void GameConfig::pop() { if (activeLevel_ > 0) { clear(); --activeLevel_; } }
int GameConfig::measureSectionText(const char* name) const {
    int size = 0;
    auto* section = findSection(name);
    if (section) for (auto* key = section->keys; key; key = key->next)
        size += static_cast<int>(std::strlen(key->name)) + 2 + static_cast<int>(std::strlen(key->value));
    return size;
}
void GameConfig::copySection(const char* name, const GameConfig& source, const char* destination) {
    if (!destination) destination = name;
    removeSection(destination);
    auto* section = source.findSection(name);
    if (section) for (auto* key = section->keys; key; key = key->next)
        writeString(destination, key->name, key->value);
}
}
namespace megusta {
// 0042d3a0: trim exactly space, tab, CR and LF; return an interior pointer.
char* GameConfig::trimWhitespace(char* text) {
    char* end = text + std::strlen(text);
    while (end != text && std::strchr(config_literals::whitespace, end[-1])) *--end = 0;
    while (*text && std::strchr(config_literals::whitespace, *text)) ++text;
    return text;
}
// 0042d1b0: save the base snapshot even when the temporary level is active.
bool GameConfig::save(ResourceMountTable& resources) const {
    if (!filename_) return false;
    auto file = resources.open(filename_, config_literals::writeMode);
    if (!file) return false;
    for (auto* section = sectionsByLevel_[0]; section; section = section->next) {
        file->printf(config_literals::sectionFormat, section->name);
        for (auto* key = section->keys; key; key = key->next)
            if (key->value) file->printf(config_literals::keyValueFormat, key->name, key->value);
        file->printf(config_literals::blankLine);
    }
    file->close();
    return true; // Original does not propagate write or close failures.
}
// 0042d240: merge into the active snapshot; only '[' and '=' have syntax roles.
bool GameConfig::load(ResourceMountTable& resources) {
    if (!filename_) return false;
    auto file = resources.open(filename_, config_literals::readMode);
    if (!file) return false;
    char section[128];
    char line[1024];
    std::strcpy(section, config_literals::initialSection);
    while (file->getLine(line, sizeof(line))) {
        char* text = trimWhitespace(line);
        if (*text == '[') {
            if (char* end = std::strchr(text, ']')) {
                *end = 0;
                std::memset(section, 0, sizeof(section));
                std::strncpy(section, trimWhitespace(text + 1), sizeof(section) - 1);
                if (std::strlen(section) > 125) engine_runtime::engineRuntime_fatalExit_impl();
            }
        } else if (char* equals = std::strchr(text, '=')) {
            *equals = 0;
            char* value = trimWhitespace(equals + 1);
            writeString(section, trimWhitespace(text), value);
        }
    }
    file->close();
    return true;
}
}
