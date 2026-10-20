add_library(lumin SHARED
    src/main.cpp

    src/core/jvm.cpp
    src/core/bridge.cpp

    src/api/module.cpp
    src/api/module_manager.cpp

    src/render/theme.cpp
    src/render/ui.cpp
    src/render/logo.cpp

    src/hook/input.cpp
    src/hook/swap.cpp

    src/modules/registry.cpp
    src/modules/combat.cpp
    src/modules/movement.cpp
    src/modules/render.cpp
    src/modules/world.cpp
    src/modules/client.cpp

    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(lumin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_compile_definitions(lumin PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    IMGUI_DEFINE_MATH_OPERATORS
)

# --- JNI -------------------------------------------------------------------
# The DLL only needs jni.h; the VM itself lives in the host process.
find_package(JNI QUIET)
if(JNI_INCLUDE_DIRS)
    target_include_directories(lumin PRIVATE ${JNI_INCLUDE_DIRS})
elseif(DEFINED ENV{JAVA_HOME})
    target_include_directories(lumin PRIVATE
        $ENV{JAVA_HOME}/include
        $ENV{JAVA_HOME}/include/win32
        $ENV{JAVA_HOME}/include/linux
    )
else()
    message(FATAL_ERROR "No JNI headers found. Install a JDK 21 and set JAVA_HOME.")
endif()

# --- MinHook ---------------------------------------------------------------
add_subdirectory(third_party/minhook EXCLUDE_FROM_ALL)
target_link_libraries(lumin PRIVATE minhook)

if(WIN32)
    target_link_libraries(lumin PRIVATE opengl32 gdi32 user32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(lumin PRIVATE OpenGL::GL)
endif()

set_target_properties(lumin PROPERTIES OUTPUT_NAME "lumin" PREFIX "")

# Static CRT so the DLL needs nothing but the game's own runtime.
if(MINGW)
    target_link_options(lumin PRIVATE -static -static-libgcc -static-libstdc++)

    # std::thread on mingw needs winpthreads; -static below pulls it in.
    find_package(Threads REQUIRED)
    target_link_libraries(lumin PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
elseif(MSVC)
    set_property(TARGET lumin PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
