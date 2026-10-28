#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <memory>
#include <vector>
#include "theme.h"
#include "settings.h"
#include "solar.h"
#include "ui_flyout.h"
#include "ui_settings.h"

#define WM_TRAYICON (WM_USER + 101)
#define ID_TIMER_SOLAR 5001

static NOTIFYICONDATAW g_nid = { sizeof(g_nid) };
static std::unique_ptr<MonitorManager> g_monitorMgr;
static std::unique_ptr<SolarTracker> g_solarTracker;
static std::unique_ptr<FlyoutWindow> g_flyout;
static std::unique_ptr<SettingsWindow> g_settings;
static HWND g_hTrayWnd = nullptr;
static UINT g_uTaskbarRestartMsg = 0;

HICON CreateMonitorTrayIcon(bool isDark) {
    int size = GetSystemMetrics(SM_CXSMICON);
    if (size <= 0) size = 16;

    HICON hResIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101), IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    if (hResIcon) {
        return hResIcon;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    DWORD* bits = nullptr;
    HBITMAP hColorBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&bits), nullptr, 0);

    if (bits) {
        memset(bits, 0, size * size * sizeof(DWORD));
    }

    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hColorBmp);

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

    // Post-process pixels to give full 0xFF alpha to drawn lines
    // and build a proper 1bpp monochrome mask (0 = opaque, 1 = transparent)
    int maskPitch = ((size + 15) / 16) * 2;
    std::vector<BYTE> maskBits(maskPitch * size, 0xFF);

    if (bits) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int idx = y * size + x;
                DWORD col = bits[idx] & 0x00FFFFFF;
                if (col != 0) {
                    bits[idx] = col | 0xFF000000;
                    maskBits[y * maskPitch + (x / 8)] &= ~(1 << (7 - (x % 8)));
                }
            }
        }
    }

    HBITMAP hMaskBmp = CreateBitmap(size, size, 1, 1, maskBits.data());

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
    if (msg == g_uTaskbarRestartMsg && g_uTaskbarRestartMsg != 0) {
        Shell_NotifyIconW(NIM_ADD, &g_nid);
        return 0;
    }
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
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings...");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

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
        case ID_TRAY_SETTINGS:
            if (g_settings) g_settings->Show();
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

    case WM_TIMER: {
        if (wParam == ID_TIMER_SOLAR && g_solarTracker && g_monitorMgr) {
            if (g_monitorMgr->GetDeviceCount() > 0) {
                auto* dev = g_monitorMgr->GetDevice(0);
                if (dev) {
                    int nextVal = 0;
                    if (g_solarTracker->Tick(dev->currBrightness, nextVal)) {
                        g_monitorMgr->SetBrightness(0, nextVal, false);
                        if (g_flyout) {
                            g_flyout->UpdateBrightnessDisplay(nextVal);
                        }
                    }
                }
            }
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
        KillTimer(hwnd, ID_TIMER_SOLAR);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 1. Single Instance Check: activate existing instance if running
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"MonitorMan_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"MonitorManTrayClass", L"MonitorManTray");
        if (hExisting) {
            PostMessageW(hExisting, WM_COMMAND, MAKEWPARAM(ID_TRAY_OPEN, 0), 0);
        }
        return 0;
    }

    // 2. High-DPI & Common Controls
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX icex{sizeof(icex), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icex);

    // 3. Register Hidden Window for Tray Messages
    g_uTaskbarRestartMsg = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc   = TrayWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"MonitorManTrayClass";
    RegisterClassExW(&wc);

    // Note: Do NOT use HWND_MESSAGE here, as Shell_NotifyIcon requires a standard top-level window to receive shell notifications!
    g_hTrayWnd = CreateWindowExW(0, wc.lpszClassName, L"MonitorManTray", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (!g_hTrayWnd) return 1;

    // 4. Initialize Monitors, Solar Tracker, & Flyout
    g_monitorMgr = std::make_unique<MonitorManager>();
    g_solarTracker = std::make_unique<SolarTracker>();

    g_flyout = std::make_unique<FlyoutWindow>(g_monitorMgr.get());
    if (!g_flyout->Create(hInstance)) {
        return 1;
    }

    g_flyout->SetOnBrightnessChanged([](int val) {
        if (g_solarTracker) {
            g_solarTracker->OnUserBrightnessChanged(val);
        }
    });

    g_settings = std::make_unique<SettingsWindow>(g_monitorMgr.get(), g_solarTracker.get());
    g_settings->SetOnSizeModeChanged([](SizeMode mode) {
        if (g_flyout) g_flyout->SetSizeMode(mode);
    });
    g_settings->SetOnMonitorsRefreshed([]() {
        if (g_flyout) g_flyout->ShowAtTray();
    });
    g_settings->Create(hInstance);

    g_flyout->SetOnOpenSettings([]() {
        if (g_settings) g_settings->Show();
    });

    // 5. Add System Tray Icon, Timer, & Global Hotkeys
    RegisterHotKey(g_hTrayWnd, 101, MOD_WIN | MOD_ALT, 'B');
    RegisterHotKey(g_hTrayWnd, 102, MOD_CONTROL | MOD_ALT, 'B');

    // Start 15s timer for solar daylight tracking
    SetTimer(g_hTrayWnd, ID_TIMER_SOLAR, 15000, nullptr);

    // Initial solar sync if enabled
    if (g_solarTracker->GetSettings().enabled && g_monitorMgr->GetDeviceCount() > 0) {
        auto* dev = g_monitorMgr->GetDevice(0);
        if (dev) {
            int target = g_solarTracker->GetEffectiveTarget();
            g_monitorMgr->SetBrightness(0, target, false);
            g_flyout->UpdateBrightnessDisplay(target);
        }
    }

    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hTrayWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = CreateMonitorTrayIcon(Theme::CheckIsDarkMode());
    wcscpy_s(g_nid.szTip, L"MonitorMan - Brightness & Contrast");

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Show immediately upon launch
    if (g_flyout) {
        g_flyout->ShowAtTray();
    }

    // 6. Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 7. Cleanup
    KillTimer(g_hTrayWnd, ID_TIMER_SOLAR);
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
