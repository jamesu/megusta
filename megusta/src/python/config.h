/*
 * Game-specific CPython 1.5.2 Win32 configuration.
 *
 * The preserved stock PC/config.h enables WITH_THREAD only in its MSVC and
 * Watcom branches. This MinGW-targeted Clang build does not claim _MSC_VER;
 * explicitly provide the evidenced Win32 platform features and keep the
 * game's OS thread backend disabled.
 */
#ifndef MEGUSTA_PYTHON_CONFIG_H
#define MEGUSTA_PYTHON_CONFIG_H

#if defined(__EMSCRIPTEN__)
/* wasm32 CPython embed: POSIX libc, 32-bit pointers/long, no OS threads. */
#define HAVE_LIMITS_H 1
#define HAVE_HYPOT 1
#define HAVE_CLOCK 1
#define HAVE_STRFTIME 1
#define HAVE_STRERROR 1
#define HAVE_LONG_LONG 1
#define LONG_LONG long long
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_VOID_P 4
#define WORD_BIT 32
#define COMPILER "[Emscripten wasm32]"
#define PYTHONPATH "/"
#define DONT_HAVE_SIG_ALARM 1
#define DONT_HAVE_SIG_PAUSE 1
#define HAVE_PROTOTYPES 1
#define HAVE_STDARG_PROTOTYPES 1
#define HAVE_STDLIB_H 1
#define HAVE_STDDEF_H 1
#define HAVE_FCNTL_H 1
#define HAVE_UNISTD_H 1
#define HAVE_GETPID 1
#define HAVE_PUTENV 1
#define HAVE_MKTIME 1
#define HAVE_SETVBUF 1
#define HAVE_FTIME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_TIME_H 1
#define TIME_WITH_SYS_TIME 1
#define HAVE_SIGNAL_H 1
#define HAVE_ERRNO_H 1
#define Py_PTR_FMT "%lx"
#define Py_PTR_FMT_ARG(p) ((unsigned long)(p))
#define Py_PTR_HASH(p) ((long)(p))
#else
#ifndef WIN32
#define WIN32 1
#endif
#if !defined(_MSC_VER)
/* The stock Microsoft config supplies these itself under clang-cl/MSVC. */
#ifndef MS_WIN32
#define MS_WIN32 1
#endif
#ifndef MS_WINDOWS
#define MS_WINDOWS 1
#endif
#ifndef USE_SOCKET
#define USE_SOCKET 1
#endif
#endif

/* Prevent PC/config.h from declaring static embedded objects dllimport. */
#define MS_NO_COREDLL 1
/* PC/config.h also auto-links python15.lib unless this DLL-build guard is set.
   DL_IMPORT/DL_EXPORT are reset below because this target is a static embed. */
#ifndef USE_DL_EXPORT
#define USE_DL_EXPORT 1
#endif
#include "../../upstream/Python-1.5.2/PC/config.h"

/* The stock VC6 config recognizes 32-bit x86 and Alpha, not Windows x64.
   Keep its LLP64 long size (4) but report native 64-bit pointers so CPython's
   existing PyLong_FromVoidPtr/PyLong_AsVoidPtr paths are selected. */
#if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64)
#undef SIZEOF_VOID_P
#define SIZEOF_VOID_P 8
#endif

/* Preserve the old address formatting on 32-bit builds while avoiding
   pointer-to-long truncation in x64 diagnostic repr strings. */
#if SIZEOF_VOID_P > SIZEOF_LONG
#define Py_PTR_FMT "%p"
#define Py_PTR_FMT_ARG(p) ((void *)(p))
#define Py_PTR_HASH(p) \
	((long)(((unsigned LONG_LONG)(p)) ^ \
	       (((unsigned LONG_LONG)(p)) >> (8 * SIZEOF_LONG))))
#else
#define Py_PTR_FMT "%lx"
#define Py_PTR_FMT_ARG(p) ((unsigned long)(p))
#define Py_PTR_HASH(p) ((long)(p))
#endif

/* The game is a static embed, not the upstream python15 DLL. */
#undef MS_COREDLL
#undef USE_DL_IMPORT
#undef USE_DL_EXPORT
#undef DL_IMPORT
#define DL_IMPORT(RTYPE) RTYPE
#undef DL_EXPORT
#define DL_EXPORT(RTYPE) RTYPE
#undef WITH_THREAD
#undef NT_THREADS
/* PC/config.h sets this for an old compiler quirk that Clang does not have. */
#undef BAD_STATIC_FORWARD

#ifndef COMPILER
#define COMPILER "[ANSI C]"
#endif
#ifndef PYTHONPATH
#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
#endif
#ifndef HAVE_CLOCK
#define HAVE_CLOCK 1
#endif
#ifndef HAVE_STRFTIME
#define HAVE_STRFTIME 1
#endif
#ifndef HAVE_STRERROR
#define HAVE_STRERROR 1
#endif
#ifndef HAVE_LONG_LONG
#define HAVE_LONG_LONG 1
#endif
#ifndef LONG_LONG
#define LONG_LONG long long
#endif
#ifndef WORD_BIT
#define WORD_BIT 32
#endif
#endif

#endif /* MEGUSTA_PYTHON_CONFIG_H */
