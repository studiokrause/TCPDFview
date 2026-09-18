cmake_minimum_required(VERSION 3.10)
project(tcpdfview CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(tcpdfview SHARED
    src/plugin.cpp
    src/ghostscript.cpp
    src/cache.cpp
    src/lang.cpp
)

set_target_properties(tcpdfview PROPERTIES
    PREFIX ""
    SUFFIX ".wlx"
)

if(WIN32)
    target_link_libraries(tcpdfview user32 gdi32 shell32 ole32)
endif()
