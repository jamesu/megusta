#include "directsound_internal.h"
#include <cstdlib>
#include <cstring>
namespace megusta { namespace audio { namespace ds {
ImplementationState implementation{};
#if UINTPTR_MAX == 0xffffffffu
static_assert(sizeof(Context)==156 && sizeof(Source)==92 && sizeof(Buffer)==52 && sizeof(Listener)==76,
    "Recovered 32-bit audio records");
#endif
ErrorState* createErrorState() {
    auto* error=static_cast<ErrorState*>(std::malloc(sizeof(ErrorState)));
    if (error) error->stickyError=0;
    return error;
}
void destroyErrorState(ErrorState* error) { if (error) std::free(error); }
ErrorState* contextErrorState(Context* context) { return context->errorState; }
Listener* contextListener(Context* context) { return context->listener; }
const char* errorString(Context*,int error) {
    switch (error) {
    case NoError:return "No error";
    case InvalidOperation:return "Illegal operation";
    case InvalidValue:return "Illegal value passed as argument";
    case OutOfMemory:return "Out of memory";
    default:return "Unknown";
    }
}
void setError(Context* context,int error,const char* operation) {
    if (!context) return;
    auto* state=contextErrorState(context);
    if (state && !state->stickyError) state->stickyError=error;
    std::fprintf(stderr,"OpenAL: %s in %s",errorString(context,error),operation);
    char message[200];
    std::sprintf(message,"alSetError(%s) in %s",errorString(context,error),operation);
    if (implementation.diagnostic) implementation.diagnostic(message);
}
void setDiagnostic(Diagnostic callback) { implementation.diagnostic=callback; }
Context* current() { return implementation.current; } // Saved null-context branch calls a no-op.
void initialize(int bits) {
    implementation.module=LoadLibraryA("dSound.dll");
    if (implementation.module) {
        implementation.factory=reinterpret_cast<Factory>(GetProcAddress(implementation.module,"DirectSoundCreate"));
        if (implementation.factory) {
            implementation.current=createContext(22050,bits==8 ? Stereo8 : Stereo16);
            return;
        }
    }
    setError(nullptr,InvalidOperation,"alimInit");
}
Context* createContext(std::uint32_t rate,int format) {
    auto* context=static_cast<Context*>(std::malloc(sizeof(Context)));
    if (!context) return nullptr;
    std::memset(context,0,sizeof(Context));
    context->errorState=createErrorState();
    context->sourceHandles=createHandleList();
    context->bufferHandles=createHandleList();
    if (context->errorState && context->sourceHandles && context->bufferHandles &&
        SUCCEEDED(DirectSoundCreate(nullptr,&context->device,nullptr)) &&
        setCooperativeLevel(context) && createPrimary(context)) {
        std::memset(&context->deviceCaps,0,sizeof(DSCAPS));context->deviceCaps.dwSize=sizeof(DSCAPS);
        if (SUCCEEDED(context->device->GetCaps(&context->deviceCaps))) {
            context->listener=createListener(context);
            if (context->listener && setPrimaryFormat(context,rate,format) &&
                SUCCEEDED(context->primaryBuffer->Play(0,0,DSBPLAY_LOOPING))) {
                context->notificationEvent=CreateEventA(nullptr,FALSE,FALSE,nullptr);
                context->workerThread=CreateThread(nullptr,0,notificationProc,context,0,&context->workerThreadId);
                return context;
            }
        }
    }
    destroyContext(context);return nullptr;
}
void destroyContext(Context* context) {
    if (!context) return;
    HandleNode* cursor=nullptr;
    while (auto* buffer=static_cast<Buffer*>(nextHandle(context->bufferHandles,&cursor))) destroyBuffer(buffer);
    cursor=nullptr;
    while (auto* source=static_cast<Source*>(nextHandle(context->sourceHandles,&cursor))) destroySource(source);
    destroyHandleList(context->bufferHandles);destroyHandleList(context->sourceHandles);
    destroyListener(context->listener);destroyErrorState(context->errorState);
    if (context->primaryBuffer) { context->primaryBuffer->Stop();context->primaryBuffer->Release(); }
    if (context->device) context->device->Release();
    std::free(context);
}
void shutdown() {
    auto* context=implementation.current;
    PostThreadMessageA(context->workerThreadId,WM_QUIT,0,0);
    WaitForSingleObject(context->workerThread,INFINITE);
    CloseHandle(context->workerThread);CloseHandle(context->notificationEvent);
    destroyContext(context);
    if (implementation.module) {
        implementation.factory=nullptr;FreeLibrary(implementation.module);implementation.module=nullptr;
    }
    // The executable leaves currentContext unchanged after destruction.
}
Buffer* contextBuffer(Context* context,Handle handle) {
    return static_cast<Buffer*>(handlePayload(context->bufferHandles,handle));
}
Source* contextSource(Context* context,Handle handle) {
    return static_cast<Source*>(handlePayload(context->sourceHandles,handle));
}
Listener* createListener(Context* context) {
    auto* listener=static_cast<Listener*>(std::malloc(sizeof(Listener)));
    if (!listener) { destroyListener(nullptr);return nullptr; }
    listener->dsListener=nullptr;
    if (FAILED(context->primaryBuffer->QueryInterface(IID_IDirectSound3DListener,
        reinterpret_cast<void**>(&listener->dsListener)))) { destroyListener(listener);return nullptr; }
    for (int i=0;i<3;++i) {
        listener->position[i]=0;listener->velocity[i]=0;
        listener->orientationRight[i]=i==0 ? -1.0f : 0.0f;
        listener->orientationUp[i]=i==1 ? 1.0f : 0.0f;
        listener->orientationFront[i]=i==2 ? 1.0f : 0.0f;
    }
    listener->distanceScale=1;listener->gain=1;
    context->primaryBuffer->SetVolume(gainToAttenuation(1));listener->dirty=1;
    return listener;
}
void destroyListener(Listener* listener) {
    if (listener) { if (listener->dsListener) listener->dsListener->Release();std::free(listener); }
}
Source* createSource() {
    auto* source=static_cast<Source*>(std::malloc(sizeof(Source)));
    if (!source) return nullptr;
    source->coneInnerAngle=360;source->coneOuterAngle=360;
    source->coneOuterGain=0;source->pan=0;source->pitch=1;source->gain=1;
    for (int i=2;i>=0;--i) { source->position[i]=0;source->direction[i]=0;source->velocity[i]=0; }
    source->minDistance=1;source->maxDistance=1e9f;
    source->looping=0;source->streaming=0;source->relative=0;source->boundBuffer=0;
    source->dsBuffer=nullptr;source->ds3dBuffer=nullptr;source->loadedBuffer=0;
    // dirty is not initialized by the saved constructor.
    return source;
}
void destroySource(Source* source) {
    if (!source) return;
    if (source->dsBuffer) {
        source->dsBuffer->Stop();
        if (source->ds3dBuffer) source->ds3dBuffer->Release();
        source->dsBuffer->Release();
    }
    std::free(source);
}
Buffer* createBuffer() {
    auto* buffer=static_cast<Buffer*>(std::malloc(sizeof(Buffer)));
    if (buffer) {
        buffer->format=Mono16;buffer->frequency=22050;buffer->bits=16;buffer->channels=1;
        buffer->dataBytes=0;buffer->segmentBytes=0;buffer->streamFile=nullptr;buffer->pcmData=nullptr;
        buffer->reservedZero20=0;buffer->playedBytes=0;buffer->lastPlayCursor=0;
        buffer->streaming=0;buffer->writeCursor=0;
    }
    return buffer;
}
void destroyBuffer(Buffer* buffer) {
    if (!buffer) return;
    delete buffer->streamFile;
    if (buffer->pcmData) std::free(buffer->pcmData);
    std::free(buffer);
}
} } }
