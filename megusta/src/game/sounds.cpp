#include "sounds.h"
#include "engine/string_services.h"
#include "engine/runtime_adapters.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
namespace megusta {
AudioSource::AudioSource(Sounds& owner,const char* text,SoundKind kind,int key):
    next(owner.activeSources),instanceKey(key),sourceHandle(0),ownedBufferHandle(0),flags(0),kind(kind),gain(1),owner(owner) {
    std::strcpy(name,text); owner.activeSources=this;
}
AudioSource::~AudioSource() {
    releasePlayback();
    // Compatibility lifetime correction: original stop only visits the queue
    // while playing. Destruction must not leave an unplayed queued pointer.
    if (owner.currentVoice==this) owner.currentVoice=nullptr;
    int count=0;
    for (int i=0;i<owner.voiceQueueCount;++i) if (owner.voiceQueue[i]!=this) owner.voiceQueue[count++]=owner.voiceQueue[i];
    owner.voiceQueueCount=count;
    AudioSource** link=&owner.activeSources;
    while (*link && *link!=this) link=&(*link)->next;
    if (*link) { *link=next; next=nullptr; }
}
bool AudioSource::isPlaying() const { return sourceHandle && audio::sourceIsPlaying(sourceHandle); }
bool AudioSource::loadSample() {
    if (sourceHandle) return true;
    const char* format=kind==SoundKind::Sample?"data/sounds/%s.wav":kind==SoundKind::Announcer?"data/announce/%s.wav":"data/voices/%s.wav";
    char path[128]; std::sprintf(path,format,name);
    flags&=~CachedBuffer;
    for (auto* p=owner.preloadEntries;p;p=p->next) {
        if (!strings::compareInsensitive(path,p->filename) && audio::genSources(1,&sourceHandle)) {
            audio::sourcei(sourceHandle,audio::BufferBinding,p->bufferHandle); flags|=CachedBuffer; return true;
        }
    }
    void* data=nullptr; int formatToken; std::uint32_t bytes,rate;
    audio::loadWav(path,&formatToken,&data,&bytes,&rate);
    if (data) {
        if (audio::genSources(1,&sourceHandle) && audio::genBuffers(1,&ownedBufferHandle)) {
            audio::bufferData(ownedBufferHandle,formatToken,data,bytes,rate);
            audio::unloadWav(formatToken,data);
            audio::sourcei(sourceHandle,audio::BufferBinding,ownedBufferHandle); return true;
        }
        audio::unloadWav(formatToken,data);
    }
    return false;
}
void AudioSource::releasePlayback() {
    stop();
    if (sourceHandle) { audio::deleteSources(1,&sourceHandle); sourceHandle=0; }
    if (ownedBufferHandle) { if (!(flags&CachedBuffer)) audio::deleteBuffers(1,&ownedBufferHandle); ownedBufferHandle=0; }
}
void AudioSource::applyGain() {
    if (!sourceHandle) return;
    float category=kind==SoundKind::Sample?owner.gains.sound:kind==SoundKind::Music?owner.gains.music:
        kind==SoundKind::Voice?owner.gains.character:owner.gains.announcer;
    float value=owner.gains.master*gain*category;
    if (value<1) { if (value<=0) value=0; } else value=1;
    audio::sourcef(sourceHandle,audio::Gain,value);
}
bool AudioSource::playLoadedSample() {
    if (isPlaying()) return false;
    audio::sourcef(sourceHandle,audio::Pitch,1); applyGain();
    audio::sourcei(sourceHandle,audio::Looping,(flags&Loop)!=0);
    if (flags&Spatial) {
        audio::sourcef(sourceHandle,audio::MinDistance,100);
        audio::sourcei(sourceHandle,audio::SourceRelative,0);
        audio::source3f(sourceHandle,audio::Position,position[0],position[1],position[2]);
    } else { audio::sourcei(sourceHandle,audio::SourceRelative,1); audio::source3f(sourceHandle,audio::Position,0,0,0); }
    audio::listenerf(audio::CommitDeferred,0); audio::sourcePlay(sourceHandle); return true;
}
bool AudioSource::playSample() { return loadSample() && playLoadedSample(); }
bool AudioSource::playStreamPath(const char* path) {
    releasePlayback();
    if (audio::genSources(1,&sourceHandle) && audio::genBuffers(1,&ownedBufferHandle)) {
        audio::bufferStream(ownedBufferHandle,path);
        if (!audio::getError()) {
            flags|=Streamed;
            audio::sourcei(sourceHandle,audio::SourceRelative,1);
            audio::sourcei(sourceHandle,audio::BufferBinding,ownedBufferHandle);
            audio::sourcei(sourceHandle,audio::Looping,(flags&Loop)!=0);
            audio::sourcef(sourceHandle,audio::Pitch,1); applyGain(); audio::sourcePlay(sourceHandle); return true;
        }
    }
    releasePlayback(); return false;
}
bool AudioSource::playMusicStream() { char path[128]; std::sprintf(path,"music/%s.wav",name); return playStreamPath(path); }
bool AudioSource::playVoice() {
    if (loadSample()) return playLoadedSample();
    char path[128]; std::sprintf(path,"data/voices/%s.wav",name); return playStreamPath(path);
}
bool AudioSource::playAnnouncer() {
    if (loadSample()) return playLoadedSample();
    char path[128]; std::sprintf(path,"data/announce/%s.wav",name); return playStreamPath(path);
}
bool AudioSource::playByKind() {
    switch (kind) { case SoundKind::Sample:return playSample(); case SoundKind::Music:return playMusicStream();
        case SoundKind::Voice:return playVoice(); case SoundKind::Announcer:return playAnnouncer(); }
    return false;
}
bool AudioSource::enqueueVoice(std::uint32_t delay) {
    if ((!loadSample() && kind!=SoundKind::Voice) || isPlaying()) return false;
    if (owner.voiceQueueCount>3) {
        // Compatibility: the original leaves this dropped source marked as
        // queued forever, preventing its automatic collection.
        --owner.voiceQueueCount;
        for (int i=0;i<owner.voiceQueueCount;++i) owner.voiceQueue[i]=owner.voiceQueue[i+1];
    }
    voiceDelayOrDeadlineMs=delay; flags|=QueuedVoice;
    owner.voiceQueue[owner.voiceQueueCount++]=this; owner.updateVoiceQueue(); return true;
}
void AudioSource::stop() {
    if (!isPlaying()) return; // Preserve the original outer playing guard.
    if (flags&QueuedVoice) {
        if (owner.currentVoice==this) owner.currentVoice=nullptr;
        else {
            int count=0;
            for (int i=0;i<owner.voiceQueueCount;++i) if (owner.voiceQueue[i]!=this) owner.voiceQueue[count++]=owner.voiceQueue[i];
            owner.voiceQueueCount=count;
        }
        flags&=~QueuedVoice;
    }
    if (isPlaying()) audio::sourceStop(sourceHandle);
}
void Sounds::updateVoiceQueue() {
    if (currentVoice) {
        if (currentVoice->isPlaying() || currentVoice->voiceDelayOrDeadlineMs>clock()) return;
        currentVoice->flags&=~QueuedVoice; currentVoice=nullptr;
    }
    if (voiceQueueCount>0) {
        currentVoice=voiceQueue[0]; currentVoice->voiceDelayOrDeadlineMs+=clock(); currentVoice->playByKind();
        --voiceQueueCount; for (int i=0;i<voiceQueueCount;++i) voiceQueue[i]=voiceQueue[i+1];
    }
}
void Sounds::releaseTransientPlayback() {
    for (auto* p=activeSources;p;p=p->next) if (!(p->flags&(CachedBuffer|Streamed))) p->releasePlayback();
    voiceQueueCount=0; currentVoice=nullptr; collectFinishedSources();
}
void Sounds::stopNonStreaming() {
    for (auto* p=activeSources;p;p=p->next) if (p->isPlaying() && !(p->flags&Streamed)) p->stop();
    voiceQueueCount=0; currentVoice=nullptr; collectFinishedSources();
}
void Sounds::collectFinishedSources() {
    auto* p=activeSources;
    while (p) {
        if (!p->isPlaying() && !(p->flags&QueuedVoice)) {
            p->releasePlayback();
            if (p->flags&AutoDelete) { delete p; p=activeSources; continue; }
        }
        p=p->next;
    }
    updateVoiceQueue();
}
void Sounds::refreshPlayingGains() { for (auto* p=activeSources;p;p=p->next) if (p->isPlaying()) p->applyGain(); }
AudioSource* Sounds::findSource(const char* name,int key) {
    for (auto* p=activeSources;p;p=p->next) if (!std::strcmp(name,p->name) && p->instanceKey==key) return p;
    return nullptr;
}
bool Sounds::play(const char* name,int loop) {
    if (!playbackAllowed) return false;
    auto* p=new AudioSource(*this,name,SoundKind::Sample,0); p->flags|=AutoDelete|(loop?Loop:0); return p->playByKind();
}
bool Sounds::play3d(const char* name,float x,float y,float z,int loop,int key) {
    if (!playbackAllowed) return false;
    auto* p=new AudioSource(*this,name,SoundKind::Sample,key); p->flags|=AutoDelete|Spatial|(loop?Loop:0);
    p->position[0]=x; p->position[1]=y; p->position[2]=z; return p->playByKind();
}
bool Sounds::setPos(const char* name,float x,float y,float z,int key) {
    if (auto* p=findSource(name,key)) { p->flags|=Spatial; p->position[0]=x; p->position[1]=y; p->position[2]=z; }
    return true; // Saved body does not immediately submit new coordinates.
}
bool Sounds::playVoice(const char* name,std::uint32_t delay) {
    if (!playbackAllowed) return false;
    auto* p=new AudioSource(*this,name,SoundKind::Voice,0); p->flags|=AutoDelete; return p->enqueueVoice(delay);
}
bool Sounds::playAnnouncer(const char* name) {
    if (!playbackAllowed) return false;
    auto* p=new AudioSource(*this,name,SoundKind::Announcer,0); p->flags|=AutoDelete; return p->playByKind();
}
bool Sounds::playStream(const char* name,int loop) {
    if (!playbackAllowed) return false;
    for (auto* p=activeSources;p;p=p->next) if (p->flags&Streamed) {
        if (!std::strcmp(name,p->name)) return false;
        p->releasePlayback();
    }
    collectFinishedSources();
    if (!name || !*name) return false;
    auto* p=new AudioSource(*this,name,SoundKind::Music,0); p->flags|=AutoDelete|(loop?Loop:0); return p->playByKind();
}
bool Sounds::stop(const char* name,int key) { auto* p=findSource(name,key); if (!p) return false; delete p; return true; }
bool Sounds::isPlaying(const char* name,int key) { auto* p=findSource(name,key); return p && p->isPlaying(); }
bool Sounds::preloadAny(const char* path,int group) {
    for (auto* p=preloadEntries;p;p=p->next) if (!strings::compareInsensitive(path,p->filename)) {
        std::fprintf(stderr,"AudioSource::preload: <%s> attempted twice\n",path); return false;
    }
    audio::Handle buffer=0;
    if (audio::genBuffers(1,&buffer)) {
        void* data=nullptr; int format; std::uint32_t bytes,rate;
        audio::loadWav(path,&format,&data,&bytes,&rate);
        if (data) {
            audio::bufferData(buffer,format,data,bytes,rate);
            if (audio::getError()) {
                audio::unloadWav(format,data); audio::deleteBuffers(1,&buffer);
                std::fprintf(stderr,"AudioSource::preloadAny: Out of audio memory\n"); return false;
            }
            audio::unloadWav(format,data);
            auto* entry=new AudioPreloadEntry; std::strcpy(entry->filename,path);
            entry->bufferHandle=buffer; entry->group=group; entry->next=preloadEntries; preloadEntries=entry; return true;
        }
    }
    if (buffer) audio::deleteBuffers(1,&buffer);
    return false;
}
bool Sounds::preload(const char* name,int group) { char path[128]; std::sprintf(path,"data/sounds/%s.wav",name); return preloadAny(path,group); }
bool Sounds::preloadVoice(const char* name,int group) { char path[128]; std::sprintf(path,"data/voices/%s.wav",name); return preloadAny(path,group); }
void Sounds::unloadGroup(int group) {
    auto** link=&preloadEntries;
    while (*link) {
        auto* entry=*link;
        if (group>=0 && entry->group!=group) { link=&entry->next; continue; }
        while (auto* p=findSource(entry->filename,0)) {
            if (p->isPlaying()) {
                if (p->flags&Streamed) engine_runtime::engineRuntime_fatalExit_impl();
                p->releasePlayback();
            }
        }
        *link=entry->next;
        if (entry->bufferHandle) audio::deleteBuffers(1,&entry->bufferHandle);
        delete entry;
    }
    collectFinishedSources();
}
void Sounds::preloadCommon(const char* first,const char* seventh) {
    preload(first,1); preload("hitball",1); preload("incup",1); preload("balldrop",1);
    preload("ooww1",1); preload("ooww2",1); preload(seventh,1);
    preload("button",1); preload("button2",1); preload("button3",1);
}
void Sounds::updateListener(bool active,const float* eye,const float* lookAt) {
    if (active) {
        float orientation[6]={0,0,1,static_cast<float>(lookAt[0]-eye[0]),
            static_cast<float>(lookAt[1]-eye[1]),static_cast<float>(lookAt[2]-eye[2])};
        audio::listenerfv(audio::Orientation,orientation);
        audio::listener3f(audio::Position,eye[0],eye[1],eye[2]);
        audio::listenerf(audio::CommitDeferred,0);
    } else audio::listener3f(audio::Position,0,0,0);
    collectFinishedSources();
}
}
