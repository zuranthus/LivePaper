cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(
    livepaper
    VERSION 0.0.0
    LANGUAGES CXX
)

add_library(lp-lib STATIC src/video.cpp)
target_include_directories(lp-lib PUBLIC src)
target_compile_features(lp-lib PUBLIC cxx_std_23)
find_package(tl-expected CONFIG REQUIRED)
include(AddVcpkgFFmpegTarget)
target_link_libraries(lp-lib PUBLIC tl::expected PRIVATE FFmpeg::FFmpeg)

add_executable(lp-exe src/main.cpp)
target_link_libraries(lp-exe lp-lib)

add_executable(lp-test tests/test.cpp)
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(lp-test Catch2::Catch2 lp-lib)

include(CTest)
include(Catch)
catch_discover_tests(lp-test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})