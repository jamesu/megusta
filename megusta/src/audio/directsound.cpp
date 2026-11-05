#include "directsound_internal.h"
#include <cstring>
#include <utility>
namespace megusta { namespace audio { namespace ds {
bool setCooperativeLevel(Context* context) {
    if (!context->device) return false;
    HWND window=GetActiveWindow();if (!window) window=GetForegroundWindow();if (!window) window=GetDesktopWindow();
    if (!window) return true;
    context->cooperativeLevel=DSSCL_PRIORITY;
    if (FAILED(context->device->SetCooperativeLevel(window,DSSCL_PRIORITY))) {
        if (FAILED(context->device->SetCooperativeLevel(window,DSSCL_NORMAL))) {
            setError(context,InvalidOperation,"alimSetCooperativeLevel");context->cooperativeLevel=0;return false;
        }
        context->cooperativeLevel=DSSCL_NORMAL;
    }
    return true;
}
bool createPrimary(Context* context) {
    if (!context->primaryBuffer) {
        DSBUFFERDESC description{};description.dwSize=sizeof(description);description.dwFlags=0x91;
        if (FAILED(context->device->CreateSoundBuffer(&description,&context->primaryBuffer,nullptr))) {
            setError(context,InvalidValue,"alimCreatePrimary");return false;
        }
    }
    return true;
}
bool splitFormat(int format,int* bits,int* channels) {
    const auto index=static_cast<std::uint32_t>(format)-Mono8;
    if (index>3) return false;
    *bits=(index&1) ? 16 : 8;*channels=(index&2) ? 2 : 1;return true;
}
void makePcmFormat(std::uint32_t rate,std::uint32_t bits,std::uint32_t channels,WAVEFORMATEX* format) {
    std::memset(format,0,sizeof(WAVEFORMATEX));
    format->wBitsPerSample=static_cast<WORD>(bits);format->nChannels=static_cast<WORD>(channels);
    const auto alignment=static_cast<std::uint32_t>(static_cast<WORD>(bits)>>3)*channels;
    format->nBlockAlign=static_cast<WORD>(alignment);format->wFormatTag=WAVE_FORMAT_PCM;
    format->nSamplesPerSec=rate;format->nAvgBytesPerSec=(alignment&0xffffu)*rate;
}
bool setPrimaryFormat(Context* context,std::uint32_t rate,int format) {
    int bits,channels;
    if (context->primaryBuffer && splitFormat(format,&bits,&channels)) {
        if (context->cooperativeLevel!=DSSCL_PRIORITY) bits=8;
        WAVEFORMATEX waveFormat;makePcmFormat(rate,bits,channels,&waveFormat);
        if (SUCCEEDED(context->primaryBuffer->SetFormat(&waveFormat))) return true;
    }
    setError(context,InvalidValue,"alimSetPrimaryFormat");return false;
}
bool restoreBuffer(Context* context,bool* restored) {
    if (context->primaryBuffer) {
        if (restored) *restored=false;
        DWORD status;
        if (FAILED(context->primaryBuffer->GetStatus(&status))) setError(context,InvalidOperation,"alimRestoreBuffer::GetStatus");
        else if (status&DSBSTATUS_BUFFERLOST) {
            do {
                if (context->primaryBuffer->Restore()==DSERR_BUFFERLOST) Sleep(10);
            } while (context->primaryBuffer->Restore()!=DS_OK);
            if (restored) *restored=true;
            return true;
        }
    }
    return false;
}
bool remainingTime(Context* context,Handle handle,int token,float* output) {
    if (!output) return false;
    *output=0;
    auto* source=contextSource(context,handle);if (!source) return false;
    auto* buffer=contextBuffer(context,source->loadedBuffer);
    DWORD status,playCursor,writeCursor;
    if (!buffer || !source->dsBuffer || FAILED(source->dsBuffer->GetStatus(&status))) return true;
    source->dsBuffer->GetCurrentPosition(&playCursor,&writeCursor);
    if (!(status&DSBSTATUS_PLAYING)) return playCursor==0;
    if (status&DSBSTATUS_LOOPING) return false;
    int bits,channels;splitFormat(buffer->format,&bits,&channels);
    const long double remaining=(((static_cast<long double>(buffer->dataBytes)-static_cast<std::int32_t>(playCursor))/channels)/
        (bits>>3))/(static_cast<long double>(buffer->frequency)*source->pitch);
    // These successive stores/fallthroughs occur in the saved instructions.
    // Preserve the final microsecond scale even for the two earlier selectors.
    if (token==RemainingSeconds) *output=static_cast<float>(remaining);
    else if (token!=RemainingMilliseconds && token!=RemainingMicroseconds) return true;
    if (token!=RemainingMicroseconds) *output=static_cast<float>(remaining*0.001);
    *output=static_cast<float>(remaining*1e-6);
    return true;
}
bool reuseOwnBuffer(Context* context,Source* source) {
    contextBuffer(context,source->boundBuffer);
    DWORD status;
    if (source->dsBuffer && source->loadedBuffer==source->boundBuffer &&
        (source->relative || source->ds3dBuffer) && SUCCEEDED(source->dsBuffer->GetStatus(&status)) &&
        !(status&DSBSTATUS_BUFFERLOST) && SUCCEEDED(source->dsBuffer->SetCurrentPosition(0))) {
        ++context->ownBufferReuses;return true;
    }
    return false;
}
bool recycleIdleBuffer(Context* context,Source* source) {
    contextBuffer(context,source->boundBuffer);
    HandleNode* cursor=nullptr;
    while (auto* donor=static_cast<Source*>(nextHandle(context->sourceHandles,&cursor))) {
        DWORD status;
        if (donor!=source && donor->loadedBuffer==source->boundBuffer && donor->dsBuffer &&
            donor->relative==source->relative && (donor->relative || donor->ds3dBuffer) &&
            SUCCEEDED(donor->dsBuffer->GetStatus(&status)) && !(status&3) &&
            SUCCEEDED(donor->dsBuffer->SetCurrentPosition(0))) {
            std::swap(donor->dsBuffer,source->dsBuffer);std::swap(donor->ds3dBuffer,source->ds3dBuffer);
            donor->loadedBuffer=source->loadedBuffer;source->loadedBuffer=source->boundBuffer;
            ++context->idleBufferRecycles;return true;
        }
    }
    return false;
}
bool createDSBuffer(Context* context,Source* source) {
    auto* buffer=contextBuffer(context,source->boundBuffer);
    if (source->ds3dBuffer) { source->ds3dBuffer->Release();source->ds3dBuffer=nullptr; }
    if (source->dsBuffer) { source->dsBuffer->Release();source->dsBuffer=nullptr; }
    int bits,channels;
    if (!splitFormat(buffer->format,&bits,&channels)) {
        setError(context,InvalidValue,"alisCreateDSBuffer::alimSplitFormat");return false;
    }
    DSBUFFERDESC description{};description.dwSize=sizeof(description);
    description.dwBufferBytes=buffer->dataBytes;description.dwFlags=source->relative ? 0x501e0u : 0x501b0u;
    if (source->streaming) { description.guid3DAlgorithm=GUID{};description.dwFlags=0x101e0u; }
    WAVEFORMATEX format;makePcmFormat(buffer->frequency,bits,channels,&format);description.lpwfxFormat=&format;
    if ((format.nChannels>1 || format.wFormatTag!=WAVE_FORMAT_PCM) && !source->relative)
        setError(context,InvalidValue,"alisCreateDSBuffer::Too many channels or not PCM");
    if (FAILED(context->device->CreateSoundBuffer(&description,&source->dsBuffer,nullptr))) {
        setError(context,InvalidOperation,"alisCreateDSBuffer::CreateSoundBuffer - Now Trying DS3DALG_NO_VIRTUALIZATION");
        // 004dfe82..004dfeb9 updates only guid3DAlgorithm, not the descriptor
        // header fields corrupted by the saved decompiler's stack aliases.
        description.guid3DAlgorithm={0xc241333f,0x1c1b,0x11d2,{0x94,0xf5,0x00,0xc0,0x4f,0xc2,0x8a,0xca}};
        if (FAILED(context->device->CreateSoundBuffer(&description,&source->dsBuffer,nullptr))) {
            setError(context,InvalidOperation,"alisCreateDSBuffer::CreateSoundBuffer");return false;
        }
    }
    if (!source->relative && FAILED(source->dsBuffer->QueryInterface(IID_IDirectSound3DBuffer,
        reinterpret_cast<void**>(&source->ds3dBuffer)))) {
        setError(context,InvalidOperation,"alisCreateDSBuffer::QueryInterface");
        if (source->dsBuffer) { source->dsBuffer->Release();source->dsBuffer=nullptr; }
        return false;
    }
    if (!uploadPcm(context,source)) return false;
    ++context->dsBuffersCreated;return true;
}
bool uploadPcm(Context* context,Source* source) {
    auto* buffer=contextBuffer(context,source->boundBuffer);
    if (!buffer || buffer->format<Mono8 || buffer->format>Stereo16) return false;
    void* memory;DWORD bytes;
    if (FAILED(source->dsBuffer->Lock(0,buffer->dataBytes,&memory,&bytes,nullptr,nullptr,0))) return false;
    std::memcpy(memory,buffer->pcmData,buffer->dataBytes);
    if (FAILED(source->dsBuffer->Unlock(memory,bytes,nullptr,0))) return false;
    source->loadedBuffer=source->boundBuffer;return true;
}
void updateSource(Context* context,Source* source) {
    auto* buffer=contextBuffer(context,source->boundBuffer);
    if (!source->dsBuffer) return;
    auto frequency=static_cast<std::uint32_t>(truncateX87(static_cast<long double>(buffer->frequency)*source->pitch));
    if (frequency>100000) frequency=100000;else if (frequency<100) frequency=100;
    source->dsBuffer->SetFrequency(frequency);source->dsBuffer->SetVolume(gainToAttenuation(source->gain));
    if (source->streaming) {
        context->activeStreamingSource=source;
        IDirectSoundNotify* notification=nullptr;
        if (FAILED(source->dsBuffer->QueryInterface(IID_IDirectSoundNotify,reinterpret_cast<void**>(&notification))))
            setError(context,InvalidOperation,"alimUpdateSource::QueryInterface");
        auto* positions=new DSBPOSITIONNOTIFY[16];
        for (DWORD i=0;i<16;++i) { positions[i].dwOffset=buffer->segmentBytes*(i+1)-1;positions[i].hEventNotify=context->notificationEvent; }
        if (FAILED(notification->SetNotificationPositions(16,positions)))
            setError(context,InvalidOperation,"alimUpdateSource::SetNotificationPositions");
        if (notification) notification->Release();delete[] positions;
    }
    if (source->relative==1) { source->dsBuffer->SetPan(panToUnits(source->pan));return; }
    if (source->ds3dBuffer) {
        DS3DBUFFER parameters;
        parameters.dwSize=sizeof(parameters);
        parameters.vPosition={source->position[0],source->position[2],source->position[1]};
        parameters.vVelocity={source->velocity[0],source->velocity[2],source->velocity[1]};
        parameters.dwInsideConeAngle=source->coneInnerAngle;parameters.dwOutsideConeAngle=source->coneOuterAngle;
        // The original does not initialize vConeOrientation in this bulk call.
        parameters.lConeOutsideVolume=static_cast<LONG>(truncateX87(source->coneOuterGain));
        parameters.flMinDistance=source->minDistance;parameters.flMaxDistance=source->maxDistance;parameters.dwMode=DS3DMODE_NORMAL;
        if (FAILED(source->ds3dBuffer->SetAllParameters(&parameters,DS3D_IMMEDIATE)))
            setError(context,InvalidOperation,"alimUpdateSource::SetAllParameters");
    }
}
} } }
