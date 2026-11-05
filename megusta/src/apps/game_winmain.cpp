#include "game/startup.h"
#include "win32_game_application.h"
#include <windows.h>
#include <exception>
#include <cstdio>

// 004dd3c0 is the recovered game-side WinMain bootstrap. The CRT owns
// WinMainCRTStartup and calls this platform entrypoint with the native instance.
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    try {
        megusta::runWin32GameApplication(instance,__argc,__argv);
        return 0;
    } catch (const std::exception& error) {
        // Keep startup failures observable in headless Wine/test runs as well
        // as in the native error dialog. The bootstrap may fail before its
        // stdout/stderr redirection has been installed.
        std::fprintf(stderr,"Minigolf Maniacs startup failure: %s\n",error.what());
        std::fflush(stderr);
        // Automated Wine runs need a process exit rather than a modal dialog
        // which holds the test runner open. Interactive launches keep the
        // native error dialog unless the test runner opts out explicitly.
        char noDialog[2]{};
        const DWORD length=GetEnvironmentVariableA("MEGUSTA_NO_ERROR_DIALOG",noDialog,sizeof(noDialog));
        if (length==0 || noDialog[0]!='1')
            MessageBoxA(nullptr, error.what(), "Minigolf Maniacs startup failure", MB_OK | MB_ICONERROR);
        return 1;
    }
}
