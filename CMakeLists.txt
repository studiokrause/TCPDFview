cmake_minimum_required(VERSION 3.16)
project(tcpdfview CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tcpdfview SHARED
    src/plugin.cpp
    src/pdfparse.cpp
    src/ghostscript.cpp
    src/gsrender.cpp
    src/shellthumb.cpp
    src/cache.cpp
    src/lang.cpp
    src/tcpdfview.def
)

# TC convention: 32-bit -> .wlx, 64-bit -> .wlx64
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TC_SUFFIX ".wlx64")
else()
    set(TC_SUFFIX ".wlx")
endif()

set_target_properties(tcpdfview PROPERTIES
    PREFIX ""
    OUTPUT_NAME "tcpdfview"
    SUFFIX "${TC_SUFFIX}"
)

target_include_directories(tcpdfview PRIVATE src)

if(WIN32)
    target_link_libraries(tcpdfview user32 gdi32 msimg32 shell32 ole32 comctl32)
    target_compile_definitions(tcpdfview PRIVATE UNICODE _UNICODE)
endif()
