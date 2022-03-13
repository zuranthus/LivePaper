macro(find_component _component _library _header)
    find_path(${_component}_INCLUDE_DIR ${_header})
    find_library(${_component}_LIBRARY ${_library})
    list(APPEND FFMPEG_INCLUDE_DIRS "${${_component}_INCLUDE_DIR}")
    list(APPEND FFMPEG_LIBRARIES "${${_component}_LIBRARY}")
endmacro()
find_component(AVCODEC avcodec libavcodec/avcodec.h)
find_component(AVFORMAT avformat libavformat/avformat.h)
find_component(AVUTIL avutil libavutil/avutil.h)
find_component(SWSCALE swscale libswscale/swscale.h)

add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
set_target_properties(FFmpeg::FFmpeg PROPERTIES
    INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")
