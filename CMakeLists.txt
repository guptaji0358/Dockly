cmake_minimum_required(VERSION 3.21)
project(LauncherHub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Sql Svg OpenGLWidgets)
qt_standard_project_setup()

option(LH_BUILD_WIDGETS "Build the Qt Widgets frontend" ON)
option(LH_BUILD_QML "Build the Qt Quick / QML frontend" ON)

add_subdirectory(core)
enable_testing()
add_subdirectory(tests)
if(LH_BUILD_WIDGETS)
    add_subdirectory(widgets)
endif()
if(LH_BUILD_QML)
    add_subdirectory(qml)
endif()
