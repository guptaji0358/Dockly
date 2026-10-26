// Dockly.exe is only a stub: the app itself is DLLs\DocklyApp.dll, and every Qt DLL sits in DLLs\ too.
// Windows only searches the exe's own folder, so point the loader at DLLs\ before loading the app.
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
    swprintf(libs, MAX_PATH + 16, L"%lsDLLs", dir);
    SetDllDirectoryW(libs);

    wchar_t app[MAX_PATH + 32];
    swprintf(app, MAX_PATH + 32, L"%ls\\DocklyApp.dll", libs);
    HMODULE lib = LoadLibraryExW(app, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    MainFn run = lib ? (MainFn)GetProcAddress(lib, "dockly_main") : NULL;
    if (!run) {
        MessageBoxW(NULL, L"Could not load DLLs\\DocklyApp.dll", L"Dockly", MB_ICONERROR);
        return 1;
    }
    return run(argc, argv);
}
