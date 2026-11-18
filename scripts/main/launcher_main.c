// Dockly.exe is only a stub: everything else lives in resources\. The app itself is resources\DLLs\DocklyApp.dll,
// and every Qt DLL sits in resources\DLLs\ too. Windows only searches the exe's own folder, so point the loader
// at resources\DLLs\ (and Qt at the plugins / QML modules in resources\) before loading the app.
#include <windows.h>
#include <stdlib.h>
#include <wchar.h>

typedef int (*MainFn)(int, char **);

int main(int argc, char **argv)
{
    wchar_t dir[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return 1;
    while (n > 0 && dir[n - 1] != L'\\')
        --n;
    dir[n] = 0;

    wchar_t libs[MAX_PATH + 16];
    swprintf(libs, MAX_PATH + 16, L"%lsresources\\DLLs", dir);
    SetDllDirectoryW(libs);

    wchar_t res[MAX_PATH + 16];
    swprintf(res, MAX_PATH + 16, L"%lsresources", dir);
    _wputenv_s(L"QT_PLUGIN_PATH", res);
    wchar_t qml[MAX_PATH + 16];
    swprintf(qml, MAX_PATH + 16, L"%ls\\qml", res);
    _wputenv_s(L"QML2_IMPORT_PATH", qml);

    wchar_t app[MAX_PATH + 32];
    swprintf(app, MAX_PATH + 32, L"%ls\\DocklyApp.dll", libs);
    HMODULE lib = LoadLibraryExW(app, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    MainFn run = lib ? (MainFn)GetProcAddress(lib, "dockly_main") : NULL;
    if (!run) {
        MessageBoxW(NULL, L"Could not load resources\\DLLs\\DocklyApp.dll", L"Dockly", MB_ICONERROR);
        return 1;
    }
    return run(argc, argv);
}
