// SPDX-License-Identifier: GPL-2.0

#pragma once
#include "game_config_runtime.h"
#include "audio/openal.h"
namespace megusta {
class Sounds;
enum class SoundKind : int { Sample,Music,Voice,Announcer };
enum SoundFlags : unsigned { AutoDelete=1,Loop=2,QueuedVoice=4,Spatial=8,CachedBuffer=16,Streamed=32 };
struct AudioPreloadEntry {
    char filename[256];
    int group;
    audio::Handle bufferHandle;
    AudioPreloadEntry* next;
};
class AudioSource {
public:
    AudioSource(Sounds&,const char*,SoundKind,int key);
    ~AudioSource();
    AudioSource* next;
    int instanceKey;
    char name[256];
    audio::Handle sourceHandle,ownedBufferHandle;
    unsigned flags;
    SoundKind kind;
    float gain,position[3];
    std::uint32_t voiceDelayOrDeadlineMs;
    bool isPlaying() const;
    bool loadSample();
    void releasePlayback();
    void applyGain();
    bool playSample(); bool playMusicStream(); bool playVoice(); bool playAnnouncer();
    bool playByKind(); bool enqueueVoice(std::uint32_t);
    void stop();
private:
    Sounds& owner;
    bool playLoadedSample();
    bool playStreamPath(const char*);
};
class Sounds {
public:
    Sounds(AudioCategoryGains& gains,std::uint32_t (*clock)()):gains(gains),clock(clock) {}
    AudioCategoryGains& gains;
    std::uint32_t (*clock)();
    bool playbackAllowed=true; // Original gating byte; consumer tests nonzero.
    AudioSource* activeSources=nullptr;
    AudioPreloadEntry* preloadEntries=nullptr;
    AudioSource* currentVoice=nullptr;
    AudioSource* voiceQueue[4]{};
    int voiceQueueCount=0;
    void updateVoiceQueue(); void releaseTransientPlayback(); void unloadGroup(int);
    void stopNonStreaming(); void collectFinishedSources(); void refreshPlayingGains();
    AudioSource* findSource(const char*,int);
    bool play(const char*,int loop); bool play3d(const char*,float,float,float,int loop,int key);
    bool setPos(const char*,float,float,float,int key);
    bool playVoice(const char*,std::uint32_t delay); bool playAnnouncer(const char*);
    bool playStream(const char*,int loop); bool stop(const char*,int key); bool isPlaying(const char*,int key);
    bool preloadAny(const char*,int group); bool preload(const char*,int group); bool preloadVoice(const char*,int group);
    void preloadCommon(const char* originalFirstName,const char* originalSeventhName);
    void updateListener(bool sceneActive,const float* eye,const float* lookAt);
};
}
