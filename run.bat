@echo off
setlocal EnableDelayedExpansion

rem run.bat -- clean build of cminux on Windows.
rem
rem Usage:
rem   run.bat
rem
rem This script cleans the build directory, configures CMake, and builds
rem the cminux executable. It does not run cminux on any input. Use the
rem built binary directly for that.
rem
rem Environment:
rem   CMAKE_BUILD_TYPE   Release (default) or Debug
rem   KEEP=1             do not clean the build directory before building
rem   CMAKE_JOBS         number of parallel jobs (default: 4)

cd /d "%~dp0"

set "BUILD_DIR=build"
if not defined CMAKE_BUILD_TYPE set "CMAKE_BUILD_TYPE=Release"
if not defined CMAKE_JOBS set "CMAKE_JOBS=4"

rem ---------------------------------------------------------------------------
rem 1. Clean the build directory unless KEEP=1
rem ---------------------------------------------------------------------------
if "%KEEP%"=="1" (
    echo run.bat: KEEP=1, not cleaning %BUILD_DIR%
) else (
    if exist "%BUILD_DIR%" (
        echo run.bat: removing %BUILD_DIR%
        rmdir /s /q "%BUILD_DIR%"
        if exist "%BUILD_DIR%" (
            echo run.bat: failed to remove %BUILD_DIR% >&2
            exit /b 1
        )
    )
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR%" (
    echo run.bat: failed to create %BUILD_DIR% >&2
    exit /b 1
)

rem ---------------------------------------------------------------------------
rem 2. Configure
rem ---------------------------------------------------------------------------
echo run.bat: cmake -S . -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%
cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CMAKE_BUILD_TYPE%"
if errorlevel 1 (
    echo run.bat: cmake configure failed >&2
    exit /b 1
)

rem ---------------------------------------------------------------------------
rem 3. Build
rem ---------------------------------------------------------------------------
echo run.bat: cmake --build %BUILD_DIR% --config %CMAKE_BUILD_TYPE% --parallel %CMAKE_JOBS%
cmake --build "%BUILD_DIR%" --config "%CMAKE_BUILD_TYPE%" --parallel "%CMAKE_JOBS%"
if errorlevel 1 (
    echo run.bat: cmake build failed >&2
    exit /b 1
)

rem ---------------------------------------------------------------------------
rem 4. Verify the binary exists, in either layout:
rem
rem    build\cminux.exe                  (Ninja, Make, single-config)
rem    build\<Config>\cminux.exe         (MSVC, Xcode, multi-config)
rem ---------------------------------------------------------------------------
set "CMX="
for %%C in (
    "%BUILD_DIR%\cminux.exe"
    "%BUILD_DIR%\%CMAKE_BUILD_TYPE%\cminux.exe"
    "%BUILD_DIR%\Release\cminux.exe"
    "%BUILD_DIR%\Debug\cminux.exe"
) do (
    if not defined CMX (
        if exist "%%~C" set "CMX=%%~C"
    )
)

if not defined CMX (
    echo run.bat: build succeeded but no cminux.exe was found in %BUILD_DIR% >&2
    echo run.bat: looked in %BUILD_DIR%, %BUILD_DIR%\%CMAKE_BUILD_TYPE%, %BUILD_DIR%\Release, %BUILD_DIR%\Debug >&2
    exit /b 1
)

echo run.bat: built !CMX!
exit /b 0