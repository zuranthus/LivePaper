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

        message(STATUS "${sdl_SOURCE_DIR}/include/")
    endif()
endif()

add_executable(animated-wallpaper main.c)

target_link_libraries(animated-wallpaper PRIVATE
    sdl
)

if(WIN32)
  add_custom_command(TARGET animated-wallpaper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sdl>
        $<TARGET_FILE_DIR:animated-wallpaper>)
endif()
