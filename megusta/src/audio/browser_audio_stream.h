#pragma once
#include "openal.h"
namespace megusta::audio {
struct BrowserStreamInfo { unsigned channels,rate,frames; };
bool registerBrowserStream(Handle,ResourceFile*,BrowserStreamInfo&);
void releaseBrowserStream(Handle);
void releaseBrowserStreams();
}
