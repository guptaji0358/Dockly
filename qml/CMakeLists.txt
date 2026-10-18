qt_policy(SET QTP0001 NEW) # QML modules live under :/qt/qml/<URI>

qt_add_executable(Dockly WIN32
    main.cpp
    itemimageprovider.h itemimageprovider.cpp
    iconimageprovider.h iconimageprovider.cpp
)

set_source_files_properties(Fx.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Dockly
    URI DocklyUI
    VERSION 1.0
    QML_FILES
        Main.qml
        Fx.qml
        Nebula.qml
        Tile.qml
        ItemSpace.qml
        Ring.qml
        Glass.qml
        OrbButton.qml
        HudField.qml
        AppButton.qml
        AppDialog.qml
        ItemDialog.qml
        PickDialog.qml
        ChoiceDialog.qml
        Card.qml
        Icon.qml
        ViewToolbar.qml
        DetailPanel.qml
        PickSoftwareDialog.qml
        ItemGrid.qml
        HomePage.qml
        CategoryPage.qml
        CollectionsPage.qml
        ConnectorsPage.qml
        ConnectorPicker.qml
        SettingsPage.qml
        SettingGroup.qml
        SettingCheck.qml
        SettingCombo.qml
)

# GPU effects: compiled to .qsb for every graphics API at build time.
qt_add_shaders(Dockly "dockly_shaders"
    PREFIX "/qt/qml/DocklyUI"
    FILES shaders/aurora.frag
)

target_link_libraries(Dockly PRIVATE lhcore Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::GuiPrivate)
lh_add_version_info(Dockly "Dockly")

# Browser extension (Chrome / Edge) ships next to the executable.
add_custom_command(TARGET Dockly POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/connectors $<TARGET_FILE_DIR:Dockly>/connectors)
