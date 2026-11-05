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

#ifndef WIN32
#define WIN32 1
#endif
#ifndef MS_WIN32
#define MS_WIN32 1
#endif
#ifndef MS_WINDOWS
#define MS_WINDOWS 1
#endif
#ifndef USE_SOCKET
#define USE_SOCKET 1
#endif

#include "../../upstream/Python-1.5.2/PC/config.h"

/* The game is a static embed, not the upstream python15 DLL. */
#undef MS_COREDLL
#undef USE_DL_IMPORT
#undef USE_DL_EXPORT
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

#endif /* MEGUSTA_PYTHON_CONFIG_H */
