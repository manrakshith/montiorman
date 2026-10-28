#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <memory>
#include "theme.h"
#include "settings.h"
#include "monitor.h"
#include "ui_flyout.h"

#define WM_TRAYICON (WM_USER + 101)

static NOTIFYICONDATAW g_nid = { sizeof(g_nid) };
static std::unique_ptr<MonitorManager> g_monitorMgr;
static std::unique_ptr<FlyoutWindow> g_flyout;
static HWND g_hTrayWnd = nullptr;

HICON CreateMonitorTrayIcon(bool isDark) {
    int size = GetSystemMetrics(SM_CXSMICON);
    if (size <= 0) size = 16;

    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hColorBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP hMaskBmp = CreateBitmap(size, size, 1, 1, nullptr);

    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hColorBmp);
    if (bits) {
        memset(bits, 0, size * size * 4);
    }

    COLORREF fg = isDark ? RGB(255, 255, 255) : RGB(20, 20, 20);
    HPEN pen = CreatePen(PS_SOLID, 1, fg);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN oPen = (HPEN)SelectObject(memDC, pen);
    HBRUSH oBrush = (HBRUSH)SelectObject(memDC, nullBrush);

    // Monitor screen frame
    int mLeft = 1;
    int mTop = 1;
    int mRight = size - 2;
    int mBottom = size - 5;
    RoundRect(memDC, mLeft, mTop, mRight, mBottom, 2, 2);

    // Stand neck
    MoveToEx(memDC, size / 2, mBottom, nullptr);
    LineTo(memDC, size / 2, size - 1);

    // Stand base
    MoveToEx(memDC, size / 2 - 3, size - 1, nullptr);
    LineTo(memDC, size / 2 + 4, size - 1);

    SelectObject(memDC, oPen);
    SelectObject(memDC, oBrush);
    SelectObject(memDC, oldBmp);
    DeleteObject(pen);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hColorBmp;
    ii.hbmMask = hMaskBmp;
    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hColorBmp);
    DeleteObject(hMaskBmp);
    return hIcon;
}

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONUP) {
            if (g_flyout) {
                g_flyout->Toggle(hwnd);
            }
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN, L"Open MonitorMan");

            // Size Mode submenu
            HMENU hSizeSubMenu = CreatePopupMenu();
            AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_SLEEK, L"Sleek (Slim & Compact)");
            AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_DEFAULT, L"Default (Standard)");
            AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_LARGE, L"Large (Expanded)");

            SizeMode currentMode = g_flyout ? g_flyout->GetSizeMode() : SizeMode::Default;
            UINT checkId = (currentMode == SizeMode::Sleek) ? ID_MODE_SLEEK : 
                           ((currentMode == SizeMode::Large) ? ID_MODE_LARGE : ID_MODE_DEFAULT);
            CheckMenuRadioItem(hSizeSubMenu, ID_MODE_SLEEK, ID_MODE_LARGE, checkId, MF_BYCOMMAND);

            AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSizeSubMenu), L"Size Mode");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_REFRESH, L"Refresh Monitors");

            bool autoStart = IsAutoStartEnabled();
            AppendMenuW(hMenu, MF_STRING | (autoStart ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_STARTUP, L"Start with Windows");

            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessageW(hwnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_TRAY_OPEN:
            if (g_flyout) g_flyout->ShowAtTray();
            break;
        case ID_MODE_SLEEK:
            if (g_flyout) g_flyout->SetSizeMode(SizeMode::Sleek);
            break;
        case ID_MODE_DEFAULT:
            if (g_flyout) g_flyout->SetSizeMode(SizeMode::Default);
            break;
        case ID_MODE_LARGE:
            if (g_flyout) g_flyout->SetSizeMode(SizeMode::Large);
            break;
        case ID_TRAY_REFRESH:
            if (g_monitorMgr) g_monitorMgr->Refresh();
            if (g_flyout) g_flyout->ShowAtTray();
            break;
        case ID_TRAY_STARTUP: {
            bool current = IsAutoStartEnabled();
            SetAutoStart(!current);
            break;
        }
        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_HOTKEY:
        if (g_flyout) {
            g_flyout->Toggle(hwnd);
        }
        return 0;

    case WM_SETTINGCHANGE: {
        if (g_nid.hIcon) {
            DestroyIcon(g_nid.hIcon);
            g_nid.hIcon = CreateMonitorTrayIcon(Theme::CheckIsDarkMode());
            Shell_NotifyIconW(NIM_MODIFY, &g_nid);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 1. Single Instance Check
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"MonitorMan_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // 2. High-DPI & Common Controls
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX icex{sizeof(icex), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icex);

    // 3. Register Hidden Window for Tray Messages
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc   = TrayWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"MonitorManTrayClass";
    RegisterClassExW(&wc);

    g_hTrayWnd = CreateWindowExW(0, wc.lpszClassName, L"MonitorManTray", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!g_hTrayWnd) return 1;

    // 4. Initialize Monitors & Flyout
    g_monitorMgr = std::make_unique<MonitorManager>();
    g_flyout = std::make_unique<FlyoutWindow>(g_monitorMgr.get());
    if (!g_flyout->Create(hInstance)) {
        return 1;
    }

    // 5. Add System Tray Icon & Global Hotkeys
    RegisterHotKey(g_hTrayWnd, 101, MOD_WIN | MOD_ALT, 'B');
    RegisterHotKey(g_hTrayWnd, 102, MOD_CONTROL | MOD_ALT, 'B');

    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hTrayWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = CreateMonitorTrayIcon(Theme::CheckIsDarkMode());
    wcscpy_s(g_nid.szTip, L"MonitorMan - Brightness & Contrast");

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // 6. Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 7. Cleanup
    UnregisterHotKey(g_hTrayWnd, 101);
    UnregisterHotKey(g_hTrayWnd, 102);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) {
        DestroyIcon(g_nid.hIcon);
    }
    if (hMutex) {
        CloseHandle(hMutex);
    }
    return 0;
}
