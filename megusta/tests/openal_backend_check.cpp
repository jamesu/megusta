#define CINTERFACE
#include "audio/directsound_internal.h"
#include "audio/wave.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace megusta::audio;
using namespace megusta::audio::ds;
using megusta::ResourceFile;
namespace wav=megusta::audio::wave;
static DWORD mockStatus,mockPlayCursor,mockPosition,mockFrequency,lockLength;
static LONG mockVolume,mockPan;
static int stopCalls,createCalls,notifications;
static bool failFirstCreate;
static unsigned char memory[100000];
static DSBUFFERDESC lastDescription;
static D3DVECTOR spatialPosition;
static DSBPOSITIONNOTIFY notificationPositions[16];
static IDirectSoundBuffer deviceBuffer;
static IDirectSound3DBuffer spatialBuffer;
static IDirectSoundNotify notify;
static IDirectSound3DListener deviceListener;
static float orientation[6],distanceFactor;
static int commits;
static HRESULT STDMETHODCALLTYPE status(IDirectSoundBuffer*,DWORD* output) { *output=mockStatus;return DS_OK; }
static HRESULT STDMETHODCALLTYPE position(IDirectSoundBuffer*,DWORD* play,DWORD* write) {
    if (play) *play=mockPlayCursor;if (write) *write=0;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE setPosition(IDirectSoundBuffer*,DWORD value) { mockPosition=value;return DS_OK; }
static HRESULT STDMETHODCALLTYPE frequency(IDirectSoundBuffer*,DWORD value) { mockFrequency=value;return DS_OK; }
static HRESULT STDMETHODCALLTYPE volume(IDirectSoundBuffer*,LONG value) { mockVolume=value;return DS_OK; }
static HRESULT STDMETHODCALLTYPE pan(IDirectSoundBuffer*,LONG value) { mockPan=value;return DS_OK; }
static HRESULT STDMETHODCALLTYPE stop(IDirectSoundBuffer*) { ++stopCalls;return DS_OK; }
static HRESULT STDMETHODCALLTYPE play(IDirectSoundBuffer*,DWORD,DWORD,DWORD flags) {
    mockStatus=DSBSTATUS_PLAYING|(flags&DSBPLAY_LOOPING ? DSBSTATUS_LOOPING : 0);return DS_OK;
}
static ULONG STDMETHODCALLTYPE releaseBuffer(IDirectSoundBuffer*) { return 1; }
static HRESULT STDMETHODCALLTYPE restore(IDirectSoundBuffer*) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE lock(IDirectSoundBuffer*,DWORD offset,DWORD bytes,void** first,DWORD* firstBytes,
        void** second,DWORD* secondBytes,DWORD) {
    assert(offset+bytes<=sizeof(memory));*first=memory+offset;*firstBytes=bytes;lockLength=bytes;
    if (second) *second=nullptr;if (secondBytes) *secondBytes=0;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE unlock(IDirectSoundBuffer*,void*,DWORD,void*,DWORD) { return DS_OK; }
static HRESULT STDMETHODCALLTYPE queryInterface(IDirectSoundBuffer*,REFIID id,void** output) {
    if (IsEqualIID(id,IID_IDirectSoundNotify)) { *output=&notify;return DS_OK; }
    if (IsEqualIID(id,IID_IDirectSound3DBuffer)) { *output=&spatialBuffer;return DS_OK; }
    if (IsEqualIID(id,IID_IDirectSound3DListener)) { *output=&deviceListener;return DS_OK; }
    return E_NOINTERFACE;
}
static HRESULT STDMETHODCALLTYPE setNotifications(IDirectSoundNotify*,DWORD count,LPCDSBPOSITIONNOTIFY positions) {
    assert(count==16);++notifications;std::memcpy(notificationPositions,positions,sizeof(notificationPositions));return DS_OK;
}
static ULONG STDMETHODCALLTYPE releaseNotify(IDirectSoundNotify*) { return 1; }
static ULONG STDMETHODCALLTYPE releaseSpatial(IDirectSound3DBuffer*) { return 1; }
static ULONG STDMETHODCALLTYPE releaseListener(IDirectSound3DListener*) { return 1; }
static HRESULT STDMETHODCALLTYPE listenerPosition(IDirectSound3DListener*,float x,float y,float z,DWORD flags) {
    assert(flags==DS3D_DEFERRED);spatialPosition.x=x;spatialPosition.y=y;spatialPosition.z=z;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE listenerOrientation(IDirectSound3DListener*,float a,float b,float c,float d,float e,float f,DWORD flags) {
    assert(flags==DS3D_DEFERRED);orientation[0]=a;orientation[1]=b;orientation[2]=c;
    orientation[3]=d;orientation[4]=e;orientation[5]=f;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE listenerCommit(IDirectSound3DListener*) { ++commits;return DS_OK; }
static HRESULT STDMETHODCALLTYPE listenerDistance(IDirectSound3DListener*,float value,DWORD flags) {
    assert(flags==DS3D_IMMEDIATE);distanceFactor=value;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE setSpatialPosition(IDirectSound3DBuffer*,float x,float y,float z,DWORD flags) {
    assert(flags==DS3D_DEFERRED);spatialPosition.x=x;spatialPosition.y=y;spatialPosition.z=z;return DS_OK;
}
static HRESULT STDMETHODCALLTYPE createSoundBuffer(IDirectSound*,LPCDSBUFFERDESC description,LPDIRECTSOUNDBUFFER* output,LPUNKNOWN) {
    ++createCalls;lastDescription=*description;
    if (failFirstCreate && createCalls==1) return DSERR_CONTROLUNAVAIL;
    *output=&deviceBuffer;return DS_OK;
}
static ULONG STDMETHODCALLTYPE releaseDevice(IDirectSound*) { return 1; }
static ResourceFile* makeWaveFile() {
    const wav::RiffHeader riff{0x46464952,44,0x45564157};
    const wav::ChunkHeader formatHeader{wav::FormatTag,16},dataHeader{wav::DataTag,8};
    const wav::Format format{1,1,22050,22050,1,8};
    const unsigned char data[8]={1,2,3,4,5,6,7,8};
    const std::size_t size=sizeof(riff)+sizeof(formatHeader)+sizeof(format)+sizeof(dataHeader)+sizeof(data);
    auto* bytes=static_cast<unsigned char*>(std::malloc(size));assert(bytes);
    auto* cursor=bytes;
    std::memcpy(cursor,&riff,sizeof(riff));cursor+=sizeof(riff);
    std::memcpy(cursor,&formatHeader,sizeof(formatHeader));cursor+=sizeof(formatHeader);
    std::memcpy(cursor,&format,sizeof(format));cursor+=sizeof(format);
    std::memcpy(cursor,&dataHeader,sizeof(dataHeader));cursor+=sizeof(dataHeader);
    std::memcpy(cursor,data,sizeof(data));
    return new ResourceFile(bytes,static_cast<std::int32_t>(size),bytes);
}
int main() {
    IDirectSoundBufferVtbl bufferVtable{};
    bufferVtable.QueryInterface=queryInterface;bufferVtable.Release=releaseBuffer;bufferVtable.GetStatus=status;
    bufferVtable.GetCurrentPosition=position;bufferVtable.SetCurrentPosition=setPosition;
    bufferVtable.SetFrequency=frequency;bufferVtable.SetVolume=volume;bufferVtable.SetPan=pan;
    bufferVtable.Stop=stop;bufferVtable.Play=play;bufferVtable.Restore=restore;bufferVtable.Lock=lock;bufferVtable.Unlock=unlock;
    deviceBuffer.lpVtbl=&bufferVtable;
    IDirectSound3DBufferVtbl spatialVtable{};spatialVtable.Release=releaseSpatial;spatialVtable.SetPosition=setSpatialPosition;
    spatialBuffer.lpVtbl=&spatialVtable;
    IDirectSoundNotifyVtbl notifyVtable{};notifyVtable.Release=releaseNotify;notifyVtable.SetNotificationPositions=setNotifications;
    notify.lpVtbl=&notifyVtable;
    IDirectSound3DListenerVtbl listenerVtable{};listenerVtable.Release=releaseListener;
    listenerVtable.SetPosition=listenerPosition;listenerVtable.SetVelocity=listenerPosition;
    listenerVtable.SetOrientation=listenerOrientation;listenerVtable.CommitDeferredSettings=listenerCommit;
    listenerVtable.SetDistanceFactor=listenerDistance;deviceListener.lpVtbl=&listenerVtable;
    IDirectSoundVtbl deviceVtable{};deviceVtable.CreateSoundBuffer=createSoundBuffer;deviceVtable.Release=releaseDevice;
    IDirectSound device{&deviceVtable};
    auto* context=static_cast<Context*>(std::calloc(1,sizeof(Context)));
    context->errorState=createErrorState();context->sourceHandles=createHandleList();context->bufferHandles=createHandleList();
    context->device=&device;
    context->primaryBuffer=&deviceBuffer;context->listener=createListener(context);
    assert(context->listener && context->listener->orientationRight[0]==-1 && context->listener->distanceScale==1);
    listener3f(context,Position,1,2,3);assert(spatialPosition.x==1 && spatialPosition.y==3 && spatialPosition.z==2);
    const float listenerAxes[6]={1,2,3,4,5,6};listenerfv(context,Direction,listenerAxes);
    assert(orientation[0]==4 && orientation[1]==6 && orientation[2]==5 && orientation[3]==1 && orientation[4]==3 && orientation[5]==2);
    listenerf(context,Gain,0.25f);assert(mockVolume==-1204);
    listenerf(context,CommitDeferred,0);assert(commits==1);
    distanceScale(context,1.5f);assert(distanceFactor==1.5f);
    int bits=99,channels=99;
    assert(!splitFormat(7,&bits,&channels) && bits==99 && channels==99);
    assert(splitFormat(Stereo16,&bits,&channels) && bits==16 && channels==2);
    assert(gainToAttenuation(1)==0 && gainToAttenuation(0)==-10000 && gainToAttenuation(0.5)==-602);
    assert(panToUnits(0.5)==602 && panToUnits(-0.5)==-602);
    Handle buffers[2],sources[2];
    assert(genBuffers(context,2,buffers)==2 && context->buffersGenerated==2);
    assert(genSources(context,2,sources)==2 && isBuffer(context,buffers[0]));
    unsigned char pcm[400];for (unsigned i=0;i<sizeof(pcm);++i) pcm[i]=static_cast<unsigned char>(i);
    bufferData(context,buffers[0],Stereo16,pcm,sizeof(pcm),100);
    auto* source=contextSource(context,sources[0]);
    sourcei(context,sources[0],BufferBinding,buffers[0]);sourcei(context,sources[0],SourceRelative,1);
    source->dsBuffer=&deviceBuffer;source->loadedBuffer=buffers[0];
    sourcef(context,sources[0],Pitch,1.5f);assert(mockFrequency==150);
    sourcef(context,sources[0],Pan,0.5f);assert(mockPan==602);
    sourcef(context,sources[0],Gain,-1);sourcef(context,sources[0],Pitch,4);
    assert(getError(context)==InvalidValue && getError(context)==0);
    sourcef(context,sources[0],Pan,2);assert(getError(context)==0);
    source->ds3dBuffer=&spatialBuffer;
    source3f(context,sources[0],Position,1,2,3);
    assert(spatialPosition.x==1 && spatialPosition.y==3 && spatialPosition.z==2);
    mockStatus=DSBSTATUS_PLAYING;
    int value=0;getSourcei(context,sources[0],IsPlaying,&value);assert(value==1);
    source->pitch=1;float remaining=0;
    assert(remainingTime(context,sources[0],RemainingSeconds,&remaining));
    assert(std::fabs(remaining-1e-6f)<1e-12f); // Saved scale-store fallthrough.
    getSourcei(context,sources[0],SourceRelative,&value);assert(value==0); // Saved relative-query fallthrough.
    mockStatus=0;assert(reuseOwnBuffer(context,source) && context->ownBufferReuses==1);
    auto* second=contextSource(context,sources[1]);sourcei(context,sources[1],BufferBinding,buffers[0]);sourcei(context,sources[1],SourceRelative,1);
    assert(recycleIdleBuffer(context,second) && context->idleBufferRecycles==1 && second->dsBuffer==&deviceBuffer);
    failFirstCreate=true;createCalls=0;
    assert(createDSBuffer(context,second));
    assert(createCalls==2 && lastDescription.dwSize==sizeof(DSBUFFERDESC));
    assert(lastDescription.guid3DAlgorithm.Data1==0xc241333f && std::memcmp(memory,pcm,sizeof(pcm))==0);
    assert(getError(context)==InvalidOperation && context->dsBuffersCreated==1);
    sourcePlay(context,sources[1]);assert(mockStatus&DSBSTATUS_PLAYING);
    sourceStop(context,sources[1]);assert(stopCalls && mockPosition==0);
    int format;void* data=nullptr;std::uint32_t bytes,rate;
    auto* waveFile=makeWaveFile();wav::loadData(waveFile,&format,&data,&bytes,&rate);delete waveFile;
    assert(format==Mono8 && bytes==8 && rate==22050 && static_cast<unsigned char*>(data)[7]==8);
    for (unsigned i=0;i<31;++i) assert(static_cast<unsigned char*>(data)[8+i]==0);
    std::free(data);
    bufferStream(context,buffers[1],makeWaveFile());
    auto* stream=contextBuffer(context,buffers[1]);assert(stream->streaming && stream->dataBytes==stream->segmentBytes*16);
    sourcei(context,sources[1],BufferBinding,buffers[1]);second->streaming=1;second->relative=1;
    updateSource(context,second);assert(notifications==1 && notificationPositions[15].dwOffset==stream->dataBytes-1);
    stream->dataBytes=16;stream->segmentBytes=8;context->primaryBuffer=&deviceBuffer;mockStatus=0;
    assert(fillBuffer(context,0));assert(memory[0]==1 && memory[7]==8 && memory[8]==0x80);
    assert(fillBuffer(context,1));assert(memory[8]==1 && memory[15]==8);
    mockPlayCursor=8;assert(streamNotification(context) && stream->playedBytes==8 && stream->writeCursor==8);
    mockPlayCursor=0;assert(streamNotification(context) && stream->playedBytes==16 && stream->writeCursor==0);
    std::uint8_t mono[8]={0x80,0,0,0,0,0,0,0};std::int16_t decodedMono[9];
    assert(wav::decodeMono(decodedMono,mono,sizeof(decodedMono),sizeof(mono)));
    for (auto sample:decodedMono) assert(sample==-128);
    std::uint8_t stereo[16]={0x80,0,0,0,0x7f,0,0,0};std::int16_t decodedStereo[18];
    assert(wav::decodeStereo(decodedStereo,stereo,sizeof(decodedStereo),sizeof(stereo)));
    for (unsigned i=0;i<18;i+=2) { assert(decodedStereo[i]==-128);assert(decodedStereo[i+1]==127); }
    destroyContext(context);
    return 0;
}
