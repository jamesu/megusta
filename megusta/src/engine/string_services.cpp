#include "string_services.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#if !defined(_WIN32)
#include <strings.h>
#endif
namespace megusta { namespace strings {
std::size_t length(const char* value) { return std::strlen(value); }
char* copyBounded(char* output, const char* value, std::size_t count) { return std::strncpy(output, value, count); }
char* concat(char* output, const char* value) { return std::strcat(output, value); }
char* concatBounded(char* output, const char* value, std::size_t count) { return std::strncat(output, value, count); }
// Original 004d9f70 returns -1/0/+1, rather than the byte difference.
int compare(const char* left, const char* right) {
    const auto* a = reinterpret_cast<const unsigned char*>(left);
    const auto* b = reinterpret_cast<const unsigned char*>(right);
    while (*a == *b) { if (!*a) return 0; ++a; ++b; }
    return *a < *b ? -1 : 1;
}
int compareInsensitive(const char* left, const char* right) {
#if defined(_WIN32)
    return _stricmp(left, right);
#else
    return strcasecmp(left, right);
#endif
}
int compareBounded(const char* left, const char* right, std::size_t count) { return std::strncmp(left, right, count); }
int compareInsensitiveBounded(const char* left, const char* right, std::size_t count) {
#if defined(_WIN32)
    return _strnicmp(left, right, count);
#else
    return strncasecmp(left, right, count);
#endif
}
const char* findChar(const char* text, int character) { return std::strchr(text, character); }
const char* findLastChar(const char* text, int character) { return std::strrchr(text, character); }
char* tokenize(char* text, const char* delimiters) { return std::strtok(text, delimiters); }
std::size_t spanAccepted(const char* text, const char* accepted) { return std::strspn(text, accepted); }
const char* findSubstring(const char* text, const char* pattern) { return std::strstr(text, pattern); }
char* uppercase(char* text) {
#if defined(_WIN32)
    return _strupr(text);
#else
    for (char* p=text; *p; ++p) *p=static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
    return text;
#endif
}
long toLong(const char* text, char** end, int base) { return std::strtol(text, end, base); }
double parseDouble(const char* text, char** end) { return std::strtod(text, end); }
int toInt(const char* text) { return std::atoi(text); }
double toDouble(const char* text) { return std::atof(text); }
bool isAlpha(int character) { return std::isalpha(character) != 0; }
bool isDigit(int character) { return std::isdigit(character) != 0; }
int vformat(char* output, const char* format, va_list arguments) { return std::vsprintf(output, format, arguments); }
int scanBuffer(const char* input,const char* format,...) {
    unsigned count=0; for (auto* cursor=format;*cursor;++cursor) if (*cursor=='%' && cursor[1]!='%') ++count;
    if (!count || count>20) return 0;
    va_list arguments; va_start(arguments,format); const int result=std::vsscanf(input,format,arguments); va_end(arguments); return result;
}
char* duplicate(const char* text) {
#if defined(_WIN32)
    return _strdup(text);
#else
    return ::strdup(text);
#endif
}
} }
