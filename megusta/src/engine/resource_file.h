#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include "log_capture.h"
namespace megusta {
// Shared representation for native files and borrowed archive data.
class ResourceFile {
public:
    explicit ResourceFile(std::FILE* file, LogCapture* capture = nullptr, bool ownsFile = true) noexcept
        : file_(file), capture_(capture), ownsFile_(ownsFile) {}
    ResourceFile(const void* data, std::int32_t bytes, void* ownedBuffer = nullptr) noexcept
        : data_(static_cast<const unsigned char*>(data)), bytes_(bytes), ownedBuffer_(ownedBuffer) {}
    ~ResourceFile();
    ResourceFile(const ResourceFile&) = delete;
    ResourceFile& operator=(const ResourceFile&) = delete;
    int close() noexcept;
    int eof() const;
    int getc();
    int getchar();
    int seek(std::int32_t offset, int origin);
    std::int32_t tell() const;
    void rewind();
    int flush();
    int putchar(int value);
    int putc(int value);
    int puts(const char* text);
    std::uint32_t write(const void* bytes, std::uint32_t size, std::uint32_t count);
    int printf(const char* format, ...);
    int vprintf(const char* format, va_list arguments);
    void setbuf(char* buffer);
    int error() const;
    void clearerr();
    int isatty() const;
    char* getLine(char* output, int capacity);
    std::uint32_t read(void* output, std::uint32_t elementSize, std::uint32_t count);
private:
    std::FILE* file_ = nullptr;
    const unsigned char* data_ = nullptr;
    std::int32_t bytes_ = 0;
    std::int32_t position_ = 0;
    void* ownedBuffer_ = nullptr;
    LogCapture* capture_ = nullptr; // Set only on the wrapped standard output/error streams.
    bool ownsFile_ = true;
};
}
