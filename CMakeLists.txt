cmake_minimum_required(VERSION 3.21)
project(Dockly VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Sql Svg OpenGLWidgets)
qt_standard_project_setup()

option(LH_BUILD_WIDGETS "Build the Qt Widgets frontend" ON)
option(LH_BUILD_QML "Build the Qt Quick / QML frontend" ON)

# Windows file properties (Company, Copyright, Trademark ...) for an executable target.
function(lh_add_version_info target description)
    set(RC_NAME ${target})
    set(RC_DESCRIPTION ${description})
    configure_file(${CMAKE_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc @ONLY)
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc)
endfunction()

add_subdirectory(core)
enable_testing()
add_subdirectory(tests)
if(LH_BUILD_WIDGETS)
    add_subdirectory(widgets)
endif()
if(LH_BUILD_QML)
    add_subdirectory(qml)
endif()
