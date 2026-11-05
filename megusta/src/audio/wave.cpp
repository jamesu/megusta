#include "wave.h"
#include "openal.h"
#include "../engine/resource_file.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace megusta { namespace audio { namespace wave {
// Immutable tables recovered at 004ed390 and 004ed3d0.
static const int indexAdjust[16]={-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};
static const int steps[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,
    55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,
    449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
    2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,
    11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
void decodeNibble(std::uint8_t nibble,int* predictor,int* index) {
    const int step=steps[*index];
    *index+=indexAdjust[static_cast<std::int8_t>(nibble)];
    int delta=step>>3;
    if (*index<0) *index=0;else if (*index>88) *index=88;
    if (nibble&4) delta+=step;if (nibble&2) delta+=step>>1;if (nibble&1) delta+=step>>2;
    if (nibble&8) delta=-delta;
    *predictor+=delta;
    if (*predictor>=32768) *predictor=32767;else if (*predictor< -32768) *predictor=-32768;
}
static int signedPredictor(const ImaChannelHeader& header) {
    // Both bytes are sign-extended by the executable, including the low byte.
    return header.predictorHigh*256+header.predictorLow;
}
bool decodeMono(std::int16_t* output,const std::uint8_t* input,int outputBytes,int blockBytes) {
    int remaining=outputBytes/2;
    const int perBlock=blockBytes*2-7;
    if (!remaining) return true;
    do {
        const int selected=remaining<perBlock ? remaining : perBlock;
        remaining=remaining<perBlock ? 0 : remaining-perBlock;
        const int pairs=(selected-1)/2;
        const auto& header=*reinterpret_cast<const ImaChannelHeader*>(input);
        int predictor=signedPredictor(header),index=header.stepIndex;input+=sizeof(ImaChannelHeader);
        *output++=static_cast<std::int16_t>(predictor);
        for (int i=0;i<pairs;++i) {
            const auto byte=*input++;
            decodeNibble(byte&15,&predictor,&index);*output++=static_cast<std::int16_t>(predictor);
            decodeNibble(byte>>4,&predictor,&index);*output++=static_cast<std::int16_t>(predictor);
        }
    } while (remaining);
    return true;
}
bool decodeStereo(std::int16_t* output,const std::uint8_t* input,int outputBytes,int blockBytes) {
    int remaining=outputBytes/2;
    const int perBlock=blockBytes*2-14;
    if (!remaining) return true;
    do {
        const int selected=remaining<perBlock ? remaining : perBlock;
        remaining=remaining<perBlock ? 0 : remaining-perBlock;
        const int groups=(selected-2)/16;
        const auto& header=*reinterpret_cast<const ImaStereoHeader*>(input);
        int left=signedPredictor(header.left),leftIndex=header.left.stepIndex;
        int right=signedPredictor(header.right),rightIndex=header.right.stepIndex;input+=sizeof(ImaStereoHeader);
        output[0]=static_cast<std::int16_t>(left);output[1]=static_cast<std::int16_t>(right);output+=2;
        for (int group=0;group<groups;++group) {
            const auto& samples=*reinterpret_cast<const ImaStereoGroup*>(input);
            for (int byte=0;byte<4;++byte) {
                decodeNibble(samples.left[byte]&15,&left,&leftIndex);output[byte*4]=static_cast<std::int16_t>(left);
                decodeNibble(samples.left[byte]>>4,&left,&leftIndex);output[byte*4+2]=static_cast<std::int16_t>(left);
            }
            for (int byte=0;byte<4;++byte) {
                decodeNibble(samples.right[byte]&15,&right,&rightIndex);output[byte*4+1]=static_cast<std::int16_t>(right);
                decodeNibble(samples.right[byte]>>4,&right,&rightIndex);output[byte*4+3]=static_cast<std::int16_t>(right);
            }
            input+=sizeof(ImaStereoGroup);output+=16;
        }
    } while (remaining);
    return true;
}
void loadData(megusta::ResourceFile* file,int* outputFormat,void** data,std::uint32_t* bytes,std::uint32_t* frequency) {
    *outputFormat=Mono16;*data=nullptr;*bytes=0;*frequency=22050;
    if (!file) return;
    RiffHeader riff;file->read(&riff,1,sizeof(riff));
    std::uint32_t remaining=((riff.size+1u)&~1u)-4;
    Format format;
    ImaFormatExtension extra;
    while (remaining) {
        ChunkHeader chunk;
        if (file->read(&chunk,1,sizeof(chunk))) {
            if (chunk.tag==FormatTag) {
                file->read(&format,1,sizeof(format));
                if (format.tag==1) file->seek(static_cast<std::int32_t>(chunk.size-16),SEEK_CUR);
                else { file->read(&extra,1,sizeof(extra));file->seek(static_cast<std::int32_t>(chunk.size-20),SEEK_CUR); }
                *outputFormat=(format.channels==1 ? Mono8 : Stereo8)+(format.bits!=8);
                *frequency=format.rate;
            } else if (chunk.tag==DataTag) {
                if (format.tag==1) {
                    *bytes=chunk.size;*data=std::malloc(chunk.size+31);
                    if (*data) file->read(*data,format.blockAlign,chunk.size/format.blockAlign);
                    std::memset(static_cast<std::uint8_t*>(*data)+chunk.size,0,31);
                } else if (format.tag==0x11) {
                    *bytes=chunk.size*4;*data=std::malloc(*bytes+31);
                    if (*data) {
                        auto* compressed=static_cast<std::uint8_t*>(*data)+chunk.size*3;
                        std::memset(static_cast<std::uint8_t*>(*data)+*bytes,0,31);
                        file->read(compressed,1,chunk.size);
                        const int blockBytes=((static_cast<unsigned>(extra.samplesPerBlock)+14)>>3)*format.channels*4;
                        if (format.channels==1) decodeMono(static_cast<std::int16_t*>(*data),compressed,*bytes,blockBytes);
                        else decodeStereo(static_cast<std::int16_t*>(*data),compressed,*bytes,blockBytes);
                    }
                }
            } else if (chunk.tag==SamplerTag) {
                std::uint8_t sampler[60];file->read(sampler,1,60);
                file->seek(static_cast<std::int32_t>(chunk.size-60),SEEK_CUR);
            } else file->seek(static_cast<std::int32_t>(chunk.size),SEEK_CUR);
            file->seek(static_cast<std::int32_t>(chunk.size&1),SEEK_CUR);
            remaining-=8+((chunk.size+1u)&~1u);
        }
    }
}
void readFormat(megusta::ResourceFile* file,int* outputFormat,std::uint32_t* bytes,std::uint32_t* frequency,Format* format) {
    *outputFormat=Mono16;*bytes=0;*frequency=22050;
    if (file) {
        RiffHeader riff;file->read(&riff,1,sizeof(riff));
        ChunkHeader chunk;
        if (file->read(&chunk,1,sizeof(chunk)) && chunk.tag==FormatTag)
            file->read(format,1,sizeof(Format));
    }
}
} } }
