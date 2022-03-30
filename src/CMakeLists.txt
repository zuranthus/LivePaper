add_library(livepaper-lib STATIC ffmpeg_decoder.cpp decoding_loop.cpp)
target_include_directories(livepaper-lib PUBLIC .)
target_compile_features(livepaper-lib PUBLIC cxx_std_23)
find_package(tl-expected CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
include(AddVcpkgFFmpegTarget)
target_link_libraries(livepaper-lib
    PUBLIC tl::expected Microsoft.GSL::GSL
    PRIVATE FFmpeg::FFmpeg)