## cmake 
################################################################

cmake_minimum_required(VERSION 3.20)
project(lux C)

## options
################################################################

set(common_bin_dir ${CMAKE_SOURCE_DIR}/bin/$<0:>)

if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
else()
    message(FATAL_ERROR, "unsupported platform, supported platforms include: windows (10+), linux (wayland)")
endif()

## project
###############################################################

file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS src/*.c)

if(PLATFORM_WINDOWS)
    list(FILTER lib_files EXCLUDE REGEX "_linux\\.c$")
    add_library(lux SHARED ${lib_files})

    target_link_libraries(lux PRIVATE 
        opengl32
        gdi32
        user32
    )
elseif(PLATFORM_LINUX)
    list(FILTER lib_files EXCLUDE REGEX "_win32\\.c$")
    add_library(lux SHARED ${lib_files})

    target_link_libraries(lux PRIVATE 
        wayland-client
        wayland-egl
        EGL
        GL
        m
    )    
endif()

set_target_properties(lux PROPERTIES
    C_STANDARD 17
    RUNTIME_OUTPUT_DIRECTORY ${common_bin_dir}
    ARCHIVE_OUTPUT_DIRECTORY ${common_bin_dir}
    LIBRARY_OUTPUT_DIRECTORY ${common_bin_dir}
)

target_include_directories(lux PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_definitions(lux PRIVATE
    LX_BUILD
)
