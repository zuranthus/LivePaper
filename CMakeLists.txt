cmake_minimum_required(VERSION 3.14)
project(animated-wallpaper)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

##### FFmpeg
if(NOT MSVC)
    find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE)
else()
    FetchContent_Declare(
        FFmpeg
        URL https://www.gyan.dev/ffmpeg/builds/packages/ffmpeg-4.3.2-full_build-shared.7z
    )
    FetchContent_GetProperties(FFmpeg)
    if(NOT ffmpeg_POPULATED)
        FetchContent_Populate(FFmpeg)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
        set_target_properties(FFmpeg::FFmpeg PROPERTIES
            INTERFACE_LINK_DIRECTORIES "${ffmpeg_SOURCE_DIR}/lib"
            INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_SOURCE_DIR}/include/"
            INTERFACE_LINK_LIBRARIES "avcodec;avformat;avutil;swscale"
        )
        set(FFMPEG_DLLS avcodec-58.dll avformat-58.dll avutil-56.dll swresample-3.dll swscale-5.dll)
        list(TRANSFORM FFMPEG_DLLS PREPEND "${ffmpeg_SOURCE_DIR}/bin/")
    endif()
endif()


##### SDL2
if(NOT MSVC)
    find_package(SDL2 REQUIRED)
    if (NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 UNKNOWN IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${SDL2_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        )
    endif()
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
            IMPORTED_LOCATION "${sdl2_SOURCE_DIR}/lib/x64/SDL2main.lib"
        )
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
        INTERFACE_INCLUDE_DIRECTORIES "${cute_headers_SOURCE_DIR}"
    )
endif()


##### tray (on windows only)
if(WIN32)
    FetchContent_Declare(
        tray
        URL https://raw.githubusercontent.com/zserge/tray/master/tray.h
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_GetProperties(tray)
    if(NOT tray_headers_POPULATED)
        FetchContent_Populate(tray)
        add_library(tray::tray INTERFACE IMPORTED)
        set_target_properties(tray::tray PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${tray_SOURCE_DIR}"
        )
    endif()
endif()


##### executable
add_executable(animated-wallpaper 
    $<$<PLATFORM_ID:Windows>:platform_windows.c>
    $<$<PLATFORM_ID:Linux>:platform_linux.c>
    main.c fail.c video.c
)
set_target_properties(animated-wallpaper PROPERTIES 
    WIN32_EXECUTABLE $<PLATFORM_ID:Windows>
    C_STANDARD 99
)
target_link_libraries(animated-wallpaper PRIVATE
    $<$<BOOL:${MINGW}>:mingw32>
    "$<$<PLATFORM_ID:Windows>:SDL2::SDL2main;tray::tray>"
    "$<$<PLATFORM_ID:Linux>:m;X11>"
    SDL2::SDL2 cute_headers::cute_headers FFmpeg::FFmpeg
)
add_custom_command(TARGET animated-wallpaper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icon.ico"
        $<TARGET_FILE_DIR:animated-wallpaper>
)

if(MSVC)
    add_custom_command(TARGET animated-wallpaper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:animated-wallpaper>
    )
endif()
