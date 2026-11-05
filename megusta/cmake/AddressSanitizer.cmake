option(MEGUSTA_ENABLE_ASAN "Instrument game and vendored libraries with AddressSanitizer" OFF)
if(MEGUSTA_ENABLE_ASAN)
  if(NOT MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(MSVC|Clang)$")
    message(FATAL_ERROR "This ASan configuration requires MSVC or clang-cl with the Windows MSVC ABI")
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION LESS 1928)
    message(FATAL_ERROR "AddressSanitizer requires Visual Studio 2019 16.9 or newer")
  endif()
  # CMake's Debug defaults include /RTC1, which conflicts with ASan.
  foreach(language C CXX)
    foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
      set(flag_variable "CMAKE_${language}_FLAGS_${configuration}")
      string(REGEX REPLACE "/RTC[^ ]*" "" cleaned_flags "${${flag_variable}}")
      string(REPLACE "/ZI" "/Zi" cleaned_flags "${cleaned_flags}")
      set(${flag_variable} "${cleaned_flags}" CACHE STRING "Compiler flags" FORCE)
    endforeach()
  endforeach()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fsanitize=address /Zi)
    # The Visual Studio generator invokes lld-link directly; it does not route
    # this driver option through clang-cl. Link the x86 dynamic ASan runtime
    # archives explicitly, including the thunk archive whose init members must
    # be retained for a correct DLL-CRT build.
    set(MEGUSTA_ASAN_LIBRARY_DIRS)
    execute_process(
      COMMAND "${CMAKE_CXX_COMPILER}" --print-resource-dir
      RESULT_VARIABLE MEGUSTA_ASAN_RESOURCE_RESULT
      OUTPUT_VARIABLE MEGUSTA_ASAN_RESOURCE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    if(MEGUSTA_ASAN_RESOURCE_RESULT EQUAL 0 AND MEGUSTA_ASAN_RESOURCE_DIR)
      list(APPEND MEGUSTA_ASAN_LIBRARY_DIRS
        "${MEGUSTA_ASAN_RESOURCE_DIR}/lib/windows")
    endif()
    if(DEFINED ENV{VCToolsInstallDir})
      list(APPEND MEGUSTA_ASAN_LIBRARY_DIRS
        "$ENV{VCToolsInstallDir}/lib/x86")
    endif()
    # Visual Studio installs its Windows ASan archives beside the MSVC
    # toolset, while clang-cl itself may report only its LLVM resource dir.
    # Derive the VC root from .../VC/Tools/Llvm/<host>/bin/clang-cl.exe so
    # configuration also works when VCToolsInstallDir is not exported.
    get_filename_component(MEGUSTA_CLANG_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(MEGUSTA_LLVM_HOST_DIR "${MEGUSTA_CLANG_BIN_DIR}" DIRECTORY)
    get_filename_component(MEGUSTA_LLVM_DIR "${MEGUSTA_LLVM_HOST_DIR}" DIRECTORY)
    get_filename_component(MEGUSTA_VC_TOOLS_DIR "${MEGUSTA_LLVM_DIR}" DIRECTORY)
    get_filename_component(MEGUSTA_VC_ROOT "${MEGUSTA_VC_TOOLS_DIR}" DIRECTORY)
    file(GLOB MEGUSTA_MSVC_TOOLSET_DIRS LIST_DIRECTORIES true
      "${MEGUSTA_VC_ROOT}/Tools/MSVC/*")
    list(SORT MEGUSTA_MSVC_TOOLSET_DIRS COMPARE NATURAL ORDER DESCENDING)
    foreach(toolset_dir IN LISTS MEGUSTA_MSVC_TOOLSET_DIRS)
      list(APPEND MEGUSTA_ASAN_LIBRARY_DIRS "${toolset_dir}/lib/x86")
    endforeach()
    # Require the exact .lib filenames so the runtime DLL itself can never be
    # selected as a linker input. The dynamic runtime is an import library;
    # the thunk is a static archive.
    # Clear values cached by the previous find_library()-based setup so a
    # reconfigure cannot keep a stale DLL/import path.
    unset(MEGUSTA_ASAN_DYNAMIC_LIBRARY CACHE)
    unset(MEGUSTA_ASAN_DYNAMIC_THUNK_LIBRARY CACHE)
    find_file(MEGUSTA_ASAN_DYNAMIC_LIBRARY
      NAMES clang_rt.asan_dynamic-i386.lib
      PATHS ${MEGUSTA_ASAN_LIBRARY_DIRS} NO_DEFAULT_PATH)
    find_file(MEGUSTA_ASAN_DYNAMIC_THUNK_LIBRARY
      NAMES clang_rt.asan_dynamic_runtime_thunk-i386.lib
      PATHS ${MEGUSTA_ASAN_LIBRARY_DIRS} NO_DEFAULT_PATH)
    if(NOT MEGUSTA_ASAN_DYNAMIC_LIBRARY OR NOT MEGUSTA_ASAN_DYNAMIC_THUNK_LIBRARY)
      message(FATAL_ERROR
        "ClangCL x86 ASan runtime libraries were not found. Install Visual Studio's C++ AddressSanitizer component. Searched: ${MEGUSTA_ASAN_LIBRARY_DIRS}")
    endif()
    # The dynamic runtime import library is linked normally. Only the thunk
    # archive must be force-loaded, as documented by Microsoft. Keep the import
    # library out of /WHOLEARCHIVE; this is the documented /MD link setup and
    # avoids the SafeSEH and oversized .data failures from the reported link.
    add_link_options(
      "${MEGUSTA_ASAN_DYNAMIC_LIBRARY}"
      "/WHOLEARCHIVE:${MEGUSTA_ASAN_DYNAMIC_THUNK_LIBRARY}")
    message(STATUS "ClangCL x86 ASan import library: ${MEGUSTA_ASAN_DYNAMIC_LIBRARY}")
    message(STATUS "ClangCL x86 ASan thunk library: ${MEGUSTA_ASAN_DYNAMIC_THUNK_LIBRARY}")
  else()
    add_compile_options(/fsanitize=address /Zi)
  endif()
  add_link_options(/INCREMENTAL:NO /DEBUG)
endif()
