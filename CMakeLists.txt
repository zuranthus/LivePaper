cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(animated-wallpaper)


##### SDL2
if(NOT MSVC)
    find_package(SDL2 REQUIRED)
else()
    FetchContent_Declare(
        SDL2
        URL https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip
    )
    FetchContent_GetProperties(SDL2)
    if(NOT sdl2_POPULATED)
        FetchContent_Populate(SDL2)
        add_library(SDL2::SDL2 SHARED IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll"
            IMPORTED_IMPLIB "${sdl2_SOURCE_DIR}/lib/x64/SDL2.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${sdl2_SOURCE_DIR}/include/"
        )
        add_library(SDL2::SDL2main STATIC IMPORTED)
        set_target_properties(SDL2::SDL2main PROPERTIES
            IMPORTED_LOCATION "${sdl2_SOURCE_DIR}/lib/x64/SDL2main.lib")
    endif()
endif()


##### cute_headers
FetchContent_Declare(
    cute_headers
    GIT_REPOSITORY https://github.com/RandyGaul/cute_headers.git
)
FetchContent_GetProperties(cute_headers)
if(NOT cute_headers_POPULATED)
    FetchContent_Populate(cute_headers)
    add_library(cute_headers::cute_headers INTERFACE IMPORTED)
    set_target_properties(cute_headers::cute_headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${cute_headers_SOURCE_DIR}/"
    )
endif()


##### executable
add_executable(animated-wallpaper main.c)
set_property(TARGET animated-wallpaper PROPERTY C_STANDARD 99)

if(WIN32)
    if(MINGW)
        list(APPEND PLATFORM_LIBS mingw32)
    endif()
    list(APPEND PLATFORM_LIBS SDL2::SDL2main)

    if(MSVC)
        set_property(TARGET animated-wallpaper PROPERTY VS_DPI_AWARE "PerMonitor")
        add_custom_command(TARGET animated-wallpaper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E 
            copy $<TARGET_RUNTIME_DLLS:animated-wallpaper> $<TARGET_FILE_DIR:animated-wallpaper>
            COMMAND_EXPAND_LISTS
        )
    endif()
elseif(UNIX)
    list(APPEND PLATFORM_LIBS m X11)
endif()

target_link_libraries(animated-wallpaper 
    PRIVATE ${PLATFORM_LIBS} SDL2::SDL2 cute_headers::cute_headers
)
