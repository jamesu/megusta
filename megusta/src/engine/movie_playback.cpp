#include "movie_playback.h"
#include "movie_adapters.h"
#include "resource_runtime.h"
#include "string_services.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <vfw.h>
#endif
namespace megusta::engine_movie {
namespace {
MoviePlayback playback;
const MovieHost* movieHost=nullptr;
#ifdef _WIN32
struct AviState {
    HDC dc=nullptr;
    HDRAWDIB draw=nullptr;
    PAVISTREAM stream=nullptr;
    PGETFRAME decoder=nullptr;
    HBITMAP bitmap=nullptr;
    HGDIOBJ previousBitmap=nullptr;
    std::uint8_t* pixels=nullptr;
};
#endif
void originalMovieCleanup() { /* Original netc_serviceEnd has an empty body. */ }
}
void bindMovieHost(const MovieHost& host) { movieHost=&host; }
int playMovie(const char* name,unsigned flags,int x,int y,int width,int height) {
    if (!movieHost) throw std::logic_error("Movie host/window not bound");
    return playback.playBlocking(*movieHost,name,flags,x,y,width,height);
}
bool registerMovieCleanup() { std::atexit(originalMovieCleanup); return true; }
void MoviePlayback::openAvi(const char* filename) {
#ifdef _WIN32
    if (!native_) native_=new AviState;
    auto& state=*static_cast<AviState*>(native_); AVIFileInit();
    if (AVIStreamOpenFromFileA(&state.stream,filename,streamtypeVIDEO,0,OF_READ,nullptr)) MessageBoxA(nullptr,"Failed To Open The AVI Stream","Error",MB_ICONEXCLAMATION);
    AVISTREAMINFOA info{}; AVIStreamInfoA(state.stream,&info,sizeof(info));
    width=info.rcFrame.right-info.rcFrame.left; height=info.rcFrame.bottom-info.rcFrame.top;
    frames=AVIStreamLength(state.stream); millisecondsPerFrame=AVIStreamSampleToTime(state.stream,frames)/frames;
    BITMAPINFO bitmap{}; bitmap.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); bitmap.bmiHeader.biWidth=256; bitmap.bmiHeader.biHeight=256;
    bitmap.bmiHeader.biPlanes=1; bitmap.bmiHeader.biBitCount=24; bitmap.bmiHeader.biCompression=BI_RGB;
    state.bitmap=CreateDIBSection(state.dc,&bitmap,DIB_RGB_COLORS,reinterpret_cast<void**>(&state.pixels),nullptr,0);
    state.previousBitmap=SelectObject(state.dc,state.bitmap); state.decoder=AVIStreamGetFrameOpen(state.stream,nullptr);
    if (!state.decoder) MessageBoxA(nullptr,"Failed To Open The AVI Frame","Error",MB_ICONEXCLAMATION);
#else
    // Browser and headless builds intentionally skip movies. The saved game
    // treats a successfully completed movie as result zero, so these targets
    // return immediately without opening a decoder or touching GL state.
    (void)filename;
#endif
}
void MoviePlayback::decodeAndUploadFrame(int frame) {
#ifdef _WIN32
    auto& state=*static_cast<AviState*>(native_);
    auto* header=static_cast<BITMAPINFOHEADER*>(AVIStreamGetFrame(state.decoder,frame));
    auto* pixels=reinterpret_cast<std::uint8_t*>(header)+header->biSize+header->biClrUsed*4;
    for (int row=0;row<rows_;++row) for (int column=0;column<columns_;++column) {
        const int tile=row*columns_+column;
        if (!DrawDibDraw(state.draw,state.dc,0,0,256,256,header,pixels,tileWidths_[tile]-256+column*256,
                         height-row*256-tileHeights_[tile],256,256,0)) sharedIo->stdioPrintf("WINML: Error drawing avi frame!\n");
        engineMovie_swapBgr256Square_impl(state.pixels); glBindTexture(GL_TEXTURE_2D,textures_[tile]);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,256,256,GL_RGB,GL_UNSIGNED_BYTE,state.pixels);
    }
#else
    // No decoder exists on browser/headless targets; playBlocking returns
    // before requesting a frame there.
    (void)frame;
#endif
}
void MoviePlayback::closeAvi() {
#ifdef _WIN32
    auto& state=*static_cast<AviState*>(native_);
    SelectObject(state.dc,state.previousBitmap); DeleteObject(state.bitmap);
    AVIStreamGetFrameClose(state.decoder); AVIStreamRelease(state.stream); AVIFileExit();
#else
    // There is no native AVI state on browser/headless targets.
#endif
}
bool MoviePlayback::initializeTiled(const char* filename,void* window) {
#ifdef _WIN32
    native_=new AviState; auto& state=*static_cast<AviState*>(native_);
    state.dc=CreateCompatibleDC(nullptr); state.draw=DrawDibOpen();
    RECT client; GetClientRect(static_cast<HWND>(window),&client); glGetIntegerv(GL_VIEWPORT,previousViewport_);
    // The retained body aliases the client RECT and the orthographic stack
    // arguments. Use the real client extent at this platform boundary.
    glViewport(0,0,client.right-client.left,client.bottom-client.top);
    const int screenWidth=client.right-client.left,screenHeight=client.bottom-client.top;
    if (destinationWidth==640) { destinationX=0; destinationY=(480-destinationHeight)/2; }
    destinationY=(screenHeight-destinationHeight)-destinationY;
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,screenWidth,0,screenHeight,0,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_FOG); glDisable(GL_LIGHTING);
    glClearColor(0,0,0,1); glClearDepth(1); glDepthFunc(GL_LEQUAL); glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH); glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); glEnable(GL_BLEND); glBlendFunc(GL_ONE,GL_ZERO); glColor3f(1,1,1);
    openAvi(filename); columns_=(width+255)/256; rows_=(height+255)/256; tileCount_=rows_*columns_;
    textures_.reset(new std::uint32_t[tileCount_]); tileWidths_.reset(new int[tileCount_]); tileHeights_.reset(new int[tileCount_]);
    glGenTextures(tileCount_,textures_.get());
    for (int row=0;row<rows_;++row) for (int column=0;column<columns_;++column) {
        const int tile=row*columns_+column; tileWidths_[tile]=width-column*256; tileHeights_[tile]=height-row*256;
        if (tileWidths_[tile]>256) tileWidths_[tile]=256; if (tileHeights_[tile]>256) tileHeights_[tile]=256;
        glBindTexture(GL_TEXTURE_2D,textures_[tile]); glEnable(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        if (auto error=glGetError()) sharedIo->stdioPrintf("WINML: glBindTexture error 0x%x\n",error);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,256,256,0,GL_RGB,GL_UNSIGNED_BYTE,state.pixels);
    }
    return true;
#else
    (void)filename; (void)window;
    // Keep this helper well-defined if called directly by a platform probe.
    // Normal playback does not enter it because the non-Windows path below
    // reports the movie as completed immediately.
    finished=true; currentFrame=0; elapsed=0;
    return true;
#endif
}
void MoviePlayback::releaseTiled() {
#ifdef _WIN32
    auto& state=*static_cast<AviState*>(native_);
    glDeleteTextures(tileCount_,textures_.get()); tileHeights_.reset(); tileWidths_.reset(); textures_.reset(); closeAvi();
    DrawDibClose(state.draw); DeleteDC(state.dc); glViewport(previousViewport_[0],previousViewport_[1],previousViewport_[2],previousViewport_[3]);
    delete &state; native_=nullptr;
#else
    // The skipped movie owns no textures or platform resources here.
#endif
}
void MoviePlayback::advanceTime(int milliseconds) {
    elapsed+=milliseconds; currentFrame=elapsed/millisecondsPerFrame;
    if (currentFrame>=frames) { finished=true; currentFrame=0; elapsed=0; }
}
void MoviePlayback::drawTiledFrame() {
    decodeAndUploadFrame(currentFrame);
    for (int row=0;row<rows_;++row) for (int column=0;column<columns_;++column) {
        const int tile=row*columns_+column;
        const float left=static_cast<float>(destinationX+column*256),right=left+tileWidths_[tile];
        const float top=static_cast<float>(destinationY+height-row*256),bottom=top-tileHeights_[tile];
        const float u=static_cast<float>(256-tileWidths_[tile])*0.00390625f,v=static_cast<float>(256-tileHeights_[tile])*0.00390625f;
        glBindTexture(GL_TEXTURE_2D,textures_[tile]); glBegin(GL_QUADS);
        glTexCoord2f(1,1); glVertex3f(right,top,0); glTexCoord2f(u,1); glVertex3f(left,top,0);
        glTexCoord2f(u,v); glVertex3f(left,bottom,0); glTexCoord2f(1,v); glVertex3f(right,bottom,0); glEnd();
    }
    glFlush(); glFinish();
}
int MoviePlayback::playBlocking(const MovieHost& host,const char* name,unsigned flags,int x,int y,int w,int h) {
#ifdef _WIN32
    char filename[128],buffer[256]; std::strcpy(filename,name); std::strcat(filename,".AVI");
    auto window=static_cast<HWND>(host.window); HDC dc=GetDC(window); HCURSOR cursor=nullptr;
    destinationX=x; destinationY=y; destinationWidth=w; destinationHeight=h; int result=2;
    if (host.graphicsEnabled) cursor=SetCursor(nullptr);
    if (flags&2) {
        auto previous=SelectObject(dc,GetStockObject(BLACK_BRUSH)); RECT client; GetClientRect(window,&client);
        Rectangle(dc,client.left,client.top,client.right,client.bottom); SelectObject(dc,previous);
    }
    bool alternate=false;
    if (host.graphicsEnabled) {
        if (auto module=GetModuleHandleA("opengl32.dll")) if (auto function=GetProcAddress(module,"glGetString")) {
            using GetString=const unsigned char* (WINAPI*)(unsigned);
            std::strcpy(buffer,reinterpret_cast<const char*>(reinterpret_cast<GetString>(function)(GL_RENDERER)));
            strings::uppercase(buffer); alternate=std::strstr(buffer,"voodoo3")!=nullptr;
        }
    }
    if (alternate) {
        sharedIo->stdioPrintf("WINML: Using alternate video player\n");
        if (initializeTiled(filename,window)) {
            finished=false; currentFrame=elapsed=0; DWORD previous=GetTickCount(); if (flags&2) glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            do {
                SetCursor(nullptr); const DWORD now=GetTickCount(); advanceTime(now-previous); previous=now; drawTiledFrame(); SwapBuffers(dc);
                if (finished) { result=0; break; }
                if (engineMovie_isSkipPressed_impl()) { result=1; break; }
            } while (true);
            releaseTiled();
        }
    } else {
        sharedIo->stdioPrintf("WINML: Using primary video player\n");
        if (engineMovie_sendMciCommand_impl("open avivideo",nullptr,nullptr,0)) {
            std::sprintf(buffer,"open %s alias mov style child parent %llu",filename,
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(window)));
            if (engineMovie_sendMciCommand_impl(buffer,nullptr,nullptr,0)) {
                engineMovie_sendMciCommand_impl("status mov window handle",nullptr,buffer,sizeof(buffer));
                engineMovie_sendMciCommand_impl("put mov destination",window,nullptr,0);
                std::sprintf(buffer,"put mov window at %d %d %d %d",x,y,w,h); engineMovie_sendMciCommand_impl(buffer,nullptr,nullptr,0);
                engineMovie_sendMciCommand_impl("play mov window notify",window,nullptr,0);
                while (engineMovie_isSkipPressed_impl()) {}
                do {
                    if (host.graphicsEnabled) SetCursor(nullptr);
                    engineMovie_sendMciCommand_impl("status mov mode",nullptr,buffer,sizeof(buffer)); Sleep(50);
                    if (strings::compareInsensitive(buffer,"playing") && strings::compareInsensitive(buffer,"seeking")) { result=0; break; }
                    if (engineMovie_isSkipPressed_impl()) { result=1; break; }
                } while (true);
                engineMovie_sendMciCommand_impl("close mov",nullptr,nullptr,0);
            }
            engineMovie_sendMciCommand_impl("close avivideo",nullptr,nullptr,0);
        }
    }
    ReleaseDC(window,dc); while (engineMovie_isSkipPressed_impl()) {} if (cursor) SetCursor(cursor); return result;
#else
    // The browser/headless compatibility policy is instant successful
    // completion. It preserves startup's intro fallback rules without
    // inventing a browser decoder or blocking the event loop.
    (void)host; (void)name; (void)flags; (void)x; (void)y; (void)w; (void)h;
    return 0;
#endif
}
}
