#pragma once

#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_WIN32)
#include <dirent.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include <strings.h>
#include <sys/stat.h>
#include <vector>
#endif

namespace megusta::engine_filesystem {

// Resolve path components using native spelling first, then a case-insensitive
// directory scan. Windows and macOS keep their native filesystem behavior;
// other POSIX targets (including Emscripten's virtual filesystem) get the
// case-insensitive lookup expected by the shipped game data.
inline bool resolvePathCaseInsensitive(const char* path, std::string& resolved,
                                       bool allowMissingLeaf = false) {
    if (!path || !*path) return false;

#if defined(_WIN32) || defined(__APPLE__)
    resolved = path;
    return true;
#else
    std::string normalized(path);
    for (char& c : normalized) if (c == '\\') c = '/';

    const bool absolute = normalized.front() == '/';
    std::vector<std::string> components;
    for (std::size_t begin = 0; begin < normalized.size();) {
        while (begin < normalized.size() && normalized[begin] == '/') ++begin;
        if (begin == normalized.size()) break;
        const std::size_t end = normalized.find('/', begin);
        components.emplace_back(normalized.substr(begin,
            end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    }

    std::string current = absolute ? "/" : "";
    if (components.empty()) {
        resolved = absolute ? "/" : ".";
        return true;
    }

    for (std::size_t i = 0; i < components.size(); ++i) {
        const std::string& component = components[i];
        if (component == ".") continue;

        const std::string candidate = current.empty() ? component :
            (current == "/" ? current + component : current + "/" + component);
        struct stat status{};
        if (::stat(candidate.c_str(), &status) == 0) {
            current = candidate;
            continue;
        }

        const std::string parent = current.empty() ? "." : current;
        bool found = false;
        DIR* directory = ::opendir(parent.c_str());
        if (directory) {
            const dirent* entry;
            while ((entry = ::readdir(directory)) != nullptr) {
                if (::strcasecmp(entry->d_name, component.c_str()) == 0) {
                    current = current.empty() ? entry->d_name :
                        (current == "/" ? current + entry->d_name : current + "/" + entry->d_name);
                    found = true;
                    break;
                }
            }
            ::closedir(directory);
        }
        if (found) continue;

        // A missing final filename is allowed for create modes, but only
        // when its already-resolved parent is a directory.
        if (allowMissingLeaf && i + 1 == components.size()) {
            struct stat parentStatus{};
            if (::stat(parent.c_str(), &parentStatus) == 0 && S_ISDIR(parentStatus.st_mode)) {
                current = candidate;
                continue;
            }
        }
        return false;
    }

    resolved = current.empty() ? "." : current;
    return true;
#endif
}

inline std::FILE* openFileCaseInsensitive(const char* path, const char* mode) {
    std::string resolved;
    const bool createsFile = mode && (std::strchr(mode, 'w') || std::strchr(mode, 'a'));
    if (!resolvePathCaseInsensitive(path, resolved, createsFile)) return nullptr;
    return std::fopen(resolved.c_str(), mode);
}

#if !defined(_WIN32)
inline DIR* openDirectoryCaseInsensitive(const char* path, std::string* resolvedPath = nullptr) {
    std::string resolved;
    if (!resolvePathCaseInsensitive(path, resolved)) return nullptr;
    DIR* directory = ::opendir(resolved.c_str());
    if (directory && resolvedPath) *resolvedPath = resolved;
    return directory;
}
#endif

} // namespace megusta::engine_filesystem
