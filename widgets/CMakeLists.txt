qt_add_executable(LauncherHubWidgets WIN32
    main.cpp
    itemdelegate.h itemdelegate.cpp
    itemview.h itemview.cpp
    itemactions.h itemactions.cpp
    homepage.h homepage.cpp
    categorypage.h categorypage.cpp
    collectionspage.h collectionspage.cpp
    settingspage.h settingspage.cpp
    detailpanel.h detailpanel.cpp
    viewtoolbar.h viewtoolbar.cpp
    mainwindow.h mainwindow.cpp
)

target_link_libraries(LauncherHubWidgets PRIVATE lhcore Qt6::OpenGLWidgets)
