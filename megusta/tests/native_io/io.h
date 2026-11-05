// Compatibility for running the DTS diagnostic natively on POSIX hosts.
#pragma once
#include <cstdio>
#include <unistd.h>
inline int _isatty(int fd) { return ::isatty(fd); }
inline int _fileno(std::FILE* file) { return ::fileno(file); }
