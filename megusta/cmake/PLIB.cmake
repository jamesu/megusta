set(PLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plib")
if(NOT EXISTS "${PLIB_ROOT}/megusta-patches.json")
  message(FATAL_ERROR "Run python3 tools/prepare_plib.py before configuring")
endif()

# Deliberate source allowlist: retained distribution files are not implicit inputs.
# Check whichever components are enabled, including edits made to the lists above.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/manifests/plib_build_scope.json")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/manifests/plib_build_scope.json" PLIB_SCOPE_JSON)
string(JSON PLIB_SCOPE_COUNT LENGTH "${PLIB_SCOPE_JSON}" compiled_units)
math(EXPR PLIB_SCOPE_LAST "${PLIB_SCOPE_COUNT} - 1")
set(PLIB_ALLOWED_UNITS)
foreach(i RANGE ${PLIB_SCOPE_LAST})
  string(JSON unit GET "${PLIB_SCOPE_JSON}" compiled_units ${i})
  list(APPEND PLIB_ALLOWED_UNITS "${unit}")
endforeach()
function(megusta_check_plib_scope)
  foreach(lib plib_ul plib_sg plib_net plib_ssg plib_fnt plib_pui)
    if(TARGET ${lib})
      # Shipped assertion strings/abort paths are executable behavior. Release
      # configurations must not erase guards, including inline library methods.
      target_compile_options(${lib} PUBLIC -UNDEBUG)
      get_target_property(units ${lib} SOURCES)
      foreach(source IN LISTS units)
        file(RELATIVE_PATH unit "${PLIB_ROOT}" "${source}")
        if(NOT unit IN_LIST PLIB_ALLOWED_UNITS)
          message(FATAL_ERROR "PLIB unit outside the evidenced build allowlist: ${unit}")
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()
cmake_language(DEFER CALL megusta_check_plib_scope)
add_library(plib_ul STATIC
  "${PLIB_ROOT}/src/util/ul.cxx"
  "${PLIB_ROOT}/src/util/ulError.cxx")
target_include_directories(plib_ul PUBLIC "${PLIB_ROOT}/src/util")
add_library(plib_sg STATIC
  "${PLIB_ROOT}/src/sg/sg.cxx"
  "${PLIB_ROOT}/src/sg/sgd.cxx")
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
if(NOT MSVC)
  target_compile_options(plib_sg PUBLIC -ffp-eval-method=double)
endif()
# SG arithmetic is also inline in SSG and game callers. Propagate its53-bit
# evaluation profile so those bodies do not silently use float intermediates.
if(WIN32)
  target_compile_definitions(plib_ul PUBLIC WIN32 NOMINMAX)
  target_link_libraries(plib_ul PUBLIC winmm)
else()
  target_compile_definitions(plib_ul PUBLIC MEGUSTA_NO_OPENGL)
endif()
option(MEGUSTA_BUILD_NET "Build the evidenced NET transport/buffer/message/chat/monitor units" ${WIN32})
if(MEGUSTA_BUILD_NET)
  if(NOT WIN32)
    message(FATAL_ERROR "NET target service adaptation currently supports Win32 only")
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
  target_sources(plib_net PRIVATE "${PLIB_ROOT}/src/net/netRoom.cxx")
  target_sources(plib_net PRIVATE "${PLIB_ROOT}/src/net/netRoomClient.cxx")
  target_include_directories(plib_net PUBLIC "${PLIB_ROOT}/src/net")
  target_link_libraries(plib_net PUBLIC plib_ul ws2_32)
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
option(MEGUSTA_BUILD_SSG "Build the evidenced SSG component and its native dependencies" ${WIN32})
option(MEGUSTA_BUILD_PUI "Build evidenced PUI/FNT units without GLUT" ${WIN32})
if(MEGUSTA_BUILD_SSG OR MEGUSTA_BUILD_PUI)
  add_library(glpng STATIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glpng/src/glpng.c")
  target_include_directories(glpng PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glpng/include")
  target_link_libraries(glpng PUBLIC megusta_png opengl32 glu32)
  if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(glpng PRIVATE
      -O2 -mfpmath=387 -mno-sse -mno-sse2
      -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
  elseif(MSVC)
    target_compile_options(glpng PRIVATE /O2 /fp:precise)
  endif()
endif()
if(MEGUSTA_BUILD_PUI)
  if(NOT WIN32)
    message(FATAL_ERROR "PUI window service currently supports Win32 only")
  endif()
  add_library(plib_fnt STATIC "${PLIB_ROOT}/src/fnt/fnt.cxx" "${PLIB_ROOT}/src/fnt/fntTXF.cxx")
  target_include_directories(plib_fnt PUBLIC "${PLIB_ROOT}/src/fnt")
  target_link_libraries(plib_fnt PUBLIC plib_sg glpng opengl32 glu32)
  set(PUI_UNITS pu puArrowButton puBox puButton puFilePicker puFont puFrame puGroup puInput
    puInterface puListBox puObject puOneShot puSlider puText puValue)
  set(PUI_SOURCES)
  foreach(unit IN LISTS PUI_UNITS)
    list(APPEND PUI_SOURCES "${PLIB_ROOT}/src/pui/${unit}.cxx")
  endforeach()
  add_library(plib_pui STATIC ${PUI_SOURCES})
  target_include_directories(plib_pui PUBLIC "${PLIB_ROOT}/src/pui")
  target_compile_definitions(plib_pui PUBLIC PU_NOT_USING_GLUT MEGUSTA_PUI_FNT)
  target_link_libraries(plib_pui PUBLIC plib_fnt user32)
  foreach(lib plib_fnt plib_pui)
    if(MSVC)
      target_compile_options(${lib} PRIVATE /fp:precise)
    else()
      target_compile_options(${lib} PRIVATE -fno-fast-math -ffp-contract=off -ffp-eval-method=double)
    endif()
  endforeach()
endif()
if(MEGUSTA_BUILD_SSG)
  if(NOT WIN32)
    message(FATAL_ERROR "SSG graphics platform adaptation is currently implemented for Win32 only")
  endif()
  add_library(megusta_pix STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/pix_texture.cpp")
  target_include_directories(megusta_pix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/engine")
  target_link_libraries(megusta_pix PUBLIC glpng megusta_zlib plib_ul opengl32)
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
  target_compile_features(plib_ssg PUBLIC cxx_std_17)
  target_include_directories(plib_ssg PUBLIC "${PLIB_ROOT}/src/ssg")
  # MG-PLIB-088: recovered game SSG vtables omit hot/isect traversal slots.
  target_compile_definitions(plib_ssg PUBLIC MEGUSTA_NO_SSG_COLLISION)
  target_link_libraries(plib_ssg PUBLIC plib_sg glpng megusta_pix opengl32 glu32)
  if(MSVC)
    target_compile_options(plib_ssg PRIVATE /fp:precise)
  else()
    target_compile_options(plib_ssg PRIVATE -fno-fast-math -ffp-contract=off)
  endif()
endif()
