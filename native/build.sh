#!/usr/bin/env bash
# Cross-compiles lumin.dll and lumin-loader.exe for Windows x64 from Linux.
#
#   MINGW_ROOT=/opt/mingw64 JAVA_HOME=/opt/jdk-21 ./build.sh
#
# MINGW_ROOT must contain a MinGW-w64 toolchain (bin/x86_64-w64-mingw32-g++)
# or a Debian mingw package tree (usr/bin/..., usr/x86_64-w64-mingw32/...).
set -euo pipefail

: "${JAVA_HOME:?set JAVA_HOME to a JDK 21}"
: "${MINGW_ROOT:?set MINGW_ROOT to a mingw-w64 install}"

BUILD_DIR="${BUILD_DIR:-build-win}"

# Ninja is faster and is what CI uses, but plain make works too.
GENERATOR=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR=(-G Ninja)
fi

cmake -S . -B "$BUILD_DIR" ${GENERATOR[@]+"${GENERATOR[@]}"} \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" --parallel

printf '\n[ok] %s/client/lumin.dll\n' "$BUILD_DIR"
printf '[ok] %s/injector/lumin-loader.exe\n' "$BUILD_DIR"
