#include "resource_file.h"
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
namespace megusta {
ResourceFile::~ResourceFile() { if (!file_ || ownsFile_) close(); }
// 0043a920: object deletion is expressed through the C++ destructor by callers.
int ResourceFile::close() noexcept {
    if (file_) {
        int result = std::fclose(file_);
        file_ = nullptr;
        return result;
    }
    std::free(ownedBuffer_);
    ownedBuffer_ = nullptr;
    return 0;
}
// 0043aa30: preserve CR and CRLF translation and the bytes+1 EOF sentinel.
char* ResourceFile::getLine(char* output, int capacity) {
    if (file_) return std::fgets(output, capacity, file_);
    if (position_ < bytes_) {
        if (capacity > 0 && output) {
            int written = 0;
            if (capacity > 1) {
                while (written < capacity - 1) {
                    if (position_ >= bytes_) {
                        position_ = bytes_ + 1;
                        break;
                    }
                    char value = static_cast<char>(data_[position_++]);
                    if (value == '\r') {
                        if (position_ < bytes_ && data_[position_] == '\n') ++position_;
                        value = '\n';
                    }
                    output[written++] = value;
                    if (value == '\n') break;
                }
            }
            output[written] = 0;
            return output;
        }
    } else position_ = bytes_ + 1;
    return nullptr;
}
// 0043aae0: consume partial final elements and report only complete elements.
std::uint32_t ResourceFile::read(void* output, std::uint32_t elementSize, std::uint32_t count) {
    if (file_) return static_cast<std::uint32_t>(std::fread(output, elementSize, count, file_));
    if (position_ < bytes_) {
        std::uint32_t available = static_cast<std::uint32_t>(bytes_ - position_);
        std::uint32_t requested = elementSize * count;
        if (available < requested) requested = available;
        if (requested) {
            std::memcpy(output, data_ + position_, requested);
            position_ += requested;
            // fread sets the EOF indicator when a bulk request reaches a
            // short final element. Match that for mapped/resource buffers so
            // DTS parsers that check eof() after a read detect truncation.
            if (requested < elementSize * count) position_ = bytes_ + 1;
            return requested / elementSize;
        }
    } else position_ = bytes_ + 1;
    return 0;
}
}
namespace megusta {
int ResourceFile::eof() const { return file_ ? std::feof(file_) : position_ > bytes_; }
int ResourceFile::getc() {
    if (file_) return std::getc(file_);
    if (position_ < bytes_) return data_[position_++];
    position_ = bytes_ + 1;
    return 0; // 0043a9b0 uses zero; 0043a9f0 uses -1.
}
int ResourceFile::getchar() {
    if (file_) return std::fgetc(file_);
    if (position_ < bytes_) return data_[position_++];
    position_ = bytes_ + 1;
    return -1;
}
int ResourceFile::seek(std::int32_t offset, int origin) {
    if (file_) return std::fseek(file_, offset, origin);
    if (origin == SEEK_SET) position_ = offset;
    else if (origin == SEEK_CUR) position_ = static_cast<std::int32_t>(static_cast<std::uint32_t>(position_) + static_cast<std::uint32_t>(offset));
    else if (origin == SEEK_END) position_ = bytes_; // Original ignores offset here.
    return 0;
}
std::int32_t ResourceFile::tell() const { return file_ ? static_cast<std::int32_t>(std::ftell(file_)) : position_; }
void ResourceFile::rewind() { if (file_) std::rewind(file_); else position_ = 0; }
}
namespace megusta {
int ResourceFile::flush() {
    if (capture_) { capture_->flushLine(); if (capture_->exclusive()) return 0; }
    return file_ ? std::fflush(file_) : 0;
}
int ResourceFile::putchar(int value) {
    if (capture_) { capture_->appendCharacter(value); if (capture_->exclusive()) return 0; }
    return file_ ? std::fputc(value, file_) : 0;
}
int ResourceFile::putc(int value) { return putchar(value); }
int ResourceFile::puts(const char* text) {
    if (capture_) { capture_->appendString(text); if (capture_->exclusive()) return 0; }
    return file_ ? std::fputs(text, file_) : 0;
}
std::uint32_t ResourceFile::write(const void* bytes, std::uint32_t size, std::uint32_t count) {
    if (capture_) {
        capture_->appendBytes(static_cast<const char*>(bytes), static_cast<std::int32_t>(size * count));
        if (capture_->exclusive()) return 0;
    }
    return file_ ? static_cast<std::uint32_t>(std::fwrite(bytes, size, count, file_)) : 0;
}
int ResourceFile::printf(const char* format, ...) {
    va_list arguments; va_start(arguments, format);
    int result = vprintf(format, arguments);
    va_end(arguments); return result;
}
int ResourceFile::vprintf(const char* format, va_list arguments) {
    if (capture_) {
        va_list copy; va_copy(copy, arguments);
        capture_->appendFormattedV(format, copy); va_end(copy);
        if (capture_->exclusive()) return 0;
    }
    return file_ ? std::vfprintf(file_, format, arguments) : 0;
}
void ResourceFile::setbuf(char* buffer) { if (file_) std::setbuf(file_, buffer); }
int ResourceFile::error() const { return file_ ? std::ferror(file_) : 0; }
void ResourceFile::clearerr() { if (file_) std::clearerr(file_); }
}

namespace megusta { int ResourceFile::isatty() const {
    if (!file_) return 0;
#if defined(_WIN32)
    return _isatty(_fileno(file_));
#else
    return ::isatty(fileno(file_));
#endif
} }
