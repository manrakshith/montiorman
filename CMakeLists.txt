cmake_minimum_required(VERSION 3.16)
project(MonitorMan LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    res/resource.rc
)

add_executable(monitorman WIN32 ${SOURCES})

target_include_directories(monitorman PRIVATE src)

target_link_libraries(monitorman PRIVATE
    dxva2
    dwmapi
    uxtheme
    comctl32
    gdi32
    user32
    shell32
)

if (MINGW)
    target_link_options(monitorman PRIVATE
        -mwindows
        -static
        -static-libgcc
        -static-libstdc++
        -s
    )
endif()

