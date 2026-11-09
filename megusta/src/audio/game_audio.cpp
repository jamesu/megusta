#include "openal.h"
#include "../engine/resource_runtime.h"
#include <cstdlib>
#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif
namespace megusta { namespace audio {
bool enabled=true;
int requestedBits=16;
static const OpenALApi* selectedApi=nullptr;
void bindApi(const OpenALApi& table) { selectedApi=&table; }
const OpenALApi& api() {
#ifdef MEGUSTA_AUDIO_DEFAULT_DIRECTSOUND
    return selectedApi ? *selectedApi : directSoundApi();
#else
    return *selectedApi; // External-backend builds require bindApi before use.
#endif
}
void shutdown() {
    if (enabled) { api().shutdown();api().setDiagnostic(nullptr); }
}
void initialize(int* argc,void* argvOrDiagnostic) {
#ifdef MEGUSTA_AUDIO_NO_BACKEND
    (void)argc; (void)argvOrDiagnostic; enabled=false; return;
#endif
    if (argc && *argc>1) {
        const auto* argv=static_cast<const char* const*>(argvOrDiagnostic);
        for (int i=1;i<*argc;++i) {
#if defined(_WIN32)
            if (!_stricmp(argv[i],"-nosound")) { enabled=false;return; }
#else
            if (!strcasecmp(argv[i],"-nosound")) { enabled=false;return; }
#endif
        }
    }
    if (requestedBits<1) enabled=false;
    if (!argc) api().setDiagnostic(reinterpret_cast<Diagnostic>(argvOrDiagnostic));
    api().initialize(requestedBits);
    std::atexit(shutdown);
}
void initializeBridge(int* argc,void* argvOrDiagnostic) { initialize(argc,argvOrDiagnostic); }
void distanceScale(float value) { if (enabled) api().distanceScale(value); }
void listenerf(int token,float value) { if (enabled) api().listenerf(token,value); }
void listener3f(int token,float x,float y,float z) { if (enabled) api().listener3f(token,x,y,z); }
void listenerfv(int token,const float* values) { if (enabled) api().listenerfv(token,values); }
std::uint32_t genSources(std::uint32_t count,Handle* handles) { return enabled ? api().genSources(count,handles) : 0; }
void deleteSources(std::uint32_t count,const Handle* handles) { if (enabled) api().deleteSources(count,handles); }
void sourcei(Handle source,int token,int value) { if (enabled) api().sourcei(source,token,value); }
void sourcef(Handle source,int token,float value) { if (enabled) api().sourcef(source,token,value); }
void source3f(Handle source,int token,float x,float y,float z) { if (enabled) api().source3f(source,token,x,y,z); }
void sourcePlay(Handle source) { if (enabled) api().sourcePlay(source); }
std::uint8_t sourceIsPlaying(Handle source) {
    int playing=0;if (enabled) api().getSourcei(source,IsPlaying,&playing);
    return static_cast<std::uint8_t>(playing);
}
void sourceStop(Handle source) { if (enabled) api().sourceStop(source); }
std::uint32_t genBuffers(std::uint32_t count,Handle* handles) { return enabled ? api().genBuffers(count,handles) : 0; }
void deleteBuffers(std::uint32_t count,const Handle* handles) { if (enabled) api().deleteBuffers(count,handles); }
void bufferData(Handle buffer,int format,const void* data,std::uint32_t bytes,std::uint32_t rate) {
    if (enabled) api().bufferData(buffer,format,data,bytes,rate);
}
void bufferStream(Handle buffer,const char* path) {
    if (!enabled || !sharedIo || !sharedIo->open) return;
    auto* file=sharedIo->open(path,"rb");
    api().bufferStream(buffer,file);
}
int getError() { return enabled ? api().getError() : 0; }
void loadWav(const char* path,int* format,void** data,std::uint32_t* bytes,std::uint32_t* rate) {
    if (!enabled || !sharedIo || !sharedIo->open) return;
    auto* file=sharedIo->open(path,"rb");
    if (!file) return;
    api().loadWav(file,format,data,bytes,rate);
    if (sharedIo->close) sharedIo->close(file); else delete file;
}
void unloadWav(std::uint32_t,void* data) { if (enabled && data) api().unloadWav(data); }
} }
