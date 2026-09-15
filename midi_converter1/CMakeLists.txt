cmake_minimum_required(VERSION 3.16)
project(RobloxMIDI100 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RtMidi REQUIRED rtmidi)

# X11 と XTest (Xtst) を見つける
find_package(X11 REQUIRED)
find_library(XTEST_LIB Xtst REQUIRED)

add_executable(RobloxMIDI100
    src/main.cpp
    src/windowUI.cpp
    src/x11_helper.cpp
    src/x11_helper.h
    resources.qrc
)

target_include_directories(RobloxMIDI100 PRIVATE ${RtMidi_INCLUDE_DIRS} ${X11_INCLUDE_DIR})
target_link_directories(RobloxMIDI100 PRIVATE ${RtMidi_LIBRARY_DIRS})

target_link_libraries(RobloxMIDI100 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${RtMidi_LIBRARIES}
    ${X11_LIBRARIES}
    ${XTEST_LIB}
)