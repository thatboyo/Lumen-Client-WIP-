# Cross-compile the Windows x64 artifacts from Linux.
#
#   MINGW_ROOT=/path/to/mingw64 cmake -S . -B build-win \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
#
# MINGW_ROOT is either a normal MinGW-w64 install (bin/, x86_64-w64-mingw32/) or
# a Debian mingw package tree (usr/bin/, usr/x86_64-w64-mingw32/).

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(DEFINED ENV{MINGW_ROOT} AND NOT DEFINED MINGW_ROOT)
    set(MINGW_ROOT "$ENV{MINGW_ROOT}")
endif()
if(NOT DEFINED MINGW_ROOT)
    set(MINGW_ROOT "C:/mingw64")
endif()

if(EXISTS "${MINGW_ROOT}/usr/x86_64-w64-mingw32")
    set(_mingw_prefix "${MINGW_ROOT}/usr")
else()
    set(_mingw_prefix "${MINGW_ROOT}")
endif()

file(GLOB _mingw_gxx "${_mingw_prefix}/bin/x86_64-w64-mingw32-g++*")
if(NOT _mingw_gxx)
    message(FATAL_ERROR "No x86_64-w64-mingw32-g++ under ${_mingw_prefix}/bin")
endif()
list(GET _mingw_gxx 0 CMAKE_CXX_COMPILER)

file(GLOB _mingw_gcc "${_mingw_prefix}/bin/x86_64-w64-mingw32-gcc*")
list(FILTER _mingw_gcc EXCLUDE REGEX "-[0-9]+(\\.exe)?$")
list(GET _mingw_gcc 0 CMAKE_C_COMPILER)

set(CMAKE_RC_COMPILER "${_mingw_prefix}/bin/x86_64-w64-mingw32-windres")

# Relocated toolchains do not find their sysroot by themselves.
include_directories(SYSTEM "${_mingw_prefix}/x86_64-w64-mingw32/include")
link_directories("${_mingw_prefix}/x86_64-w64-mingw32/lib")

set(CMAKE_FIND_ROOT_PATH "${_mingw_prefix}/x86_64-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
