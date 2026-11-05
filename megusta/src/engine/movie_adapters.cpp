#include "movie_adapters.h"
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace megusta::engine_movie {
void engineMovie_swapBgr256Square_impl(std::uint8_t* pixels) {
    for(int pixel=0;pixel<0x10000;++pixel,pixels+=3) {
        const std::uint8_t first=pixels[0]; pixels[0]=pixels[2]; pixels[2]=first;
    }
}
bool engineMovie_sendMciCommand_impl(const char* command,void* window,char* result,unsigned capacity) {
#ifdef _WIN32
    return ::mciSendStringA(command,result,capacity,static_cast<HWND>(window))==0;
#else
    (void)command; (void)window; (void)result; (void)capacity; return false;
#endif
}
bool engineMovie_isSkipPressed_impl() {
#ifdef _WIN32
    return (::GetAsyncKeyState(1)&0x8000)!=0 || (::GetAsyncKeyState(2)&0x8000)!=0 ||
        (::GetAsyncKeyState(0x1b)&0x8000)!=0 || (::GetAsyncKeyState(0x20)&0x8000)!=0;
#else
    return false;
#endif
}
} // namespace megusta::engine_movie
