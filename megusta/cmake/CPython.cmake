set(MEGUSTA_PYTHON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/upstream/Python-1.5.2")
set(MEGUSTA_PYTHON_SCOPE "${CMAKE_CURRENT_SOURCE_DIR}/manifests/python_build_scope.json")
if(NOT EXISTS "${MEGUSTA_PYTHON_ROOT}/PCbuild/python15.dsp")
  message(FATAL_ERROR "Vendored CPython 1.5.2 sources are missing")
endif()
if(NOT EXISTS "${MEGUSTA_PYTHON_SCOPE}")
  message(FATAL_ERROR "The recovered CPython source allowlist is missing")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MEGUSTA_PYTHON_SCOPE}")
file(READ "${MEGUSTA_PYTHON_SCOPE}" MEGUSTA_PYTHON_SCOPE_JSON)
string(JSON MEGUSTA_PYTHON_SOURCE_COUNT LENGTH "${MEGUSTA_PYTHON_SCOPE_JSON}" source_units)
math(EXPR MEGUSTA_PYTHON_SOURCE_LAST "${MEGUSTA_PYTHON_SOURCE_COUNT} - 1")
set(MEGUSTA_PYTHON_SOURCES)
foreach(i RANGE ${MEGUSTA_PYTHON_SOURCE_LAST})
  string(JSON unit GET "${MEGUSTA_PYTHON_SCOPE_JSON}" source_units ${i})
  set(source "${MEGUSTA_PYTHON_ROOT}/${unit}")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "CPython source unit from manifest is missing: ${unit}")
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
set_target_properties(megusta_python PROPERTIES
  C_STANDARD 90
  C_STANDARD_REQUIRED OFF
  C_EXTENSIONS ON)
if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  # Upstream python15 Release uses /O2. The x86 MSVC 5/6 target uses x87;
  # preserve that arithmetic profile and the old compiler's aliasing behavior.
  target_compile_options(megusta_python PRIVATE
    -O2 -mfpmath=387 -mno-sse -mno-sse2
    -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
elseif(MSVC)
  target_compile_options(megusta_python PRIVATE /O2)
  set_property(TARGET megusta_python PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
target_link_libraries(megusta_python PUBLIC advapi32 winmm wsock32)
if(MSVC)
  # PC/config.h carries an automatic python15 DLL import-library directive.
  # This target is a static embed, so ignore that default-library pragma even
  # if a stale object or direct upstream config include still emits it.
  target_link_options(megusta_python INTERFACE
    "/NODEFAULTLIB:python15.lib" "/NODEFAULTLIB:python15_d.lib")
endif()
