#include "resource_bridge.h"
#include "shape.h"
namespace megusta { namespace dts {
namespace {
ResourceFile* (*serviceOpen)(const char*,const char*)=nullptr;
int (*serviceClose)(ResourceFile*)=nullptr;
ResourceFile* openIfl(const char* path,const char* mode) {
    return serviceOpen ? serviceOpen(path,mode) : nullptr;
}
void closeIfl(ResourceFile* file) {
    if (serviceClose) serviceClose(file);
}
}
void bindIflResourceService(ResourceFile* (*open)(const char*,const char*),int (*close)(ResourceFile*)) {
    serviceOpen=open; serviceClose=close; iflOpenFile=open ? openIfl : nullptr; iflCloseFile=close ? closeIfl : nullptr;
}
} }
