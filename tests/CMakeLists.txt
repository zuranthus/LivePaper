add_executable(lp-test main.cpp expected.cpp ffmpeg_decoder.cpp)
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(lp-test Catch2::Catch2 lp-lib)

include(CTest)
include(Catch)
catch_discover_tests(lp-test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})