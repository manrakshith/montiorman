#pragma once

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <string>
#include <functional>
#include "theme.h"
#include "settings.h"
#include "monitor.h"
#include "solar.h"

class SettingsWindow {
private:
    HWND m_hwnd = nullptr;
    Theme m_theme;
    MonitorManager* m_monitorMgr = nullptr;
    SolarTracker* m_solarTracker = nullptr;
    std::function<void(SizeMode)> m_onSizeModeChanged;
    std::function<void()> m_onMonitorsRefreshed;

    HFONT m_fontTitle = nullptr;
    HFONT m_fontSection = nullptr;
    HFONT m_fontBody = nullptr;
    HFONT m_fontSmall = nullptr;
    HFONT m_fontIcon = nullptr;

    SizeMode m_currentMode = SizeMode::Default;
    bool m_autoStart = false;
    int m_currentTab = 0; // 0 = General, 1 = Solar Daylight Tracking

    // Tab hit rects
    RECT m_rcTabGen{0, 0, 0, 0};
    RECT m_rcTabSolar{0, 0, 0, 0};

    // General Tab hit rects
    RECT m_rcModeSleek{0, 0, 0, 0};
    RECT m_rcModeDefault{0, 0, 0, 0};
    RECT m_rcModeLarge{0, 0, 0, 0};
    RECT m_rcAutoStart{0, 0, 0, 0};
    RECT m_rcRefreshBtn{0, 0, 0, 0};

    // Solar Tab hit rects
    RECT m_rcSolarEnable{0, 0, 0, 0};
    RECT m_rcSolarModeBell{0, 0, 0, 0};
    RECT m_rcSolarMode4Pt{0, 0, 0, 0};
    RECT m_rcMinMinus{0, 0, 0, 0}, m_rcMinPlus{0, 0, 0, 0};
    RECT m_rcMaxMinus{0, 0, 0, 0}, m_rcMaxPlus{0, 0, 0, 0};
    RECT m_rcWp1Minus{0, 0, 0, 0}, m_rcWp1Plus{0, 0, 0, 0};
    RECT m_rcWp2Minus{0, 0, 0, 0}, m_rcWp2Plus{0, 0, 0, 0};
    RECT m_rcWp3Minus{0, 0, 0, 0}, m_rcWp3Plus{0, 0, 0, 0};
    RECT m_rcWp4Minus{0, 0, 0, 0}, m_rcWp4Plus{0, 0, 0, 0};
    RECT m_rcResetOffset{0, 0, 0, 0};

    // Global Footer
    RECT m_rcCloseBtn{0, 0, 0, 0};

    int m_hoveredElement = 0;
    bool m_trackingMouse = false;

    int GetDpi() const {
        if (m_hwnd) {
            UINT dpi = GetDpiForWindow(m_hwnd);
            if (dpi > 0) return static_cast<int>(dpi);
        }
        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
        return (dpi > 0) ? dpi : 96;
    }

    int Scale(int px) const {
        return MulDiv(px, GetDpi(), 96);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        SettingsWindow* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<SettingsWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        } else {
            self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self) {
            return self->HandleMessage(msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void InitFonts() {
        if (m_fontTitle)   DeleteObject(m_fontTitle);
        if (m_fontSection) DeleteObject(m_fontSection);
        if (m_fontBody)    DeleteObject(m_fontBody);
        if (m_fontSmall)   DeleteObject(m_fontSmall);
        if (m_fontIcon)    DeleteObject(m_fontIcon);

        m_fontTitle   = CreateSegoeFont(15, FW_BOLD);
        m_fontSection = CreateSegoeFont(10, FW_SEMIBOLD);
        m_fontBody    = CreateSegoeFont(9, FW_NORMAL);
        m_fontSmall   = CreateSegoeFont(8, FW_NORMAL);
        m_fontIcon    = CreateIconFont(16);
    }

    void ApplyThemeToDwm() {
        m_theme.Refresh();
        BOOL dark = m_theme.isDark ? TRUE : FALSE;
        DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        DWORD corner = DWMWCP_ROUND;
        DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));
    }

    void SyncState() {
        m_currentMode = LoadSizeMode();
        m_autoStart   = IsAutoStartEnabled();
        m_theme.Refresh();
    }

    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE: {
            InitFonts();
            ApplyThemeToDwm();
            SyncState();
            return 0;
        }

        case WM_SETTINGCHANGE: {
            ApplyThemeToDwm();
            InitFonts();
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }

        case WM_CLOSE: {
            ShowWindow(m_hwnd, SW_HIDE);
            return 0;
        }

        case WM_SETCURSOR: {
            if (m_hoveredElement != 0) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            POINT pt = {x, y};

            if (!m_trackingMouse) {
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, m_hwnd, 0};
                TrackMouseEvent(&tme);
                m_trackingMouse = true;
            }

            int prevHover = m_hoveredElement;
            m_hoveredElement = 0;

            // Tabs
            if (PtInRect(&m_rcTabGen, pt))         m_hoveredElement = 10;
            else if (PtInRect(&m_rcTabSolar, pt))   m_hoveredElement = 11;
            else if (PtInRect(&m_rcCloseBtn, pt))   m_hoveredElement = 6;
            else if (m_currentTab == 0) {
                if (PtInRect(&m_rcModeSleek, pt))          m_hoveredElement = 1;
                else if (PtInRect(&m_rcModeDefault, pt))    m_hoveredElement = 2;
                else if (PtInRect(&m_rcModeLarge, pt))      m_hoveredElement = 3;
                else if (PtInRect(&m_rcAutoStart, pt))      m_hoveredElement = 4;
                else if (PtInRect(&m_rcRefreshBtn, pt))     m_hoveredElement = 5;
            } else if (m_currentTab == 1) {
                if (PtInRect(&m_rcSolarEnable, pt))         m_hoveredElement = 20;
                else if (PtInRect(&m_rcSolarModeBell, pt))   m_hoveredElement = 21;
                else if (PtInRect(&m_rcSolarMode4Pt, pt))    m_hoveredElement = 22;
                else if (PtInRect(&m_rcMinMinus, pt))       m_hoveredElement = 30;
                else if (PtInRect(&m_rcMinPlus, pt))        m_hoveredElement = 31;
                else if (PtInRect(&m_rcMaxMinus, pt))       m_hoveredElement = 32;
                else if (PtInRect(&m_rcMaxPlus, pt))        m_hoveredElement = 33;
                else if (PtInRect(&m_rcWp1Minus, pt))       m_hoveredElement = 40;
                else if (PtInRect(&m_rcWp1Plus, pt))        m_hoveredElement = 41;
                else if (PtInRect(&m_rcWp2Minus, pt))       m_hoveredElement = 42;
                else if (PtInRect(&m_rcWp2Plus, pt))        m_hoveredElement = 43;
                else if (PtInRect(&m_rcWp3Minus, pt))       m_hoveredElement = 44;
                else if (PtInRect(&m_rcWp3Plus, pt))        m_hoveredElement = 45;
                else if (PtInRect(&m_rcWp4Minus, pt))       m_hoveredElement = 46;
                else if (PtInRect(&m_rcWp4Plus, pt))        m_hoveredElement = 47;
                else if (PtInRect(&m_rcResetOffset, pt))    m_hoveredElement = 50;
            }

            if (prevHover != m_hoveredElement) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            m_trackingMouse = false;
            if (m_hoveredElement != 0) {
                m_hoveredElement = 0;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            POINT pt = {x, y};

            if (PtInRect(&m_rcTabGen, pt)) {
                m_currentTab = 0;
                InvalidateRect(m_hwnd, nullptr, TRUE);
                return 0;
            } else if (PtInRect(&m_rcTabSolar, pt)) {
                m_currentTab = 1;
                InvalidateRect(m_hwnd, nullptr, TRUE);
                return 0;
            } else if (PtInRect(&m_rcCloseBtn, pt)) {
                ShowWindow(m_hwnd, SW_HIDE);
                return 0;
            }

            if (m_currentTab == 0) {
                if (PtInRect(&m_rcModeSleek, pt)) {
                    m_currentMode = SizeMode::Sleek;
                    SaveSizeMode(m_currentMode);
                    if (m_onSizeModeChanged) m_onSizeModeChanged(m_currentMode);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcModeDefault, pt)) {
                    m_currentMode = SizeMode::Default;
                    SaveSizeMode(m_currentMode);
                    if (m_onSizeModeChanged) m_onSizeModeChanged(m_currentMode);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcModeLarge, pt)) {
                    m_currentMode = SizeMode::Large;
                    SaveSizeMode(m_currentMode);
                    if (m_onSizeModeChanged) m_onSizeModeChanged(m_currentMode);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcAutoStart, pt)) {
                    m_autoStart = !m_autoStart;
                    SetAutoStart(m_autoStart);
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcRefreshBtn, pt)) {
                    if (m_monitorMgr) m_monitorMgr->Refresh();
                    if (m_onMonitorsRefreshed) m_onMonitorsRefreshed();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
            } else if (m_currentTab == 1 && m_solarTracker) {
                auto& s = m_solarTracker->GetSettings();
                if (PtInRect(&m_rcSolarEnable, pt)) {
                    s.enabled = !s.enabled;
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcSolarModeBell, pt)) {
                    s.mode = SolarMode::BellCurve;
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcSolarMode4Pt, pt)) {
                    s.mode = SolarMode::Schedule4Point;
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcMinMinus, pt)) {
                    s.minBrightness = std::max(0, s.minBrightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcMinPlus, pt)) {
                    s.minBrightness = std::min(s.maxBrightness, s.minBrightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcMaxMinus, pt)) {
                    s.maxBrightness = std::max(s.minBrightness, s.maxBrightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcMaxPlus, pt)) {
                    s.maxBrightness = std::min(100, s.maxBrightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp1Minus, pt)) {
                    s.wp1.brightness = std::max(0, s.wp1.brightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp1Plus, pt)) {
                    s.wp1.brightness = std::min(100, s.wp1.brightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp2Minus, pt)) {
                    s.wp2.brightness = std::max(0, s.wp2.brightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp2Plus, pt)) {
                    s.wp2.brightness = std::min(100, s.wp2.brightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp3Minus, pt)) {
                    s.wp3.brightness = std::max(0, s.wp3.brightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp3Plus, pt)) {
                    s.wp3.brightness = std::min(100, s.wp3.brightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp4Minus, pt)) {
                    s.wp4.brightness = std::max(0, s.wp4.brightness - 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcWp4Plus, pt)) {
                    s.wp4.brightness = std::min(100, s.wp4.brightness + 5);
                    m_solarTracker->Save();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                } else if (PtInRect(&m_rcResetOffset, pt)) {
                    m_solarTracker->ResetOffset();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
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

            // Clean dark background
            HBRUSH hBgBrush = CreateSolidBrush(m_theme.bg);
            FillRect(memDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            SetBkMode(memDC, TRANSPARENT);

            int cardPad = Scale(32);
            int cardLeft = cardPad;
            int cardRight = w - cardPad;

            // 1. Header (Icon + Title + Subtitle) - Sleek monochrome, NO accent color
            SelectObject(memDC, m_fontIcon);
            SetTextColor(memDC, m_theme.textPrimary);
            RECT rcIcon = {cardLeft, Scale(18), cardLeft + Scale(32), Scale(52)};
            DrawTextW(memDC, L"\xE713", -1, &rcIcon, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(memDC, m_fontTitle);
            SetTextColor(memDC, m_theme.textPrimary);
            RECT rcTitle = {cardLeft + Scale(36), Scale(14), cardRight, Scale(40)};
            DrawTextW(memDC, L"Settings", -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(memDC, m_fontSmall);
            SetTextColor(memDC, m_theme.textSecondary);
            RECT rcSub = {cardLeft + Scale(36), Scale(40), cardRight, Scale(58)};
            DrawTextW(memDC, L"Preferences, display hardware, and daylight automation", -1, &rcSub, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // 2. Tab Bar - Native Windows 11 style pills (Clean dark elevated pill, NO bright purple!)
            int tabY = Scale(66);
            int tabH = Scale(34);
            int genTabW = Scale(110);
            int solTabW = Scale(210);

            m_rcTabGen   = {cardLeft, tabY, cardLeft + genTabW, tabY + tabH};
            m_rcTabSolar = {cardLeft + genTabW + Scale(8), tabY, cardLeft + genTabW + Scale(8) + solTabW, tabY + tabH};

            auto DrawTabPill = [&](const RECT& r, const wchar_t* title, bool active, bool hover) {
                COLORREF bg;
                COLORREF fg;
                COLORREF borderCol;

                if (active) {
                    bg = m_theme.isDark ? RGB(52, 52, 52) : RGB(236, 236, 236);
                    fg = m_theme.textPrimary;
                    borderCol = m_theme.isDark ? RGB(75, 75, 75) : RGB(200, 200, 200);
                } else if (hover) {
                    bg = m_theme.isDark ? RGB(40, 40, 40) : RGB(244, 244, 244);
                    fg = m_theme.textPrimary;
                    borderCol = m_theme.isDark ? RGB(58, 58, 58) : RGB(220, 220, 220);
                } else {
                    bg = m_theme.isDark ? RGB(32, 32, 32) : RGB(250, 250, 250);
                    fg = m_theme.textSecondary;
                    borderCol = m_theme.border;
                }

                HBRUSH hB = CreateSolidBrush(bg);
                HPEN hP = CreatePen(PS_SOLID, 1, borderCol);
                HBRUSH oB = (HBRUSH)SelectObject(memDC, hB);
                HPEN oP = (HPEN)SelectObject(memDC, hP);
                RoundRect(memDC, r.left, r.top, r.right, r.bottom, Scale(6), Scale(6));
                SelectObject(memDC, oP);
                SelectObject(memDC, oB);
                DeleteObject(hB);
                DeleteObject(hP);

                SelectObject(memDC, active ? m_fontSection : m_fontBody);
                SetTextColor(memDC, fg);
                DrawTextW(memDC, title, -1, const_cast<RECT*>(&r), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            };

            DrawTabPill(m_rcTabGen, L"General", m_currentTab == 0, m_hoveredElement == 10);
            DrawTabPill(m_rcTabSolar, L"☼  Solar Day Tracking", m_currentTab == 1, m_hoveredElement == 11);

            // Card drawing helper
            auto DrawCard = [&](int top, int bottom) {
                HBRUSH hCardBrush = CreateSolidBrush(m_theme.cardBg);
                HPEN hCardPen = CreatePen(PS_SOLID, 1, m_theme.border);
                HBRUSH oB = (HBRUSH)SelectObject(memDC, hCardBrush);
                HPEN oP = (HPEN)SelectObject(memDC, hCardPen);
                RoundRect(memDC, cardLeft, top, cardRight, bottom, Scale(8), Scale(8));
                SelectObject(memDC, oP);
                SelectObject(memDC, oB);
                DeleteObject(hCardBrush);
                DeleteObject(hCardPen);
            };

            // Radio Row Helper (Sleek monochrome dot, NO accent color!)
            auto DrawRadioRow = [&](const RECT& rowRc, bool selected, bool hovered, const wchar_t* title, const wchar_t* desc) {
                if (hovered) {
                    HBRUSH hHovBrush = CreateSolidBrush(m_theme.isDark ? RGB(48, 48, 48) : RGB(238, 238, 238));
                    HPEN hNullP = (HPEN)GetStockObject(NULL_PEN);
                    HBRUSH oOldB = (HBRUSH)SelectObject(memDC, hHovBrush);
                    HPEN oOldP = (HPEN)SelectObject(memDC, hNullP);
                    RoundRect(memDC, rowRc.left - Scale(6), rowRc.top, rowRc.right + Scale(6), rowRc.bottom, Scale(6), Scale(6));
                    SelectObject(memDC, oOldP);
                    SelectObject(memDC, oOldB);
                    DeleteObject(hHovBrush);
                }

                int cX = rowRc.left + Scale(14);
                int cY = (rowRc.top + rowRc.bottom) / 2;
                int rOut = Scale(8);

                COLORREF ringCol = selected ? (m_theme.isDark ? RGB(200, 200, 200) : RGB(60, 60, 60)) : m_theme.textMuted;
                HPEN hCircPen = CreatePen(PS_SOLID, selected ? 2 : 1, ringCol);
                HBRUSH hCircBrush = CreateSolidBrush(m_theme.isDark ? RGB(32, 32, 32) : RGB(255, 255, 255));
                HPEN oldP = (HPEN)SelectObject(memDC, hCircPen);
                HBRUSH oldB = (HBRUSH)SelectObject(memDC, hCircBrush);
                Ellipse(memDC, cX - rOut, cY - rOut, cX + rOut, cY + rOut);

                if (selected) {
                    HBRUSH hDotBrush = CreateSolidBrush(RGB(255, 255, 255));
                    HPEN hNullP = (HPEN)GetStockObject(NULL_PEN);
                    SelectObject(memDC, hDotBrush);
                    SelectObject(memDC, hNullP);
                    int rIn = Scale(3);
                    Ellipse(memDC, cX - rIn, cY - rIn, cX + rIn, cY + rIn);
                    DeleteObject(hDotBrush);
                }

                SelectObject(memDC, oldP);
                SelectObject(memDC, oldB);
                DeleteObject(hCircPen);
                DeleteObject(hCircBrush);

                SelectObject(memDC, m_fontBody);
                SetTextColor(memDC, selected ? m_theme.textPrimary : m_theme.textSecondary);
                RECT txtRc = {cX + Scale(24), rowRc.top, rowRc.right, rowRc.bottom};
                std::wstring fullStr = std::wstring(title) + L"  —  " + desc;
                DrawTextW(memDC, fullStr.c_str(), -1, &txtRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            };

            // Stepper Helper (Clean white numbers, sleek neutral buttons, NO accent color!)
            auto DrawStepper = [&](const RECT& rowRc, const wchar_t* label, int val, RECT& rcMinus, RECT& rcPlus, int minusHoverId, int plusHoverId) {
                int btnW = Scale(32);
                int btnH = Scale(28);
                int midY = (rowRc.top + rowRc.bottom) / 2;
                int right = rowRc.right - Scale(16);

                rcPlus = {right - btnW, midY - btnH / 2, right, midY + btnH / 2};
                RECT rcVal = {right - btnW - Scale(60), midY - btnH / 2, right - btnW - Scale(4), midY + btnH / 2};
                rcMinus = {right - btnW - Scale(60) - btnW - Scale(4), midY - btnH / 2, right - btnW - Scale(60) - Scale(4), midY + btnH / 2};

                RECT lblRc = {rowRc.left + Scale(16), rowRc.top, rcMinus.left - Scale(12), rowRc.bottom};
                SelectObject(memDC, m_fontBody);
                SetTextColor(memDC, m_theme.textPrimary);
                DrawTextW(memDC, label, -1, &lblRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                auto DrawMiniBtn = [&](const RECT& r, const wchar_t* txt, bool hover) {
                    COLORREF bg = hover 
                        ? (m_theme.isDark ? RGB(58, 58, 58) : RGB(228, 228, 228))
                        : (m_theme.isDark ? RGB(45, 45, 45) : RGB(242, 242, 242));
                    COLORREF bdr = hover
                        ? (m_theme.isDark ? RGB(90, 90, 90) : RGB(180, 180, 180))
                        : m_theme.border;
                    HBRUSH hB = CreateSolidBrush(bg);
                    HPEN hP = CreatePen(PS_SOLID, 1, bdr);
                    HBRUSH oB = (HBRUSH)SelectObject(memDC, hB);
                    HPEN oP = (HPEN)SelectObject(memDC, hP);
                    RoundRect(memDC, r.left, r.top, r.right, r.bottom, Scale(4), Scale(4));
                    SelectObject(memDC, oP);
                    SelectObject(memDC, oB);
                    DeleteObject(hB);
                    DeleteObject(hP);

                    SelectObject(memDC, m_fontSection);
                    SetTextColor(memDC, hover ? m_theme.textPrimary : m_theme.textSecondary);
                    DrawTextW(memDC, txt, -1, const_cast<RECT*>(&r), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                };

                DrawMiniBtn(rcMinus, L"−", m_hoveredElement == minusHoverId);

                // Value box (Crisp white text)
                std::wstring valStr = std::to_wstring(val) + L"%";
                SelectObject(memDC, m_fontSection);
                SetTextColor(memDC, m_theme.textPrimary);
                DrawTextW(memDC, valStr.c_str(), -1, &rcVal, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                DrawMiniBtn(rcPlus, L"+", m_hoveredElement == plusHoverId);
            };

            int contentY = Scale(112);

            if (m_currentTab == 0) {
                // ==================== TAB 0: GENERAL ====================
                // Card 1: Connected Display
                int c1Top = contentY;
                int c1Bottom = c1Top + Scale(98);
                DrawCard(c1Top, c1Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcC1Label = {cardLeft + Scale(20), c1Top + Scale(12), cardRight - Scale(170), c1Top + Scale(28)};
                DrawTextW(memDC, L"CONNECTED DISPLAY", -1, &rcC1Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                auto* dev = (m_monitorMgr && m_monitorMgr->GetDeviceCount() > 0) ? m_monitorMgr->GetDevice(0) : nullptr;
                std::wstring monName = dev ? dev->name : L"Generic Display";

                SelectObject(memDC, m_fontSection);
                SetTextColor(memDC, m_theme.textPrimary);
                RECT rcC1Name = {cardLeft + Scale(20), c1Top + Scale(32), cardRight - Scale(170), c1Top + Scale(58)};
                DrawTextW(memDC, monName.c_str(), -1, &rcC1Name, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textSecondary);
                RECT rcC1Status = {cardLeft + Scale(20), c1Top + Scale(62), cardRight - Scale(170), c1Top + Scale(84)};
                DrawTextW(memDC, L"VESA DDC/CI Hardware Luminance & Contrast Control Active", -1, &rcC1Status, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Refresh Button (Clean neutral styling)
                m_rcRefreshBtn = {cardRight - Scale(160), c1Top + Scale(28), cardRight - Scale(20), c1Top + Scale(68)};
                bool isRefHover = (m_hoveredElement == 5);
                COLORREF refBg = isRefHover 
                    ? (m_theme.isDark ? RGB(58, 58, 58) : RGB(228, 228, 228))
                    : (m_theme.isDark ? RGB(45, 45, 45) : RGB(242, 242, 242));
                COLORREF refBdr = isRefHover
                    ? (m_theme.isDark ? RGB(90, 90, 90) : RGB(180, 180, 180))
                    : m_theme.border;
                HBRUSH hRefBrush = CreateSolidBrush(refBg);
                HPEN hRefPen = CreatePen(PS_SOLID, 1, refBdr);
                HBRUSH oB = (HBRUSH)SelectObject(memDC, hRefBrush);
                HPEN oP = (HPEN)SelectObject(memDC, hRefPen);
                RoundRect(memDC, m_rcRefreshBtn.left, m_rcRefreshBtn.top, m_rcRefreshBtn.right, m_rcRefreshBtn.bottom, Scale(6), Scale(6));
                SelectObject(memDC, oP);
                SelectObject(memDC, oB);
                DeleteObject(hRefBrush);
                DeleteObject(hRefPen);

                SelectObject(memDC, m_fontBody);
                SetTextColor(memDC, isRefHover ? m_theme.textPrimary : m_theme.textSecondary);
                DrawTextW(memDC, L"Refresh Displays", -1, &m_rcRefreshBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Card 2: Flyout Size Mode
                int c2Top = c1Bottom + Scale(14);
                int c2Bottom = c2Top + Scale(160);
                DrawCard(c2Top, c2Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcC2Label = {cardLeft + Scale(20), c2Top + Scale(12), cardRight - Scale(20), c2Top + Scale(28)};
                DrawTextW(memDC, L"FLYOUT SIZE MODE", -1, &rcC2Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                int rH = Scale(38);
                m_rcModeSleek   = {cardLeft + Scale(20), c2Top + Scale(34), cardRight - Scale(20), c2Top + Scale(34) + rH};
                m_rcModeDefault = {cardLeft + Scale(20), c2Top + Scale(34) + rH, cardRight - Scale(20), c2Top + Scale(34) + rH * 2};
                m_rcModeLarge   = {cardLeft + Scale(20), c2Top + Scale(34) + rH * 2, cardRight - Scale(20), c2Top + Scale(34) + rH * 3};

                DrawRadioRow(m_rcModeSleek,   m_currentMode == SizeMode::Sleek,   m_hoveredElement == 1, L"Sleek",   L"340 × 82 px (Slim & Compact footprint)");
                DrawRadioRow(m_rcModeDefault, m_currentMode == SizeMode::Default, m_hoveredElement == 2, L"Default", L"390 × 96 px (Standard Windows 11 Fluent layout)");
                DrawRadioRow(m_rcModeLarge,   m_currentMode == SizeMode::Large,   m_hoveredElement == 3, L"Large",   L"440 × 114 px (Spacious touch-friendly targets)");

                // Card 3: System Integration & Hotkeys
                int c3Top = c2Bottom + Scale(14);
                int c3Bottom = c3Top + Scale(120);
                DrawCard(c3Top, c3Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcC3Label = {cardLeft + Scale(20), c3Top + Scale(12), cardRight - Scale(20), c3Top + Scale(28)};
                DrawTextW(memDC, L"SYSTEM INTEGRATION", -1, &rcC3Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                m_rcAutoStart = {cardLeft + Scale(20), c3Top + Scale(34), cardRight - Scale(20), c3Top + Scale(72)};
                bool isAutoHov = (m_hoveredElement == 4);
                if (isAutoHov) {
                    HBRUSH hHovBrush = CreateSolidBrush(m_theme.isDark ? RGB(48, 48, 48) : RGB(238, 238, 238));
                    HPEN hNullP = (HPEN)GetStockObject(NULL_PEN);
                    HBRUSH oOldB = (HBRUSH)SelectObject(memDC, hHovBrush);
                    HPEN oOldP = (HPEN)SelectObject(memDC, hNullP);
                    RoundRect(memDC, m_rcAutoStart.left - Scale(6), m_rcAutoStart.top, m_rcAutoStart.right + Scale(6), m_rcAutoStart.bottom, Scale(6), Scale(6));
                    SelectObject(memDC, oOldP);
                    SelectObject(memDC, oOldB);
                    DeleteObject(hHovBrush);
                }

                // Sleek monochrome checkbox (NO purple!)
                int boxX = m_rcAutoStart.left + Scale(4);
                int boxSize = Scale(18);
                int boxY = (m_rcAutoStart.top + m_rcAutoStart.bottom) / 2 - boxSize / 2;
                RECT boxRc = {boxX, boxY, boxX + boxSize, boxY + boxSize};

                COLORREF chkBg = m_autoStart 
                    ? (m_theme.isDark ? RGB(65, 65, 65) : RGB(210, 210, 210))
                    : (m_theme.isDark ? RGB(36, 36, 36) : RGB(255, 255, 255));
                COLORREF chkBdr = m_autoStart
                    ? (m_theme.isDark ? RGB(110, 110, 110) : RGB(150, 150, 150))
                    : m_theme.border;
                HBRUSH hBoxBrush = CreateSolidBrush(chkBg);
                HPEN hBoxPen = CreatePen(PS_SOLID, 1, chkBdr);
                HBRUSH oBoxB = (HBRUSH)SelectObject(memDC, hBoxBrush);
                HPEN oBoxP = (HPEN)SelectObject(memDC, hBoxPen);
                RoundRect(memDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom, Scale(4), Scale(4));
                SelectObject(memDC, oBoxP);
                SelectObject(memDC, oBoxB);
                DeleteObject(hBoxBrush);
                DeleteObject(hBoxPen);

                if (m_autoStart) {
                    HPEN hCheckPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    HPEN oCP = (HPEN)SelectObject(memDC, hCheckPen);
                    MoveToEx(memDC, boxX + Scale(4), boxY + Scale(9), nullptr);
                    LineTo(memDC, boxX + Scale(7), boxY + Scale(13));
                    LineTo(memDC, boxX + Scale(13), boxY + Scale(5));
                    SelectObject(memDC, oCP);
                    DeleteObject(hCheckPen);
                }

                SelectObject(memDC, m_fontBody);
                SetTextColor(memDC, m_theme.textPrimary);
                RECT txtAutoRc = {boxX + boxSize + Scale(12), m_rcAutoStart.top, m_rcAutoStart.right, m_rcAutoStart.bottom};
                DrawTextW(memDC, L"Start MonitorMan automatically when Windows starts", -1, &txtAutoRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                HPEN hDivPen = CreatePen(PS_SOLID, 1, m_theme.border);
                HPEN oDP = (HPEN)SelectObject(memDC, hDivPen);
                MoveToEx(memDC, cardLeft + Scale(20), c3Top + Scale(78), nullptr);
                LineTo(memDC, cardRight - Scale(20), c3Top + Scale(78));
                SelectObject(memDC, oDP);
                DeleteObject(hDivPen);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textSecondary);
                RECT rcHotkeys = {cardLeft + Scale(20), c3Top + Scale(86), cardRight - Scale(20), c3Bottom - Scale(10)};
                DrawTextW(memDC, L"Global Shortcuts:   Win + Alt + B   or   Ctrl + Alt + B   to summon flyout anywhere", -1, &rcHotkeys, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            } else if (m_currentTab == 1 && m_solarTracker) {
                // ==================== TAB 1: SOLAR DAYLIGHT TRACKING ====================
                auto& s = m_solarTracker->GetSettings();

                // Card 1: Master Enable & Tracking Modes
                int c1Top = contentY;
                int c1Bottom = c1Top + Scale(136);
                DrawCard(c1Top, c1Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcS1Label = {cardLeft + Scale(20), c1Top + Scale(12), cardRight - Scale(20), c1Top + Scale(28)};
                DrawTextW(memDC, L"AUTOMATIC DAYLIGHT & CIRCADIAN TRACKING", -1, &rcS1Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Master Checkbox
                m_rcSolarEnable = {cardLeft + Scale(20), c1Top + Scale(32), cardRight - Scale(20), c1Top + Scale(64)};
                bool isSolarHov = (m_hoveredElement == 20);
                if (isSolarHov) {
                    HBRUSH hHovBrush = CreateSolidBrush(m_theme.isDark ? RGB(48, 48, 48) : RGB(238, 238, 238));
                    HPEN hNullP = (HPEN)GetStockObject(NULL_PEN);
                    HBRUSH oOldB = (HBRUSH)SelectObject(memDC, hHovBrush);
                    HPEN oOldP = (HPEN)SelectObject(memDC, hNullP);
                    RoundRect(memDC, m_rcSolarEnable.left - Scale(6), m_rcSolarEnable.top, m_rcSolarEnable.right + Scale(6), m_rcSolarEnable.bottom, Scale(6), Scale(6));
                    SelectObject(memDC, oOldP);
                    SelectObject(memDC, oOldB);
                    DeleteObject(hHovBrush);
                }

                int boxX = m_rcSolarEnable.left + Scale(4);
                int boxSize = Scale(18);
                int boxY = (m_rcSolarEnable.top + m_rcSolarEnable.bottom) / 2 - boxSize / 2;
                RECT boxRc = {boxX, boxY, boxX + boxSize, boxY + boxSize};

                COLORREF chkBg = s.enabled 
                    ? (m_theme.isDark ? RGB(65, 65, 65) : RGB(210, 210, 210))
                    : (m_theme.isDark ? RGB(36, 36, 36) : RGB(255, 255, 255));
                COLORREF chkBdr = s.enabled
                    ? (m_theme.isDark ? RGB(110, 110, 110) : RGB(150, 150, 150))
                    : m_theme.border;
                HBRUSH hBoxBrush = CreateSolidBrush(chkBg);
                HPEN hBoxPen = CreatePen(PS_SOLID, 1, chkBdr);
                HBRUSH oBoxB = (HBRUSH)SelectObject(memDC, hBoxBrush);
                HPEN oBoxP = (HPEN)SelectObject(memDC, hBoxPen);
                RoundRect(memDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom, Scale(4), Scale(4));
                SelectObject(memDC, oBoxP);
                SelectObject(memDC, oBoxB);
                DeleteObject(hBoxBrush);
                DeleteObject(hBoxPen);

                if (s.enabled) {
                    HPEN hCheckPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    HPEN oCP = (HPEN)SelectObject(memDC, hCheckPen);
                    MoveToEx(memDC, boxX + Scale(4), boxY + Scale(9), nullptr);
                    LineTo(memDC, boxX + Scale(7), boxY + Scale(13));
                    LineTo(memDC, boxX + Scale(13), boxY + Scale(5));
                    SelectObject(memDC, oCP);
                    DeleteObject(hCheckPen);
                }

                SelectObject(memDC, m_fontSection);
                SetTextColor(memDC, m_theme.textPrimary);
                RECT txtSolarRc = {boxX + boxSize + Scale(12), m_rcSolarEnable.top, m_rcSolarEnable.right, m_rcSolarEnable.bottom};
                DrawTextW(memDC, L"Enable Solar Daylight Tracking (Follow Natural Sun Luminance)", -1, &txtSolarRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Two Mode Radios
                int rModeH = Scale(32);
                m_rcSolarModeBell = {cardLeft + Scale(20), c1Top + Scale(66), cardRight - Scale(20), c1Top + Scale(66) + rModeH};
                m_rcSolarMode4Pt  = {cardLeft + Scale(20), c1Top + Scale(66) + rModeH, cardRight - Scale(20), c1Top + Scale(66) + rModeH * 2};

                DrawRadioRow(m_rcSolarModeBell, s.mode == SolarMode::BellCurve, m_hoveredElement == 21, L"Smooth Solar Bell Curve", L"Continuous sinusoidal curve tracking sunrise to sunset");
                DrawRadioRow(m_rcSolarMode4Pt,  s.mode == SolarMode::Schedule4Point, m_hoveredElement == 22, L"4-Point Custom Schedule", L"Specific morning, midday, evening, and night targets");

                // Card 2: Brightness Limits & Levels
                int c2Top = c1Bottom + Scale(14);
                int c2Bottom = (s.mode == SolarMode::BellCurve) ? (c2Top + Scale(146)) : (c2Top + Scale(176));
                DrawCard(c2Top, c2Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcS2Label = {cardLeft + Scale(20), c2Top + Scale(12), cardRight - Scale(20), c2Top + Scale(28)};
                DrawTextW(memDC, L"CONFIGURABLE BRIGHTNESS LEVELS", -1, &rcS2Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                if (s.mode == SolarMode::BellCurve) {
                    int stepH = Scale(36);
                    RECT rcRowMin = {cardLeft + Scale(20), c2Top + Scale(34), cardRight - Scale(20), c2Top + Scale(34) + stepH};
                    RECT rcRowMax = {cardLeft + Scale(20), c2Top + Scale(34) + stepH, cardRight - Scale(20), c2Top + Scale(34) + stepH * 2};

                    DrawStepper(rcRowMin, L"Night Minimum Floor (Resting brightness during dark hours)", s.minBrightness, m_rcMinMinus, m_rcMinPlus, 30, 31);
                    DrawStepper(rcRowMax, L"Day Peak Maximum (Midday brightness to counteract room & window glare)", s.maxBrightness, m_rcMaxMinus, m_rcMaxPlus, 32, 33);

                    SelectObject(memDC, m_fontSmall);
                    SetTextColor(memDC, m_theme.textSecondary);
                    RECT rcInfo = {cardLeft + Scale(20), c2Top + Scale(34) + stepH * 2 + Scale(6), cardRight - Scale(20), c2Bottom - Scale(8)};
                    DrawTextW(memDC, L"Cycle Timeline:   Sunrise at 06:30 AM  •  Solar Noon Peak at 12:30 PM  •  Sunset at 06:30 PM", -1, &rcInfo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                } else {
                    int stepH = Scale(34);
                    RECT rcW1 = {cardLeft + Scale(20), c2Top + Scale(32), cardRight - Scale(20), c2Top + Scale(32) + stepH};
                    RECT rcW2 = {cardLeft + Scale(20), c2Top + Scale(32) + stepH, cardRight - Scale(20), c2Top + Scale(32) + stepH * 2};
                    RECT rcW3 = {cardLeft + Scale(20), c2Top + Scale(32) + stepH * 2, cardRight - Scale(20), c2Top + Scale(32) + stepH * 3};
                    RECT rcW4 = {cardLeft + Scale(20), c2Top + Scale(32) + stepH * 3, cardRight - Scale(20), c2Top + Scale(32) + stepH * 4};

                    DrawStepper(rcW1, L"Morning Ramp (07:00 AM)", s.wp1.brightness, m_rcWp1Minus, m_rcWp1Plus, 40, 41);
                    DrawStepper(rcW2, L"Midday Solar Peak (01:00 PM)", s.wp2.brightness, m_rcWp2Minus, m_rcWp2Plus, 42, 43);
                    DrawStepper(rcW3, L"Evening Dusk (06:30 PM)", s.wp3.brightness, m_rcWp3Minus, m_rcWp3Plus, 44, 45);
                    DrawStepper(rcW4, L"Night Floor (11:00 PM)", s.wp4.brightness, m_rcWp4Minus, m_rcWp4Plus, 46, 47);
                }

                // Card 3: Live Solar Status & Manual Freedom (Completely separated, NO text collision!)
                int c3Top = c2Bottom + Scale(14);
                int c3Bottom = c3Top + Scale(96);
                DrawCard(c3Top, c3Bottom);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textMuted);
                RECT rcS3Label = {cardLeft + Scale(20), c3Top + Scale(12), cardRight - Scale(20), c3Top + Scale(28)};
                DrawTextW(memDC, L"LIVE STATUS & MANUAL OVERRIDE FREEDOM", -1, &rcS3Label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                int currMin = m_solarTracker->GetCurrentMinuteOfDay();
                int baseVal = m_solarTracker->GetBaseCurveValue(currMin);
                int effVal  = m_solarTracker->GetEffectiveTarget(currMin);
                int offVal  = s.manualOffset;

                wchar_t statusBuf[256];
                swprintf_s(statusBuf, L"Live Cycle: %d%% Calculated Base  •  Manual Offset: %+d%%  ➔  Target: %d%%", baseVal, offVal, effVal);

                // Reset Offset button on the top-right of Card 3
                int btnW = Scale(150);
                int btnH = Scale(32);
                m_rcResetOffset = {cardRight - btnW - Scale(16), c3Top + Scale(28), cardRight - Scale(16), c3Top + Scale(28) + btnH};
                bool isResHover = (m_hoveredElement == 50);
                COLORREF resBg = isResHover 
                    ? (m_theme.isDark ? RGB(58, 58, 58) : RGB(228, 228, 228))
                    : (m_theme.isDark ? RGB(45, 45, 45) : RGB(242, 242, 242));
                COLORREF resBdr = isResHover
                    ? (m_theme.isDark ? RGB(90, 90, 90) : RGB(180, 180, 180))
                    : m_theme.border;
                HBRUSH hResBrush = CreateSolidBrush(resBg);
                HPEN hResPen = CreatePen(PS_SOLID, 1, resBdr);
                HBRUSH oResB = (HBRUSH)SelectObject(memDC, hResBrush);
                HPEN oResP = (HPEN)SelectObject(memDC, hResPen);
                RoundRect(memDC, m_rcResetOffset.left, m_rcResetOffset.top, m_rcResetOffset.right, m_rcResetOffset.bottom, Scale(6), Scale(6));
                SelectObject(memDC, oResP);
                SelectObject(memDC, oResB);
                DeleteObject(hResBrush);
                DeleteObject(hResPen);

                SelectObject(memDC, m_fontBody);
                SetTextColor(memDC, isResHover ? m_theme.textPrimary : m_theme.textSecondary);
                DrawTextW(memDC, L"Reset Offset (0%)", -1, &m_rcResetOffset, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // Status text cleanly padded to left of Reset button (ZERO overlap!)
                SelectObject(memDC, m_fontSection);
                SetTextColor(memDC, s.enabled ? m_theme.textPrimary : m_theme.textMuted);
                RECT rcLive = {cardLeft + Scale(20), c3Top + Scale(32), m_rcResetOffset.left - Scale(16), c3Top + Scale(54)};
                DrawTextW(memDC, statusBuf, -1, &rcLive, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                SelectObject(memDC, m_fontSmall);
                SetTextColor(memDC, m_theme.textSecondary);
                RECT rcHint = {cardLeft + Scale(20), c3Top + Scale(58), cardRight - Scale(20), c3Bottom - Scale(10)};
                DrawTextW(memDC, L"Manual override freedom: adjusting the brightness slider at any time preserves your preference across the daylight curve.", -1, &rcHint, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            }

            // 5. Footer (Version info & Close Button)
            SelectObject(memDC, m_fontSmall);
            SetTextColor(memDC, m_theme.textMuted);
            RECT rcVer = {cardLeft, h - Scale(46), cardLeft + Scale(350), h - Scale(16)};
            DrawTextW(memDC, L"MonitorMan v1.0 • Native C++17 (Win32)", -1, &rcVer, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            int closeW = Scale(100);
            int closeH = Scale(34);
            m_rcCloseBtn = {cardRight - closeW, h - Scale(48), cardRight, h - Scale(48) + closeH};
            bool isCloseHov = (m_hoveredElement == 6);
            COLORREF closeBg = isCloseHov 
                ? (m_theme.isDark ? RGB(58, 58, 58) : RGB(228, 228, 228))
                : (m_theme.isDark ? RGB(45, 45, 45) : RGB(242, 242, 242));
            COLORREF closeBdr = isCloseHov
                ? (m_theme.isDark ? RGB(90, 90, 90) : RGB(180, 180, 180))
                : m_theme.border;
            HBRUSH hCloseBrush = CreateSolidBrush(closeBg);
            HPEN hClosePen = CreatePen(PS_SOLID, 1, closeBdr);
            HBRUSH oCB = (HBRUSH)SelectObject(memDC, hCloseBrush);
            HPEN oCP = (HPEN)SelectObject(memDC, hClosePen);
            RoundRect(memDC, m_rcCloseBtn.left, m_rcCloseBtn.top, m_rcCloseBtn.right, m_rcCloseBtn.bottom, Scale(6), Scale(6));
            SelectObject(memDC, oCP);
            SelectObject(memDC, oCB);
            DeleteObject(hCloseBrush);
            DeleteObject(hClosePen);

            SelectObject(memDC, m_fontBody);
            SetTextColor(memDC, isCloseHov ? m_theme.textPrimary : m_theme.textSecondary);
            DrawTextW(memDC, L"Close", -1, &m_rcCloseBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(m_hwnd, &ps);
            return 0;
        }

        case WM_DESTROY: {
            if (m_fontTitle)   DeleteObject(m_fontTitle);
            if (m_fontSection) DeleteObject(m_fontSection);
            if (m_fontBody)    DeleteObject(m_fontBody);
            if (m_fontSmall)   DeleteObject(m_fontSmall);
            if (m_fontIcon)    DeleteObject(m_fontIcon);
            return 0;
        }
        }
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

public:
    SettingsWindow(MonitorManager* mgr, SolarTracker* solar) : m_monitorMgr(mgr), m_solarTracker(solar) {}

    void SetOnSizeModeChanged(std::function<void(SizeMode)> cb) {
        m_onSizeModeChanged = cb;
    }

    void SetOnMonitorsRefreshed(std::function<void()> cb) {
        m_onMonitorsRefreshed = cb;
    }

    bool Create(HINSTANCE hInstance) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc   = SettingsWindow::WndProc;
        wc.hInstance     = hInstance;
        wc.lpszClassName = L"MonitorManSettingsClass";
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);

        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
        if (dpi <= 0) dpi = 96;

        int clientW = MulDiv(780, dpi, 96);
        int clientH = MulDiv(630, dpi, 96);

        RECT clientRc = {0, 0, clientW, clientH};
        DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
        DWORD exStyle = WS_EX_DLGMODALFRAME;
        AdjustWindowRectEx(&clientRc, style, FALSE, exStyle);

        int winW = clientRc.right - clientRc.left;
        int winH = clientRc.bottom - clientRc.top;

        RECT rcWorkArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
        int x = (rcWorkArea.right - rcWorkArea.left - winW) / 2;
        int y = (rcWorkArea.bottom - rcWorkArea.top - winH) / 2;

        m_hwnd = CreateWindowExW(
            exStyle,
            wc.lpszClassName,
            L"MonitorMan Settings",
            style,
            x, y, winW, winH,
            nullptr, nullptr, hInstance, this
        );

        if (!m_hwnd) return false;
        return true;
    }

    void Show() {
        if (!m_hwnd) return;
        SyncState();
        ApplyThemeToDwm();
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    HWND GetHwnd() const { return m_hwnd; }
};
