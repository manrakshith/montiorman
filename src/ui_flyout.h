#pragma once

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <string>
#include <memory>
#include "theme.h"
#include "settings.h"
#include "monitor.h"
#include "ui_slider.h"

class FlyoutWindow {
private:
    HWND m_hwnd = nullptr;
    Theme m_theme;
    HFONT m_fontIcon = nullptr;
    HFONT m_fontValue = nullptr;
    HBRUSH m_hEditBgBrush = nullptr;

    MonitorManager* m_monitorMgr = nullptr;
    size_t m_currentDeviceIndex = 0;
    SizeMode m_sizeMode = SizeMode::Default;

    FluentSlider m_brightnessSlider;
    FluentSlider m_contrastSlider;
    bool m_trackingMouse = false;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        FlyoutWindow* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<FlyoutWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        } else {
            self = reinterpret_cast<FlyoutWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self) {
            return self->HandleMessage(msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void GetFlyoutDimensions(int& width, int& height) const {
        if (m_sizeMode == SizeMode::Sleek) {
            width = 280;
            height = 74;
        } else if (m_sizeMode == SizeMode::Large) {
            width = 410;
            height = 118;
        } else { // Default
            width = 330;
            height = 92;
        }
    }

    void InitFonts() {
        if (m_fontIcon) DeleteObject(m_fontIcon);
        if (m_fontValue) DeleteObject(m_fontValue);

        if (m_sizeMode == SizeMode::Sleek) {
            m_fontIcon  = CreateSegoeFont(11, FW_NORMAL);
            m_fontValue = CreateSegoeFont(9, FW_SEMIBOLD);
        } else if (m_sizeMode == SizeMode::Large) {
            m_fontIcon  = CreateSegoeFont(16, FW_NORMAL);
            m_fontValue = CreateSegoeFont(12, FW_SEMIBOLD);
        } else { // Default
            m_fontIcon  = CreateSegoeFont(13, FW_NORMAL);
            m_fontValue = CreateSegoeFont(10, FW_SEMIBOLD);
        }
    }

    void UpdateLayout() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int w = rc.right - rc.left;

        int rowHeight = 36;
        int topPadding = 8;
        int spacing = 4;

        if (m_sizeMode == SizeMode::Sleek) {
            rowHeight = 28;
            topPadding = 6;
            spacing = 2;
        } else if (m_sizeMode == SizeMode::Large) {
            rowHeight = 46;
            topPadding = 11;
            spacing = 6;
        }

        m_brightnessSlider.SetBounds(10, topPadding, w - 20, rowHeight, m_sizeMode);
        m_contrastSlider.SetBounds(10, topPadding + rowHeight + spacing, w - 20, rowHeight, m_sizeMode);
    }

    void SyncFromMonitor() {
        if (!m_monitorMgr || m_monitorMgr->GetDeviceCount() == 0) return;
        if (m_currentDeviceIndex >= m_monitorMgr->GetDeviceCount()) {
            m_currentDeviceIndex = 0;
        }

        auto* dev = m_monitorMgr->GetDevice(m_currentDeviceIndex);
        if (!dev) return;

        m_brightnessSlider.minVal = dev->minBrightness;
        m_brightnessSlider.maxVal = dev->maxBrightness;
        m_brightnessSlider.currentVal = dev->currBrightness;

        m_contrastSlider.minVal = dev->minContrast;
        m_contrastSlider.maxVal = dev->maxContrast;
        m_contrastSlider.currentVal = dev->currContrast;
    }

    void ApplyThemeToDwm() {
        m_theme.Refresh();
        BOOL dark = m_theme.isDark ? TRUE : FALSE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        DWORD corner = DWMWCP_ROUND;
        DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

        if (m_hEditBgBrush) DeleteObject(m_hEditBgBrush);
        m_hEditBgBrush = CreateSolidBrush(m_theme.isDark ? RGB(45, 45, 45) : RGB(255, 255, 255));
    }

    void ShowSettingsMenu(POINT pt) {
        HMENU hMenu = CreatePopupMenu();

        HMENU hSizeSubMenu = CreatePopupMenu();
        AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_SLEEK, L"Sleek (Slim & Compact)");
        AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_DEFAULT, L"Default (Standard)");
        AppendMenuW(hSizeSubMenu, MF_STRING, ID_MODE_LARGE, L"Large (Expanded)");

        UINT checkId = (m_sizeMode == SizeMode::Sleek) ? ID_MODE_SLEEK : 
                       ((m_sizeMode == SizeMode::Large) ? ID_MODE_LARGE : ID_MODE_DEFAULT);
        CheckMenuRadioItem(hSizeSubMenu, ID_MODE_SLEEK, ID_MODE_LARGE, checkId, MF_BYCOMMAND);

        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hSizeSubMenu), L"Size Mode");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_REFRESH, L"Refresh Monitors");

        bool autoStart = IsAutoStartEnabled();
        AppendMenuW(hMenu, MF_STRING | (autoStart ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_STARTUP, L"Start with Windows");

        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

        SetForegroundWindow(m_hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, m_hwnd, nullptr);
        PostMessageW(m_hwnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            m_sizeMode = LoadSizeMode();
            InitFonts();
            ApplyThemeToDwm();

            m_brightnessSlider.editControlId = 1001;
            m_brightnessSlider.icon = L"\x2600"; // ☀️ Sun symbol
            m_brightnessSlider.onValueChanged = [this](int val) {
                if (m_monitorMgr) {
                    m_monitorMgr->SetBrightnessAsync(m_currentDeviceIndex, val);
                }
            };

            m_contrastSlider.editControlId = 1002;
            m_contrastSlider.icon = L"\x25D0"; // ◐ Contrast symbol
            m_contrastSlider.onValueChanged = [this](int val) {
                if (m_monitorMgr) {
                    m_monitorMgr->SetContrastAsync(m_currentDeviceIndex, val);
                }
            };
            return 0;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, m_theme.textPrimary);
            SetBkColor(hdc, m_theme.isDark ? RGB(45, 45, 45) : RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(m_hEditBgBrush);
        }

        case WM_ACTIVATE: {
            if (LOWORD(wParam) == WA_INACTIVE) {
                HWND hLostTo = reinterpret_cast<HWND>(lParam);
                if (hLostTo && (hLostTo == m_brightnessSlider.hEdit || hLostTo == m_contrastSlider.hEdit)) {
                    return 0;
                }
                m_brightnessSlider.CommitEdit();
                m_contrastSlider.CommitEdit();
                ShowWindow(m_hwnd, SW_HIDE);
            }
            return 0;
        }

        case WM_RBUTTONUP: {
            POINT pt;
            GetCursorPos(&pt);
            ShowSettingsMenu(pt);
            return 0;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
            case ID_TRAY_OPEN:
                ShowAtTray();
                break;
            case ID_MODE_SLEEK:
                SetSizeMode(SizeMode::Sleek);
                break;
            case ID_MODE_DEFAULT:
                SetSizeMode(SizeMode::Default);
                break;
            case ID_MODE_LARGE:
                SetSizeMode(SizeMode::Large);
                break;
            case ID_TRAY_REFRESH:
                if (m_monitorMgr) m_monitorMgr->Refresh();
                SyncFromMonitor();
                InvalidateRect(m_hwnd, nullptr, TRUE);
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

        case WM_MOUSEWHEEL: {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(m_hwnd, &pt);

            bool changed = false;
            changed |= m_brightnessSlider.OnMouseWheel(pt.x, pt.y, zDelta);
            changed |= m_contrastSlider.OnMouseWheel(pt.x, pt.y, zDelta);

            if (changed) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_SETTINGCHANGE: {
            ApplyThemeToDwm();
            InitFonts();
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (!m_trackingMouse) {
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, m_hwnd, 0};
                TrackMouseEvent(&tme);
                m_trackingMouse = true;
            }

            bool needRepaint = false;
            needRepaint |= m_brightnessSlider.OnMouseMove(x, y);
            needRepaint |= m_contrastSlider.OnMouseMove(x, y);

            if (needRepaint) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            m_trackingMouse = false;
            bool needRepaint = false;
            if (m_brightnessSlider.isHovered || m_brightnessSlider.isNumBoxHovered) {
                m_brightnessSlider.isHovered = false;
                m_brightnessSlider.isNumBoxHovered = false;
                needRepaint = true;
            }
            if (m_contrastSlider.isHovered || m_contrastSlider.isNumBoxHovered) {
                m_contrastSlider.isHovered = false;
                m_contrastSlider.isNumBoxHovered = false;
                needRepaint = true;
            }
            if (needRepaint) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetCapture(m_hwnd);
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if (m_brightnessSlider.OnLButtonDown(x, y, m_hwnd, m_fontValue)) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
                return 0;
            }
            if (m_contrastSlider.OnLButtonDown(x, y, m_hwnd, m_fontValue)) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
                return 0;
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            ReleaseCapture();
            bool rep = false;
            rep |= m_brightnessSlider.OnLButtonUp();
            rep |= m_contrastSlider.OnLButtonUp();
            if (rep) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);

            RECT rc;
            GetClientRect(m_hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            // Windows 11 Dark Acrylic / Light Surface
            HBRUSH hBgBrush = CreateSolidBrush(m_theme.bg);
            FillRect(memDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // Draw Sliders & Editable Number Badges
            m_brightnessSlider.Draw(memDC, m_theme, m_fontIcon, m_fontValue);
            m_contrastSlider.Draw(memDC, m_theme, m_fontIcon, m_fontValue);

            // Subtle outline border
            HPEN borderPen = CreatePen(PS_SOLID, 1, m_theme.border);
            HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            HPEN oP = (HPEN)SelectObject(memDC, borderPen);
            HBRUSH oB = (HBRUSH)SelectObject(memDC, nullBrush);
            RoundRect(memDC, 0, 0, w, h, 14, 14);
            SelectObject(memDC, oP);
            SelectObject(memDC, oB);
            DeleteObject(borderPen);

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(m_hwnd, &ps);
            return 0;
        }

        case WM_DESTROY: {
            if (m_fontIcon) DeleteObject(m_fontIcon);
            if (m_fontValue) DeleteObject(m_fontValue);
            if (m_hEditBgBrush) DeleteObject(m_hEditBgBrush);
            return 0;
        }
        }
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

public:
    FlyoutWindow(MonitorManager* mgr) : m_monitorMgr(mgr) {}

    bool Create(HINSTANCE hInstance) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc   = FlyoutWindow::WndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = L"MonitorManFlyoutClass";
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);

        m_sizeMode = LoadSizeMode();
        int width = 330;
        int height = 92;
        GetFlyoutDimensions(width, height);

        // WS_EX_TOOLWINDOW: Never appears on the taskbar!
        // WS_POPUP: Clean borderless flyout with rounded corners!
        m_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            wc.lpszClassName,
            L"MonitorMan",
            WS_POPUP,
            0, 0, width, height,
            nullptr, nullptr, hInstance, this
        );

        if (!m_hwnd) return false;

        InitFonts();
        UpdateLayout();
        SyncFromMonitor();
        return true;
    }

    HWND GetHwnd() const { return m_hwnd; }

    SizeMode GetSizeMode() const { return m_sizeMode; }

    void SetSizeMode(SizeMode mode) {
        m_sizeMode = mode;
        SaveSizeMode(mode);
        InitFonts();

        int w = 330;
        int h = 92;
        GetFlyoutDimensions(w, h);

        RECT rcWorkArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

        int x = rcWorkArea.right - w - 12;
        int y = rcWorkArea.bottom - h - 12;

        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW);
        UpdateLayout();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void Toggle(HWND) {
        if (IsWindowVisible(m_hwnd)) {
            m_brightnessSlider.CommitEdit();
            m_contrastSlider.CommitEdit();
            ShowWindow(m_hwnd, SW_HIDE);
        } else {
            ShowAtTray();
        }
    }

    void ShowAtTray() {
        SyncFromMonitor();
        ApplyThemeToDwm();

        int w = 330;
        int h = 92;
        GetFlyoutDimensions(w, h);

        RECT rcWorkArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

        int x = rcWorkArea.right - w - 12;
        int y = rcWorkArea.bottom - h - 12;

        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, w, h, SWP_SHOWWINDOW);
        SetForegroundWindow(m_hwnd);
        UpdateLayout();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
};
