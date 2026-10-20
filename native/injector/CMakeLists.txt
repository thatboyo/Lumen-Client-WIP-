add_executable(lumin-loader main.cpp)
add_dependencies(lumin-loader lumin)

set_target_properties(lumin-loader PROPERTIES OUTPUT_NAME "lumin-loader")

target_compile_definitions(lumin-loader PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(lumin-loader PRIVATE advapi32)

if(MINGW)
    # wmain() needs the Unicode entry point.
    target_link_options(lumin-loader PRIVATE -municode -static -static-libgcc -static-libstdc++)
elseif(MSVC)
    set_property(TARGET lumin-loader PROPERTY MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Keep the pair together: running the loader from the build tree finds the DLL.
# A custom target (rather than POST_BUILD) re-copies whenever the DLL changes,
# even if the loader itself did not relink.
add_custom_target(copy-lumin ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:lumin> $<TARGET_FILE_DIR:lumin-loader>/$<TARGET_FILE_NAME:lumin>
    DEPENDS lumin lumin-loader
    COMMENT "Copying lumin.dll next to the loader"
)
