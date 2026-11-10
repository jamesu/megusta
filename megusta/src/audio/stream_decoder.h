#pragma once
#include "openal.h"
namespace megusta::audio {
struct StreamInfo { unsigned channels,rate,frames; };
bool registerStream(Handle,ResourceFile*,StreamInfo&);
void releaseStream(Handle);
void releaseStreams();
const float* readStreamChunk(Handle,unsigned frame,unsigned requested,unsigned& count);
}
