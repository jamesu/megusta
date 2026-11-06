#include "directsound_internal.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
namespace megusta { namespace audio { namespace ds {
int getError(Context* context) {
    if (!context) return InvalidOperation;
    auto* state=contextErrorState(context);
    if (!state) return -1;
    const int result=state->stickyError;state->stickyError=0;return result;
}
std::uint32_t genBuffers(Context* context,std::uint32_t count,Handle* handles) {
    if (!handles) { setError(context,InvalidOperation,"alGenBuffers");return 0; }
    std::uint32_t generated=0;
    while (generated<count) {
        auto* node=insertHandle(context->bufferHandles);
        handles[generated]=handleValue(node);
        auto* buffer=createBuffer();
        if (!node || !buffer) {
            destroyBuffer(buffer);removeHandle(context->bufferHandles,node);
            setError(context,OutOfMemory,"alGenBuffers");break;
        }
        setHandlePayload(context->bufferHandles,node,buffer);++generated;
    }
    context->buffersGenerated+=generated;return generated;
}
void deleteBuffers(Context* context,std::uint32_t count,const Handle* handles) {
    if (!handles) { setError(context,InvalidOperation,"alDeleteBuffers");return; }
    for (std::uint32_t i=0;i<count;++i) {
        auto* buffer=contextBuffer(context,handles[i]);
        if (!buffer) setError(context,InvalidValue,"alDeleteBuffers");
        else { destroyBuffer(buffer);removeHandle(context->bufferHandles,findHandle(context->bufferHandles,handles[i])); }
    }
}
bool isBuffer(Context* context,Handle handle) {
    return containsHandle(context->bufferHandles,handle);
}
std::uint32_t genSources(Context* context,std::uint32_t count,Handle* handles) {
    if (!handles) { setError(context,InvalidOperation,"alGenSources");return 0; }
    std::uint32_t generated=0;
    while (generated<count) {
        auto* node=insertHandle(context->sourceHandles);
        handles[generated]=handleValue(node);
        auto* source=createSource();
        if (!node || !source) {
            destroySource(source);removeHandle(context->sourceHandles,node);
            setError(context,OutOfMemory,"alGenSources");break;
        }
        setHandlePayload(context->sourceHandles,node,source);++generated;
    }
    return generated;
}
void deleteSources(Context* context,std::uint32_t count,const Handle* handles) {
    if (!handles) { setError(context,InvalidOperation,"alDeleteSources");return; }
    for (std::uint32_t i=0;i<count;++i) {
        auto* source=contextSource(context,handles[i]);
        if (!source) setError(context,InvalidValue,"alDeleteSources");
        else { destroySource(source);removeHandle(context->sourceHandles,findHandle(context->sourceHandles,handles[i])); }
    }
}
void listenerf(Context* context,int token,float value) {
    auto* listener=contextListener(context);
    if (!listener) { setError(context,InvalidOperation,"alListenerf");return; }
    if (token==CommitDeferred) listener->dsListener->CommitDeferredSettings();
    else if (token!=Gain) setError(context,InvalidValue,"alListenerf");
    else if (value>=0) { listener->gain=value;context->primaryBuffer->SetVolume(gainToAttenuation(value)); }
    else setError(context,InvalidValue,"alListenerf(AL_GAIN)");
    listener->dirty=1;
}
void listener3f(Context* context,int token,float x,float y,float z) {
    auto* listener=contextListener(context);
    if (!listener) { setError(context,InvalidOperation,"alListener3f");return; }
    if (token==Position || token==Velocity) {
        float* destination=token==Position ? listener->position : listener->velocity;
        destination[2]=z;destination[0]=x;destination[1]=y;
        if (token==Position) listener->dsListener->SetPosition(x,z,y,DS3D_DEFERRED);
        else listener->dsListener->SetVelocity(x,z,y,DS3D_DEFERRED);
    } else setError(context,InvalidValue,"alListener3f");
    listener->dirty=1;
}
void listenerfv(Context* context,int token,const float* values) {
    auto* listener=contextListener(context);
    if (!listener || !values) { setError(context,InvalidOperation,"alListenerfv");return; }
    if (token==Position || token==Velocity) {
        float* destination=token==Position ? listener->position : listener->velocity;
        destination[0]=values[0];destination[1]=values[1];destination[2]=values[2];
        if (token==Position) listener->dsListener->SetPosition(values[0],values[2],values[1],DS3D_DEFERRED);
        else listener->dsListener->SetVelocity(values[0],values[2],values[1],DS3D_DEFERRED);
    } else if (token==Orientation) {
        for (int i=0;i<3;++i) listener->orientationFront[i]=values[i];
        for (int i=0;i<3;++i) listener->orientationUp[i]=values[i+3];
        crossProduct(values,values+3,listener->orientationRight);
        listener->dsListener->SetOrientation(values[3],values[5],values[4],values[0],values[2],values[1],DS3D_DEFERRED);
    } else setError(context,InvalidValue,"alListenerfv");
    listener->dirty=1;
}
void distanceScale(Context* context,float scale) {
    auto* listener=contextListener(context);
    if (listener && scale!=0) {
        listener->distanceScale=scale;listener->dsListener->SetDistanceFactor(scale,DS3D_IMMEDIATE);
        listener->dirty=1;
    } else setError(context,InvalidOperation,"alimDistanceScale");
}
void sourcef(Context* context,Handle handle,int token,float value) {
    auto* source=contextSource(context,handle);
    if (!source) { setError(context,InvalidOperation,"alSourcef");return; }
    const char* error=nullptr;
    switch (token) {
    case ConeOuterGain:
        if (value>=0) {
            source->coneOuterGain=value;
            if (source->ds3dBuffer) source->ds3dBuffer->SetConeOutsideVolume(gainToAttenuation(value),DS3D_DEFERRED);
        } else error="alSourcef(AL_CONE_OUTER_GAIN)";
        break;
    case Pitch:
        if (value>=0.5f && value<=2.0f) {
            source->pitch=value;
            auto* buffer=contextBuffer(context,source->boundBuffer);
            if (buffer && source->dsBuffer) {
                auto frequency=static_cast<std::uint32_t>(truncateX87(static_cast<long double>(buffer->frequency)*source->pitch));
                if (frequency>100000) frequency=100000;else if (frequency<100) frequency=100;
                source->dsBuffer->SetFrequency(frequency);
            }
        } else error="alSourcef(AL_PITCH)";
        break;
    case Gain:
        if (value>=0) { source->gain=value;if (source->dsBuffer) source->dsBuffer->SetVolume(gainToAttenuation(value)); }
        else error="alSourcef(AL_GAIN)";
        break;
    case Pan:
        if (value>=-1 && value<=1) { source->pan=value;if (source->dsBuffer) source->dsBuffer->SetPan(panToUnits(value)); }
        break; // Invalid pan still marks dirty but does not report an error.
    case MinDistance:
        if (value>=1 && value<1e9f) {
            source->minDistance=value;if (source->ds3dBuffer) source->ds3dBuffer->SetMinDistance(value,DS3D_DEFERRED);
        } else error="alSourcef(AL_MIN_DISTANCE)";
        break;
    case MaxDistance:
        if (value>1 && value<=1e9f) {
            source->maxDistance=value;if (source->ds3dBuffer) source->ds3dBuffer->SetMaxDistance(value,DS3D_DEFERRED);
        } else error="alSourcef(AL_MAX_DISTANCE)";
        break;
    default:error="alSourcef";break;
    }
    if (error) setError(context,InvalidValue,error);
    source->dirty=1;
}
void source3f(Context* context,Handle handle,int token,float x,float y,float z) {
    auto* source=contextSource(context,handle);
    if (!source) { setError(context,InvalidOperation,"alSource3f");return; }
    float* target=nullptr;
    switch (token) { case Position:target=source->position;break;case Direction:target=source->direction;break;
        case Velocity:target=source->velocity;break;default:break; }
    if (target) {
        target[0]=x;target[1]=y;target[2]=z;
        if (source->ds3dBuffer) {
            if (token==Position) source->ds3dBuffer->SetPosition(x,z,y,DS3D_DEFERRED);
            else if (token==Direction) source->ds3dBuffer->SetConeOrientation(x,z,y,DS3D_DEFERRED);
            else source->ds3dBuffer->SetVelocity(x,z,y,DS3D_DEFERRED);
        }
    } else setError(context,InvalidValue,"alSource3f");
    source->dirty=1;
}
void sourcei(Context* context,Handle handle,int token,int value) {
    auto* source=contextSource(context,handle);
    if (!source) { setError(context,InvalidOperation,"alSourcei");return; }
    const char* error=nullptr;
    switch (token) {
    case Looping:case SourceRelative:case Streaming:
        if (value!=0 && value!=1) error=token==Looping ? "alSourcei(AL_LOOPING)" :
            token==Streaming ? "alSourcei(AL_STREAMING)" : "alSourcei(AL_SOURCE_RELATIVE)";
        else if (token==Looping) source->looping=value;
        else if (token==Streaming) source->streaming=value;
        else source->relative=value;
        break;
    case ConeInnerAngle:case ConeOuterAngle:
        if (value<0 || value>360) error=token==ConeInnerAngle ? "alSourcei(AL_CONE_INNER_ANGLE)" : "alSourcei(AL_CONE_OUTER_ANGLE)";
        else {
            if (token==ConeInnerAngle) source->coneInnerAngle=value;else source->coneOuterAngle=value;
            if (source->ds3dBuffer) source->ds3dBuffer->SetConeAngles(source->coneInnerAngle,source->coneOuterAngle,DS3D_DEFERRED);
        }
        break;
    case BufferBinding:
        if (isBuffer(context,static_cast<Handle>(value))) {
            auto* buffer=contextBuffer(context,value);source->boundBuffer=value;source->streaming=buffer->streaming;
        } else error="alSourcei(AL_BUFFER)";
        break;
    default:error="alSourcei";break;
    }
    if (error) setError(context,InvalidValue,error);
    source->dirty=1;
}
void getSourcei(Context* context,Handle handle,int token,int* output) {
    auto* source=contextSource(context,handle);
    if (!source || !output) { setError(context,InvalidOperation,"alGetSourcei");return; }
    switch (token) {
    case ConeInnerAngle:*output=source->coneInnerAngle;return;
    case ConeOuterAngle:*output=source->coneOuterAngle;return;
    case Looping:*output=static_cast<std::int8_t>(source->looping);return;
    case Streaming:*output=static_cast<std::int8_t>(source->streaming);return;
    case BufferBinding:*output=source->boundBuffer;return;
    case IsPlaying: {
        DWORD status;
        *output=source->dsBuffer && SUCCEEDED(source->dsBuffer->GetStatus(&status)) && (status&DSBSTATUS_PLAYING) ? 1 : 0;
        return;
    }
    case SourceRelative:*output=static_cast<std::int8_t>(source->relative);[[fallthrough]];
    case RemainingSeconds:case RemainingMilliseconds:case RemainingMicroseconds: {
        float remaining;
        if (remainingTime(context,handle,token,&remaining))
            *output=static_cast<std::int32_t>(truncateX87(std::ceil(static_cast<double>(remaining))));
        else { *output=0;setError(context,InvalidValue,"alGetSourcei"); }
        return;
    }
    default:setError(context,InvalidValue,"alGetSourcei");return;
    }
}
void sourcePlay(Context* context,Handle handle) {
    auto* source=contextSource(context,handle);
    if (!source) { setError(context,InvalidOperation,"alSourcePlay::alimContextSource");return; }
    if (!contextBuffer(context,source->boundBuffer)) { setError(context,InvalidOperation,"alSourcePlay::alimContextBuffer");return; }
    if (!reuseOwnBuffer(context,source) && !recycleIdleBuffer(context,source) && !createDSBuffer(context,source)) {
        setError(context,InvalidOperation,"alSourcePlay::!alisCreateDSBuffer");return;
    }
    updateSource(context,source);
    source->dsBuffer->Play(0,0,source->looping==1 ? DSBPLAY_LOOPING : 0);
}
void sourceStop(Context* context,Handle handle) {
    auto* source=contextSource(context,handle);
    if (!source) { setError(context,InvalidOperation,"alSourceStop");return; }
    if (source->dsBuffer) { source->dsBuffer->Stop();source->dsBuffer->SetCurrentPosition(0); }
}
void bufferData(Context* context,Handle handle,int format,const void* data,std::uint32_t size,std::uint32_t rate) {
    auto* buffer=contextBuffer(context,handle);
    if (!buffer || !data) { setError(context,InvalidOperation,"alBufferData");return; }
    auto* storage=std::malloc(size);
    if (!storage) { setError(context,OutOfMemory,"alBufferData");return; }
    if (format<Mono8 || format>Stereo16) {
        // Preserve the saved invalid-format assignment/free order, including
        // its dangling buffer pointer. Normal callers validate their formats.
        buffer->pcmData=storage;std::free(storage);setError(context,InvalidValue,"alBufferData");return;
    }
    if (buffer->pcmData) std::free(buffer->pcmData);
    buffer->format=format;buffer->frequency=rate;
    buffer->bits=(format&1) ? 16 : 8;buffer->channels=(format&2) ? 2 : 1;
    buffer->dataBytes=size;buffer->pcmData=storage;std::memcpy(storage,data,size);
}
} } }
