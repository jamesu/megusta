#include "portable_mixer.h"
#include "wave.h"
#ifdef __EMSCRIPTEN__
#include "platform/browser/audio_output.h"
#else
#include <SDL.h>
#endif
#include <cstdlib>
#include <cstring>
#include <new>
namespace megusta::audio {
namespace {
PortableMixer mixer;
#ifndef __EMSCRIPTEN__
SDL_AudioDeviceID device=0;
unsigned outputRate=22050;
#endif
Diagnostic diagnostic=nullptr;
struct Lock {
#ifndef __EMSCRIPTEN__
    Lock(){if(device)SDL_LockAudioDevice(device);}
    ~Lock(){if(device)SDL_UnlockAudioDevice(device);}
#endif
};
void render(float* data,unsigned frames,unsigned rate) {
    try{mixer.render(data,frames,rate);}
    catch(...){std::memset(data,0,frames*2*sizeof(float));mixer.fail(OutOfMemory);}
}
#ifndef __EMSCRIPTEN__
void callback(void*,Uint8* data,int bytes){
    render(reinterpret_cast<float*>(data),bytes/(2*sizeof(float)),outputRate);
}
#endif
void shutdownDevice(){
#ifdef __EMSCRIPTEN__
    closeBrowserAudioOutput();mixer.clear();
#else
    if(device){SDL_PauseAudioDevice(device,1);{Lock lock;mixer.clear();}SDL_CloseAudioDevice(device);device=0;SDL_QuitSubSystem(SDL_INIT_AUDIO);}
    else mixer.clear();
#endif
}
void initialize(int){
    shutdownDevice();
#ifdef __EMSCRIPTEN__
    enabled=openBrowserAudioOutput(render,[](){
        enabled=false;mixer.clear();
        if(diagnostic)diagnostic("Browser AudioWorklet output failed");
    });
    if(!enabled && diagnostic)diagnostic("Could not open browser AudioWorklet output");
#else
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){enabled=false;if(diagnostic)diagnostic(SDL_GetError());return;}
    SDL_AudioSpec wanted{},obtained{};wanted.freq=22050;wanted.format=AUDIO_F32SYS;wanted.channels=2;
    wanted.samples=1024;wanted.callback=callback;
    device=SDL_OpenAudioDevice(nullptr,0,&wanted,&obtained,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if(!device){enabled=false;if(diagnostic)diagnostic(SDL_GetError());SDL_QuitSubSystem(SDL_INIT_AUDIO);return;}
    outputRate=obtained.freq;enabled=true;SDL_PauseAudioDevice(device,0);
#endif
}
template<class F> void change(F f){Lock lock;try{f();}catch(const std::bad_alloc&){mixer.fail(OutOfMemory);}}
}
const OpenALApi& sdlMixerApi(){
    static const OpenALApi table={
        initialize,shutdownDevice,[](Diagnostic d){diagnostic=d;},
        [](float v){change([&]{mixer.distanceScale(v);});},
        [](int t,float v){change([&]{mixer.listenerf(t,v);});},
        [](int t,float x,float y,float z){change([&]{mixer.listener3f(t,x,y,z);});},
        [](int t,const float* v){change([&]{mixer.listenerfv(t,v);});},
        [](unsigned n,Handle* h){unsigned result=0;change([&]{result=mixer.genSources(n,h);});return result;},
        [](unsigned n,const Handle* h){change([&]{mixer.deleteSources(n,h);});},
        [](Handle h,int t,int v){change([&]{mixer.sourcei(h,t,v);});},
        [](Handle h,int t,float v){change([&]{mixer.sourcef(h,t,v);});},
        [](Handle h,int t,float x,float y,float z){change([&]{mixer.source3f(h,t,x,y,z);});},
        [](Handle h,int t,int* v){change([&]{mixer.getSourcei(h,t,v);});},
        [](Handle h){change([&]{mixer.play(h);});},[](Handle h){change([&]{mixer.stop(h);});},
        [](unsigned n,Handle* h){unsigned result=0;change([&]{result=mixer.genBuffers(n,h);});return result;},
        [](unsigned n,const Handle* h){change([&]{mixer.deleteBuffers(n,h);});},
        [](Handle h){Lock lock;return mixer.isBuffer(h);},
        [](Handle h,int t,const void* p,unsigned bytes,unsigned rate){change([&]{mixer.bufferData(h,t,p,bytes,rate);});},
        [](Handle h,ResourceFile* f){change([&]{mixer.bufferStream(h,f);});},
        [](){Lock lock;return mixer.getError();},wave::loadData,[](void* p){std::free(p);}
    };return table;
}
}
