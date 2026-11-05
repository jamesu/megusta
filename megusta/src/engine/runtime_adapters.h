#pragma once
#include <cstddef>
#include <ctime>

#if defined(_MSC_VER)
#define MEGUSTA_RUNTIME_THISCALL __thiscall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define MEGUSTA_RUNTIME_THISCALL __attribute__((thiscall))
#else
#define MEGUSTA_RUNTIME_THISCALL
#endif

namespace megusta::engine_runtime {
using ExitHandler = void (*)(int status);
struct LaunchServices {
    void* context=nullptr;
    void* window=nullptr;
    int (*openUrl)(void*,const char*)=nullptr;
    int (*execute)(void*,const char*,const char* const*)=nullptr;
    const char* (*executablePath)(void*)=nullptr;
};
void bindLaunchServices(const LaunchServices&);
void bindLaunchWindow(void* window);
const char* currentExecutablePath();

void ssgAllocation_free_adapter(void* allocation);
[[noreturn]] void engineRuntime_fatalExit_impl();
int engineRuntime_openUrlOrExec_impl(const char* command,const char* const* arguments);
char* engineRuntime_getenv_adapter(const char* name);
void engineRuntime_perror_adapter(const char* prefix);
const char* engineRuntime_errorString_impl(int error);
void engineRuntime_qsort_adapter(void* base,std::size_t count,std::size_t width,
                                 int (*compare)(const void*,const void*));
void* engineRuntime_bsearch_adapter(const void* key,const void* base,std::size_t count,
                                    std::size_t width,int (*compare)(const void*,const void*));
char* engineRuntime_asctime_adapter(const int* fields);
char* engineTime_ctime_adapter(const std::time_t* value);
int* engineRuntime_localtimeCopy_adapter(const std::time_t* value);
std::time_t engineRuntime_time_adapter(std::time_t* value);

bool MEGUSTA_RUNTIME_THISCALL engineCtype_isalnum_adapter(void* unusedThis,int character);
int engineCtype_isxdigit_adapter(int character);
int engineCtype_isspace_adapter(int character);
bool MEGUSTA_RUNTIME_THISCALL engineCtype_isupper_adapter(void* unusedThis,int character);
bool MEGUSTA_RUNTIME_THISCALL engineCtype_islower_adapter(void* unusedThis,int character);
int engineCtype_asciiToUpper_impl(int character);
int engineCtype_asciiToLower_impl(int character);
char* engineString_formatStatic_impl(const char* format,...);

void* engineMemory_calloc_adapter(std::size_t count,std::size_t bytes);
void* engineMemory_realloc_adapter(void* allocation,std::size_t bytes);
void* engineMemory_copy_impl(void* destination,const void* source,std::size_t bytes);
void* engineMemory_overlapSafeCopy_adapter(void* destination,const void* source,std::size_t bytes);
void* engineMemory_findByte_adapter(const void* data,int value,std::size_t bytes);
[[noreturn]] void engineRuntime_exit_adapter(int status);
ExitHandler setExitHandler(ExitHandler handler) noexcept;
int engineFile_unlink_adapter(const char* path);
} // namespace megusta::engine_runtime

