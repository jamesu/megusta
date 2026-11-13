// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
namespace megusta { class ResourceFile; }
namespace megusta { namespace audio {
using Handle=std::uint32_t;
using Diagnostic=void (*)(const char*);
// Historical executable tokens; several differ from contemporary OpenAL.
enum Token : std::int32_t {
    SourceRelative=0x202, RemainingSeconds=0x206, RemainingMilliseconds=0x207,
    RemainingMicroseconds=0x208, CommitDeferred=0x209, IsPlaying=0x210,
    ConeInnerAngle=0x1001,ConeOuterAngle=0x1002,ConeOuterGain=0x1003,
    Pitch=0x1004,Position=0x1005,Direction=0x1006,Orientation=0x1006,Velocity=0x1007,
    Looping=0x1008,Streaming=0x1009,BufferBinding=0x100a,Gain=0x100b,
    Pan=0x1010,MinDistance=0x1012,MaxDistance=0x1013,
    Mono8=0x1100,Mono16=0x1101,Stereo8=0x1102,Stereo16=0x1103,
};
enum Error : std::int32_t { NoError=0,InvalidValue=0xa001,InvalidOperation=0xa002,OutOfMemory=0xa003 };
// Backend boundary. No DirectSound, Windows or backend object types cross it.
// A modern OpenAL backend can translate the historical Token values here.
// Counts return the number actually allocated, matching this game's ABI.
struct OpenALApi {
    void (*initialize)(int requestedBits);
    void (*shutdown)();
    void (*setDiagnostic)(Diagnostic);
    void (*distanceScale)(float);
    void (*listenerf)(int,float);
    void (*listener3f)(int,float,float,float);
    void (*listenerfv)(int,const float*);
    std::uint32_t (*genSources)(std::uint32_t,Handle*);
    void (*deleteSources)(std::uint32_t,const Handle*);
    void (*sourcei)(Handle,int,int);
    void (*sourcef)(Handle,int,float);
    void (*source3f)(Handle,int,float,float,float);
    void (*getSourcei)(Handle,int,int*);
    void (*sourcePlay)(Handle);
    void (*sourceStop)(Handle);
    std::uint32_t (*genBuffers)(std::uint32_t,Handle*);
    void (*deleteBuffers)(std::uint32_t,const Handle*);
    bool (*isBuffer)(Handle);
    void (*bufferData)(Handle,int,const void*,std::uint32_t,std::uint32_t);
    // loadWav borrows the mounted stream for this call. bufferStream takes
    // ownership of it, including when the backend reports an error.
    void (*bufferStream)(Handle,megusta::ResourceFile*);
    int (*getError)();
    void (*loadWav)(megusta::ResourceFile*,int*,void**,std::uint32_t*,std::uint32_t*);
    void (*unloadWav)(void*);
};
const OpenALApi& directSoundApi();
// Install before initialization, or after shutdown. The table must outlive use.
void bindApi(const OpenALApi&);
const OpenALApi& api();
extern bool enabled;
extern int requestedBits;
void initialize(int* argc,void* argvOrDiagnostic);
void initializeBridge(int* argc,void* argvOrDiagnostic);
void shutdown();
void distanceScale(float);
void listenerf(int,float);
void listener3f(int,float,float,float);
void listenerfv(int,const float*);
std::uint32_t genSources(std::uint32_t,Handle*);
void deleteSources(std::uint32_t,const Handle*);
void sourcei(Handle,int,int);
void sourcef(Handle,int,float);
void source3f(Handle,int,float,float,float);
void sourcePlay(Handle);
std::uint8_t sourceIsPlaying(Handle);
void sourceStop(Handle);
std::uint32_t genBuffers(std::uint32_t,Handle*);
void deleteBuffers(std::uint32_t,const Handle*);
void bufferData(Handle,int,const void*,std::uint32_t,std::uint32_t);
void bufferStream(Handle,const char*);
int getError();
void loadWav(const char*,int*,void**,std::uint32_t*,std::uint32_t*);
void unloadWav(std::uint32_t format,void*);
} }
