#include "directory_services.h"
#include "../game/config_literals.h"
#include <cstring>
#include <new>
namespace megusta {
// 004dc110. The fixed Win32 search buffer and one trailing-separator removal
// follow the recovered native backend; no directory entries are filtered out.
EngineDirectoryIterator* EngineDirectoryIterator::open(const char* path) {
    auto* iterator = new (std::nothrow) EngineDirectoryIterator;
    if (iterator) {
        char search[260];
        std::strcpy(search, path);
        std::size_t length = std::strlen(search);
        if (length && std::strchr(config_literals::directorySeparators, search[length - 1]))
            search[length - 1] = 0;
        std::strcat(search, config_literals::directoryWildcard);
        iterator->findHandle_ = FindFirstFileA(search, &iterator->findData_);
        if (iterator->findHandle_ == INVALID_HANDLE_VALUE) { delete iterator; return nullptr; }
    }
    return iterator;
}
// 004dc200. The first call consumes FindFirstFile's result without advancing.
char* EngineDirectoryIterator::nextName() {
    if (firstResultPending_) firstResultPending_ = false;
    else {
        if (finished_) return nullptr;
        if (!FindNextFileA(findHandle_, &findData_)) finished_ = true;
    }
    if (finished_) return nullptr;
    std::strcpy(name_, findData_.cFileName);
    return name_;
}
char* EngineDirectoryIterator::nextFileName() {
    while (char* name=nextName())
        if (!(findData_.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) return name;
    return nullptr;
}
EngineDirectoryIterator::~EngineDirectoryIterator() {
    if (findHandle_ != INVALID_HANDLE_VALUE) FindClose(findHandle_);
}
// 004dc280. Null close is successful and the return ignores FindClose failure.
int EngineDirectoryIterator::close(EngineDirectoryIterator* iterator) { delete iterator; return 0; }
namespace { char* nextNative(EngineDirectoryIterator* iterator) { return iterator->nextName(); } }
DirectoryServices nativeDirectoryServices = {EngineDirectoryIterator::open, nextNative, EngineDirectoryIterator::close};
// 0043af10/20/30: retain typed service forwarding rather than cloning the backend.
EngineDirectoryIterator* resourceDirectoryOpen(const char* path) { return nativeDirectoryServices.open(path); }
char* resourceDirectoryNext(EngineDirectoryIterator* iterator) { return nativeDirectoryServices.nextName(iterator); }
int resourceDirectoryClose(EngineDirectoryIterator* iterator) { return nativeDirectoryServices.close(iterator); }
// 0043af40 is an intentional original failure callback, not missing behavior.
int unsupportedResourceOperation() noexcept { return -1; }
}
