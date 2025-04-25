@REM #change current directory to this file

@rem change driver letter
@%~d0

@rem change current directory
@cd %~dp0

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