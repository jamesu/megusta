#include "resource_runtime.h"
#include "string_services.h"
#include "../game/config_literals.h"
namespace megusta {
SharedIoServices* sharedIo = nullptr;
namespace {
ResourceRuntime* activeRuntime = nullptr;
ResourceFile* openFile(const char* path, const char* mode) { return activeRuntime->mounts().open(path, mode).release(); }
int closeFile(ResourceFile* file) {
    if (!file) return 0;
    int result = file->close(); delete file; return result;
}
int formatFile(ResourceFile* file, const char* format, ...) {
    va_list arguments; va_start(arguments, format);
    int result = file->vprintf(format, arguments);
    va_end(arguments); return result;
}
// 0043ae00: capture stdout formatting, respecting exclusive capture mode.
int printStandard(const char* format, ...) {
    va_list arguments; va_start(arguments, format);
    int result = sharedIo->standardOutput->vprintf(format, arguments);
    va_end(arguments); return result;
}
}
ResourceRuntime::ResourceRuntime()
    : input_(stdin, nullptr, false), output_(stdout, &capture_, false), error_(stderr, &capture_, false),
      services_{
        openFile, closeFile,
        [](ResourceFile* f) { return f->eof(); },
        [](ResourceFile* f) { return f->flush(); },
        [](ResourceFile* f) { return f->getchar(); },
        [](char* text, int count, ResourceFile* f) { return f->getLine(text, count); },
        formatFile,
        [](int c, ResourceFile* f) { return f->putchar(c); },
        [](const char* text, ResourceFile* f) { return f->puts(text); },
        [](void* data, std::uint32_t size, std::uint32_t count, ResourceFile* f) { return f->read(data, size, count); },
        [](ResourceFile* f, std::int32_t offset, int origin) { return f->seek(offset, origin); },
        [](ResourceFile* f) { return f->tell(); },
        [](const void* data, std::uint32_t size, std::uint32_t count, ResourceFile* f) { return f->write(data, size, count); },
        [](ResourceFile* f) { return f->getc(); },
        [](ResourceFile* f) { return f->isatty(); },
        [](int c, ResourceFile* f) { return f->putc(c); },
        printStandard,
        [](ResourceFile* f) { f->rewind(); },
        [](ResourceFile* f, char* buffer) { f->setbuf(buffer); },
        [](ResourceFile* f, const char* format, va_list args) { return f->vprintf(format, args); },
        [](ResourceFile* f) { f->clearerr(); },
        [](ResourceFile* f) { return f->error(); },
        [](const char* path, ResourceStatus& status) { return activeRuntime->mounts().queryStat(path, status); },
        {resourceDirectoryOpen, resourceDirectoryNext, resourceDirectoryClose},
        unsupportedResourceOperation, &input_, &output_, &error_} {}
ResourceRuntime::~ResourceRuntime() {
    if (activeRuntime == this) { activeRuntime = nullptr; sharedIo = nullptr; }
}
// 0043a5d0: initialize capture, publish the shared wrappers, then apply -mod mounts.
void ResourceRuntime::initializeFromArgs(int argc, const char* const* argv) {
    capture_.initializeFromArgs(argc, argv);
    mounts_.resetMountCount();
    activeRuntime = this;
    sharedIo = &services_;
    for (int i = 1; i < argc; ++i) {
        if (strings::compareInsensitive(argv[i], config_literals::mountOption) == 0 && i + 1 < argc)
            mounts_.add("data/", argv[++i]);
    }
}
}
