// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstddef>
class netMessage;
namespace megusta {
class ResourceMountTable;
struct ConfigKey { ConfigKey* next; char* name; char* value; int valueCapacity; };
struct ConfigSection { ConfigSection* next; char* name; ConfigKey* keys; };
class GameConfig {
public:
    explicit GameConfig(const char* filename) noexcept;
    virtual ~GameConfig();
    GameConfig(const GameConfig&) = delete;
    GameConfig& operator=(const GameConfig&) = delete;
    virtual void changed(const char* section, const char* key, const char* value);
    ConfigSection* findSection(const char* name) const;
    ConfigKey* findKey(const char* section, const char* key) const;
    char** readSections() const;
    char** readKeys(const char* section, int* count = nullptr) const;
    char* readString(const char* section, const char* key, const char* fallback);
    void writeString(const char* section, const char* key, const char* value);
    int readInt(const char* section, const char* key, int fallback);
    void writeInt(const char* section, const char* key, int value);
    bool readBool(const char* section, const char* key, bool fallback);
    void writeBool(const char* section, const char* key, bool value);
    void removeSection(const char* name);
    void clear();
    void mergeSectionList(const ConfigSection* sections);
    void push();
    void pop();
    int measureSectionText(const char* section) const;
    void copySection(const char* section, const GameConfig& source, const char* destination = nullptr);
    bool load(ResourceMountTable& resources);
    bool save(ResourceMountTable& resources) const;
    static char* trimWhitespace(char* text);
    void writeSectionMessage(const char* section, netMessage& message);
    void readSectionMessage(const char* section, const netMessage& message);
private:
    const char* filename_; // Borrowed, as in the recovered constructor.
    ConfigSection* sectionsByLevel_[2];
    int activeLevel_;
};
}
