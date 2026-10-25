add_library(lhcore STATIC
    lhtypes.h
    item.h item.cpp
    appsettings.h appsettings.cpp
    iconpack.h iconpack.cpp
    database.h database.cpp
    theme.h theme.cpp
    softwarescanner.h softwarescanner.cpp
    appcore.h appcore.cpp
    itemmodel.h itemmodel.cpp
    tray.h tray.cpp
    hotkey.h hotkey.cpp
    connectors.h connectors.cpp
)

target_include_directories(lhcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(lhcore PUBLIC LH_VERSION="${PROJECT_VERSION}")
target_link_libraries(lhcore PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Concurrent Qt6::Sql Qt6::Svg)
