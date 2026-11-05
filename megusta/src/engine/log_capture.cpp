#include "log_capture.h"
#include "string_services.h"
#include <cstring>
#include <cstdio>
#include <cctype>
namespace megusta {
void LogCapture::initializeFromArgs(int argc, const char* const* argv) {
    sink_ = nullptr; sinkContext_ = nullptr; lineLength_ = 0;
    includePrefixes_.clear(); excludePrefixes_.clear();
    hasIncludes_ = hasExcludes_ = exclusive_ = false;
    for (int i = 1; i < argc; ++i) {
        if (!strings::compareInsensitive(argv[i], "-log_include")) {
            if (i + 1 < argc) { const char* value = argv[++i]; hasIncludes_ = value != nullptr; includePrefixes_ = value ? value : ""; }
        } else if (!strings::compareInsensitive(argv[i], "-log_exclude")) {
            if (i + 1 < argc) { const char* value = argv[++i]; hasExcludes_ = value != nullptr; excludePrefixes_ = value ? value : ""; }
        } else if (!strings::compareInsensitive(argv[i], "-log_exclusive")) exclusive_ = true;
    }
}
bool LogCapture::acceptLine(const char* text) const {
    const auto hasPrefix = [](const char* value) {
        while (strings::isAlpha(static_cast<unsigned char>(*value))) ++value;
        return *value == ':';
    };
    const auto matches = [text](const std::string& prefixes) {
        const char* current = prefixes.c_str();
        while (const char* end = std::strchr(current, ':')) {
            if (!strings::compareInsensitiveBounded(text, current, end - current + 1)) return true;
            current = end + 1;
        }
        return false;
    };
    if (hasExcludes_) {
        if (!hasPrefix(text)) return true;
        if (matches(excludePrefixes_)) return false;
    }
    if (hasIncludes_ && hasPrefix(text)) return matches(includePrefixes_);
    return true;
}
void LogCapture::flushLine() {
    if (sink_ && lineLength_ > 0 && acceptLine(line_)) {
        line_[lineLength_] = 0;
        sink_(line_, sinkContext_);
    }
    lineLength_ = 0;
}
void LogCapture::appendCharacter(int value) {
    if (value == 10 || value == 13) flushLine();
    if (lineLength_ < 255 && value > 31 && value < 127) line_[lineLength_++] = static_cast<char>(value);
}
void LogCapture::appendString(const char* text) {
    if (text) while (*text) appendCharacter(*text++);
}
void LogCapture::appendFormattedV(const char* format, va_list arguments) {
    char text[1024]; strings::vformat(text, format, arguments); appendString(text);
}
void LogCapture::appendBytes(const char* bytes, int count) {
    if (bytes) while (count > 0) { appendCharacter(*bytes++); --count; }
}
}
