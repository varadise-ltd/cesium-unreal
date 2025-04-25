@REM #change current directory to this file

@rem change driver letter
@%~d0

@rem change current directory
@cd %~dp0

set CMAKE_TARGET_VERSION_MAJOR=3

@echo off
for /f "tokens=3" %%i in ('cmake --version ^| findstr /b "cmake version"') do set VERSION=%%i
if not defined VERSION (
    echo Error: Unable to determine CMake version.
    exit /b 1
)

:: Extract major version
for /f "tokens=1 delims=." %%a in ("%VERSION%") do set CMAKE_VERSION_MAJOR=%%a
if not defined CMAKE_VERSION_MAJOR (
    echo Error: Invalid CMake version format.
    exit /b 1
)

if %CMAKE_VERSION_MAJOR% NEQ %CMAKE_TARGET_VERSION_MAJOR% (
    echo only cmake version %CMAKE_TARGET_VERSION_MAJOR% is supported
    exit /b 1
)
else (
    echo check cmake version success
)

@echo on

git checkout dev_varadise

git config --list --local

git config submodule.extern/cesium-native.url https://github.com/CesiumGS/cesium-native.git
git config submodule.extern/MikkTSpace.url https://github.com/mmikk/MikkTSpace.git
git config submodule.extern/tidy-html5.url https://github.com/htacg/tidy-html5
git config submodule.extern/swl-variant.url https://github.com/kring/swl-variant.git

git submodule update --init --recursive

PUSHD "extern"
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target install
cmake --build build --config Debug --target install
POPD