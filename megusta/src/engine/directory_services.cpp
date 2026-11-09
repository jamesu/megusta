#include "directory_services.h"
#include "case_insensitive_path.h"
#include "../game/config_literals.h"
#include <cstring>
#include <cstdio>
#include <new>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif
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
#if defined(_WIN32)
        iterator->findHandle_ = FindFirstFileA(search, &iterator->findData_);
        if (iterator->findHandle_ == INVALID_HANDLE_VALUE) { delete iterator; return nullptr; }
#else
        char* wildcard = std::strrchr(search, '/');
        if (!wildcard) wildcard = std::strrchr(search, '\\');
        if (wildcard) *wildcard = 0;
        else std::strcpy(search, ".");
        std::string resolvedDirectory;
        iterator->findHandle_ = engine_filesystem::openDirectoryCaseInsensitive(
            search, &resolvedDirectory);
        if (!iterator->findHandle_) { delete iterator; return nullptr; }
        std::strncpy(iterator->directory_, resolvedDirectory.c_str(),
                     sizeof(iterator->directory_)-1);
#endif
    }
    return iterator;
}
// 004dc200. The first call consumes FindFirstFile's result without advancing.
char* EngineDirectoryIterator::nextName() {
#if defined(_WIN32)
    if (firstResultPending_) firstResultPending_ = false;
    else {
        if (finished_) return nullptr;
        if (!FindNextFileA(findHandle_, &findData_)) finished_ = true;
    }
    if (finished_) return nullptr;
    std::strcpy(name_, findData_.cFileName);
#else
    if (finished_) return nullptr;
    dirent* entry = readdir(findHandle_);
    if (!entry) { finished_ = true; return nullptr; }
    std::strncpy(name_, entry->d_name, sizeof(name_)-1);
    name_[sizeof(name_)-1] = 0;
    struct stat status{};
    currentIsDirectory_ = entry->d_type == DT_DIR;
    if (entry->d_type == DT_UNKNOWN) {
        char path[512];
        std::snprintf(path, sizeof(path), "%s/%s", directory_, entry->d_name);
        std::string resolvedPath;
        currentIsDirectory_ = engine_filesystem::resolvePathCaseInsensitive(path, resolvedPath) &&
            stat(resolvedPath.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
    }
#endif
    return name_;
}
char* EngineDirectoryIterator::nextFileName() {
    while (char* name=nextName())
#if defined(_WIN32)
        if (!(findData_.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) return name;
#else
        if (!currentIsDirectory_) return name;
#endif
    return nullptr;
}
EngineDirectoryIterator::~EngineDirectoryIterator() {
#if defined(_WIN32)
    if (findHandle_ != INVALID_HANDLE_VALUE) FindClose(findHandle_);
#else
    if (findHandle_) closedir(findHandle_);
#endif
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
