#include "packed_codec.h"

namespace megusta::engine_codec {

int packedCodec_decodeFlaggedBackrefs_impl(const std::uint8_t* input,
        std::uint8_t* output,std::uint32_t outputCapacity) {
    int decodedCount=0;
    const std::uint8_t* flags=input+1;
    if(input[0]>1) {
        decodedCount=static_cast<int>(flags[0])*0x1000000+
            static_cast<int>(input[2])*0x10000;
        flags=input+3;
    }
    if(input[0]!=0) { decodedCount+=static_cast<int>(*flags)*0x100; ++flags; }
    std::uint8_t flagByte=flags[1];
    const unsigned tokenCount=static_cast<unsigned>(decodedCount)+*flags;
    const std::uint8_t* literals=flags+((tokenCount+7)>>3)+1;
    const std::uint8_t* flagCursor=flags+2;
    const std::uint8_t* flagReload=flagCursor;
    std::uint8_t* outputStart=output;
    unsigned flagIndex=0;
    while(flagIndex<tokenCount && outputCapacity!=0) {
        int produced=1;
        if(static_cast<std::int8_t>(flagByte)<0) {
            const unsigned code=(static_cast<unsigned>(literals[0])<<8)|literals[1];
            literals+=2;
            unsigned length=(code&0x3f)+3;
            const std::uint8_t* source=output-1-(code>>6);
            if(outputCapacity<length) length=outputCapacity;
            for(unsigned i=0;i<length;++i) *output++=*source++;
            produced=static_cast<int>(length);
            flagCursor=flagReload;
        } else {
            *output++=*literals++;
        }
        outputCapacity-=static_cast<unsigned>(produced);
        if((flagIndex&7)==7) { flagByte=*flagCursor++; flagReload=flagCursor; }
        else flagByte=static_cast<std::uint8_t>(flagByte<<1);
        ++flagIndex;
    }
    return static_cast<int>(output-outputStart);
}

int packedCodec_decodeLiteralRunBackrefs_impl(const std::uint8_t* input,
        int inputBytes,std::uint8_t* output,int outputBytes) {
    std::uint8_t* outputStart=output;
    const std::uint8_t* source=input;
    const std::uint8_t* sourceEnd=input+inputBytes;
    std::uint8_t* outputEnd=output+outputBytes;
    std::uint8_t* copiedEnd=output;
    do {
        const std::uint8_t control=*source++;
        if(control<0x20) {
            unsigned count=static_cast<unsigned>(control)+1;
            copiedEnd=output;
            do { *copiedEnd++=*source++; } while(--count!=0);
        } else {
            unsigned count=control>>5;
            if(count==7) { count=static_cast<unsigned>(*source)+7; ++source; }
            const unsigned distance=((control&0x1f)<<8)+static_cast<unsigned>(*source)+1;
            ++source;
            const std::uint8_t* back=output-distance;
            *output=*back;
            copiedEnd=output+2;
            back+=2;
            output[1]=*back++;
            do { *copiedEnd++=*back++; } while(--count!=0);
        }
        if(copiedEnd<outputEnd && source<sourceEnd) {
            output=copiedEnd;
        } else break;
    } while(true);
    return static_cast<int>(copiedEnd-outputStart);
}

} // namespace megusta::engine_codec
