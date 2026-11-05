#include "netMessage.h"
#include "game_config.h"
namespace megusta {
// 0042d4c0: use the actual PLIB message serializers and their capacity semantics.
void GameConfig::writeSectionMessage(const char* section, netMessage& message) {
    int count;
    char** keys = readKeys(section, &count);
    message.puti(count);
    for (int i = 0; i < count; ++i) {
        message.puts(keys[i]);
        message.puts(readString(section, keys[i], nullptr));
    }
}
// 0042d620: consume the full strings even when their retained text is truncated.
void GameConfig::readSectionMessage(const char* section, const netMessage& message) {
    removeSection(section);
    int count = message.geti();
    char key[128];
    char value[1024];
    while (count > 0) {
        message.gets(key, sizeof(key));
        message.gets(value, sizeof(value));
        writeString(section, key, value);
        --count;
    }
}
}
