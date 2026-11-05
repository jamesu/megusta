#pragma once
#include "openal.h"
#include "../engine/resource_file.h"
#define DIRECTSOUND_VERSION 0x0700
#include <windows.h>
#include <dsound.h>
#include <cstdio>
namespace megusta { namespace audio { namespace ds {
struct ErrorState { int stickyError; };
struct HandleNode { HandleNode* owner; void* payload; HandleNode* next; HandleNode* previous; };
struct Buffer {
    std::uint32_t format,frequency,bits,channels,dataBytes,segmentBytes;
    ResourceFile* streamFile; void* pcmData; std::uint32_t reservedZero20;
    std::uint8_t streaming;
    std::uint32_t playedBytes,lastPlayCursor,writeCursor;
};
struct Source {
    int coneInnerAngle,coneOuterAngle; float coneOuterGain,pan,pitch,gain;
    float position[3],direction[3],velocity[3],minDistance,maxDistance;
    std::uint8_t looping,streaming,relative;
    Handle boundBuffer; int dirty;
    IDirectSoundBuffer* dsBuffer; IDirectSound3DBuffer* ds3dBuffer;
    Handle loadedBuffer;
};
struct Listener {
    float position[3],velocity[3],orientationRight[3],orientationUp[3],orientationFront[3];
    float gain,distanceScale; int dirty; IDirectSound3DListener* dsListener;
};
struct Context {
    ErrorState* errorState; Listener* listener;
    HandleNode* sourceHandles; HandleNode* bufferHandles;
    Source* activeStreamingSource;
    std::uint32_t buffersGenerated,dsBuffersCreated,ownBufferReuses,idleBufferRecycles;
    IDirectSound* device; IDirectSoundBuffer* primaryBuffer;
    HANDLE notificationEvent,workerThread; DWORD workerThreadId;
    DSCAPS deviceCaps; DWORD cooperativeLevel;
};
using Factory=HRESULT (WINAPI *)(LPCGUID,LPDIRECTSOUND*,LPUNKNOWN);
struct ImplementationState { Diagnostic diagnostic; Context* current; HMODULE module; Factory factory; };
extern ImplementationState implementation;
ErrorState* createErrorState(); void destroyErrorState(ErrorState*);
void setError(Context*,int,const char*); const char* errorString(Context*,int);
void setDiagnostic(Diagnostic); void initialize(int); void shutdown(); Context* current();
Context* createContext(std::uint32_t,int); void destroyContext(Context*);
ErrorState* contextErrorState(Context*); Listener* contextListener(Context*);
Buffer* contextBuffer(Context*,Handle); Source* contextSource(Context*,Handle);
Listener* createListener(Context*); void destroyListener(Listener*);
Source* createSource(); void destroySource(Source*);
Buffer* createBuffer(); void destroyBuffer(Buffer*);
HandleNode* createHandleList(); void destroyHandleList(HandleNode*);
void* nextHandle(HandleNode*,HandleNode**); HandleNode* insertHandle(HandleNode*);
void removeHandle(HandleNode*,HandleNode*); bool containsHandle(HandleNode*,HandleNode*);
void* handlePayload(HandleNode*,HandleNode*); void setHandlePayload(HandleNode*,HandleNode*,void*);
int gainToAttenuation(double); int panToUnits(double); void crossProduct(const float*,const float*,float*);
std::int64_t truncateX87(long double);
int getError(Context*);
std::uint32_t genBuffers(Context*,std::uint32_t,Handle*); void deleteBuffers(Context*,std::uint32_t,const Handle*);
bool isBuffer(Context*,Handle); std::uint32_t genSources(Context*,std::uint32_t,Handle*);
void deleteSources(Context*,std::uint32_t,const Handle*);
void listenerf(Context*,int,float);void listener3f(Context*,int,float,float,float);
void listenerfv(Context*,int,const float*); void distanceScale(Context*,float);
void sourcef(Context*,Handle,int,float);void source3f(Context*,Handle,int,float,float,float);
void sourcei(Context*,Handle,int,int);void getSourcei(Context*,Handle,int,int*);
void sourcePlay(Context*,Handle);void sourceStop(Context*,Handle);
void bufferData(Context*,Handle,int,const void*,std::uint32_t,std::uint32_t);
void bufferStream(Context*,Handle,ResourceFile*);
bool setCooperativeLevel(Context*);bool createPrimary(Context*);
bool splitFormat(int,int*,int*); bool setPrimaryFormat(Context*,std::uint32_t,int);
void makePcmFormat(std::uint32_t,std::uint32_t,std::uint32_t,WAVEFORMATEX*);
bool restoreBuffer(Context*,bool*);bool remainingTime(Context*,Handle,int,float*);
bool reuseOwnBuffer(Context*,Source*);bool recycleIdleBuffer(Context*,Source*);
bool createDSBuffer(Context*,Source*);bool uploadPcm(Context*,Source*);void updateSource(Context*,Source*);
DWORD WINAPI notificationProc(void*);bool streamNotification(Context*);
bool fillBuffer(Context*,int);bool resetStream(ResourceFile*);bool readStream(ResourceFile*,void*,std::uint32_t,std::uint32_t*);
} } }
