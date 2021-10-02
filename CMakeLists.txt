cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(animated-wallpaper)

if(WIN32)
    FetchContent_Declare(
        sdl
        URL https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip
    )

    FetchContent_GetProperties(sdl)
    if(NOT sdl_POPULATED)
        FetchContent_Populate(sdl)
        add_library(sdl SHARED IMPORTED)
        set_target_properties(sdl PROPERTIES
            IMPORTED_LOCATION "${sdl_SOURCE_DIR}/lib/x64/SDL2.dll"
            IMPORTED_IMPLIB "${sdl_SOURCE_DIR}/lib/x64/SDL2.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${sdl_SOURCE_DIR}/include/"
        )
        target_link_libraries(sdl INTERFACE "${sdl_SOURCE_DIR}/lib/x64/SDL2main.lib")
    endif()
endif()

FetchContent_Declare(
    cute_headers
    GIT_REPOSITORY https://github.com/RandyGaul/cute_headers.git
)
FetchContent_GetProperties(cute_headers)
if(NOT cute_headers_POPULATED)
    FetchContent_Populate(cute_headers)
    add_library(cute_headers INTERFACE IMPORTED)
    set_target_properties(cute_headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${cute_headers_SOURCE_DIR}/"
    )
endif()

add_executable(animated-wallpaper main.c)
set_property(TARGET animated-wallpaper PROPERTY C_STANDARD 99)

target_link_libraries(animated-wallpaper 
    PRIVATE sdl cute_headers
)

if(WIN32)
    set_property(TARGET animated-wallpaper PROPERTY VS_DPI_AWARE "PerMonitor")

    add_custom_command(TARGET animated-wallpaper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sdl>
            $<TARGET_FILE_DIR:animated-wallpaper>)
endif()
