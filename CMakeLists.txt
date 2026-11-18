cmake_minimum_required(VERSION 3.21)
project(Dockly VERSION 1.0.0 LANGUAGES C CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Sql Svg Qml Quick QuickControls2 ShaderTools GuiPrivate)
qt_standard_project_setup()

# Windows file properties (Company, Copyright, Trademark ...) for an executable target.
function(lh_add_version_info target description)
    set(RC_NAME ${target})
    set(RC_DESCRIPTION ${description})
    configure_file(${CMAKE_SOURCE_DIR}/cmake/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc @ONLY)
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${target}.rc)
endfunction()

enable_testing()
add_subdirectory(scripts)
add_subdirectory(qml)
