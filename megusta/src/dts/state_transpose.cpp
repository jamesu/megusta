#include "state_transpose.h"
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>
namespace megusta { namespace dts {
void rearrangeStates(int first,int frames,int tracks,void* data,unsigned stride) {
    // Preserve the original 32-bit byte-count and pointer-offset arithmetic.
    // The saved function allocates even when the product is zero.
    const std::uint32_t frameTrackCount=static_cast<std::uint32_t>(frames)*
                                       static_cast<std::uint32_t>(tracks);
    const std::uint32_t byteCount=frameTrackCount*stride;
    const std::uint32_t sourceOffset=static_cast<std::uint32_t>(first)*stride;
    const std::uint32_t trackStride=static_cast<std::uint32_t>(frames)*stride;
    const std::uint32_t frameStride=static_cast<std::uint32_t>(tracks)*stride;
    auto* start=static_cast<unsigned char*>(data)+sourceOffset;
    auto* copy=static_cast<unsigned char*>(::operator new(byteCount));
    std::memcpy(copy,start,byteCount);
    for (int frame=0;frame<frames;++frame)
        for (int track=0;track<tracks;++track)
            std::memcpy(start+static_cast<std::uint32_t>(track)*trackStride+
                        static_cast<std::uint32_t>(frame)*stride,
                        copy+static_cast<std::uint32_t>(frame)*frameStride+
                        static_cast<std::uint32_t>(track)*stride,stride);
    ::operator delete(copy);
}
bool rearrangeStates(void* data,std::size_t bytes,int first,int frames,int tracks,unsigned stride) {
    if (first<0 || frames<0 || tracks<0 || !stride) return false;
    std::size_t offset=static_cast<std::size_t>(first)*stride;
    if (offset>bytes) return false;
    std::size_t available=(bytes-offset)/stride;
    if (tracks && static_cast<std::size_t>(frames)>available/static_cast<unsigned>(tracks)) return false;
    std::size_t count=static_cast<std::size_t>(frames)*static_cast<unsigned>(tracks);
    if (!count) return true;
    if (!data) return false;
    rearrangeStates(first,frames,tracks,data,stride);
    return true;
}
} }

