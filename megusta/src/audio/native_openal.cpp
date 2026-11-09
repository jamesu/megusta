#include "openal.h"
#include "wave.h"
#include "engine/resource_file.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <cstdlib>
namespace megusta::audio {
namespace {
ALCdevice* device=nullptr;
ALCcontext* context=nullptr;
float scale=1;
int token(int value) {
    switch(value) {
    case Pitch:return AL_PITCH;
    case Gain:return AL_GAIN;
    case Position:return AL_POSITION;
    case Direction:return AL_DIRECTION;
    case Velocity:return AL_VELOCITY;
    case SourceRelative:return AL_SOURCE_RELATIVE;
    case ConeOuterGain:return AL_CONE_OUTER_GAIN;
    case Looping:return AL_LOOPING;
    case BufferBinding:return AL_BUFFER;
    case MinDistance:return AL_REFERENCE_DISTANCE;
    case MaxDistance:return AL_MAX_DISTANCE;
    default:return value;
    }
}
void initialize(int) {
    device=alcOpenDevice(nullptr);
    if(device) context=alcCreateContext(device,nullptr);
    if(context) { alcMakeContextCurrent(context); alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED); }
}
void shutdownBackend() {
    alcMakeContextCurrent(nullptr);
    if(context)alcDestroyContext(context);
    if(device)alcCloseDevice(device);
    context=nullptr;device=nullptr;
}
void sourceVector(Handle source,int name,float x,float y,float z) {
    const float factor=name==Position || name==Velocity?scale:1;
    alSource3f(source,token(name),x*factor,z*factor,y*factor);
}
void sourceInteger(Handle source,int name,int value) {
    if(name==Streaming || name==CommitDeferred)return;
    alSourcei(source,token(name),value);
}
void sourceFloat(Handle source,int name,float value) {
    if(name==Pan) { alSourcei(source,AL_SOURCE_RELATIVE,AL_TRUE);alSource3f(source,AL_POSITION,value,0,-1);return; }
    alSourcef(source,token(name),(name==MinDistance || name==MaxDistance)?value*scale:value);
}
void sourceGet(Handle source,int name,int* value) {
    if(name==IsPlaying) { ALint state=0;alGetSourcei(source,AL_SOURCE_STATE,&state);*value=state==AL_PLAYING; }
    else alGetSourcei(source,token(name),value);
}
void stream(Handle buffer,ResourceFile* file) {
    if(!file)return;
    int format=0;void* data=nullptr;std::uint32_t bytes=0,rate=0;
    wave::loadData(file,&format,&data,&bytes,&rate);
    delete file;
    if(data) { alBufferData(buffer,format,data,bytes,rate);std::free(data); }
}
}
const OpenALApi& nativeOpenALApi() {
    static const OpenALApi table{
        initialize,shutdownBackend,[](Diagnostic){},[](float value){scale=value;},
        [](int name,float value){if(name!=CommitDeferred)alListenerf(token(name),value);},
        [](int name,float x,float y,float z){alListener3f(token(name),x*scale,z*scale,y*scale);},
        [](int name,const float* v){
            if(name==Orientation) { const float orientation[]={v[3],v[5],v[4],v[0],v[2],v[1]};alListenerfv(AL_ORIENTATION,orientation); }
            else alListener3f(token(name),v[0]*scale,v[2]*scale,v[1]*scale);
        },
        [](std::uint32_t n,Handle* h){alGenSources(n,h);return alGetError()==AL_NO_ERROR?n:0u;},
        [](std::uint32_t n,const Handle* h){alDeleteSources(n,h);},
        sourceInteger,sourceFloat,sourceVector,sourceGet,
        [](Handle h){alSourcePlay(h);},[](Handle h){alSourceStop(h);},
        [](std::uint32_t n,Handle* h){alGenBuffers(n,h);return alGetError()==AL_NO_ERROR?n:0u;},
        [](std::uint32_t n,const Handle* h){alDeleteBuffers(n,h);},
        [](Handle h){return alIsBuffer(h)!=AL_FALSE;},
        [](Handle h,int f,const void* p,std::uint32_t n,std::uint32_t r){alBufferData(h,f,p,n,r);},
        stream,[](){return static_cast<int>(alGetError());},wave::loadData,[](void* p){std::free(p);}
    };
    return table;
}
}
