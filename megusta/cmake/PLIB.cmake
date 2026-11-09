set(PLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plib")
if(NOT EXISTS "${PLIB_ROOT}/src/ssg/ssg.h")
  message(FATAL_ERROR "Vendored PLIB sources are missing: ${PLIB_ROOT}")
endif()

# gl4es uses a separate symbol namespace on Emscripten (gl4es_gl*).
# PLIB and glpng are separate static targets: adding gl4es headers only to
# megusta_game does not affect how these libraries compile their GL calls.
# The top-level project configures gl4es before including this file.
if(EMSCRIPTEN AND MEGUSTA_BROWSER)
  if(NOT DEFINED MEGUSTA_GL4ES_DIR)
    set(MEGUSTA_GL4ES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/gl4es")
  endif()
  if(NOT EXISTS "${MEGUSTA_GL4ES_DIR}/include/GL/gl.h")
    message(FATAL_ERROR "gl4es headers not found: ${MEGUSTA_GL4ES_DIR}/include/GL/gl.h")
  endif()
endif()

function(megusta_plib_use_gl4es target)
  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    # BEFORE ensures <GL/gl.h> resolves to gl4es's symbol-mangling header.
    target_include_directories(${target} BEFORE PRIVATE "${MEGUSTA_GL4ES_DIR}/include")
  endif()
endfunction()

# Deliberate source allowlist: only the source files listed by the targets below
# are compiled. The recovery scope manifest remains an audit aid, not a configure
# or build dependency.
function(megusta_keep_plib_assertions target)
  # Shipped assertion strings/abort paths are executable behavior. Release
  # configurations must not erase guards, including inline library methods.
  target_compile_options(${target} PUBLIC -UNDEBUG)
endfunction()
add_library(plib_ul STATIC
  "${PLIB_ROOT}/src/util/ul.cxx"
  "${PLIB_ROOT}/src/util/ulError.cxx")
megusta_plib_use_gl4es(plib_ul)
megusta_keep_plib_assertions(plib_ul)
target_include_directories(plib_ul PUBLIC "${PLIB_ROOT}/src/util")
add_library(plib_sg STATIC
  "${PLIB_ROOT}/src/sg/sg.cxx"
  "${PLIB_ROOT}/src/sg/sgd.cxx")
megusta_plib_use_gl4es(plib_sg)
megusta_keep_plib_assertions(plib_sg)
target_include_directories(plib_sg PUBLIC "${PLIB_ROOT}/src/sg")
target_link_libraries(plib_sg PUBLIC plib_ul)
foreach(lib plib_ul plib_sg)
  if(MSVC)
    target_compile_options(${lib} PRIVATE /fp:precise)
  else()
    target_compile_options(${lib} PRIVATE -fno-fast-math -ffp-contract=off)
  endif()
endforeach()
# Provisional SG arithmetic profile; original x87 parity is a separate check.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(plib_sg PUBLIC -ffp-eval-method=double)
endif()
# SG arithmetic is also inline in SSG and game callers. Propagate its53-bit
# evaluation profile so those bodies do not silently use float intermediates.
if(WIN32)
  target_compile_definitions(plib_ul PUBLIC WIN32 NOMINMAX)
  target_link_libraries(plib_ul PUBLIC winmm)
elseif(NOT MEGUSTA_NATIVE_SDL AND NOT (EMSCRIPTEN AND MEGUSTA_BROWSER))
  target_compile_definitions(plib_ul PUBLIC MEGUSTA_NO_OPENGL)
endif()
set(MEGUSTA_PLIB_DEFAULT ${WIN32})
if(MEGUSTA_NATIVE_SDL)
  set(MEGUSTA_PLIB_DEFAULT ON)
endif()
option(MEGUSTA_BUILD_NET "Build the evidenced NET transport/buffer/message/chat/monitor units" ${MEGUSTA_PLIB_DEFAULT})
if(MEGUSTA_BUILD_NET)
  if(NOT WIN32 AND NOT EMSCRIPTEN AND NOT MEGUSTA_NATIVE_SDL)
    message(FATAL_ERROR "NET target service adaptation currently supports Win32 or Emscripten POSIX sockets")
  endif()
  # Explicit selection: room master/server implementations have no positive
  # match in the current inventory; room records/client await protocol patches.
  add_library(plib_net STATIC
    "${PLIB_ROOT}/src/net/netSocket.cxx"
    "${PLIB_ROOT}/src/net/netChannel.cxx"
    "${PLIB_ROOT}/src/net/netBuffer.cxx"
    "${PLIB_ROOT}/src/net/netMessage.cxx"
    "${PLIB_ROOT}/src/net/netChat.cxx"
    "${PLIB_ROOT}/src/net/netMonitor.cxx")
  megusta_keep_plib_assertions(plib_net)
  megusta_plib_use_gl4es(plib_net)
  target_sources(plib_net PRIVATE "${PLIB_ROOT}/src/net/netRoom.cxx")
  target_sources(plib_net PRIVATE "${PLIB_ROOT}/src/net/netRoomClient.cxx")
  target_include_directories(plib_net PUBLIC "${PLIB_ROOT}/src/net")
  target_link_libraries(plib_net PUBLIC plib_ul)
  if(WIN32)
    target_link_libraries(plib_net PUBLIC ws2_32)
  endif()
  # Original poll capacity is 256; propagate before any Winsock header import.
  target_compile_definitions(plib_net PUBLIC FD_SETSIZE=256)
  # Retained header serializers type-pun float words as the original compiler
  # allowed. This applies to clients emitting those inline methods as well.
  if(MSVC)
    target_compile_options(plib_net PRIVATE /fp:precise)
  else()
    target_compile_options(plib_net PUBLIC -fno-strict-aliasing)
    target_compile_options(plib_net PRIVATE -fno-fast-math -ffp-contract=off)
  endif()
endif()
option(MEGUSTA_BUILD_SSG "Build the evidenced SSG component and its native dependencies" ${MEGUSTA_PLIB_DEFAULT})
option(MEGUSTA_BUILD_PUI "Build evidenced PUI/FNT units without GLUT" ${MEGUSTA_PLIB_DEFAULT})
if(MEGUSTA_BUILD_SSG OR MEGUSTA_BUILD_PUI)
  add_library(glpng STATIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glpng/src/glpng.c")
  megusta_plib_use_gl4es(glpng)
  target_include_directories(glpng PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glpng/include")
  target_link_libraries(glpng PUBLIC megusta_png)
  if(EMSCRIPTEN)
    # Retain glpng's legacy function alias. Browser builds use gl4es,
    # not Emscripten's LEGACY_GL_EMULATION, for the GL entry points.
    #target_compile_definitions(glpng PRIVATE glColorTableEXT=glColorTable)
  endif()
  if(WIN32)
    target_link_libraries(glpng PUBLIC opengl32 glu32)
  endif()
  if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(glpng PRIVATE -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT EMSCRIPTEN)
      target_compile_options(glpng PRIVATE -mfpmath=387 -mno-sse -mno-sse2)
    endif()
  elseif(MSVC)
    target_compile_options(glpng PRIVATE /O2 /fp:precise)
  endif()
endif()
if(MEGUSTA_BUILD_PUI)
  if(NOT WIN32 AND NOT MEGUSTA_NATIVE_SDL AND NOT (EMSCRIPTEN AND MEGUSTA_BROWSER))
    message(FATAL_ERROR "PUI window service currently supports Win32 and the SDL browser target")
  endif()
  add_library(plib_fnt STATIC "${PLIB_ROOT}/src/fnt/fnt.cxx" "${PLIB_ROOT}/src/fnt/fntTXF.cxx")
  megusta_plib_use_gl4es(plib_fnt)
  megusta_keep_plib_assertions(plib_fnt)
  target_include_directories(plib_fnt PUBLIC "${PLIB_ROOT}/src/fnt")
  target_link_libraries(plib_fnt PUBLIC plib_sg glpng)
  if(WIN32)
    target_link_libraries(plib_fnt PUBLIC opengl32 glu32)
  endif()
  set(PUI_UNITS pu puArrowButton puBox puButton puFilePicker puFont puFrame puGroup puInput
    puInterface puListBox puObject puOneShot puSlider puText puValue)
  set(PUI_SOURCES)
  foreach(unit IN LISTS PUI_UNITS)
    list(APPEND PUI_SOURCES "${PLIB_ROOT}/src/pui/${unit}.cxx")
  endforeach()
  add_library(plib_pui STATIC ${PUI_SOURCES})
  megusta_plib_use_gl4es(plib_pui)
  megusta_keep_plib_assertions(plib_pui)
  target_include_directories(plib_pui PUBLIC "${PLIB_ROOT}/src/pui")
  target_compile_definitions(plib_pui PUBLIC PU_NOT_USING_GLUT MEGUSTA_PUI_FNT)
  target_link_libraries(plib_pui PUBLIC plib_fnt)
  if(WIN32)
    target_link_libraries(plib_pui PUBLIC user32)
  elseif(EMSCRIPTEN OR MEGUSTA_NATIVE_SDL)
    target_link_libraries(plib_pui PUBLIC megusta_platform)
  endif()
  foreach(lib plib_fnt plib_pui)
    if(MSVC)
      target_compile_options(${lib} PRIVATE /fp:precise)
    else()
      target_compile_options(${lib} PRIVATE -fno-fast-math -ffp-contract=off)
      if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${lib} PRIVATE -ffp-eval-method=double)
      endif()
    endif()
  endforeach()
endif()
if(MEGUSTA_BUILD_SSG)
  if(NOT WIN32 AND NOT MEGUSTA_NATIVE_SDL AND NOT (EMSCRIPTEN AND MEGUSTA_BROWSER))
    message(FATAL_ERROR "SSG graphics platform adaptation is currently implemented for Win32 and the SDL browser target")
  endif()
  add_library(megusta_pix STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/pix_texture.cpp")
  megusta_plib_use_gl4es(megusta_pix)
  target_include_directories(megusta_pix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/engine")
  target_link_libraries(megusta_pix PUBLIC glpng megusta_zlib plib_ul)
  if(WIN32)
    target_link_libraries(megusta_pix PUBLIC opengl32)
  endif()
  set(SSG_UNITS ssg ssgBase ssgBaseTransform ssgBranch ssgContext ssgConvertASX
    ssgCutout ssgDList ssgEntity ssgLeaf ssgCompiledLeaf ssgList ssgRoot
    ssgSelector ssgSimpleState ssgState ssgStateTables ssgStats ssgTexTrans
    ssgTexture ssgTransform ssgStateSelector ssgLoadTexture
    ssgLoadPNG ssgLoadTGA ssgLoadSSG
    ssgSimpleList ssgVtxTable ssgIO ssgSaveASE ssgLoadASE ssgASESupt ssgParser ssgVtxArray
    ssgLoad ssgLoadASX ssgASXGeometry
    ssgASEIndexedGeometry ssgASETriangleGeometry ssgAnimatedTransform ssgGameAnimation)
  set(SSG_SOURCES)
  foreach(unit IN LISTS SSG_UNITS)
    list(APPEND SSG_SOURCES "${PLIB_ROOT}/src/ssg/${unit}.cxx")
  endforeach()
  add_library(plib_ssg STATIC ${SSG_SOURCES})
  megusta_plib_use_gl4es(plib_ssg)
  megusta_keep_plib_assertions(plib_ssg)
  target_compile_features(plib_ssg PUBLIC cxx_std_17)
  target_include_directories(plib_ssg PUBLIC "${PLIB_ROOT}/src/ssg")
  # MG-PLIB-088: recovered game SSG vtables omit hot/isect traversal slots.
  target_compile_definitions(plib_ssg PUBLIC MEGUSTA_NO_SSG_COLLISION)
  target_link_libraries(plib_ssg PUBLIC plib_sg glpng megusta_pix)
  if(WIN32)
    target_link_libraries(plib_ssg PUBLIC opengl32 glu32)
  endif()
  if(MSVC)
    target_compile_options(plib_ssg PRIVATE /fp:precise)
  else()
    target_compile_options(plib_ssg PRIVATE -fno-fast-math -ffp-contract=off)
  endif()
endif()

if(MEGUSTA_NATIVE_SDL)
  target_link_libraries(plib_sg PUBLIC OpenGL::GL)
  target_link_libraries(plib_ssg PRIVATE SDL2::SDL2)
  if(TARGET OpenGL::GLU)
    target_link_libraries(plib_sg PUBLIC OpenGL::GLU)
  endif()
endif()
