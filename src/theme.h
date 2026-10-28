#pragma once

#include <windows.h>
#include <dwmapi.h>
#include <string>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWCP_ROUND 2
#endif

struct Theme {
    bool isDark = true;
    COLORREF bg;
    COLORREF cardBg;
    COLORREF border;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF textMuted;
    COLORREF trackBg;
    COLORREF trackHover;
    COLORREF accent;
    COLORREF thumbBg;
    COLORREF thumbBorder;

    static bool CheckIsDarkMode() {
        DWORD val = 1;
        DWORD size = sizeof(val);
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&val, &size);
            RegCloseKey(hKey);
        }
        return (val == 0);
    }

    static COLORREF QuerySystemAccent() {
        DWORD color = 0;
        DWORD size = sizeof(color);
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, L"AccentColor", nullptr, nullptr, (LPBYTE)&color, &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                BYTE r = static_cast<BYTE>(color & 0x000000FF);
                BYTE g = static_cast<BYTE>((color & 0x0000FF00) >> 8);
                BYTE b = static_cast<BYTE>((color & 0x00FF0000) >> 16);
                return RGB(r, g, b);
            }
            RegCloseKey(hKey);
        }
        return RGB(0, 120, 215); // Fallback Windows Blue
    }

    void Refresh() {
        isDark = CheckIsDarkMode();
        accent = QuerySystemAccent();

        if (isDark) {
            bg            = RGB(32, 32, 32);       // Win11 Dark flyout background
            cardBg        = RGB(43, 43, 43);       // Card background
            border        = RGB(56, 56, 56);       // Subtle border
            textPrimary   = RGB(255, 255, 255);   // Crisp white
            textSecondary = RGB(200, 200, 200);   // Subtle grey
            textMuted     = RGB(140, 140, 140);   // Secondary info
            trackBg       = RGB(60, 60, 60);       // Unfilled slider track
            trackHover    = RGB(75, 75, 75);
            thumbBg       = RGB(255, 255, 255);
            thumbBorder   = RGB(100, 100, 100);
        } else {
            bg            = RGB(249, 249, 249);   // Win11 Light flyout background
            cardBg        = RGB(255, 255, 255);   // Card background
            border        = RGB(225, 225, 225);   // Light border
            textPrimary   = RGB(26, 26, 26);       // Dark text
            textSecondary = RGB(95, 95, 95);      // Neutral grey
            textMuted     = RGB(150, 150, 150);
            trackBg       = RGB(220, 220, 220);   // Light unfilled slider track
            trackHover    = RGB(205, 205, 205);
            thumbBg       = RGB(255, 255, 255);
            thumbBorder   = RGB(200, 200, 200);
        }
    }
};

inline HFONT CreateSegoeFont(int pointSize, int weight = FW_NORMAL) {
    HDC hdc = GetDC(nullptr);
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);

    int height = -MulDiv(pointSize, logPixelsY, 72);

    // Prefer "Segoe UI Variable Text", then "Segoe UI"
    HFONT hFont = CreateFontW(
        height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Variable Text"
    );

    if (!hFont) {
        hFont = CreateFontW(
            height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
    }
    return hFont;
}

inline HFONT CreateIconFont(int pointSize) {
    HDC hdc = GetDC(nullptr);
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);

    int height = -MulDiv(pointSize, logPixelsY, 72);

    // Prefer Windows 11 "Segoe Fluent Icons", then Windows 10 "Segoe MDL2 Assets"
    HFONT hFont = CreateFontW(
        height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe Fluent Icons"
    );

    if (!hFont) {
        hFont = CreateFontW(
            height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe MDL2 Assets"
        );
    }
    return hFont;
}

