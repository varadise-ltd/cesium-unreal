include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CXX_FLAGS "/MD /Zp8")
set(VCPKG_C_FLAGS "/MD /Zp8")
set(VCPKG_ENV_PASSTHROUGH "UNREAL_ENGINE_ROOT")
