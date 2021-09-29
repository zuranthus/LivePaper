cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(animated-wallpaper)

FetchContent_Declare(
    tigr
    GIT_REPOSITORY https://github.com/erkkah/tigr.git
    GIT_TAG v2.1.1
)
FetchContent_GetProperties(tigr)
if(NOT tigr_POPULATED)
    FetchContent_Populate(tigr)
    add_library(tigr IMPORTED INTERFACE)
    set_target_properties(tigr PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${tigr_SOURCE_DIR}"
        INTERFACE_SOURCES "${tigr_SOURCE_DIR}/tigr.c"
    )
endif()

add_executable(animated-wallpaper main.c)
set_property(TARGET animated-wallpaper PROPERTY C_STANDARD 99)

target_link_libraries(animated-wallpaper 
    PRIVATE opengl32 gdi32 tigr
)
