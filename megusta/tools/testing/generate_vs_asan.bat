@echo off
setlocal
for %%I in ("%~dp0..\..") do set "MEGUSTA_PROJECT_DIR=%%~fI"
rem Run from any directory. Requires VS 2022 Desktop development with C++,
rem Windows SDK, C++ CMake tools, and C++ AddressSanitizer, C++ Clang tools, and MSBuild LLVM toolset components.
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake is unavailable. Run from a Visual Studio Developer Command Prompt.
  exit /b 1
)
set "MEGUSTA_ASAN_BUILD=%MEGUSTA_PROJECT_DIR%\build\vs2022-clang-asan"
if not "%~1"=="" set "MEGUSTA_ASAN_BUILD=%~f1"
set "MEGUSTA_VS_CWD_ARG="
if not "%~2"=="" set MEGUSTA_VS_CWD_ARG="-DMEGUSTA_VS_DEBUGGER_WORKING_DIRECTORY=%~f2"
cmake -S "%MEGUSTA_PROJECT_DIR%" -B "%MEGUSTA_ASAN_BUILD%" -G "Visual Studio 17 2022" -A Win32 -T ClangCL -DMEGUSTA_ENABLE_ASAN=ON %MEGUSTA_VS_CWD_ARG% -DCMAKE_CONFIGURATION_TYPES="Debug;RelWithDebInfo" -DCMAKE_VS_DEBUG_INFORMATION_FORMAT=ProgramDatabase
if errorlevel 1 exit /b 1
echo.
echo Generated: "%MEGUSTA_ASAN_BUILD%\megusta.sln"
if not "%~2"=="" echo Visual Studio debugger working directory: "%~f2"
echo Build with:
echo   cmake --build "%MEGUSTA_ASAN_BUILD%" --config RelWithDebInfo --target megusta_game --parallel
echo Startup selects the executable or debugger working directory containing minigolf.cfg.
echo Optional second argument to this script sets it explicitly to a writable COPY of the MGM assets.
echo Do not run against the shipped source data.
echo Run from a VS Developer Command Prompt so the ASan runtime DLL is on PATH.
endlocal
