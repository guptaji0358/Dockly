# Dockly.exe: stub that loads DLLs\DocklyApp.dll (see launcher/main.c).
add_executable(Dockly WIN32 main.c)
target_link_options(Dockly PRIVATE -static -mwindows)
lh_add_version_info(Dockly "Dockly")
