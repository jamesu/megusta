#include "directsound_internal.h"
#include "wave.h"
#include <cstdlib>
#include <cstring>
namespace megusta { namespace audio { namespace ds {
DWORD WINAPI notificationProc(void* argument) {
    auto* context=static_cast<Context*>(argument);
    bool quit=false;
    do {
        DWORD result;
        do {
            while ((result=MsgWaitForMultipleObjects(1,&context->notificationEvent,FALSE,INFINITE,0xbf))==WAIT_OBJECT_0) {
                if (!streamNotification(context)) {
                    setError(context,InvalidOperation,"alimNotificationProc::alimStreamNotification");return 0;
                }
            }
            MSG message;
            if (result!=WAIT_OBJECT_0+1 || !PeekMessageA(&message,nullptr,0,0,PM_REMOVE)) continue;
            do {
                if (message.message==WM_QUIT) quit=true;
            } while (PeekMessageA(&message,nullptr,0,0,PM_REMOVE));
            break;
        } while (true);
    } while (!quit);
    return 0;
}
bool resetStream(ResourceFile* file) {
    if (!file) return false;
    file->seek(0,SEEK_SET);
    wave::RiffHeader riff;file->read(&riff,1,sizeof(riff));
    std::uint32_t remaining=((riff.size+1u)&~1u)-4;
    while (remaining) {
        wave::ChunkHeader chunk;
        if (file->read(&chunk,1,sizeof(chunk))) {
            if (chunk.tag==wave::FormatTag) {
                wave::Format format;file->read(&format,1,sizeof(format));
                if (format.tag==1) file->seek(static_cast<std::int32_t>(chunk.size-16),SEEK_CUR);
                else { std::uint32_t extra;file->read(&extra,1,4);file->seek(static_cast<std::int32_t>(chunk.size-20),SEEK_CUR); }
            } else if (chunk.tag==wave::DataTag) return true;
            else if (chunk.tag==wave::SamplerTag) {
                std::uint8_t sampler[60];file->read(sampler,1,60);
                file->seek(static_cast<std::int32_t>(chunk.size-60),SEEK_CUR);
            } else file->seek(static_cast<std::int32_t>(chunk.size),SEEK_CUR);
            file->seek(static_cast<std::int32_t>(chunk.size&1),SEEK_CUR);remaining-=8+((chunk.size+1u)&~1u);
        }
    }
    return false;
}
bool readStream(ResourceFile* file,void* data,std::uint32_t bytes,std::uint32_t* read) {
    if (!file) return false;
    *read=file->read(data,1,bytes);return true;
}
bool fillBuffer(Context* context,int looping) {
    if (!context->primaryBuffer) return false;
    auto* source=context->activeStreamingSource;
    auto* buffer=contextBuffer(context,source->boundBuffer);
    restoreBuffer(context,nullptr);
    void* memory=nullptr;DWORD length;
    if (FAILED(source->dsBuffer->Lock(0,buffer->dataBytes,&memory,&length,nullptr,nullptr,0))) {
        setError(context,InvalidOperation,"alimFillBufferWithSound::Lock");return false;
    }
    resetStream(buffer->streamFile);
    std::uint32_t read;
    if (!readStream(buffer->streamFile,memory,length,&read)) {
        setError(context,InvalidOperation,"alimFillBufferWithSound::alimRead");return false;
    }
    if (!read) std::memset(memory,buffer->bits==8 ? 0x80 : 0,length);
    else if (read<length) {
        if (looping) {
            auto filled=read;
            do {
                resetStream(buffer->streamFile);
                readStream(buffer->streamFile,static_cast<std::uint8_t*>(memory)+filled,length-filled,&read);
                filled+=read;
            } while (filled<length);
        } else std::memset(static_cast<std::uint8_t*>(memory)+read,buffer->bits==8 ? 0x80 : 0,length-read);
    }
    source->dsBuffer->Unlock(memory,length,nullptr,0);return true;
}
bool streamNotification(Context* context) {
    auto* buffer=contextBuffer(context,context->activeStreamingSource->boundBuffer);
    if (!context->primaryBuffer || !buffer) return false;
    bool restored=false;restoreBuffer(context,&restored);
    if (restored) {
        if (fillBuffer(context,0)) return true;
        setError(context,InvalidOperation,"alimStreamNotification::alimFillBufferWithSound");return false;
    }
    void* first=nullptr;void* second=nullptr;DWORD firstBytes,secondBytes;
    auto* device=context->activeStreamingSource->dsBuffer;
    if (FAILED(device->Lock(buffer->writeCursor,buffer->segmentBytes,&first,&firstBytes,&second,&secondBytes,0))) {
        setError(context,InvalidOperation,"alimStreamNotification::Lock");return false;
    }
    if (second) { setError(context,InvalidOperation,"alimStreamNotification::pDSLockedBuffer2 != NULL");return false; }
    std::uint32_t read;
    if (!readStream(buffer->streamFile,first,firstBytes,&read)) {
        setError(context,InvalidOperation,"alimStreamNotification::alimRead");return false;
    }
    bool eof=false;
    if (read<firstBytes) {
        if (!context->activeStreamingSource->looping) {
            eof=true;std::memset(static_cast<std::uint8_t*>(first)+read,buffer->bits==8 ? 0x80 : 0,firstBytes-read);
        } else {
            auto filled=read;
            do {
                resetStream(buffer->streamFile);
                readStream(buffer->streamFile,static_cast<std::uint8_t*>(first)+filled,firstBytes-filled,&read);
                filled+=read;
            } while (filled<firstBytes);
        }
    }
    device->Unlock(first,firstBytes,nullptr,0);
    DWORD cursor;
    if (FAILED(device->GetCurrentPosition(&cursor,nullptr))) {
        setError(context,InvalidOperation,"alimStreamNotification::GetCurrentPosition");return false;
    }
    buffer->playedBytes+=(cursor<buffer->lastPlayCursor ? buffer->dataBytes : 0)-buffer->lastPlayCursor+cursor;
    buffer->lastPlayCursor=cursor;
    if (eof && buffer->playedBytes>=buffer->dataBytes) device->Stop();
    buffer->writeCursor=(firstBytes+buffer->writeCursor)%buffer->dataBytes;
    return true;
}
void bufferStream(Context* context,Handle handle,ResourceFile* file) {
    wave::Format format{};int unusedFormat;std::uint32_t unusedBytes,unusedRate;
    wave::readFormat(file,&unusedFormat,&unusedBytes,&unusedRate,&format);
    if (!format.bytesPerSecond) { setError(context,InvalidValue,"alimBufferStream::alutGetWAVFormat missing file");delete file;return; }
    auto* buffer=contextBuffer(context,handle);
    if (!buffer) { setError(context,InvalidOperation,"alimBufferStream::alimContextBuffer");delete file;return; }
    buffer->segmentBytes=(static_cast<std::uint32_t>(format.blockAlign)*format.rate*3)>>4;
    buffer->segmentBytes-=buffer->segmentBytes%format.blockAlign;buffer->dataBytes=buffer->segmentBytes*16;
    buffer->format=(format.channels==1 ? Mono8 : Stereo8)+(format.bits!=8);
    buffer->frequency=format.rate;buffer->bits=format.bits;buffer->channels=format.channels;
    void* memory=std::malloc(buffer->dataBytes);
    if (!memory) { setError(context,OutOfMemory,"alimBufferStream::malloc");delete file;return; }
    if (buffer->pcmData) std::free(buffer->pcmData);
    buffer->pcmData=memory;std::memset(memory,buffer->bits==8 ? 0x80 : 0,buffer->dataBytes);
    buffer->streaming=1;buffer->streamFile=file;
    if (!resetStream(buffer->streamFile)) setError(context,InvalidOperation,"alimBufferStream::alimReset");
}
} } }
