#pragma once

#include <windows.h>
#include <cmath>
#include <algorithm>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum class SolarMode {
    BellCurve = 1,      // Smooth sinusoidal day/night curve
    Schedule4Point = 2  // 4-point custom waypoints
};

struct SolarWaypoint {
    int hour = 0;       // 0 - 23
    int minute = 0;     // 0 - 59
    int brightness = 50; // 0 - 100
};

struct SolarSettings {
    bool enabled = false;
    SolarMode mode = SolarMode::BellCurve;

    // Minimum (Night) and Maximum (Day peak) Brightness
    int minBrightness = 20; // 0 - 100%
    int maxBrightness = 90; // 0 - 100%

    // Bell Curve Sunrise / Sunset (minutes from midnight)
    int sunriseMinute = 6 * 60 + 30;  // 06:30 AM
    int sunsetMinute  = 18 * 60 + 30; // 06:30 PM

    // 4-Point Custom Schedule
    SolarWaypoint wp1 = { 7,  0, 35}; // Morning ramp
    SolarWaypoint wp2 = {13,  0, 90}; // Midday solar peak
    SolarWaypoint wp3 = {18, 30, 45}; // Evening dusk
    SolarWaypoint wp4 = {23,  0, 20}; // Night minimum

    // User manual adjustment offset (-100 to +100)
    int manualOffset = 0;
};

inline void LoadSolarSettings(SolarSettings& s) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MonitorMan\\Solar", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwVal = 0;
        DWORD dwSize = sizeof(dwVal);

        if (RegQueryValueExW(hKey, L"Enabled", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.enabled = (dwVal != 0);
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Mode", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.mode = (dwVal == 2) ? SolarMode::Schedule4Point : SolarMode::BellCurve;
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"MinBrightness", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.minBrightness = std::clamp(static_cast<int>(dwVal), 0, 100);
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"MaxBrightness", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.maxBrightness = std::clamp(static_cast<int>(dwVal), 0, 100);
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"SunriseMinute", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.sunriseMinute = std::clamp(static_cast<int>(dwVal), 0, 1439);
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"SunsetMinute", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.sunsetMinute = std::clamp(static_cast<int>(dwVal), 0, 1439);
        }
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"ManualOffset", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            s.manualOffset = static_cast<int>(dwVal);
        }

        // Waypoints
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Wp1Bright", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) s.wp1.brightness = static_cast<int>(dwVal);
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Wp2Bright", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) s.wp2.brightness = static_cast<int>(dwVal);
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Wp3Bright", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) s.wp3.brightness = static_cast<int>(dwVal);
        dwSize = sizeof(dwVal);
        if (RegQueryValueExW(hKey, L"Wp4Bright", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) s.wp4.brightness = static_cast<int>(dwVal);

        RegCloseKey(hKey);
    }
}

inline void SaveSolarSettings(const SolarSettings& s) {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\MonitorMan\\Solar", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD dwVal = s.enabled ? 1 : 0;
        RegSetValueExW(hKey, L"Enabled", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = (s.mode == SolarMode::Schedule4Point) ? 2 : 1;
        RegSetValueExW(hKey, L"Mode", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.minBrightness);
        RegSetValueExW(hKey, L"MinBrightness", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.maxBrightness);
        RegSetValueExW(hKey, L"MaxBrightness", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.sunriseMinute);
        RegSetValueExW(hKey, L"SunriseMinute", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.sunsetMinute);
        RegSetValueExW(hKey, L"SunsetMinute", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.manualOffset);
        RegSetValueExW(hKey, L"ManualOffset", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        dwVal = static_cast<DWORD>(s.wp1.brightness);
        RegSetValueExW(hKey, L"Wp1Bright", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));
        dwVal = static_cast<DWORD>(s.wp2.brightness);
        RegSetValueExW(hKey, L"Wp2Bright", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));
        dwVal = static_cast<DWORD>(s.wp3.brightness);
        RegSetValueExW(hKey, L"Wp3Bright", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));
        dwVal = static_cast<DWORD>(s.wp4.brightness);
        RegSetValueExW(hKey, L"Wp4Bright", 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(dwVal));

        RegCloseKey(hKey);
    }
}

// Calculate base solar brightness (0-100) before applying user manual offset
inline int CalculateSolarBase(const SolarSettings& s, int minuteOfDay) {
    int m = (minuteOfDay % 1440 + 1440) % 1440;

    if (s.mode == SolarMode::BellCurve) {
        int rise = s.sunriseMinute;
        int set  = s.sunsetMinute;

        // If current time is during the night (before sunrise or after sunset)
        if (rise < set) {
            if (m < rise || m > set) {
                return s.minBrightness;
            }
            double frac = static_cast<double>(m - rise) / static_cast<double>(set - rise);
            double factor = std::sin(M_PI * frac); // 0 at rise, 1.0 at noon, 0 at set
            return s.minBrightness + static_cast<int>(std::round((s.maxBrightness - s.minBrightness) * factor));
        } else {
            // Polar / wrapped cases fallback
            return s.minBrightness;
        }
    } else {
        // 4-Point Waypoints:
        int t1 = s.wp1.hour * 60 + s.wp1.minute;
        int t2 = s.wp2.hour * 60 + s.wp2.minute;
        int t3 = s.wp3.hour * 60 + s.wp3.minute;
        int t4 = s.wp4.hour * 60 + s.wp4.minute;

        struct WP { int t; int b; };
        WP wps[4] = {
            { t1, s.wp1.brightness },
            { t2, s.wp2.brightness },
            { t3, s.wp3.brightness },
            { t4, s.wp4.brightness }
        };

        // Find surrounding points
        int tA = wps[3].t;
        int bA = wps[3].b;
        int tB = wps[0].t;
        int bB = wps[0].b;

        for (int i = 0; i < 3; ++i) {
            if (m >= wps[i].t && m < wps[i + 1].t) {
                tA = wps[i].t;
                bA = wps[i].b;
                tB = wps[i + 1].t;
                bB = wps[i + 1].b;
                break;
            }
        }
        if (m >= wps[3].t || m < wps[0].t) {
            tA = wps[3].t;
            bA = wps[3].b;
            tB = wps[0].t;
            bB = wps[0].b;
        }

        int span = (tB - tA + 1440) % 1440;
        if (span == 0) return bA;
        int elapsed = (m - tA + 1440) % 1440;
        double u = static_cast<double>(elapsed) / static_cast<double>(span);
        // Smooth cosine interpolation
        double f = (1.0 - std::cos(M_PI * u)) / 2.0;
        return bA + static_cast<int>(std::round((bB - bA) * f));
    }
}

class SolarTracker {
private:
    SolarSettings m_settings;
    int m_lastMinute = -1;

public:
    SolarTracker() {
        LoadSolarSettings(m_settings);
    }

    const SolarSettings& GetSettings() const { return m_settings; }
    SolarSettings& GetSettings() { return m_settings; }

    void Save() {
        SaveSolarSettings(m_settings);
    }

    void SetEnabled(bool en) {
        m_settings.enabled = en;
        Save();
    }

    void SetMode(SolarMode mode) {
        m_settings.mode = mode;
        Save();
    }

    void SetMinBrightness(int val) {
        m_settings.minBrightness = std::clamp(val, 0, 100);
        Save();
    }

    void SetMaxBrightness(int val) {
        m_settings.maxBrightness = std::clamp(val, 0, 100);
        Save();
    }

    void ResetOffset() {
        m_settings.manualOffset = 0;
        Save();
    }

    int GetCurrentMinuteOfDay() const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        return st.wHour * 60 + st.wMinute;
    }

    int GetBaseCurveValue(int minute = -1) const {
        if (minute < 0) minute = GetCurrentMinuteOfDay();
        return CalculateSolarBase(m_settings, minute);
    }

    int GetEffectiveTarget(int minute = -1) const {
        if (minute < 0) minute = GetCurrentMinuteOfDay();
        int base = CalculateSolarBase(m_settings, minute);
        return std::clamp(base + m_settings.manualOffset, 0, 100);
    }

    // Called when the user manually drags the slider in the flyout
    // Updates the offset so the monitor can still track solar curve relative to user preference!
    void OnUserBrightnessChanged(int userVal) {
        if (!m_settings.enabled) return;
        int base = GetBaseCurveValue();
        m_settings.manualOffset = userVal - base;
        Save();
    }

    // Returns true if brightness should be updated, and outputs target brightness
    bool Tick(int currentHardwareBrightness, int& outTarget) {
        if (!m_settings.enabled) return false;

        int minute = GetCurrentMinuteOfDay();
        int target = GetEffectiveTarget(minute);

        if (target == currentHardwareBrightness) {
            return false;
        }

        // Micro-step towards target (1% per tick) for imperceptible smooth transitions
        int step = (target > currentHardwareBrightness)
            ? std::min(target, currentHardwareBrightness + 1)
            : std::max(target, currentHardwareBrightness - 1);

        outTarget = step;
        return true;
    }
};
