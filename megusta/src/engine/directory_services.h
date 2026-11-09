#pragma once
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif
namespace megusta {
class EngineDirectoryIterator {
public:
    static EngineDirectoryIterator* open(const char* path);
    char* nextName();
    char* nextFileName();
    static int close(EngineDirectoryIterator* iterator);
    ~EngineDirectoryIterator();
    EngineDirectoryIterator(const EngineDirectoryIterator&) = delete;
    EngineDirectoryIterator& operator=(const EngineDirectoryIterator&) = delete;
private:
    EngineDirectoryIterator() = default;
#if defined(_WIN32)
    WIN32_FIND_DATAA findData_;
    HANDLE findHandle_ = INVALID_HANDLE_VALUE;
#else
    DIR* findHandle_ = nullptr;
    bool currentIsDirectory_ = false;
    char directory_[512]{};
#endif
    bool firstResultPending_ = true;
    bool finished_ = false;
    char name_[256];
};
struct DirectoryServices {
    EngineDirectoryIterator* (*open)(const char*);
    char* (*nextName)(EngineDirectoryIterator*);
    int (*close)(EngineDirectoryIterator*);
};
// Saved native services, corresponding to the original forwarding callback table.
extern DirectoryServices nativeDirectoryServices;
EngineDirectoryIterator* resourceDirectoryOpen(const char* path);
char* resourceDirectoryNext(EngineDirectoryIterator* iterator);
int resourceDirectoryClose(EngineDirectoryIterator* iterator);
int unsupportedResourceOperation() noexcept;
}
