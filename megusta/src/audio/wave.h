#pragma once
#include <cstdint>
namespace megusta { class ResourceFile; }
namespace megusta { namespace audio { namespace wave {
struct RiffHeader { std::uint32_t tag,size,type; };
struct ChunkHeader { std::uint32_t tag,size; };
constexpr std::uint32_t FormatTag=0x20746d66u,DataTag=0x61746164u,SamplerTag=0x6c706d73u;
#pragma pack(push,1)
struct Format { std::uint16_t tag,channels; std::uint32_t rate,bytesPerSecond; std::uint16_t blockAlign,bits; };
struct ImaFormatExtension { std::uint16_t extensionBytes,samplesPerBlock; };
struct ImaChannelHeader { std::int8_t predictorLow,predictorHigh,stepIndex; std::uint8_t reserved; };
struct ImaStereoHeader { ImaChannelHeader left,right; };
struct ImaStereoGroup { std::uint8_t left[4],right[4]; };
#pragma pack(pop)
static_assert(sizeof(Format)==16,"WAV on-disk format prefix");
static_assert(sizeof(ImaChannelHeader)==4 && sizeof(ImaStereoHeader)==8 && sizeof(ImaStereoGroup)==8,
    "IMA on-disk records");
void loadData(megusta::ResourceFile*,int*,void**,std::uint32_t*,std::uint32_t*);
void readFormat(megusta::ResourceFile*,int*,std::uint32_t*,std::uint32_t*,Format*);
bool decodeMono(std::int16_t*,const std::uint8_t*,int outputBytes,int blockBytes);
bool decodeStereo(std::int16_t*,const std::uint8_t*,int outputBytes,int blockBytes);
void decodeNibble(std::uint8_t,int* predictor,int* index);
} } }
