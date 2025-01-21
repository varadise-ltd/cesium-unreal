@REM #change current directory to this file

@rem change driver letter
@%~d0

@rem change current directory
@cd %~dp0

PUSHD "extern"
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target install
cmake --build build --config Debug --target install
POPD