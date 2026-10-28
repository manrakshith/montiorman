#include <windows.h>
#include <dwmapi.h>
#include <fstream>
#include <string>

std::ofstream g_log("qs_debug.log", std::ios::app);

void Check() {
    HWND h1 = FindWindowW(L"ControlCenterWindow", nullptr);
    HWND h2 = FindWindowW(L"Windows.UI.Core.CoreWindow", L"Quick settings");
    HWND h3 = FindWindowW(nullptr, L"Quick settings");

    auto LogHwnd = [](const char* name, HWND h) {
        if (!h) return;
        int cloaked = 0;
        DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        RECT r;
        GetWindowRect(h, &r);
        g_log << name << ": h=" << h << " Vis=" << IsWindowVisible(h) 
              << " Cloaked=" << cloaked << " Rect=(" << r.left << "," << r.top << "," << r.right << "," << r.bottom << ")" << std::endl;
    };

    if (h1) LogHwnd("ControlCenterWindow", h1);
    if (h2) LogHwnd("CoreWindow_QuickSettings", h2);
    if (h3 && h3 != h1 && h3 != h2) LogHwnd("Title_QuickSettings", h3);
}

int main() {
    g_log << "--- Starting 15s scan ---" << std::endl;
    for (int i = 0; i < 150; ++i) {
        Check();
        Sleep(100);
    }
    g_log << "--- Done ---" << std::endl;
    return 0;
}
