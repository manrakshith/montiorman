#pragma once

#include <windows.h>
#include <string>
#include "ui_slider.h"

#define ID_TRAY_OPEN     2001
#define ID_TRAY_SETTINGS 2002
#define ID_TRAY_REFRESH  2003
#define ID_TRAY_STARTUP  2004
#define ID_TRAY_EXIT     2005

#define ID_MODE_SLEEK   3101
#define ID_MODE_DEFAULT 3102
#define ID_MODE_LARGE   3103

inline bool IsAutoStartEnabled() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH];
        DWORD size = sizeof(path);
        LSTATUS status = RegQueryValueExW(hKey, L"MonitorMan", nullptr, nullptr, (LPBYTE)path, &size);
        RegCloseKey(hKey);
        return (status == ERROR_SUCCESS);
    }
    return false;
}

inline void SetAutoStart(bool enable) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            std::wstring quoted = L"\"" + std::wstring(path) + L"\"";
            RegSetValueExW(hKey, L"MonitorMan", 0, REG_SZ, 
                reinterpret_cast<const BYTE*>(quoted.c_str()), 
                static_cast<DWORD>((quoted.length() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"MonitorMan");
        }
        RegCloseKey(hKey);
    }
}

inline SizeMode LoadSizeMode() {
    HKEY hKey = nullptr;
    DWORD mode = static_cast<DWORD>(SizeMode::Default);
    DWORD size = sizeof(mode);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MonitorMan", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"SizeMode", nullptr, nullptr, (LPBYTE)&mode, &size);
        RegCloseKey(hKey);
    }
    if (mode > 2) mode = 1;
    return static_cast<SizeMode>(mode);
}

inline void SaveSizeMode(SizeMode mode) {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\MonitorMan", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = static_cast<DWORD>(mode);
        RegSetValueExW(hKey, L"SizeMode", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));
        RegCloseKey(hKey);
    }
}

