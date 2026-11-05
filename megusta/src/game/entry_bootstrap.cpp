#include "startup.h"
#include "engine/runtime_adapters.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
namespace megusta {
namespace {
GameEntryServices* entryServices=nullptr;
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
    auto& s=requireGameEntryServices();
    runGameStartup(argc,argv,s.resources,s.config,s.viewport,s.platform,s.startup,s.driver,s.host);
}
void engineRuntimeWinMainBootstrap(void* instance) {
    auto& services=requireGameEntryServices();
#ifdef _WIN32
    if (HMODULE directDraw=LoadLibraryA("DDRAW.DLL")) FreeLibrary(directDraw);
#endif
    services.nativeInstance=instance;
    char path[260]; std::strcpy(path,engine_runtime::currentExecutablePath());
    char* separator=std::strrchr(path,'\\'); if (!separator) separator=std::strrchr(path,'/');
    if (separator) { *separator='\0'; std::error_code error; std::filesystem::current_path(path,error); }
    if (redirectStandardStream("stdout.txt",stdout,1)) std::setvbuf(stdout,nullptr,_IOLBF,512);
    if (redirectStandardStream("stderr.txt",stderr,2)) std::setbuf(stderr,nullptr);
    gameRuntimeStartupBridge(services.argc,services.argv);
}
}
