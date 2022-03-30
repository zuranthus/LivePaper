add_executable(livepaper-tests
    main.cpp expected.cpp ffmpeg_decoder.cpp decoding_loop.cpp stopwatch.cpp)
find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(livepaper-tests Catch2::Catch2 livepaper-lib)

include(CTest)
include(Catch)
catch_discover_tests(livepaper-tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})