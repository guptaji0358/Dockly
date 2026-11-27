# Core library, test executable and launcher stub. Layout:
#   include/  headers          src/  implementation (.cpp)          main/  entry points (one per executable / DLL)

set(LH_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LH_SRC     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LH_MAIN    ${CMAKE_CURRENT_SOURCE_DIR}/main)
set(LH_INCLUDE ${LH_INCLUDE} PARENT_SCOPE)
set(LH_SRC     ${LH_SRC}     PARENT_SCOPE)
set(LH_MAIN    ${LH_MAIN}    PARENT_SCOPE)

add_library(lhcore STATIC
    ${LH_INCLUDE}/lhtypes.h
    ${LH_INCLUDE}/item.h ${LH_SRC}/item.cpp
    ${LH_INCLUDE}/appsettings.h ${LH_SRC}/appsettings.cpp
    ${LH_INCLUDE}/iconpack.h ${LH_SRC}/iconpack.cpp
    ${LH_INCLUDE}/database.h ${LH_SRC}/database.cpp
    ${LH_INCLUDE}/theme.h ${LH_SRC}/theme.cpp
    ${LH_INCLUDE}/softwarescanner.h ${LH_SRC}/softwarescanner.cpp
    ${LH_INCLUDE}/appcore.h ${LH_SRC}/appcore.cpp
    ${LH_INCLUDE}/itemmodel.h ${LH_SRC}/itemmodel.cpp
    ${LH_INCLUDE}/tray.h ${LH_SRC}/tray.cpp
    ${LH_INCLUDE}/hotkey.h ${LH_SRC}/hotkey.cpp
    ${LH_INCLUDE}/dockguard.h ${LH_SRC}/dockguard.cpp
    ${LH_INCLUDE}/connectors.h ${LH_SRC}/connectors.cpp
    ${LH_INCLUDE}/sites.h ${LH_SRC}/sites.cpp
    ${LH_INCLUDE}/runningapps.h ${LH_SRC}/runningapps.cpp
    ${LH_INCLUDE}/procinspect.h ${LH_SRC}/procinspect.cpp ${LH_SRC}/procinspect_attr.cpp ${LH_SRC}/procinspect_win.cpp
    ${LH_INCLUDE}/guardengine.h ${LH_SRC}/guardengine.cpp ${LH_SRC}/guardengine_win.cpp
    ${LH_INCLUDE}/guardledger.h ${LH_SRC}/guardledger.cpp
    ${LH_INCLUDE}/guardexec.h ${LH_SRC}/guardexec.cpp ${LH_SRC}/guardexec_win.cpp
    ${LH_INCLUDE}/guardstore.h ${LH_SRC}/guardstore.cpp
    ${LH_INCLUDE}/guardstartup.h ${LH_SRC}/guardstartup.cpp ${LH_SRC}/guardstartup_win.cpp
    ${LH_SRC}/appcore_guard.cpp
    ${LH_INCLUDE}/gamepad.h ${LH_SRC}/gamepad.cpp
    ${LH_INCLUDE}/perfmonitor.h ${LH_SRC}/perfmonitor.cpp
)

target_include_directories(lhcore PUBLIC ${LH_INCLUDE})
target_compile_definitions(lhcore PUBLIC LH_VERSION="${PROJECT_VERSION}")
target_link_libraries(lhcore PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Sql Qt6::Svg)

# core_test.exe
find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(core_test ${LH_MAIN}/core_test.cpp)
if(WIN32)
    target_link_libraries(lhcore PUBLIC psapi dwmapi pdh version ole32 shell32 uuid gdi32 crypt32)
endif()
target_link_libraries(core_test PRIVATE lhcore Qt6::Test)
add_test(NAME core_test COMMAND core_test)
qt_add_executable(guard_test ${LH_MAIN}/guard_test.cpp)
target_link_libraries(guard_test PRIVATE lhcore Qt6::Test)
add_test(NAME guard_test COMMAND guard_test)

# Dockly.exe: stub that loads DLLs\DocklyApp.dll (see main/launcher_main.c).
add_executable(Dockly WIN32 ${LH_MAIN}/launcher_main.c)
target_link_options(Dockly PRIVATE -static -mwindows)
lh_add_version_info(Dockly "Dockly")

qt_add_executable(guard3_test ${LH_MAIN}/guard3_test.cpp)
target_link_libraries(guard3_test PRIVATE lhcore Qt6::Test)
add_test(NAME guard3_test COMMAND guard3_test)
