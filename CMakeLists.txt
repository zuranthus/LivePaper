cmake_minimum_required(VERSION 3.14)
project(live-paper VERSION 0.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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


##### tray (on windows only)
if(WIN32)
    FetchContent_Declare(
        tray
        URL https://raw.githubusercontent.com/zserge/tray/master/tray.h
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_GetProperties(tray)
    if(NOT tray_POPULATED)
        FetchContent_Populate(tray)
        add_library(tray::tray INTERFACE IMPORTED)
        set_target_properties(tray::tray PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${tray_SOURCE_DIR}"
        )
    endif()
endif()


##### argtable
FetchContent_Declare(
    argtable
    URL https://github.com/argtable/argtable3/releases/download/v3.2.1.52f24e5/argtable-v3.2.1.52f24e5-amalgamation.tar.gz
)
FetchContent_GetProperties(argtable)
if(NOT argtable_POPULATED)
    FetchContent_Populate(argtable)
    add_library(argtable::argtable INTERFACE IMPORTED)
    set_target_properties(argtable::argtable PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${argtable_SOURCE_DIR}/"
        INTERFACE_SOURCES "${argtable_SOURCE_DIR}/argtable3.c"
    )
endif()


##### executable
add_executable(live-paper 
    $<$<PLATFORM_ID:Windows>:src/platform_windows.c;assets/win.rc>
    $<$<PLATFORM_ID:Linux>:src/platform_linux.c>
    src/main.c src/fail.c src/video.c
)
set_target_properties(live-paper PROPERTIES 
    WIN32_EXECUTABLE $<PLATFORM_ID:Windows>
    C_STANDARD 99
)
target_link_libraries(live-paper PRIVATE
    $<$<BOOL:${MINGW}>:mingw32>
    "$<$<PLATFORM_ID:Windows>:SDL2::SDL2main;tray::tray;shell32;shlwapi>"
    "$<$<PLATFORM_ID:Linux>:m;X11>"
    SDL2::SDL2 FFmpeg::FFmpeg argtable::argtable
)
add_custom_command(TARGET live-paper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico"
        $<TARGET_FILE_DIR:live-paper>
)
if(MSVC)
    add_custom_command(TARGET live-paper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:live-paper>
    )
endif()


# binaries
if(MSVC)
    set(INSTALL_DIR ".")
    install(TARGETS live-paper RUNTIME DESTINATION ${INSTALL_DIR})
    install(FILES $<TARGET_FILE:SDL2::SDL2> ${FFMPEG_DLLS} "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico" 
        DESTINATION ${INSTALL_DIR}
    )

    set(CPACK_PACKAGE_DIRECTORY dist)
    set(CPACK_GENERATOR WIX ZIP)
    set(CPACK_PACKAGE_NAME LivePaper)
    set(CPACK_PACKAGE_VENDOR Zuranthus)
    set(CPACK_PACKAGE_EXECUTABLES live-paper LivePaper)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/zuranthus/LivePaper")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")

    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_WIX_UPGRADE_GUID A73EF541-19D7-4E50-8CC6-B4E1BAFB760D)

    include(CPack)
endif()
