@echo off
setlocal
for %%I in ("%~dp0..\..") do set "MEGUSTA_PROJECT_DIR=%%~fI"
rem Generate a native x64 Windows build with Visual Studio's clang-cl toolset.
rem Requires VS 2022 Desktop development with C++, Windows SDK, CMake tools,
rem Clang tools and the Windows OpenGL/DirectSound SDK libraries.
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake is unavailable. Run from a Visual Studio Developer Command Prompt.
  exit /b 1
)
set "MEGUSTA_X64_BUILD=%MEGUSTA_PROJECT_DIR%\build\vs2022-clang-x64"
if not "%~1"=="" set "MEGUSTA_X64_BUILD=%~f1"
set "MEGUSTA_VS_CWD_ARG="
if not "%~2"=="" set MEGUSTA_VS_CWD_ARG="-DMEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY=%~f2"
cmake -S "%MEGUSTA_PROJECT_DIR%" -B "%MEGUSTA_X64_BUILD%" -G "Visual Studio 17 2022" -A x64 -T ClangCL -DMEGUSTA_ENABLE_ASAN=OFF %MEGUSTA_VS_CWD_ARG% -DCMAKE_CONFIGURATION_TYPES="Debug;RelWithDebInfo" -DCMAKE_VS_DEBUG_INFORMATION_FORMAT=ProgramDatabase
if errorlevel 1 exit /b 1
echo.
echo Generated: "%MEGUSTA_X64_BUILD%\megusta.sln"
if not "%~2"=="" echo Visual Studio debugger working directory: "%~f2"
echo Build with:
echo   cmake --build "%MEGUSTA_X64_BUILD%" --config RelWithDebInfo --target megusta_game --parallel
echo This target is native x64 and uses native pointer widths. It is not ABI-identical to the original Win32 executable.
endlocal
