# Build and run

Run these commands from `megusta/`. Requirements: CMake 3.20+, a C/C++17
toolchain and its build generator. Native macOS/Linux also needs SDL2 and
desktop OpenGL development files; Windows needs Visual Studio 2022 ClangCL
and the Windows SDK. Browser builds need an activated Emscripten SDK.

## macOS/Linux

```sh
tools/testing/build_native_sdl.sh
tools/testing/run_native_sdl.sh /path/to/MGM -window -nointro
```

The launcher copies assets into `build/<platform>/run/`. Existing session
files are preserved, so update that copy when changing assets or preferences.

## Windows x64

From a Visual Studio Developer Command Prompt:

```bat
tools\testing\generate_vs_x64.bat
cmake --build build/vs2022-clang-x64 --config RelWithDebInfo --target megusta_game --parallel
```

Run from a writable copy of the game asset directory. The generation script
accepts optional build-directory and debugger-working-directory arguments.

## Browser wasm32

```sh
emcmake cmake -S . -B build/wasm32-browser \
  -DMEGUSTA_BROWSER=ON -DMEGUSTA_HEADLESS=OFF \
  -DMEGUSTA_PLATFORM_ONLY=OFF \
  -DMEGUSTA_BUILD_SSG=ON -DMEGUSTA_BUILD_PUI=ON -DMEGUSTA_BUILD_NET=ON \
  -DMEGUSTA_ASSET_DIR=/path/to/MGM
cmake --build build/wasm32-browser --target megusta_game --parallel 8
python3 -m http.server 8000 --directory build/wasm32-browser
```

Open `http://localhost:8000/megusta_game.html`. Serve the generated HTML, JS,
WASM and data files together. Assets and `MINIGOLF.CFG` are packaged from
`MEGUSTA_ASSET_DIR`, excluding its top-level `out` directory. Rebuild after
changing them, then reload the page.

## Configuration

Native runs read `MINIGOLF.CFG` from the working asset directory. To select
a default mod:

```ini
[Prefs]
DefaultMod=mods/my-mod
```

Paths are relative to the asset directory; `-mod <path>` overrides this setting.
Leave it empty for the base game.
