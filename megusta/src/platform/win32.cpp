#include "win32.h"
#include "../engine/runtime_adapters.h"
#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0700
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dinput.h>
#include <GL/gl.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
namespace megusta::platform {
namespace {
// DINPUT.DLL is loaded dynamically below, so this target deliberately has no
// dinput.lib dependency. Recreate the SDK's standard DIJOYSTATE object map
// locally; this is the same axis/slider/POV/button layout as c_dfDIJoystick.
const DIDATAFORMAT* joystickDataFormat() {
    static DIOBJECTDATAFORMAT objects[44]{};
    static const DIDATAFORMAT format=[] {
        unsigned i=0;
        objects[i++]={&GUID_XAxis,offsetof(DIJOYSTATE,lX),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        objects[i++]={&GUID_YAxis,offsetof(DIJOYSTATE,lY),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        objects[i++]={&GUID_ZAxis,offsetof(DIJOYSTATE,lZ),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        objects[i++]={&GUID_RxAxis,offsetof(DIJOYSTATE,lRx),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        objects[i++]={&GUID_RyAxis,offsetof(DIJOYSTATE,lRy),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        objects[i++]={&GUID_RzAxis,offsetof(DIJOYSTATE,lRz),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(0)),0};
        for (unsigned n=0;n<2;++n)
            objects[i++]={&GUID_Slider,offsetof(DIJOYSTATE,rglSlider)+n*sizeof(LONG),static_cast<DWORD>(DIDFT_AXIS|DIDFT_MAKEINSTANCE(n)),0};
        for (unsigned n=0;n<4;++n)
            objects[i++]={&GUID_POV,offsetof(DIJOYSTATE,rgdwPOV)+n*sizeof(DWORD),static_cast<DWORD>(DIDFT_POV|DIDFT_MAKEINSTANCE(n)),0};
        for (unsigned n=0;n<32;++n)
            objects[i++]={&GUID_Button,offsetof(DIJOYSTATE,rgbButtons)+n,static_cast<DWORD>(DIDFT_BUTTON|DIDFT_MAKEINSTANCE(n)),0};
        return DIDATAFORMAT{sizeof(DIDATAFORMAT),sizeof(DIOBJECTDATAFORMAT),DIDF_ABSAXIS,
            sizeof(DIJOYSTATE),i,objects};
    }();
    return &format;
}
}
struct Win32Runtime::Native {
    Win32Runtime* owner;
    HWND window=nullptr; HDC dc=nullptr; HGLRC gl=nullptr;
    HMODULE inputModule=nullptr; IDirectInput7A* input=nullptr; IDirectInputDevice2A* joystick=nullptr;
    UINT_PTR timer=0;
    bool displayChanged=false;
    bool classRegistered=false;
    static int special(WPARAM key) {
        if (key>=VK_F1 && key<=VK_F12) return static_cast<int>(key-VK_F1)+1;
        switch (key) { case VK_LEFT:return 100; case VK_UP:return 101; case VK_RIGHT:return 102; case VK_DOWN:return 103;
            case VK_PRIOR:return 104;case VK_NEXT:return 105;case VK_HOME:return 106;case VK_END:return 107;case VK_INSERT:return 108;default:return 0; }
    }
    static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
        auto* runtime=reinterpret_cast<Win32Runtime*>(GetWindowLongPtrA(window,GWLP_USERDATA));
        if (message==WM_NCCREATE) { runtime=static_cast<Win32Runtime*>(reinterpret_cast<CREATESTRUCTA*>(l)->lpCreateParams);
            SetWindowLongPtrA(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(runtime)); }
        if (!runtime) return DefWindowProcA(window,message,w,l);
        auto& s=runtime->state; auto& cb=runtime->callbacks;
        switch (message) {
        case WM_CLOSE: DestroyWindow(window); return 0;
        case WM_DESTROY:
            if (runtime->native->gl) { wglMakeCurrent(nullptr,nullptr); wglDeleteContext(runtime->native->gl); runtime->native->gl=nullptr; }
            if (runtime->native->dc) { ReleaseDC(window,runtime->native->dc); runtime->native->dc=nullptr; }
            s.created=false; runtime->native->window=nullptr; PostQuitMessage(0); return 0;
        case WM_SIZE: s.width=LOWORD(l); s.height=HIWORD(l); s.minimized=w==SIZE_MINIMIZED; return 0;
        case WM_ACTIVATE: s.active=LOWORD(w)!=WA_INACTIVE;
            if (!s.active) s.modifiers=-1;
            // 004dd9b0: acquire on activation and unacquire on deactivation;
            // the saved helper ignores either HRESULT.
            if (runtime->native->joystick) { if (s.active) runtime->native->joystick->Acquire(); else runtime->native->joystick->Unacquire(); }
            return 0;
        case WM_CHAR: runtime->updateModifiers();
            if (w==27 && s.exitOnEscape) { PostMessageA(window,WM_CLOSE,0,0); return 0; }
            if (cb.keyDown && w<=255) cb.keyDown(cb.context,static_cast<std::uint8_t>(w)); return 0;
        case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP: {
            runtime->updateModifiers(); int key=special(w);
            if (key) { auto callback=(message==WM_KEYUP || message==WM_SYSKEYUP)?cb.specialUp:cb.specialDown;
                if (callback) callback(cb.context,key); return 0; } break; }
        case WM_MOUSEMOVE: { auto callback=(w&(MK_LBUTTON|MK_MBUTTON|MK_RBUTTON))?cb.mouseMotion:cb.passiveMotion;
            if (callback) callback(cb.context,GET_X_LPARAM(l),GET_Y_LPARAM(l)); return 0; }
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONUP: {
            runtime->updateModifiers(); int button=(message==WM_LBUTTONDOWN || message==WM_LBUTTONUP)?0:
                (message==WM_MBUTTONDOWN || message==WM_MBUTTONUP)?1:2;
            int down=(message==WM_LBUTTONDOWN || message==WM_MBUTTONDOWN || message==WM_RBUTTONDOWN)?0:1;
            if (cb.mouseButton) cb.mouseButton(cb.context,button,down,GET_X_LPARAM(l),GET_Y_LPARAM(l)); return 0; }
        case WM_TIMER: runtime->dispatchJoystick(); return 0;
        default: break;
        }
        return DefWindowProcA(window,message,w,l);
    }
    static BOOL CALLBACK device(const DIDEVICEINSTANCEA* instance,void* context) {
        auto& self=*static_cast<Native*>(context);
        if (SUCCEEDED(self.input->CreateDeviceEx(instance->guidInstance,IID_IDirectInputDevice2A,
            reinterpret_cast<void**>(&self.joystick),nullptr))) return DIENUM_STOP;
        return DIENUM_CONTINUE;
    }
    static BOOL CALLBACK axis(const DIDEVICEOBJECTINSTANCEA* object,void* context) {
        auto& self=*static_cast<Native*>(context);
        DIPROPRANGE range{}; range.diph.dwSize=sizeof(range); range.diph.dwHeaderSize=sizeof(range.diph);
        range.diph.dwObj=object->dwType; range.diph.dwHow=DIPH_BYID; range.lMin=-1000; range.lMax=1000;
        self.joystick->SetProperty(DIPROP_RANGE,&range.diph); return DIENUM_CONTINUE;
    }
};
Win32Runtime::Win32Runtime():native(new Native{this}) {}
Win32Runtime::~Win32Runtime() { shutdown(); delete native; }
void Win32Runtime::initializeFromArgs(int argc,char* const* argv) {
    HDC desktop=GetDC(nullptr);
    state.desktopWidth=GetSystemMetrics(SM_CXSCREEN);
    state.desktopHeight=GetSystemMetrics(SM_CYSCREEN);
    state.desktopBits=GetDeviceCaps(desktop,PLANES)*GetDeviceCaps(desktop,BITSPIXEL);
    ReleaseDC(nullptr,desktop);
    bool first=!state.argumentsInitialized;
    Runtime::initializeFromArgs(argc,argv);
    if (first) {
        registerWindowClass();
        HMODULE input=LoadLibraryA("DINPUT.DLL");
        if (input) { state.joystickAvailable=true; FreeLibrary(input); }
    }
}
bool Win32Runtime::activateExistingInstance() { // 004d9630
    HANDLE mutex=CreateMutexA(nullptr,FALSE,state.className.c_str());
    DWORD result=WaitForSingleObject(mutex,10000);
    bool activated=result==WAIT_TIMEOUT;
    if (!activated) {
        HWND window=FindWindowA(state.className.c_str(),nullptr);
        if (window) {
            HWND popup=GetLastActivePopup(window);
            BringWindowToTop(window);
            if (IsIconic(popup)) ShowWindow(popup,SW_RESTORE);
            else SetForegroundWindow(popup);
            activated=true;
        }
    }
    ReleaseMutex(mutex);
    // Original leaks this handle; the adaptation owns and closes it.
    if (mutex) CloseHandle(mutex);
    return activated;
}
void Win32Runtime::registerWindowClass() { // 004d8940
    if (native->classRegistered) return;
    if (!state.allowMultipleInstances && activateExistingInstance()) std::exit(0);
    WNDCLASSEXA cls{}; cls.cbSize=sizeof(cls);
    cls.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    cls.lpfnWndProc=Native::procedure; cls.hInstance=GetModuleHandleA(nullptr);
    cls.hIcon=LoadIconA(cls.hInstance,MAKEINTRESOURCEA(101)); cls.hIconSm=cls.hIcon;
    cls.hCursor=LoadCursor(nullptr,IDC_ARROW); cls.lpszClassName=state.className.c_str();
    if (!RegisterClassExA(&cls)) {
        const DWORD error=GetLastError();
        if (callbacks.error) callbacks.error(callbacks.context,"RegisterClassEx failed");
        LPSTR message=nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,error,0x400,reinterpret_cast<LPSTR>(&message),0,nullptr);
        if (callbacks.error) {
            if (message) callbacks.error(callbacks.context,message);
            callbacks.error(callbacks.context,"\n");
        }
        MessageBoxA(nullptr,message,"GetLastError",0x40);
        if (message) LocalFree(message);
        std::exit(1);
    }
    native->classRegistered=true;
}
// 004d8340: try matching enumerated modes, then the saved-index mode with
// dimensions/depth, then depth and dimensions separately. Preserve the index-0
// sentinel behavior of the original rather than choosing a new best mode.
bool Win32Runtime::selectDisplayMode(int width,int height,int bits) {
    auto report=[this](const char* format,long a=0,long b=0,long c=0,long d=0) {
        char message[256];
        std::snprintf(message,sizeof(message),format,a,b,c,d);
        if (callbacks.error) callbacks.error(callbacks.context,message);
    };
    DWORD fallback=0,index=0;
    DEVMODEA mode{}; mode.dmSize=sizeof(mode);
    while (EnumDisplaySettingsA(nullptr,index,&mode)) {
        if (mode.dmBitsPerPel==static_cast<DWORD>(bits) && mode.dmPelsWidth==static_cast<DWORD>(width) &&
            mode.dmPelsHeight==static_cast<DWORD>(height)) {
            report("Trying %ldx%ldx%ld-bit color (%ld hertz) ",
                static_cast<long>(mode.dmPelsWidth),static_cast<long>(mode.dmPelsHeight),
                static_cast<long>(mode.dmBitsPerPel),static_cast<long>(mode.dmDisplayFrequency));
            if (ChangeDisplaySettingsA(&mode,CDS_FULLSCREEN)==DISP_CHANGE_SUCCESSFUL) return true;
            report("Nope, didnt work ");
            if (fallback==0) fallback=index;
        }
        ++index;
    }
    report("No matching mode to %ldx%ldx%ld-bit color available ",
        static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    // dmSize is supplied explicitly for modern SDKs. No DEVMODE storage is
    // reused as a sprintf buffer as it appears in the old stack decompilation.
    EnumDisplaySettingsA(nullptr,fallback,&mode);
    mode.dmBitsPerPel=bits; mode.dmPelsWidth=width; mode.dmPelsHeight=height;
    mode.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
    report("Trying %ldx%ldx%ld-bit color (Win95 Hack) ",
        static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    LONG result=ChangeDisplaySettingsA(&mode,CDS_FULLSCREEN);
    if (result==DISP_CHANGE_SUCCESSFUL) return true;
    report("Nope, didnt work ");
    mode.dmFields=DM_BITSPERPEL;
    report("Trying %ldx%ldx%ld-bit color (Separate BPP switch) ",
        static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    result=ChangeDisplaySettingsA(&mode,CDS_FULLSCREEN);
    if (result==DISP_CHANGE_SUCCESSFUL) {
        mode.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;
        report("Trying %ldx%ldx%ld-bit color (Separate resolution switch) ",
            static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
        result=ChangeDisplaySettingsA(&mode,CDS_FULLSCREEN);
        if (result==DISP_CHANGE_SUCCESSFUL) return true;
        ChangeDisplaySettingsA(nullptr,0);
    }
    report("Nope, didnt work ");
    if (result==1)
        report("You must change your video mode to %ldx%ldx%ld-bit color and then restart ",
            static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    else if (result==-2)
        report("The video mode %ldx%ldx%ld-bit color is not supported ",
            static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    else if (result==-1)
        report("Hardware failed to change to %ldx%ldx%ld-bit color ",
            static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    else
        report("Failed to change to %ldx%ldx%ld-bit color - unknown reason ",
            static_cast<long>(width),static_cast<long>(height),static_cast<long>(bits));
    return false;
}
bool Win32Runtime::create(const char* title) {
    if (state.created || state.width<=0 || state.height<=0) return false;
    HINSTANCE instance=GetModuleHandleA(nullptr);
    registerWindowClass();
    DWORD style=WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
    int x=CW_USEDEFAULT,y=CW_USEDEFAULT;
    RECT bounds{0,0,state.width,state.height};
    if (state.argumentsInitialized && !(state.width<state.desktopWidth && state.height<state.desktopHeight && state.desktopBits>8))
        state.fullscreenRequested=true;
    if (state.fullscreenRequested) {
        style=WS_POPUP|WS_CLIPCHILDREN;
        if (state.fullscreenMode==FullscreenMode::Borderless) {
            // New default: cover the primary monitor without changing its
            // resolution, refresh rate or bit depth. No desktop restoration
            // is needed on shutdown because no mode change occurred.
            MONITORINFO monitor{}; monitor.cbSize=sizeof(monitor);
            if (!GetMonitorInfoA(MonitorFromPoint(POINT{0,0},MONITOR_DEFAULTTOPRIMARY),&monitor)) return false;
            bounds=monitor.rcMonitor;
            x=bounds.left; y=bounds.top;
        } else {
            if (!selectDisplayMode(state.width,state.height,state.colorBits)) return false;
            native->displayChanged=true;
            x=y=0;
        }
    } else {
        AdjustWindowRect(&bounds,style,FALSE);
    }
    native->window=CreateWindowExA(WS_EX_APPWINDOW,state.className.c_str(),title,style,x,y,
        bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,instance,this);
    if (state.fullscreenRequested && state.fullscreenMode==FullscreenMode::Exclusive) {
        UINT screenSaverState=0;
        SystemParametersInfoA(0x61,1,&screenSaverState,0);
    }
    if (!native->window) { shutdown(); return false; }
    native->dc=GetDC(native->window);
    PIXELFORMATDESCRIPTOR format{}; format.nSize=sizeof(format); format.nVersion=1;
    format.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    format.iPixelType=PFD_TYPE_RGBA; format.cColorBits=static_cast<BYTE>(state.colorBits); format.cDepthBits=static_cast<BYTE>(state.depthBits);
    int pixel=ChoosePixelFormat(native->dc,&format);
    if (!pixel) {
        if (callbacks.error) callbacks.error(callbacks.context,"ChoosePixelFormat failed ");
        MessageBoxA(nullptr,"ChoosePixelFormat failed","Error",MB_OK);
        shutdown(); return false;
    }
    // The saved helper ignores both return values and reports success after
    // calling these APIs. Keep that control flow; WGL context creation below
    // supplies the next observable failure point.
    (void)DescribePixelFormat(native->dc,pixel,sizeof(format),&format);
    (void)SetPixelFormat(native->dc,pixel,&format);
    native->gl=wglCreateContext(native->dc);
    if (!native->gl || !wglMakeCurrent(native->dc,native->gl)) { shutdown(); return false; }
    state.created=state.active=true; ShowWindow(native->window,SW_SHOW); UpdateWindow(native->window);
    // Compatibility wiring for the unavailable original WNDPROC body: prepare
    // the real device before Driver registers its joystick callback.
    state.joystickAvailable=initializeJoystick();
    return true;
}
std::uint32_t Win32Runtime::milliseconds() const { return timeGetTime(); }
void Win32Runtime::sleepMicroseconds(std::uint32_t usec) { Sleep(usec/1000); }
int Win32Runtime::systemProperty(int code) const { return code==3?1:code==4?GetSystemMetrics(SM_MOUSEPRESENT):0; }
void Win32Runtime::updateModifiers() { state.modifiers=0;
    if (GetAsyncKeyState(VK_SHIFT)!=0) state.modifiers|=Shift;
    if (GetAsyncKeyState(VK_CONTROL)!=0) state.modifiers|=Control;
    if (GetAsyncKeyState(VK_MENU)!=0) state.modifiers|=Alt;
}
void Win32Runtime::setPriority(int value) { state.priority=value;
    SetPriorityClass(GetCurrentProcess(),value==0?IDLE_PRIORITY_CLASS:value==1?NORMAL_PRIORITY_CLASS:value==3?REALTIME_PRIORITY_CLASS:HIGH_PRIORITY_CLASS);
}
void Win32Runtime::setCursorMode(CursorMode mode) {
    if (!state.active) return;
    if (mode==CursorMode::Released && state.cursorCaptured) { ReleaseCapture(); ShowCursor(TRUE); state.cursorCaptured=false; }
    else if (mode==CursorMode::Captured && !state.cursorCaptured) { SetCapture(native->window); ShowCursor(FALSE); state.cursorCaptured=true; }
}
void* Win32Runtime::nativeWindowHandle() const { return native->window; }
void Win32Runtime::warpCursor(int x,int y) { if (state.active) { POINT point{x,y}; ClientToScreen(native->window,&point); SetCursorPos(point.x,point.y); } }
void Win32Runtime::setJoystickTimer(unsigned interval) { native->timer=SetTimer(native->window,0,interval,nullptr); }
bool Win32Runtime::initializeJoystick() {
    if (!native->window) return false;
    if (native->joystick && state.joystickAvailable) return true;
    if (native->joystick) { native->joystick->Release(); native->joystick=nullptr; }
    if (native->input) { native->input->Release(); native->input=nullptr; }
    if (native->inputModule) { FreeLibrary(native->inputModule); native->inputModule=nullptr; }
    auto reportFailure=[this](int line,const char* operation) {
        char message[128];
        std::snprintf(message,sizeof(message),"joystick.cxx LINE %d - %s ",line,operation);
        if (callbacks.error) callbacks.error(callbacks.context,message);
    };
    native->inputModule=LoadLibraryA("DINPUT.DLL");
    if (!native->inputModule) { reportFailure(0x43,"LoadLibrary(\"DINPUT.DLL\")"); return false; }
    using Create=HRESULT (WINAPI*)(HINSTANCE,DWORD,REFIID,void**,IUnknown*);
    auto create=reinterpret_cast<Create>(GetProcAddress(native->inputModule,"DirectInputCreateEx"));
    if (!create) { reportFailure(0x4a,"GetProcAddress"); return false; }
    HRESULT result=create(GetModuleHandleA(nullptr),0x700,IID_IDirectInput7A,
        reinterpret_cast<void**>(&native->input),nullptr);
    if (FAILED(result)) { reportFailure(0x52,"DirectInputCreateEx"); return false; }
    result=native->input->EnumDevices(DIDEVTYPE_JOYSTICK,Native::device,native,DIEDFL_ATTACHEDONLY);
    if (FAILED(result)) { reportFailure(0x59,"EnumDevices"); return false; }
    if (!native->joystick) { reportFailure(0x60,"Joystick not found"); return false; }
    result=native->joystick->SetDataFormat(joystickDataFormat());
    if (FAILED(result)) { reportFailure(0x68,"SetDataFormat"); return false; }
    result=native->joystick->SetCooperativeLevel(native->window,DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
    if (FAILED(result)) { reportFailure(0x70,"SetCooperativeLevel"); return false; }
    DIDEVCAPS capabilities{}; capabilities.dwSize=sizeof(capabilities);
    result=native->joystick->GetCapabilities(&capabilities);
    if (FAILED(result)) { reportFailure(0x79,"GetCapabilities"); return false; }
    native->joystick->EnumObjects(Native::axis,native,DIDFT_AXIS);
    native->joystick->Acquire();
    state.joystickAvailable=true; return true;
}
bool Win32Runtime::pollJoystick(JoystickSample& sample) {
    if (!native->joystick || FAILED(native->joystick->Poll())) return false;
    DIJOYSTATE raw{};
    HRESULT result;
    // The saved loop is Poll -> GetDeviceState; on INPUTLOST it Acquires,
    // polls again only when Acquire itself reports INPUTLOST, then retries.
    // Do not silently collapse this into one Acquire/GetDeviceState retry.
    for (;;) {
        result=native->joystick->GetDeviceState(sizeof(raw),&raw);
        if (result!=DIERR_INPUTLOST) break;
        HRESULT acquired=native->joystick->Acquire();
        if (FAILED(acquired)) return false;
        if (acquired!=DIERR_INPUTLOST) break;
        if (FAILED(native->joystick->Poll())) return false;
    }
    if (FAILED(result)) return false;
    sample.axes[0]=raw.lX; sample.axes[1]=raw.lY; sample.axes[2]=raw.lZ; sample.axes[3]=raw.lRz;
    for (int i=0;i<32;++i) sample.buttons[i]=raw.rgbButtons[i]; return true;
}
bool Win32Runtime::pumpEvents() {
    MSG message;
    while (PeekMessageA(&message,nullptr,0,0,PM_REMOVE)) {
        if (message.message==WM_QUIT) return false;
        TranslateMessage(&message); DispatchMessageA(&message);
    }
    return state.created;
}
void Win32Runtime::finishAndPresent() { if (native->gl) { glFinish(); SwapBuffers(native->dc); } }
int Win32Runtime::run() {
    const int priority=state.priority;
    setPriority(priority);
    while (state.created) {
        if (priority==3) setPriority(2);
        MSG message{};
        BOOL available=PeekMessageA(&message,nullptr,0,0,0);
        while (available==1) {
            const BOOL received=GetMessageA(&message,nullptr,0,0);
            if (received==0) engine_runtime::engineRuntime_exit_adapter(static_cast<int>(message.wParam));
            TranslateMessage(&message);
            DispatchMessageA(&message);
            available=PeekMessageA(&message,nullptr,0,0,0);
        }
        if (priority==3) setPriority(3);
        oneFrame();
    }
    return 0;
}
void Win32Runtime::shutdown() {
    // 004d96d0 makes both calls before tearing down the native window and
    // ignores their return values. Preserve that behavior even when this run
    // never changed display mode.
    UINT screenSaverState=0;
    SystemParametersInfoA(0x61,0,&screenSaverState,0);
    ChangeDisplaySettingsA(nullptr,0);
    native->displayChanged=false;
    if (native->timer) { KillTimer(native->window,native->timer); native->timer=0; }
    // 004ddbf0: release device, DirectInput interface, and the owned DLL.
    if (native->joystick) { native->joystick->Unacquire(); native->joystick->Release(); native->joystick=nullptr; }
    if (native->input) { native->input->Release(); native->input=nullptr; }
    if (native->inputModule) { FreeLibrary(native->inputModule); native->inputModule=nullptr; }
    if (state.cursorCaptured) { ReleaseCapture(); ShowCursor(TRUE); }
    if (native->gl) { wglMakeCurrent(nullptr,nullptr); wglDeleteContext(native->gl); native->gl=nullptr; }
    if (native->window && native->dc) ReleaseDC(native->window,native->dc);
    native->dc=nullptr;
    if (native->window) DestroyWindow(native->window);
    native->window=nullptr;
    state.created=state.active=state.cursorCaptured=state.joystickAvailable=false;
}
}
#endif
