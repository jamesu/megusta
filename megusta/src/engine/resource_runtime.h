#pragma once
#include "resource_mount.h"
#include "directory_services.h"
namespace megusta {
// Typed replacement for the original shared-I/O callback globals. No VC6
// object or function-pointer-table layout is imposed on this interface.
struct SharedIoServices {
    ResourceFile* (*open)(const char*, const char*);
    int (*close)(ResourceFile*);
    int (*eof)(ResourceFile*);
    int (*flush)(ResourceFile*);
    int (*getCharacter)(ResourceFile*);
    char* (*getLine)(char*, int, ResourceFile*);
    int (*formatted)(ResourceFile*, const char*, ...);
    int (*putCharacter)(int, ResourceFile*);
    int (*putString)(const char*, ResourceFile*);
    std::uint32_t (*read)(void*, std::uint32_t, std::uint32_t, ResourceFile*);
    int (*seek)(ResourceFile*, std::int32_t, int);
    std::int32_t (*tell)(ResourceFile*);
    std::uint32_t (*write)(const void*, std::uint32_t, std::uint32_t, ResourceFile*);
    int (*getc)(ResourceFile*);
    int (*isatty)(ResourceFile*);
    int (*putc)(int, ResourceFile*);
    int (*stdioPrintf)(const char*, ...);
    void (*rewind)(ResourceFile*);
    void (*setbuf)(ResourceFile*, char*);
    int (*vprintf)(ResourceFile*, const char*, va_list);
    void (*clearerr)(ResourceFile*);
    int (*error)(ResourceFile*);
    int (*queryStat)(const char*, ResourceStatus&);
    DirectoryServices directory;
    int (*unsupportedOperation)();
    ResourceFile* standardInput;
    ResourceFile* standardOutput;
    ResourceFile* standardError;
};
extern SharedIoServices* sharedIo;
class ResourceRuntime {
public:
    ResourceRuntime();
    ~ResourceRuntime();
    ResourceRuntime(const ResourceRuntime&) = delete;
    ResourceRuntime& operator=(const ResourceRuntime&) = delete;
    void initializeFromArgs(int argc, const char* const* argv);
    ResourceMountTable& mounts() { return mounts_; }
    LogCapture& capture() { return capture_; }
    SharedIoServices& services() { return services_; }
private:
    LogCapture capture_;
    ResourceFile input_, output_, error_;
    ResourceMountTable mounts_;
    SharedIoServices services_;
};
}
