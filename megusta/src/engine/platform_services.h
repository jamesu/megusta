#pragma once
#include <cstdint>
#include "file_status.h"
#include "directory_services.h"

namespace megusta::engine_platform {

using FileStatus=EngineFileStatus;
struct DirectoryEntry {
    char name[256];
    std::int32_t isDirectory;
};
struct DirectoryContext {
    char path[256];
    EngineDirectoryIterator* platformIterator;
    DirectoryEntry entry;
};

char* enginePath_joinNormalize_impl(char* output,const char* base,const char* name);
int engineFile_queryStat_impl(const char* path,FileStatus* status);
std::int32_t engineFile_getSize_impl(const char* path);
void engineDiagnostic_printWithSeverity_impl(int severity,const char* message);
void engineDiagnostic_print_adapter(int unused,const char* message);
void engineWindow_reportLastError_impl();
DirectoryContext* engineDirectory_openIterator_impl(char* path);
DirectoryEntry* engineDirectory_nextEntry_impl(DirectoryContext* context);

} // namespace megusta::engine_platform
