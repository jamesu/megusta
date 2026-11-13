// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstddef>
#include <cstdarg>
namespace megusta { namespace strings {
std::size_t length(const char* value);
char* copyBounded(char* output, const char* value, std::size_t count);
char* concat(char* output, const char* value);
char* concatBounded(char* output, const char* value, std::size_t count);
int compare(const char* left, const char* right);
int compareInsensitive(const char* left, const char* right);
int compareBounded(const char* left, const char* right, std::size_t count);
int compareInsensitiveBounded(const char* left, const char* right, std::size_t count);
const char* findChar(const char* text, int character);
const char* findLastChar(const char* text, int character);
char* tokenize(char* text, const char* delimiters);
std::size_t spanAccepted(const char* text, const char* accepted);
const char* findSubstring(const char* text, const char* pattern);
char* uppercase(char* text);
long toLong(const char* text, char** end, int base);
double parseDouble(const char* text, char** end);
int toInt(const char* text);
double toDouble(const char* text);
bool isAlpha(int character);
bool isDigit(int character);
int vformat(char* output, const char* format, va_list arguments);
int scanBuffer(const char* input,const char* format,...);
char* duplicate(const char* text);
} }
