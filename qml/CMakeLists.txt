find_package(Qt6 REQUIRED COMPONENTS Qml Quick QuickControls2)
qt_policy(SET QTP0001 NEW) # QML modules live under :/qt/qml/<URI>

qt_add_executable(DocklyQml WIN32
    main.cpp
    itemimageprovider.h itemimageprovider.cpp
    iconimageprovider.h iconimageprovider.cpp
)

qt_add_qml_module(DocklyQml
    URI LauncherUI
    VERSION 1.0
    QML_FILES
        Main.qml
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
        SettingsPage.qml
        SettingGroup.qml
        SettingCheck.qml
        SettingCombo.qml
)

target_link_libraries(DocklyQml PRIVATE lhcore Qt6::Qml Qt6::Quick Qt6::QuickControls2)
lh_add_version_info(DocklyQml "Dockly (Qt Quick)")
