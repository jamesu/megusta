#include "platform_services.h"
#include "case_insensitive_path.h"
#include <cstdio>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <new>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

namespace megusta::engine_platform {

char* enginePath_joinNormalize_impl(char* output,const char* base,const char* name) {
    if (!name) { *output='\0'; return output; }
    if (!*name || *name=='/' || !base || !*base) std::strcpy(output,name);
    else { std::strcpy(output,base); std::strcat(output,"/"); std::strcat(output,name); }
    for(char* cursor=output;*cursor;++cursor) if(*cursor=='\\') *cursor='/';
    return output;
}

int engineFile_queryStat_impl(const char* path,FileStatus* status) {
#ifdef _WIN32
    struct _stat native{};
    if (_stat(path,&native)!=0) return -1;
    const auto mode=native.st_mode;
    status->byteCount=static_cast<std::int32_t>(native.st_size);
    status->modificationTime=static_cast<std::int32_t>(native.st_mtime);
    status->kindFlags=(mode&_S_IFDIR) ? 1 : (mode&_S_IFREG) ? 2 : 0;
#else
    std::string resolvedPath;
    if (!engine_filesystem::resolvePathCaseInsensitive(path, resolvedPath)) return -1;
    struct stat native{};
    if (stat(resolvedPath.c_str(),&native)!=0) return -1;
    status->byteCount=static_cast<std::int32_t>(native.st_size);
    status->modificationTime=static_cast<std::int32_t>(native.st_mtime);
    status->kindFlags=S_ISDIR(native.st_mode) ? 1 : S_ISREG(native.st_mode) ? 2 : 0;
#endif
    status->reserved=0;
    return 0;
}

std::int32_t engineFile_getSize_impl(const char* path) {
    FileStatus status{};
    // Despite this recovered analyst label, load_source_module uses the
    // returned value as the .pyc modification timestamp.
    return engineFile_queryStat_impl(path,&status)==0 ? status.modificationTime : -1;
}

void engineDiagnostic_printWithSeverity_impl(int severity,const char* message) {
    std::fputs(severity==0 ? "BT!:" : "ERR:",stdout);
    std::fputs(message,stdout);
}
void engineDiagnostic_print_adapter(int,const char* message) { std::fputs(message,stdout); }

void engineWindow_reportLastError_impl() {
#ifdef _WIN32
    char* message=nullptr;
    const DWORD error=::GetLastError();
    ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,error,0x400,
        reinterpret_cast<char*>(&message),0,nullptr);
    if(message) {
        engineDiagnostic_printWithSeverity_impl(1,message);
        engineDiagnostic_print_adapter(1,"\n");
        ::MessageBoxA(nullptr,message,"GetLastError",MB_ICONINFORMATION);
        ::LocalFree(message);
    }
#else
    engineDiagnostic_printWithSeverity_impl(1,std::strerror(errno));
#endif
}

DirectoryContext* engineDirectory_openIterator_impl(char* path) {
    auto* context=new(std::nothrow) DirectoryContext{};
    if(!context) return nullptr;
    std::strcpy(context->path,path);
    context->platformIterator=EngineDirectoryIterator::open(path);
    return context;
}

DirectoryEntry* engineDirectory_nextEntry_impl(DirectoryContext* context) {
    char* name=context->platformIterator->nextName();
    if(!name) return nullptr;
    std::strcpy(context->entry.name,name);
    char fullPath[260];
    std::sprintf(fullPath,"%s/%s",context->path,context->entry.name);
    FileStatus status{};
    context->entry.isDirectory=engineFile_queryStat_impl(fullPath,&status)==0
        ? status.kindFlags&1 : 0;
    return &context->entry;
}

} // namespace megusta::engine_platform
