#include "putt_map_io.h"
#include "../engine/resource_mount.h"
#include <ul.h>
#include <cstdlib>
namespace megusta {
namespace {
void* open(void* context,const char* path,const char* mode) {
    return static_cast<PuttMapResourceIO*>(context)->mounts().open(path,mode).release();
}
unsigned read(void*,void* output,unsigned size,unsigned count,void* file) {
    return static_cast<ResourceFile*>(file)->read(output,size,count);
}
char* getLine(void*,char* output,int size,void* file) {
    return static_cast<ResourceFile*>(file)->getLine(output,size);
}
void close(void*,void* file) { delete static_cast<ResourceFile*>(file); }
void reportError(void*,const char* format,const char* name) { ulSetError(UL_WARNING,format,name); }
void fatal() { std::exit(1); }
}
PuttMapResourceIO::PuttMapResourceIO(ResourceMountTable& mounts)
    : mounts_(mounts),services_{this,open,read,getLine,close,reportError,fatal} {}
}
