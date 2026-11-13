// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdarg>
#include <string>
namespace megusta {
class LogCapture {
public:
    using Sink = void (*)(const char*, void*);
    void initializeFromArgs(int argc, const char* const* argv);
    bool acceptLine(const char* text) const;
    void flushLine();
    void appendCharacter(int value);
    void appendString(const char* text);
    void appendFormattedV(const char* format, va_list arguments);
    void appendBytes(const char* bytes, int count);
    void setSink(Sink sink, void* context) { sink_ = sink; sinkContext_ = context; }
    bool exclusive() const { return exclusive_; }
private:
    Sink sink_ = nullptr;
    void* sinkContext_ = nullptr;
    char line_[256] = {};
    unsigned lineLength_ = 0;
    std::string includePrefixes_, excludePrefixes_;
    bool hasIncludes_ = false, hasExcludes_ = false, exclusive_ = false;
};
}
