@echo off
setlocal

rem Builds lumin.dll and lumin-loader.exe. Needs CMake 3.20+, a 64-bit
rem toolchain (Visual Studio 2022 or MinGW-w64) and a JDK 21 for jni.h.

if "%JAVA_HOME%"=="" (
    echo [!] JAVA_HOME is not set; CMake needs it to find jni.h
    exit /b 1
)

set BUILD_DIR=build

cmake -S . -B %BUILD_DIR% -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [!] configure failed
    exit /b 1
)

cmake --build %BUILD_DIR% --config Release
if errorlevel 1 (
    echo [!] build failed
    exit /b 1
)

echo.
echo [ok] %BUILD_DIR%\client\Release\lumin.dll
echo [ok] %BUILD_DIR%\injector\Release\lumin-loader.exe
