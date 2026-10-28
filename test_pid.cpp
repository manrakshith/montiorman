#include <windows.h>
#include <stdio.h>

BOOL CALLBACK EnumThreadWnd(HWND hwnd, LPARAM) {
    wchar_t cls[128] = {0};
    wchar_t tit[128] = {0};
    GetClassNameW(hwnd, cls, 128);
    GetWindowTextW(hwnd, tit, 128);
    printf("HWND: %p Class:[%ls] Title:[%ls] Vis:%d\n", hwnd, cls, tit, IsWindowVisible(hwnd));
    return TRUE;
}

int main() {
    // Find PID of monitorman
    DWORD pid = 0;
    HWND hFind = FindWindowW(nullptr, nullptr);
    // Find process
    PROCESSENTRY32 pe;
    // ...
    return 0;
}
