cmake_minimum_required(VERSION 3.20)

project(lumin LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Both artifacts are x64 only: javaw.exe from modern launchers is 64-bit.
if(WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "lumin targets 64-bit Windows only")
endif()

add_subdirectory(client)
add_subdirectory(injector)
