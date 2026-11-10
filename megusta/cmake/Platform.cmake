# No project includes escape megusta; all OS handles stay behind the adapter.


if(EMSCRIPTEN AND MEGUSTA_BROWSER)
  if(NOT DEFINED MEGUSTA_GL4ES_DIR)
    set(MEGUSTA_GL4ES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/gl4es")
  endif()
  if(NOT EXISTS "${MEGUSTA_GL4ES_DIR}/include/GL/gl.h")
    message(FATAL_ERROR "gl4es headers not found: ${MEGUSTA_GL4ES_DIR}/include/GL/gl.h")
  endif()
endif()

function(megusta_platform_use_gl4es target)
  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    # BEFORE ensures <GL/gl.h> resolves to gl4es's symbol-mangling header.
    target_include_directories(${target} BEFORE PRIVATE "${MEGUSTA_GL4ES_DIR}/include")
  endif()
endfunction()



add_library(megusta_platform STATIC src/platform/runtime.cpp src/platform/headless/headless_runtime.cpp)
target_include_directories(megusta_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(megusta_platform PUBLIC cxx_std_17)
if(EMSCRIPTEN)
  target_compile_definitions(megusta_platform PUBLIC MEGUSTA_PORTABLE_NUMERIC=1)
endif()
if(WIN32)
  target_sources(megusta_platform PRIVATE src/platform/windows/win32_runtime.cpp)
  # DirectInput is loaded dynamically by win32.cpp (DINPUT.DLL/GetProcAddress),
  # so there is no import library dependency. dxguid supplies the COM IIDs.
  target_link_libraries(megusta_platform PUBLIC user32 gdi32 opengl32 winmm dxguid)
elseif(EMSCRIPTEN AND MEGUSTA_BROWSER)
  target_sources(megusta_platform PRIVATE src/platform/sdl/sdl_runtime.cpp src/platform/sdl/presentation.cpp src/platform/browser/browser_runtime.cpp)
  target_compile_definitions(megusta_platform PUBLIC MEGUSTA_BROWSER=1 MEGUSTA_SDL=1)
  target_compile_options(megusta_platform PUBLIC "-sUSE_SDL=2")
  target_link_options(megusta_platform PUBLIC "-sUSE_SDL=2")
  target_link_options(megusta_platform PUBLIC "-sMIN_WEBGL_VERSION=1" "-sMAX_WEBGL_VERSION=1")

  megusta_platform_use_gl4es(megusta_platform)
  # PLIB/SSG and the recovered renderer use the desktop fixed-function GL
  # interface. Emscripten translates this subset to WebGL for the browser.
elseif(MEGUSTA_NATIVE_SDL)
  find_package(SDL2 REQUIRED CONFIG)
  find_package(OpenGL REQUIRED)
  target_sources(megusta_platform PRIVATE src/platform/sdl/sdl_runtime.cpp src/platform/sdl/presentation.cpp)
  target_compile_definitions(megusta_platform PUBLIC MEGUSTA_SDL=1 MEGUSTA_PORTABLE_NUMERIC=1)
  target_link_libraries(megusta_platform PUBLIC SDL2::SDL2 OpenGL::GL)
  add_executable(megusta_sdl_pointer_check tests/sdl_pointer_runtime_check.cpp)
  target_link_libraries(megusta_sdl_pointer_check PRIVATE megusta_platform)
endif()
if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(megusta_platform PRIVATE -fno-fast-math -ffp-contract=off)
  if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    target_compile_options(megusta_platform PRIVATE -mfpmath=387 -mno-sse -mno-sse2)
  endif()
endif()
