#include "startup.h"
#include "engine/case_insensitive_path.h"
#include "engine/runtime_adapters.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
namespace megusta {
namespace {
GameEntryServices* entryServices=nullptr;
bool startupDirectorySelected=false;
bool redirectStandardStream(const char* path,FILE* stream,int descriptor) {
    FILE* probe=std::fopen(path,"w"); if (!probe) return false; std::fclose(probe);
    if (!std::freopen(path,"w",stream)) {
        FILE* replacement=std::fopen(path,"w");
        if (replacement) {
            // Compatibility: use the compiler's descriptor API instead of
            // copying the VC6 FILE structure into a different CRT's FILE.
#ifdef _WIN32
            _dup2(_fileno(replacement),descriptor);
#else
            dup2(fileno(replacement),descriptor);
#endif
            std::fclose(replacement);
        }
    }
    return true;
}
void selectStartupDirectory() {
    if (startupDirectorySelected) return;
    startupDirectorySelected=true;
    std::vector<std::filesystem::path> candidates;
    if (const char* executable=engine_runtime::tryCurrentExecutablePath()) {
        const std::filesystem::path executableDirectory=std::filesystem::path(executable).parent_path();
        if (!executableDirectory.empty()) candidates.push_back(executableDirectory);
    }
    // Platform hooks can contribute ordered locations such as a macOS app
    // bundle's Resources directory without changing the generic selection.
    for (std::size_t index=0;;++index) {
        const char* directory=engine_runtime::platformStartupDirectory(index);
        if (!directory) break;
        candidates.emplace_back(directory);
    }
    std::error_code currentPathError;
    const auto launchDirectory=std::filesystem::current_path(currentPathError);
    if (!currentPathError) candidates.push_back(launchDirectory);

    for (const auto& directory:candidates) {
        const auto configCandidate=directory/"minigolf.cfg";
        std::string resolved;
        std::error_code error;
        bool found=engine_filesystem::resolvePathCaseInsensitive(configCandidate.string().c_str(),resolved) &&
            std::filesystem::is_regular_file(resolved,error) && !error;
        if (!found) {
            // Also cover case-sensitive macOS volumes, where the native path
            // resolver intentionally leaves spelling unchanged.
            error.clear();
            for (std::filesystem::directory_iterator it(directory,error),end; !error && it!=end; it.increment(error)) {
                std::string actual=it->path().filename().string();
                std::string expected="minigolf.cfg";
                std::transform(actual.begin(),actual.end(),actual.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                if (actual==expected && it->is_regular_file(error) && !error) { found=true; break; }
            }
        }
        if (!found) continue;
        std::filesystem::current_path(directory,error);
        if (!error) return;
    }
}
}
void bindGameEntryServices(GameEntryServices& services) { entryServices=&services; }
namespace {
GameEntryServices& requireGameEntryServices() {
    if (!entryServices)
        throw std::logic_error(
            "GameEntryServices are not bound: the game application composition root is missing");
    return *entryServices;
}
}
void gameRuntimeStartupBridge(int argc,char** argv) {
    selectStartupDirectory();
    auto& s=requireGameEntryServices();
    runGameStartup(argc,argv,s.resources,s.config,s.viewport,s.platform,s.startup,s.driver,s.host);
}
void engineRuntimeWinMainBootstrap(void* instance) {
    auto& services=requireGameEntryServices();
#ifdef _WIN32
    if (HMODULE directDraw=LoadLibraryA("DDRAW.DLL")) FreeLibrary(directDraw);
#endif
    services.nativeInstance=instance;
    selectStartupDirectory();
    if (redirectStandardStream("stdout.txt",stdout,1)) std::setvbuf(stdout,nullptr,_IOLBF,512);
    if (redirectStandardStream("stderr.txt",stderr,2)) std::setbuf(stderr,nullptr);
    gameRuntimeStartupBridge(services.argc,services.argv);
}
}
