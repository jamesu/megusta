#include "runtime_adapters.h"
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <process.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdexcept>

namespace megusta::engine_runtime {
namespace {
LaunchServices launchServices;
ExitHandler activeExitHandler=nullptr;
int localTimeCopy[9]{};
std::vector<char> formatBuffer(256, '\0');
const char* const directInputMessages[]={
    "EWOULDBLOCK","EINPROGRESS","EALREADY","ENOTSOCK","EDESTADDRREQ",
    "EMSGSIZE","EPROTOTYPE","ENOPROTOOPT","EPROTONOSUPPORT","ESOCKTNOSUPPORT",
    "EOPNOTSUPP","EPFNOSUPPORT","EAFNOSUPPORT","EADDRINUSE","EADDRNOTAVAIL",
    "ENETDOWN","ENETUNREACH","ENETRESET","ECONNABORTED","ECONNRESET","ENOBUFS",
    "EISCONN","ENOTCONN","ESHUTDOWN","ETOOMANYREFS","ETIMEDOUT","ECONNREFUSED",
    "ELOOP","ENAMETOOLONG","EHOSTDOWN","EHOSTUNREACH","ENOTEMPTY","EPROCLIM",
    "EUSERS","EDQUOT","ESTALE","EREMOTE"
};
}

void bindLaunchServices(const LaunchServices& services) { launchServices=services; }
void bindLaunchWindow(void* window) { launchServices.window=window; }
const char* currentExecutablePath() {
#ifdef _WIN32
    static char path[260]; GetModuleFileNameA(nullptr,path,sizeof(path)); return path;
#else
    if (launchServices.executablePath) return launchServices.executablePath(launchServices.context);
    throw std::logic_error("Configuration launching requires the platform executable path");
#endif
}
void ssgAllocation_free_adapter(void* allocation) { std::free(allocation); }
[[noreturn]] void engineRuntime_fatalExit_impl() { std::exit(1); }
int engineRuntime_openUrlOrExec_impl(const char* command,const char* const* arguments) {
    if(!command) return -1;
    // The saved branch tests exactly the five-byte "http:" prefix, minimizes
    // the game, waits 250 ms, and asks the platform shell to open the URL.
    if(std::strncmp(command,"http:",5)==0) {
#ifdef _WIN32
        if (launchServices.openUrl) (void)launchServices.openUrl(launchServices.context,command);
        else {
            const auto window=static_cast<HWND>(launchServices.window);
            if (window) ShowWindow(window,SW_MINIMIZE);
            Sleep(250);
            const auto result=reinterpret_cast<INT_PTR>(ShellExecuteA(window,"open",command,nullptr,nullptr,SW_SHOWNORMAL));
            if (result>32 && window) ShowWindow(window,SW_MINIMIZE);
        }
#elif defined(__EMSCRIPTEN__)
        if (launchServices.openUrl) (void)launchServices.openUrl(launchServices.context,command);
#else
        if (launchServices.openUrl) (void)launchServices.openUrl(launchServices.context,command);
#endif
        return 0;
    }
    if (launchServices.execute) return launchServices.execute(launchServices.context,command,arguments);
#if defined(_WIN32)
    return _execvp(command,const_cast<char* const*>(arguments));
#elif !defined(__EMSCRIPTEN__)
    return execvp(command,const_cast<char* const*>(arguments));
#else
    return -1;
#endif
}
char* engineRuntime_getenv_adapter(const char* name) { return std::getenv(name); }
void engineRuntime_perror_adapter(const char* prefix) { std::perror(prefix); }
const char* engineRuntime_errorString_impl(int error) {
    if(error>0x22 && error<0x48) return directInputMessages[error-0x23];
    return std::strerror(error);
}
void engineRuntime_qsort_adapter(void* base,std::size_t count,std::size_t width,
                                 int (*compare)(const void*,const void*)) {
    std::qsort(base,count,width,compare);
}
void* engineRuntime_bsearch_adapter(const void* key,const void* base,std::size_t count,
                                    std::size_t width,int (*compare)(const void*,const void*)) {
    return std::bsearch(key,base,count,width,compare);
}
char* engineRuntime_asctime_adapter(const int* fields) {
    std::tm value{};
    value.tm_sec=fields[0]; value.tm_min=fields[1]; value.tm_hour=fields[2];
    value.tm_mday=fields[3]; value.tm_mon=fields[4]; value.tm_year=fields[5];
    value.tm_wday=fields[6]; value.tm_yday=fields[7]; value.tm_isdst=fields[8];
    return std::asctime(&value);
}
char* engineTime_ctime_adapter(const std::time_t* value) { return std::ctime(value); }
int* engineRuntime_localtimeCopy_adapter(const std::time_t* value) {
    const std::tm* fields=std::localtime(value);
    if(!fields) return nullptr;
    localTimeCopy[0]=fields->tm_sec; localTimeCopy[1]=fields->tm_min;
    localTimeCopy[2]=fields->tm_hour; localTimeCopy[3]=fields->tm_mday;
    localTimeCopy[4]=fields->tm_mon; localTimeCopy[5]=fields->tm_year;
    localTimeCopy[6]=fields->tm_wday; localTimeCopy[7]=fields->tm_yday;
    localTimeCopy[8]=fields->tm_isdst;
    return localTimeCopy;
}
std::time_t engineRuntime_time_adapter(std::time_t* value) { return std::time(value); }

bool MEGUSTA_RUNTIME_THISCALL engineCtype_isalnum_adapter(void*,int character) {
    return std::isalnum(static_cast<unsigned char>(character))!=0;
}
int engineCtype_isxdigit_adapter(int character) {
    return std::isxdigit(static_cast<unsigned char>(character))!=0;
}
int engineCtype_isspace_adapter(int character) {
    return std::isspace(static_cast<unsigned char>(character))!=0;
}
bool MEGUSTA_RUNTIME_THISCALL engineCtype_isupper_adapter(void*,int character) {
    return std::isupper(static_cast<unsigned char>(character))!=0;
}
bool MEGUSTA_RUNTIME_THISCALL engineCtype_islower_adapter(void*,int character) {
    return std::islower(static_cast<unsigned char>(character))!=0;
}
int engineCtype_asciiToUpper_impl(int character) {
    if(character>0x60 && character<0x7b) character-=0x20;
    return character;
}
int engineCtype_asciiToLower_impl(int character) {
    if(character>0x40 && character<0x5b) character+=0x20;
    return character;
}
char* engineString_formatStatic_impl(const char* format,...) {
    va_list arguments; va_start(arguments,format);
    // The recovered helper uses one process-wide buffer and unbounded
    // vsprintf semantics. Grow that buffer as needed instead of inventing a
    // fixed capacity; the returned pointer remains valid until the next call.
    for (;;) {
        va_list copy;
        va_copy(copy,arguments);
        const int length=std::vsnprintf(formatBuffer.data(),formatBuffer.size(),format,copy);
        va_end(copy);
        if (length>=0 && static_cast<std::size_t>(length)<formatBuffer.size()) break;
        const std::size_t next=length>=0 ? static_cast<std::size_t>(length)+1 : formatBuffer.size()*2;
        formatBuffer.resize(next);
    }
    va_end(arguments);
    return formatBuffer.data();
}

void* engineMemory_calloc_adapter(std::size_t count,std::size_t bytes) {
    return std::calloc(count,bytes);
}
void* engineMemory_realloc_adapter(void* allocation,std::size_t bytes) {
    return std::realloc(allocation,bytes);
}
void* engineMemory_copy_impl(void* destination,const void* source,std::size_t bytes) {
    return std::memcpy(destination,source,bytes);
}
void* engineMemory_overlapSafeCopy_adapter(void* destination,const void* source,std::size_t bytes) {
    return std::memmove(destination,source,bytes);
}
void* engineMemory_findByte_adapter(const void* data,int value,std::size_t bytes) {
    return const_cast<void*>(std::memchr(data,value,bytes));
}
ExitHandler setExitHandler(ExitHandler handler) noexcept {
    ExitHandler previous=activeExitHandler;
    activeExitHandler=handler;
    return previous;
}
[[noreturn]] void engineRuntime_exit_adapter(int status) {
    if (activeExitHandler) activeExitHandler(status);
    std::exit(status);
}
int engineFile_unlink_adapter(const char* path) {
#ifdef _WIN32
    return ::_unlink(path);
#else
    return ::unlink(path);
#endif
}
} // namespace megusta::engine_runtime
