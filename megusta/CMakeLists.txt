cmake_minimum_required(VERSION 3.20)
project(megusta LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MEGUSTA_OPENGL_ADAPTER_SOURCES
  src/engine/opengl_adapter.cpp
  src/engine/opengl_adapter_fog_methods.cpp
  src/engine/opengl_adapter_is_enabled.cpp)
if(WIN32)
  add_compile_definitions(NOMINMAX)
endif()
include(cmake/AddressSanitizer.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(MEGUSTA_DTS_ENABLE_ABI_WRAPPERS
  "Link DTS address/ABI entry wrappers into megusta_scene_inspect" OFF)
option(MEGUSTA_GAME_PRESERVE_LAUNCH_CWD
  "Preserve the debugger/launcher working directory instead of changing to the executable directory" OFF)
set(MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY "" CACHE PATH
  "Optional Visual Studio debugger working directory for megusta_game")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_compile_definitions(MEGUSTA_NATIVE_64BIT=1)
elseif(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Megusta supports 32-bit and 64-bit targets only")
endif()
if(MEGUSTA_DTS_ENABLE_ABI_WRAPPERS AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "Recovered DTS address/ABI wrappers are 32-bit only; disable MEGUSTA_DTS_ENABLE_ABI_WRAPPERS for x64")
endif()
option(MEGUSTA_BROWSER "Build a browser Wasm platform adapter" OFF)
option(MEGUSTA_HEADLESS "Build without browser/window services" OFF)
option(MEGUSTA_NATIVE_SDL "Build the native Linux/macOS SDL game" OFF)
if(MEGUSTA_NATIVE_SDL AND (WIN32 OR EMSCRIPTEN OR MEGUSTA_HEADLESS))
  message(FATAL_ERROR "MEGUSTA_NATIVE_SDL requires a native Linux/macOS windowed target")
endif()
if(MEGUSTA_NATIVE_SDL)
  add_compile_definitions(MEGUSTA_SDL=1 MEGUSTA_PORTABLE_NUMERIC=1 GL_SILENCE_DEPRECATION=1 GL_GLEXT_PROTOTYPES=1)
endif()
if(APPLE AND MEGUSTA_NATIVE_SDL)
  include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos/include")
endif()
if(MEGUSTA_HEADLESS AND MEGUSTA_BROWSER)
  message(FATAL_ERROR "MEGUSTA_HEADLESS and MEGUSTA_BROWSER are mutually exclusive")
endif()
set(MEGUSTA_ASSET_DIR "" CACHE PATH "External shipped asset directory; its top-level out directory is excluded")
include(cmake/Assets.cmake)
option(MEGUSTA_PLATFORM_ONLY "Configure the isolated platform adapter without game/library dependencies" OFF)


  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    #target_sources(megusta_game PRIVATE src/platform/browser/legacy_gl.cpp)

    # gl4es supplies legacy desktop OpenGL on top of WebGL 1 / GLES 2.
    # Keep it confined to the browser game, not the scene-inspection/tests.
    set(MEGUSTA_GL4ES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/upstream/gl4es" CACHE PATH
      "Path to the gl4es source tree")
    if(NOT EXISTS "${MEGUSTA_GL4ES_DIR}/CMakeLists.txt")
      message(FATAL_ERROR
        "Browser megusta_game requires gl4es at ${MEGUSTA_GL4ES_DIR}; "
        "clone https://github.com/ptitSeb/gl4es there or set MEGUSTA_GL4ES_DIR")
    endif()
    set(STATICLIB ON CACHE BOOL "Build gl4es as a static library" FORCE)
    set(NOX11 ON CACHE BOOL "Disable X11 in gl4es" FORCE)
    set(NOEGL ON CACHE BOOL "Use an externally created GLES context" FORCE)
    set(NO_LOADER ON CACHE BOOL "Disable gl4es dynamic GLES loading" FORCE)
    set(NO_INIT_CONSTRUCTOR ON CACHE BOOL "Initialize gl4es explicitly" FORCE)
    set(DEFAULT_ES 2 CACHE STRING "Use the GLES 2 backend" FORCE)
    add_subdirectory("${MEGUSTA_GL4ES_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/gl4es")
  endif()


include(cmake/Platform.cmake)
if(MEGUSTA_PLATFORM_ONLY)
  add_executable(megusta_platform_probe tests/platform_probe.cpp tests/platform_check.cpp)
  target_link_libraries(megusta_platform_probe PRIVATE megusta_platform)
  megusta_package_assets(megusta_platform_probe)
  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    set_target_properties(megusta_platform_probe PROPERTIES SUFFIX ".html")
    target_link_options(megusta_platform_probe PRIVATE "-sENVIRONMENT=web")
  elseif(EMSCRIPTEN)
    target_link_options(megusta_platform_probe PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1")
  endif()
  return()
endif()
set(U "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
if(NOT EXISTS "${U}/zlib-1.1.3/zlib.h" OR NOT EXISTS "${U}/libpng-1.0.8/png.h")
  message(FATAL_ERROR "Required vendored zlib 1.1.3/libpng 1.0.8 source trees are missing")
endif()
set(Z "${U}/zlib-1.1.3")
set(P "${U}/libpng-1.0.8")
set(ZFILES adler32 compress crc32 deflate gzio infblock infcodes inffast inflate inftrees infutil trees uncompr zutil)
set(PFILES png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans pngwio pngwrite pngwtran pngwutil)
foreach(f IN LISTS ZFILES)
  list(APPEND ZSRC "${Z}/${f}.c")
endforeach()
foreach(f IN LISTS PFILES)
  list(APPEND PSRC "${P}/${f}.c")
endforeach()
add_library(megusta_zlib STATIC ${ZSRC})
target_include_directories(megusta_zlib PUBLIC "${Z}")
add_library(megusta_png STATIC ${PSRC})
target_include_directories(megusta_png PUBLIC "${P}")
target_link_libraries(megusta_png PUBLIC megusta_zlib)
if(NOT MSVC AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  # Period-appropriate optimized C build. The PNG path uses x87 arithmetic;
  # the zlib codec is integer-only and keeps conservative aliasing semantics.
  target_compile_options(megusta_zlib PRIVATE -O2 -fno-strict-aliasing)
  target_compile_options(megusta_png PRIVATE -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT EMSCRIPTEN)
    target_compile_options(megusta_png PRIVATE -mfpmath=387 -mno-sse -mno-sse2)
  endif()
elseif(MSVC)
  target_compile_options(megusta_zlib PRIVATE /O2)
  target_compile_options(megusta_png PRIVATE /O2 /fp:precise)
endif()
add_executable(megusta_probe tests/dependencies.c)
target_link_libraries(megusta_probe PRIVATE megusta_png)

# PLIB is compiled from the complete patched distribution in vendor/plib.
include(cmake/PLIB.cmake)
if(WIN32 OR MEGUSTA_NATIVE_SDL OR (EMSCRIPTEN AND MEGUSTA_BROWSER))
  include(cmake/CPython.cmake)
endif()
function(megusta_apply_vc6_x87_model target)
  if(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target} PRIVATE -O2 -fno-fast-math -ffp-contract=off -fno-strict-aliasing)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT EMSCRIPTEN)
      target_compile_options(${target} PRIVATE -mfpmath=387 -mno-sse -mno-sse2)
    endif()
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /fp:precise)
  endif()
endfunction()
if(WIN32)
  # The recovered executable was built with VC6 on 32-bit x86, where scalar
  # floating-point code uses the x87 stack. Keep DTS arithmetic on that model
  # for the runtime, prefix probe, and focused checks; double-evaluation alone
  # does not select x87 when the compiler targets i686.
  # CPython 1.5.2 is the recovered embedded runtime used by the game.
  option(MEGUSTA_AUDIO_DIRECTSOUND_BACKEND "Use the recovered DirectSound OpenAL backend by default" ON)
  add_library(megusta_audio STATIC src/audio/game_audio.cpp)
  target_include_directories(megusta_audio PUBLIC src)
  if(MEGUSTA_AUDIO_DIRECTSOUND_BACKEND)
    add_library(megusta_openal_ds STATIC src/audio/context.cpp src/audio/support.cpp
      src/audio/api.cpp src/audio/directsound.cpp src/audio/streaming.cpp
      src/audio/wave.cpp src/audio/backend_table.cpp)
    target_include_directories(megusta_openal_ds PUBLIC src)
    target_link_libraries(megusta_openal_ds PUBLIC dsound dxguid user32)
    megusta_apply_vc6_x87_model(megusta_openal_ds)
    target_compile_definitions(megusta_audio PRIVATE MEGUSTA_AUDIO_DEFAULT_DIRECTSOUND)
    target_link_libraries(megusta_audio PUBLIC megusta_openal_ds)
    add_executable(openal_backend_check tests/openal_backend_check.cpp
      src/engine/resource_file.cpp src/engine/log_capture.cpp src/engine/string_services.cpp)
    target_link_libraries(openal_backend_check PRIVATE megusta_openal_ds)
    megusta_apply_vc6_x87_model(openal_backend_check)
  endif()
  megusta_apply_vc6_x87_model(megusta_audio)
  # This fixture deliberately compiles the frontend without the default-backend
  # define or any DirectSound library, exercising the replacement boundary.
  add_executable(openal_external_dispatch_check tests/openal_dispatch_check.cpp src/audio/game_audio.cpp
    src/engine/resource_file.cpp src/engine/log_capture.cpp src/engine/string_services.cpp)
  target_include_directories(openal_external_dispatch_check PRIVATE src)
  megusta_apply_vc6_x87_model(openal_external_dispatch_check)
  set(MEGUSTA_DTS_WRAPPER_SOURCES
    src/dts/wrappers/bitmap_support_wrappers.cpp
    src/dts/wrappers/graphics_state_wrappers.cpp
    src/dts/wrappers/integer_set_wrappers.cpp
    src/dts/wrappers/legacy_alloc_wrappers.cpp
    src/dts/wrappers/legacy_mesh_wrappers.cpp
    src/dts/wrappers/material_list_wrappers.cpp
    src/dts/wrappers/matrix_wrappers.cpp
    src/dts/wrappers/mesh_wrappers.cpp
    src/dts/wrappers/mesh_lifecycle_wrappers.cpp
    src/dts/wrappers/mesh_render_wrappers.cpp
    src/dts/wrappers/scratch_release_wrappers.cpp
    src/dts/wrappers/sequence_wrappers.cpp
    src/dts/wrappers/shape_wrappers.cpp
    src/dts/wrappers/shape_alloc_wrappers.cpp
    src/dts/wrappers/shape_instance_wrappers.cpp
    src/dts/wrappers/state_transpose_wrappers.cpp
    src/dts/wrappers/texture_manager_wrappers.cpp
    src/dts/wrappers/thread_wrappers.cpp)
  add_executable(megusta_dts_prefix_probe tests/dts_prefix_probe.cpp
    src/dts/shape.cpp src/dts/matrix.cpp src/dts/shape_instance.cpp src/dts/sequence.cpp src/dts/thread.cpp src/dts/state_transpose.cpp src/dts/integer_set.cpp src/dts/legacy_alloc.cpp src/dts/vector_storage.cpp src/dts/shared_storage.cpp
    src/dts/legacy_mesh.cpp src/dts/material_list.cpp src/dts/shape_alloc.cpp src/dts/mesh.cpp src/dts/mesh_lifecycle.cpp src/dts/mesh_render.cpp src/dts/shadow_support.cpp src/dts/scratch_storage.cpp src/dts/scratch_release.cpp src/dts/texture_manager.cpp src/dts/graphics_state.cpp src/dts/static_state.cpp src/dts/bitmap_support.cpp
    src/engine/resource_file.cpp src/engine/log_capture.cpp src/engine/string_services.cpp
    ${MEGUSTA_OPENGL_ADAPTER_SOURCES})
  target_include_directories(megusta_dts_prefix_probe PRIVATE src)
  target_sources(megusta_dts_prefix_probe PRIVATE ${MEGUSTA_DTS_WRAPPER_SOURCES})
  target_link_libraries(megusta_dts_prefix_probe PRIVATE opengl32 megusta_png)
  megusta_apply_vc6_x87_model(megusta_dts_prefix_probe)
  enable_testing()
  add_library(megusta_dts_check_support STATIC
    src/dts/shape.cpp src/dts/matrix.cpp src/dts/shape_instance.cpp src/dts/resource_bridge.cpp src/dts/sequence.cpp src/dts/thread.cpp src/dts/vector_storage.cpp src/dts/shared_storage.cpp
    src/dts/state_transpose.cpp src/dts/integer_set.cpp src/dts/legacy_alloc.cpp src/dts/legacy_mesh.cpp
    src/dts/material_list.cpp src/dts/shape_alloc.cpp src/dts/mesh.cpp src/dts/mesh_lifecycle.cpp src/dts/mesh_render.cpp src/dts/shadow_support.cpp src/dts/scratch_storage.cpp src/dts/scratch_release.cpp src/dts/texture_manager.cpp src/dts/graphics_state.cpp src/dts/static_state.cpp src/dts/bitmap_support.cpp
    src/engine/resource_file.cpp src/engine/log_capture.cpp src/engine/string_services.cpp)
  target_include_directories(megusta_dts_check_support PUBLIC src)
  target_sources(megusta_dts_check_support PRIVATE ${MEGUSTA_DTS_WRAPPER_SOURCES})
  target_link_libraries(megusta_dts_check_support PUBLIC megusta_png)
  megusta_apply_vc6_x87_model(megusta_dts_check_support)
  file(GLOB DTS_CHECK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/dts_*_check.cpp")
  foreach(check_source IN LISTS DTS_CHECK_SOURCES)
    get_filename_component(check_name "${check_source}" NAME_WE)
    add_executable(${check_name} "${check_source}")
    target_link_libraries(${check_name} PRIVATE megusta_dts_check_support)
    if(CMAKE_CROSSCOMPILING)
      add_test(NAME ${check_name} COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_dts_ctest.py" "$<TARGET_FILE:${check_name}>")
      set_tests_properties(${check_name} PROPERTIES RUN_SERIAL TRUE)
    else()
      add_test(NAME ${check_name} COMMAND ${check_name})
    endif()
  endforeach()
  foreach(check_source IN LISTS DTS_CHECK_SOURCES)
    get_filename_component(check_name "${check_source}" NAME_WE)
    # These fixtures supply selected OpenGLAdapter methods themselves. Other
    # DTS checks use the platform adapter for unexercised render sections that
    # remain linked into shared translation units.
    if(check_name STREQUAL "dts_fog_render_check")
      # This fixture defines the methods it records. Link only the remaining
      # adapter methods so the test overrides are ordinary definitions.
      target_sources(${check_name} PRIVATE src/engine/opengl_adapter.cpp)
    elseif(check_name STREQUAL "dts_skinning_check")
      # The skinning fixture overrides isEnabled; fog_methods contains the
      # other adapter methods that this test needs.
      target_sources(${check_name} PRIVATE src/engine/opengl_adapter.cpp src/engine/opengl_adapter_fog_methods.cpp)
    else()
      target_sources(${check_name} PRIVATE ${MEGUSTA_OPENGL_ADAPTER_SOURCES})
    endif()
    target_link_libraries(${check_name} PRIVATE opengl32)
  endforeach()
endif()
if(EMSCRIPTEN AND MEGUSTA_BROWSER)
  add_library(megusta_audio STATIC src/audio/game_audio.cpp src/audio/browser_webaudio.cpp src/audio/browser_audio_stream.cpp src/audio/wave.cpp)
  target_include_directories(megusta_audio PUBLIC src)
  target_compile_definitions(megusta_audio PRIVATE MEGUSTA_AUDIO_DEFAULT_BROWSER)
endif()
if(MEGUSTA_NATIVE_SDL)
  find_package(OpenAL REQUIRED)
  add_library(megusta_audio STATIC src/audio/game_audio.cpp src/audio/native_openal.cpp src/audio/wave.cpp)
  target_include_directories(megusta_audio PUBLIC src PRIVATE ${OPENAL_INCLUDE_DIR})
  target_compile_definitions(megusta_audio PRIVATE MEGUSTA_AUDIO_DEFAULT_NATIVE)
  target_link_libraries(megusta_audio PUBLIC ${OPENAL_LIBRARY})
endif()
add_executable(megusta_plib_check tests/plib_core.cpp)
target_link_libraries(megusta_plib_check PRIVATE plib_sg plib_ul)
if(MEGUSTA_BUILD_PUI)
  add_executable(megusta_pui_check tests/plib_pui.cpp)
  target_link_libraries(megusta_pui_check PRIVATE plib_pui)
endif()
if(MEGUSTA_BUILD_NET)
  add_executable(megusta_net_check tests/plib_net.cpp)
  target_link_libraries(megusta_net_check PRIVATE plib_net)
endif()
if(MEGUSTA_BUILD_SSG)
  add_executable(megusta_ssg_check tests/plib_scene_graph.cpp)
  target_link_libraries(megusta_ssg_check PRIVATE plib_ssg)
  target_sources(megusta_ssg_check PRIVATE src/engine/resource_file.cpp src/engine/resource_mount.cpp
    src/engine/resource_runtime.cpp src/engine/log_capture.cpp src/engine/string_services.cpp
    src/engine/multi_texture.cpp src/engine/runtime_adapters.cpp
    src/engine/directory_services.cpp src/engine/mapped_resource_index.cpp)
  add_executable(megusta_serialization_check tests/plib_serialization.cpp)
  target_link_libraries(megusta_serialization_check PRIVATE plib_ssg)
  target_sources(megusta_serialization_check PRIVATE src/engine/resource_file.cpp src/engine/resource_mount.cpp
    src/engine/resource_runtime.cpp src/engine/log_capture.cpp src/engine/string_services.cpp
    src/engine/multi_texture.cpp src/engine/runtime_adapters.cpp
    src/engine/directory_services.cpp src/engine/mapped_resource_index.cpp)
  add_executable(megusta_state_check tests/plib_states.cpp)
  target_link_libraries(megusta_state_check PRIVATE plib_ssg)
  target_sources(megusta_state_check PRIVATE src/engine/resource_file.cpp src/engine/resource_mount.cpp
    src/engine/resource_runtime.cpp src/engine/log_capture.cpp src/engine/string_services.cpp
    src/engine/multi_texture.cpp src/engine/runtime_adapters.cpp
    src/engine/directory_services.cpp src/engine/mapped_resource_index.cpp)
  # The reference oracle mirrors the original 32-bit process layout and has
  # explicit x86 assembly checks. Do not add unusable oracle targets to x64.
  if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_library(megusta_original_plib_oracle SHARED tests/original_plib_leaf.cpp)
    target_link_libraries(megusta_original_plib_oracle PRIVATE plib_ssg)
    if(MEGUSTA_BUILD_PUI)
      target_link_libraries(megusta_original_plib_oracle PRIVATE plib_pui)
      target_compile_definitions(megusta_original_plib_oracle PRIVATE MEGUSTA_ORACLE_PUI)
    endif()
    if(MEGUSTA_BUILD_NET)
      target_link_libraries(megusta_original_plib_oracle PRIVATE plib_net)
      target_compile_definitions(megusta_original_plib_oracle PRIVATE MEGUSTA_ORACLE_NET)
    endif()
    set_target_properties(megusta_original_plib_oracle PROPERTIES PREFIX "")
    if(NOT MSVC)
      target_link_options(megusta_original_plib_oracle PRIVATE "-Wl,--add-stdcall-alias")
    endif()
    add_executable(megusta_original_plib_check tests/original_host.cpp)
    target_compile_features(megusta_original_plib_check PRIVATE cxx_std_17)
    if(MSVC)
      target_link_options(megusta_original_plib_check PRIVATE /BASE:0x10000000)
    else()
      target_link_options(megusta_original_plib_check PRIVATE "-Wl,--image-base,0x10000000")
    endif()
  endif()
  add_executable(megusta_scene_inspect src/apps/scene_inspect.cpp)
  target_link_libraries(megusta_scene_inspect PRIVATE megusta_platform)
  if(WIN32)
    target_link_libraries(megusta_scene_inspect PRIVATE ws2_32 winmm avifil32 msvfw32 shell32)
  endif()
  target_sources(megusta_scene_inspect PRIVATE src/platform/game_bridge.cpp tests/platform_check.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui.cpp src/game/powerups.cpp src/game/powerups_runtime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_file_picker.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/mapped_resource_index.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/math.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/socket.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/runtime_adapters.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/platform_services.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/packed_codec.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/movie_adapters.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/recovered_leaf_functions.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/vector_primitives.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/scene_record_accessors.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/scene_runtime.cpp src/game/gui_runtime.cpp src/game/gui_list.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_widgets.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_lozenges.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_zoom.cpp src/game/gui_shell.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_shape_previews.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/native_game_helpers.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/entry_bootstrap.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/spatial_records.cpp src/engine/render_projection.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gameplay_messages.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_tooltip.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/ground_shadow.cpp src/game/shape_shadow.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/balls_render.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/ai_commands.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/spatial_editor.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/hud.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/gl_setup.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/global_lifetime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/movie_playback.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/recovered_binding_primitives.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/ssg_native_adapters.cpp)
  target_sources(megusta_scene_inspect PRIVATE
    src/engine/resource_file.cpp src/engine/resource_mount.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/game_config.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/effect.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client.cpp src/game/room_client_chat.cpp src/game/network_player_states.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_messages.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_servers.cpp src/game/room_client_commands.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_control.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_lifecycle.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_send.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/room_client_sync.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/console.cpp src/game/console_input.cpp src/game/console_render.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/network_sync.cpp src/game/room_client_protocol.cpp src/game/room_client_bootstrap.cpp src/game/room_client_discovery.cpp src/game/room_client_session.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/effect_runtime.cpp src/game/effect_render.cpp src/game/effect_simulation.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/game_config_runtime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/spatial_object.cpp src/game/spatial_runtime.cpp src/game/spatial_camera.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/scene_triangle.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/scene_object.cpp src/game/task.cpp src/game/task_python.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/ball.cpp src/game/ball_collision.cpp src/game/ball_physics.cpp src/game/balls.cpp src/game/ball_render.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/ball_pair_collision.cpp src/game/random.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/ball_gameplay.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/player.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/player_ai.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/camera.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/camera_control.cpp src/game/camera_heading.cpp src/game/camera_frame.cpp)
  add_executable(camera_control_check tests/camera_control_check.cpp
    src/game/camera.cpp src/game/camera_control.cpp src/game/camera_heading.cpp
    src/game/scene_object.cpp src/game/scene_triangle.cpp tests/player_get_ball_fixture.cpp
    src/game/recovered_binding_primitives.cpp src/engine/math.cpp
    src/engine/resource_file.cpp src/engine/resource_mount.cpp src/engine/resource_runtime.cpp
    src/engine/log_capture.cpp src/engine/string_services.cpp src/engine/multi_texture.cpp
    src/engine/runtime_adapters.cpp src/engine/directory_services.cpp src/engine/mapped_resource_index.cpp)
  target_include_directories(camera_control_check PRIVATE src)
  target_include_directories(camera_control_check PRIVATE vendor/plib/src/net)
  target_link_libraries(camera_control_check PRIVATE plib_ssg)
  megusta_apply_vc6_x87_model(camera_control_check)
  target_sources(megusta_scene_inspect PRIVATE src/game/game_shape_adapter.cpp src/game/game_shape_runtime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/dts/integer_set.cpp src/dts/shape.cpp src/dts/matrix.cpp src/dts/shape_instance.cpp src/dts/resource_bridge.cpp src/dts/shape_alloc.cpp src/dts/legacy_alloc.cpp src/dts/sequence.cpp src/dts/thread.cpp src/dts/state_transpose.cpp src/dts/legacy_mesh.cpp src/dts/material_list.cpp src/dts/mesh.cpp src/dts/mesh_lifecycle.cpp src/dts/mesh_render.cpp src/dts/vector_storage.cpp src/dts/shared_storage.cpp src/dts/shadow_support.cpp src/dts/scratch_storage.cpp src/dts/scratch_release.cpp src/dts/texture_manager.cpp src/dts/graphics_state.cpp src/dts/static_state.cpp src/dts/bitmap_support.cpp)
  if(MEGUSTA_DTS_ENABLE_ABI_WRAPPERS)
    target_sources(megusta_scene_inspect PRIVATE ${MEGUSTA_DTS_WRAPPER_SOURCES})
  endif()
  target_sources(megusta_scene_inspect PRIVATE src/game/putter.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/putt_guide.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/putt_map.cpp src/game/putt_selection.cpp src/game/putt_script.cpp src/game/putt_map_runtime.cpp src/game/putt_estimate.cpp src/game/putt_map_io.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gameplay_runtime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/hole.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/driver.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/driver_script.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/driver_initialize.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/python_resource_import.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_assets.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/sounds.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/gui_render.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/sound_bindings.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/game_variables.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/gui_bindings.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/remaining_game_bindings.cpp)
  target_sources(megusta_scene_inspect PRIVATE tests/priority_pass_check.cpp)
  if(CMAKE_CROSSCOMPILING)
    add_test(NAME game_priority_pass COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_dts_ctest.py"
      "$<TARGET_FILE:megusta_scene_inspect>" --check-priority-pass)
  else()
    add_test(NAME game_priority_pass COMMAND megusta_scene_inspect --check-priority-pass)
  endif()
  set_tests_properties(game_priority_pass PROPERTIES RUN_SERIAL TRUE)
  target_sources(megusta_scene_inspect PRIVATE src/engine/multi_texture.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/startup.cpp tests/startup_check.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/driver_input.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/network.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/swig_runtime.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/game_basic.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/config_tasks.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/point_font.cpp src/bindings/player_ball.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/bindings/ssg_bindings.cpp src/bindings/object_players.cpp src/bindings/control_bindings.cpp)
  target_sources(megusta_scene_inspect PRIVATE tests/network_bindings_check.cpp)
  target_sources(megusta_scene_inspect PRIVATE tests/swig_gameplay_check.cpp)
  target_sources(megusta_scene_inspect PRIVATE tests/swig_world_check.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/driver_autorun.cpp)
  target_sources(megusta_scene_inspect PRIVATE ${MEGUSTA_OPENGL_ADAPTER_SOURCES}
    src/engine/gl_primitives.cpp src/engine/gl_token_table.cpp src/game/hole_render_backend.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/input.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/scene_grid.cpp src/game/triangle_distance.cpp src/game/geometry_queries.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/game/game_config_messages.cpp)
  target_link_libraries(megusta_scene_inspect PRIVATE plib_net)
  target_sources(megusta_scene_inspect PRIVATE src/engine/log_capture.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/string_services.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/directory_services.cpp)
  target_sources(megusta_scene_inspect PRIVATE src/engine/resource_runtime.cpp)
  megusta_apply_vc6_x87_model(megusta_scene_inspect)
  target_include_directories(megusta_scene_inspect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_include_directories(megusta_scene_inspect PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plib/src/net"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/plib/src/pui")
  target_link_libraries(megusta_scene_inspect PRIVATE
    plib_ssg glpng megusta_png megusta_zlib megusta_audio)
  # Keep every evidenced PLIB component available to the application linker.
  # Normal archive extraction/section collection still omits unused code until
  # the corresponding recovered game callers are integrated.
  foreach(component plib_pui plib_fnt)
    if(TARGET ${component})
      target_link_libraries(megusta_scene_inspect PRIVATE ${component})
    endif()
  endforeach()
  if(TARGET megusta_python)
    target_link_libraries(megusta_scene_inspect PRIVATE megusta_python)
  endif()

  # Native game image. Reuse the same recovered implementation units and
  # library closure as the integration executable, but omit its CLI and test
  # drivers. The Win32 application composition root binds entry, gameplay,
  # world, rendering and platform services before startup runs.
  if(WIN32)
    add_executable(megusta_game WIN32 src/apps/game_winmain.cpp)
  else()
    add_executable(megusta_game src/apps/sdl_game_main.cpp)
  endif()
  target_sources(megusta_game PRIVATE src/apps/game_application.cpp)
  get_target_property(MEGUSTA_GAME_SOURCE_CANDIDATES megusta_scene_inspect SOURCES)
  set(MEGUSTA_GAME_SOURCES)
  foreach(source IN LISTS MEGUSTA_GAME_SOURCE_CANDIDATES)
    if(source MATCHES "^tests/" OR source STREQUAL "src/apps/scene_inspect.cpp")
      continue()
    endif()
    list(APPEND MEGUSTA_GAME_SOURCES "${source}")
  endforeach()
  target_sources(megusta_game PRIVATE ${MEGUSTA_GAME_SOURCES})
  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    # gl4es's GL/gl.h remaps gl* calls to gl4es_gl* symbols on Emscripten.
    target_include_directories(megusta_game BEFORE PRIVATE "${MEGUSTA_GL4ES_DIR}/include")
    target_link_libraries(megusta_game PRIVATE GL)

  endif()
  target_sources(megusta_game PRIVATE src/game/application_input.cpp)
  if(WIN32)
    # Native startup callbacks belong to the Windows executable, not the
    # scene-inspection harness.
    target_sources(megusta_game PRIVATE src/game/win32_startup_host.cpp)
  elseif(MEGUSTA_SDL OR MEGUSTA_NATIVE_SDL OR (EMSCRIPTEN AND MEGUSTA_BROWSER))
    target_sources(megusta_game PRIVATE src/platform/sdl/startup_host.cpp)
  endif()
  get_target_property(MEGUSTA_GAME_LINKS megusta_scene_inspect LINK_LIBRARIES)
  target_link_libraries(megusta_game PRIVATE ${MEGUSTA_GAME_LINKS})
  target_include_directories(megusta_game PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_definitions(megusta_game PRIVATE MEGUSTA_GAME_EXECUTABLE=1)
  if(EMSCRIPTEN AND MEGUSTA_BROWSER)
    set_target_properties(megusta_game PROPERTIES SUFFIX ".html")
    target_link_options(megusta_game PRIVATE
      "-sENVIRONMENT=web" "-sALLOW_MEMORY_GROWTH=1" "-Wl,--error-limit=0" "-gsource-map"
      # Coin deployment alone uses a 60,000-byte stack array. Match the
      # Windows default reserve instead of Emscripten's 64 KiB stack, and
      # diagnose overflow before it corrupts lower linear-memory allocations.
      "-sSTACK_SIZE=1048576" "-sSTACK_OVERFLOW_CHECK=2"
      "SHELL:-s FULL_ES2=1" "SHELL:-s MIN_WEBGL_VERSION=1" "SHELL:-s MAX_WEBGL_VERSION=1")
    megusta_package_assets(megusta_game)
  endif()
  if(MEGUSTA_GAME_PRESERVE_LAUNCH_CWD)
    target_compile_definitions(megusta_game PRIVATE MEGUSTA_GAME_PRESERVE_LAUNCH_CWD=1)
  endif()
  if(MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY)
    if(NOT IS_DIRECTORY "${MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY}")
      message(FATAL_ERROR
        "MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY is not a directory: ${MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY}")
    endif()
    set_property(TARGET megusta_game PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
      "${MEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY}")
  endif()
  megusta_apply_vc6_x87_model(megusta_game)
endif()
if(WIN32)
  # Whole original source units remain intact. Unreferenced functions/data are
  # emitted separately and discarded from final artifacts, with linker maps.
  get_property(MEGUSTA_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS MEGUSTA_TARGETS)
    if(MSVC)
      target_compile_options(${target} PRIVATE /Gy /Gw)
    else()
      target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
    endif()
    get_target_property(kind ${target} TYPE)
    if(kind STREQUAL "EXECUTABLE" OR kind STREQUAL "SHARED_LIBRARY")
      if(MSVC)
        target_link_options(${target} PRIVATE /OPT:REF "/MAP:${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
      else()
        target_link_options(${target} PRIVATE "LINKER:--gc-sections" "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
      endif()
    endif()
  endforeach()
endif()
if(WIN32 AND TARGET megusta_scene_inspect)
  # Audit the full selected library dependency closure without altering the
  # application's normal section collection or claiming live game integration.
  add_executable(megusta_library_link_check tests/library_link_check.cpp)
  target_sources(megusta_library_link_check PRIVATE
    src/engine/resource_file.cpp src/engine/log_capture.cpp
    src/engine/string_services.cpp src/engine/multi_texture.cpp
    src/engine/runtime_adapters.cpp src/engine/resource_mount.cpp
    src/engine/mapped_resource_index.cpp)
  if(MSVC)
    target_link_options(megusta_library_link_check PRIVATE /OPT:NOREF "/MAP:${CMAKE_CURRENT_BINARY_DIR}/megusta_library_link_check.map")
  else()
    target_link_options(megusta_library_link_check PRIVATE
      "LINKER:--no-gc-sections" "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/megusta_library_link_check.map")
  endif()
  foreach(component megusta_python plib_ul plib_sg plib_ssg plib_fnt plib_pui
      plib_net glpng megusta_png megusta_zlib megusta_pix megusta_audio megusta_openal_ds)
    if(TARGET ${component})
      target_link_libraries(megusta_library_link_check PRIVATE ${component})
      if(MSVC)
        target_link_options(megusta_library_link_check PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${component}>")
      else()
        target_link_options(megusta_library_link_check PRIVATE
          "LINKER:--whole-archive,$<TARGET_FILE:${component}>,--no-whole-archive")
      endif()
    endif()
  endforeach()
endif()
if(EMSCRIPTEN)
  foreach(app megusta_probe megusta_plib_check)
    if(MEGUSTA_BROWSER)
      set_target_properties(${app} PROPERTIES SUFFIX ".html")
      target_link_options(${app} PRIVATE "-sENVIRONMENT=web" "-sASSERTIONS=1")
    else()
      set_target_properties(${app} PROPERTIES SUFFIX ".js")
      target_link_options(${app} PRIVATE "-sENVIRONMENT=node" "-sEXIT_RUNTIME=1" "-sASSERTIONS=1")
    endif()
  endforeach()
endif()
