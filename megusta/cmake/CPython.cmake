set(MEGUSTA_PYTHON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/upstream/Python-1.5.2")
if(NOT EXISTS "${MEGUSTA_PYTHON_ROOT}/PCbuild/python15.dsp")
  message(FATAL_ERROR "Vendored CPython 1.5.2 sources are missing")
endif()
# Keep the embedded runtime source closure explicit here. Recovery manifests
# document the inventory but are not required to configure or build the game.
if(WIN32)
  set(MEGUSTA_PYTHON_PLATFORM_UNITS PC/dl_nt.c PC/getpathp.c PC/import_nt.c)
else()
  # Browser build uses CPython's POSIX path and OS module ports; runtime
  # extension loading remains disabled by the recovered NO_DYNAMIC_LINK path.
  set(MEGUSTA_PYTHON_PLATFORM_UNITS Modules/getpath.c)
endif()
set(MEGUSTA_PYTHON_UNITS
  Modules/arraymodule.c
  Modules/binascii.c
  Modules/cPickle.c
  Modules/cStringIO.c
  Modules/cmathmodule.c
  Modules/errnomodule.c
  Modules/mathmodule.c
  Modules/md5c.c
  Modules/md5module.c
  Modules/newmodule.c
  Modules/operator.c
  Modules/regexmodule.c
  Modules/regexpr.c
  Modules/rotormodule.c
  Modules/shamodule.c
  Modules/socketmodule.c
  Modules/soundex.c
  Modules/stropmodule.c
  Modules/structmodule.c
  Modules/timemodule.c
  Objects/abstract.c
  Objects/bufferobject.c
  Objects/classobject.c
  Objects/cobject.c
  Objects/complexobject.c
  Objects/dictobject.c
  Objects/fileobject.c
  Objects/floatobject.c
  Objects/frameobject.c
  Objects/funcobject.c
  Objects/intobject.c
  Objects/listobject.c
  Objects/longobject.c
  Objects/methodobject.c
  Objects/moduleobject.c
  Objects/object.c
  Objects/rangeobject.c
  Objects/sliceobject.c
  Objects/stringobject.c
  Objects/tupleobject.c
  Objects/typeobject.c
  Parser/acceler.c
  Parser/bitset.c
  Parser/grammar1.c
  Parser/listnode.c
  Parser/metagrammar.c
  Parser/myreadline.c
  Parser/node.c
  Parser/parser.c
  Parser/parsetok.c
  Parser/tokenizer.c
  Python/bltinmodule.c
  Python/ceval.c
  Python/compile.c
  Python/errors.c
  Python/frozen.c
  Python/getargs.c
  Python/getcompiler.c
  Python/getcopyright.c
  Python/getmtime.c
  Python/getopt.c
  Python/getplatform.c
  Python/getversion.c
  Python/graminit.c
  Python/import.c
  Python/marshal.c
  Python/modsupport.c
  Python/mystrtoul.c
  Python/pyfpe.c
  Python/pystate.c
  Python/pythonrun.c
  Python/structmember.c
  Python/sysmodule.c
  Python/traceback.c
  Python/importdl.c
  Modules/getbuildinfo.c
  ${MEGUSTA_PYTHON_PLATFORM_UNITS})
set(MEGUSTA_PYTHON_SOURCES)
foreach(unit IN LISTS MEGUSTA_PYTHON_UNITS)
  set(source "${MEGUSTA_PYTHON_ROOT}/${unit}")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "Required CPython source unit is missing: ${unit}")
  endif()
  list(APPEND MEGUSTA_PYTHON_SOURCES "${source}")
endforeach()
list(APPEND MEGUSTA_PYTHON_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/python/builtin_config.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/python/interrupt_stubs.c")

add_library(megusta_python STATIC ${MEGUSTA_PYTHON_SOURCES})
# Saved 0049fa40 is the upstream unsupported-dynamic-module branch, even on
# Win32. Saved 004b5190 reports this fixed original build identity.
set_source_files_properties("${MEGUSTA_PYTHON_ROOT}/Python/importdl.c" PROPERTIES
  COMPILE_DEFINITIONS NO_DYNAMIC_LINK)
set_source_files_properties("${MEGUSTA_PYTHON_ROOT}/Modules/getbuildinfo.c" PROPERTIES
  COMPILE_DEFINITIONS "BUILD=0;DATE=\"Aug 14 2001\";TIME=\"16:17:42\"")
target_include_directories(megusta_python BEFORE PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/python")
target_include_directories(megusta_python PUBLIC
  "${MEGUSTA_PYTHON_ROOT}/Include"
  "${MEGUSTA_PYTHON_ROOT}/PC"
  "${MEGUSTA_PYTHON_ROOT}/Python"
  "${MEGUSTA_PYTHON_ROOT}/Parser"
  "${MEGUSTA_PYTHON_ROOT}/Objects"
  "${MEGUSTA_PYTHON_ROOT}/Modules")
target_compile_definitions(megusta_python PRIVATE
  NDEBUG USE_DL_EXPORT)
if(EMSCRIPTEN AND MEGUSTA_BROWSER)
  target_compile_definitions(megusta_python PRIVATE MEGUSTA_BROWSER_RELOAD_ON_EXIT=1)
endif()
set_target_properties(megusta_python PROPERTIES
  C_STANDARD 90
  C_STANDARD_REQUIRED OFF
  C_EXTENSIONS ON)
if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  # The original x86 build uses x87. x64 uses the platform's native FP ABI.
  target_compile_options(megusta_python PRIVATE -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT EMSCRIPTEN)
    target_compile_options(megusta_python PRIVATE -mfpmath=387 -mno-sse -mno-sse2)
  endif()
elseif(MSVC)
  target_compile_options(megusta_python PRIVATE /O2)
  set_property(TARGET megusta_python PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
if(WIN32)
  target_link_libraries(megusta_python PUBLIC advapi32 winmm wsock32)
endif()
if(MSVC)
  # PC/config.h carries an automatic python15 DLL import-library directive.
  # This target is a static embed, so ignore that default-library pragma even
  # if a stale object or direct upstream config include still emits it.
  target_link_options(megusta_python INTERFACE
    "/NODEFAULTLIB:python15.lib" "/NODEFAULTLIB:python15_d.lib")
endif()
