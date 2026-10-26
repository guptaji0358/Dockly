qt_policy(SET QTP0001 NEW) # QML modules live under :/qt/qml/<URI>
qt_policy(SET QTP0004 NEW) # each QML subdirectory gets its own qmldir

qt_add_library(DocklyApp SHARED
    ${LH_MAIN}/app_main.cpp
    ${LH_INCLUDE}/itemimageprovider.h ${LH_SRC}/itemimageprovider.cpp
    ${LH_INCLUDE}/iconimageprovider.h ${LH_SRC}/iconimageprovider.cpp
)

set_source_files_properties(components/Fx.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(DocklyApp
    URI DocklyUI
    NO_PLUGIN
    VERSION 1.0
    QML_FILES
        Main.qml
        DockWindow.qml
        components/Fx.qml
        components/Nebula.qml
        components/Tile.qml
        components/ItemSpace.qml
        components/Ring.qml
        components/Glass.qml
        components/OrbButton.qml
        components/HudField.qml
        components/AppButton.qml
        dialogs/AppDialog.qml
        dialogs/ItemDialog.qml
        dialogs/PickDialog.qml
        dialogs/ChoiceDialog.qml
        components/Card.qml
        components/Icon.qml
        components/ViewToolbar.qml
        components/DetailPanel.qml
        dialogs/PickSoftwareDialog.qml
        components/ItemGrid.qml
        pages/HomePage.qml
        pages/CategoryPage.qml
        pages/CollectionsPage.qml
        pages/ConnectorsPage.qml
        dialogs/ConnectorPicker.qml
        pages/SettingsPage.qml
        components/SettingGroup.qml
        components/SettingCheck.qml
        components/SettingCombo.qml
)

# GPU effects: compiled to .qsb for every graphics API at build time.
qt_add_shaders(DocklyApp "dockly_shaders"
    PREFIX "/qt/qml/DocklyUI"
    FILES shaders/aurora.frag
)

target_link_libraries(DocklyApp PRIVATE lhcore Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::GuiPrivate)

# Browser extension (Chrome / Edge) ships next to the executable.
add_custom_command(TARGET DocklyApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/connectors $<TARGET_FILE_DIR:DocklyApp>/connectors)
