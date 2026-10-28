#pragma once

#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct PhysicalMonitorDevice {
    std::wstring name;
    HMONITOR hMon = nullptr;
    HANDLE hPhysicalMonitor = nullptr;
    
    bool supportsBrightness = true;
    DWORD minBrightness = 0;
    DWORD maxBrightness = 100;
    DWORD currBrightness = 50;
    
    bool supportsContrast = true;
    DWORD minContrast = 0;
    DWORD maxContrast = 100;
    DWORD currContrast = 50;
};

class MonitorManager {
private:
    struct HMonitorData {
        DWORD count = 0;
        PHYSICAL_MONITOR* pMonitors = nullptr;
    };

    std::vector<HMonitorData> m_hMonitors;
    std::vector<PhysicalMonitorDevice> m_devices;

    DWORD m_lastBrightnessTime = 0;
    DWORD m_lastContrastTime = 0;

    static std::wstring GetMonitorNameFromEDID() {
        HKEY hDisplayKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY", 0, KEY_READ, &hDisplayKey) != ERROR_SUCCESS) {
            return L"";
        }

        wchar_t monitorSubKey[256];
        DWORD subKeyLen = 256;
        for (DWORD i = 0; RegEnumKeyExW(hDisplayKey, i, monitorSubKey, &subKeyLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; ++i) {
            subKeyLen = 256;
            HKEY hMonKey = nullptr;
            if (RegOpenKeyExW(hDisplayKey, monitorSubKey, 0, KEY_READ, &hMonKey) == ERROR_SUCCESS) {
                wchar_t instanceKey[256];
                DWORD instLen = 256;
                for (DWORD j = 0; RegEnumKeyExW(hMonKey, j, instanceKey, &instLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; ++j) {
                    instLen = 256;
                    std::wstring paramPath = std::wstring(instanceKey) + L"\\Device Parameters";
                    HKEY hParamKey = nullptr;
                    if (RegOpenKeyExW(hMonKey, paramPath.c_str(), 0, KEY_READ, &hParamKey) == ERROR_SUCCESS) {
                        BYTE edid[512] = {0};
                        DWORD edidSize = sizeof(edid);
                        if (RegQueryValueExW(hParamKey, L"EDID", nullptr, nullptr, edid, &edidSize) == ERROR_SUCCESS && edidSize >= 128) {
                            for (int blk = 54; blk <= 108; blk += 18) {
                                if (edid[blk] == 0x00 && edid[blk + 1] == 0x00 && edid[blk + 2] == 0x00 && edid[blk + 3] == 0xFC) {
                                    std::string name;
                                    for (int c = blk + 5; c < blk + 18; ++c) {
                                        if (edid[c] == 0x0A || edid[c] == 0x00) break;
                                        name += (char)edid[c];
                                    }
                                    while (!name.empty() && name.back() == ' ') name.pop_back();
                                    if (!name.empty()) {
                                        RegCloseKey(hParamKey);
                                        RegCloseKey(hMonKey);
                                        RegCloseKey(hDisplayKey);
                                        return std::wstring(name.begin(), name.end());
                                    }
                                }
                            }
                        }
                        RegCloseKey(hParamKey);
                    }
                }
                RegCloseKey(hMonKey);
            }
        }
        RegCloseKey(hDisplayKey);
        return L"";
    }

    static BOOL CALLBACK EnumCallback(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
        auto* self = reinterpret_cast<MonitorManager*>(lParam);

        std::wstring friendlyName;
        MONITORINFOEXW mi{sizeof(mi)};
        if (GetMonitorInfoW(hMon, &mi)) {
            DISPLAY_DEVICEW ddMon{sizeof(ddMon)};
            if (EnumDisplayDevicesW(mi.szDevice, 0, &ddMon, 0)) {
                if (ddMon.DeviceString[0] != L'\0') {
                    friendlyName = ddMon.DeviceString;
                }
            }
        }

        DWORD count = 0;
        if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMon, &count) && count > 0) {
            PHYSICAL_MONITOR* pPhys = new PHYSICAL_MONITOR[count];
            if (GetPhysicalMonitorsFromHMONITOR(hMon, count, pPhys)) {
                HMonitorData data;
                data.count = count;
                data.pMonitors = pPhys;
                self->m_hMonitors.push_back(data);

                for (DWORD i = 0; i < count; ++i) {
                    PhysicalMonitorDevice dev;
                    std::wstring dName = pPhys[i].szPhysicalMonitorDescription;
                    if (dName.empty() || dName == L"Generic PnP Monitor" || dName == L"Generic Monitor") {
                        std::wstring edidName = GetMonitorNameFromEDID();
                        if (!edidName.empty()) {
                            dName = edidName;
                        } else if (!friendlyName.empty() && friendlyName != L"Generic PnP Monitor") {
                            dName = friendlyName;
                        } else {
                            dName = L"Display " + std::to_wstring(self->m_devices.size() + 1);
                        }
                    }
                    dev.name = dName;
                    dev.hMon = hMon;
                    dev.hPhysicalMonitor = pPhys[i].hPhysicalMonitor;

                    // Query Brightness
                    DWORD bMin = 0, bCur = 50, bMax = 100;
                    if (GetMonitorBrightness(dev.hPhysicalMonitor, &bMin, &bCur, &bMax)) {
                        dev.supportsBrightness = true;
                        dev.minBrightness = bMin;
                        dev.maxBrightness = bMax;
                        dev.currBrightness = bCur;
                    } else {
                        // Some monitors only support write, not readback
                        dev.supportsBrightness = true;
                        dev.minBrightness = 0;
                        dev.maxBrightness = 100;
                        dev.currBrightness = 50;
                    }

                    // Query Contrast
                    DWORD cMin = 0, cCur = 50, cMax = 100;
                    if (GetMonitorContrast(dev.hPhysicalMonitor, &cMin, &cCur, &cMax)) {
                        dev.supportsContrast = true;
                        dev.minContrast = cMin;
                        dev.maxContrast = cMax;
                        dev.currContrast = cCur;
                    } else {
                        dev.supportsContrast = true;
                        dev.minContrast = 0;
                        dev.maxContrast = 100;
                        dev.currContrast = 50;
                    }

                    // Always add device
                    self->m_devices.push_back(dev);
                }
            } else {
                delete[] pPhys;
            }
        }
        return TRUE;
    }

public:
    MonitorManager() {
        Refresh();
    }

    ~MonitorManager() {
        CleanupHandles();
    }

    void CleanupHandles() {
        for (auto& hData : m_hMonitors) {
            if (hData.pMonitors && hData.count > 0) {
                DestroyPhysicalMonitors(hData.count, hData.pMonitors);
                delete[] hData.pMonitors;
            }
        }
        m_hMonitors.clear();
        m_devices.clear();
    }

    void Refresh() {
        CleanupHandles();
        EnumDisplayMonitors(nullptr, nullptr, EnumCallback, reinterpret_cast<LPARAM>(this));

        // Fallback: If no monitor was added by enumeration, query primary desktop monitor directly
        if (m_devices.empty()) {
            HMONITOR hPrimary = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
            if (hPrimary) {
                DWORD count = 0;
                if (GetNumberOfPhysicalMonitorsFromHMONITOR(hPrimary, &count) && count > 0) {
                    PHYSICAL_MONITOR* pPhys = new PHYSICAL_MONITOR[count];
                    if (GetPhysicalMonitorsFromHMONITOR(hPrimary, count, pPhys)) {
                        HMonitorData data;
                        data.count = count;
                        data.pMonitors = pPhys;
                        m_hMonitors.push_back(data);

                        for (DWORD i = 0; i < count; ++i) {
                            PhysicalMonitorDevice dev;
                            std::wstring edidName = GetMonitorNameFromEDID();
                            dev.name = !edidName.empty() ? edidName : L"Primary Display";
                            dev.hMon = hPrimary;
                            dev.hPhysicalMonitor = pPhys[i].hPhysicalMonitor;

                            DWORD bMin = 0, bCur = 50, bMax = 100;
                            dev.supportsBrightness = GetMonitorBrightness(dev.hPhysicalMonitor, &bMin, &bCur, &bMax);
                            dev.minBrightness = dev.supportsBrightness ? bMin : 0;
                            dev.maxBrightness = dev.supportsBrightness ? bMax : 100;
                            dev.currBrightness = dev.supportsBrightness ? bCur : 50;

                            DWORD cMin = 0, cCur = 50, cMax = 100;
                            dev.supportsContrast = GetMonitorContrast(dev.hPhysicalMonitor, &cMin, &cCur, &cMax);
                            dev.minContrast = dev.supportsContrast ? cMin : 0;
                            dev.maxContrast = dev.supportsContrast ? cMax : 100;
                            dev.currContrast = dev.supportsContrast ? cCur : 50;

                            m_devices.push_back(dev);
                        }
                    } else {
                        delete[] pPhys;
                    }
                }
            }
        }
    }

    const std::vector<PhysicalMonitorDevice>& GetDevices() const {
        return m_devices;
    }

    size_t GetDeviceCount() const {
        return m_devices.size();
    }

    PhysicalMonitorDevice* GetDevice(size_t index) {
        if (index < m_devices.size()) {
            return &m_devices[index];
        }
        return nullptr;
    }

    void SetBrightness(size_t index, DWORD val, bool force = false) {
        DWORD now = GetTickCount();
        if (!force && (now - m_lastBrightnessTime < 30)) {
            return;
        }
        m_lastBrightnessTime = now;

        if (index >= m_devices.size()) return;
        auto& dev = m_devices[index];
        dev.currBrightness = val;

        HMONITOR targetMon = dev.hMon ? dev.hMon : MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
        if (targetMon) {
            DWORD count = 0;
            if (GetNumberOfPhysicalMonitorsFromHMONITOR(targetMon, &count) && count > 0) {
                PHYSICAL_MONITOR* arr = new PHYSICAL_MONITOR[count];
                if (GetPhysicalMonitorsFromHMONITOR(targetMon, count, arr)) {
                    SetMonitorBrightness(arr[0].hPhysicalMonitor, val);
                    DestroyPhysicalMonitors(count, arr);
                }
                delete[] arr;
            }
        }
    }

    void SetContrast(size_t index, DWORD val, bool force = false) {
        DWORD now = GetTickCount();
        if (!force && (now - m_lastContrastTime < 30)) {
            return;
        }
        m_lastContrastTime = now;

        if (index >= m_devices.size()) return;
        auto& dev = m_devices[index];
        dev.currContrast = val;

        HMONITOR targetMon = dev.hMon ? dev.hMon : MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
        if (targetMon) {
            DWORD count = 0;
            if (GetNumberOfPhysicalMonitorsFromHMONITOR(targetMon, &count) && count > 0) {
                PHYSICAL_MONITOR* arr = new PHYSICAL_MONITOR[count];
                if (GetPhysicalMonitorsFromHMONITOR(targetMon, count, arr)) {
                    SetMonitorContrast(arr[0].hPhysicalMonitor, val);
                    DestroyPhysicalMonitors(count, arr);
                }
                delete[] arr;
            }
        }
    }

    void ReadLiveValues(size_t index) {
        if (index >= m_devices.size()) return;
        auto& dev = m_devices[index];
        HMONITOR targetMon = dev.hMon ? dev.hMon : MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
        if (targetMon) {
            DWORD count = 0;
            if (GetNumberOfPhysicalMonitorsFromHMONITOR(targetMon, &count) && count > 0) {
                PHYSICAL_MONITOR* arr = new PHYSICAL_MONITOR[count];
                if (GetPhysicalMonitorsFromHMONITOR(targetMon, count, arr)) {
                    DWORD bMin = 0, bCur = 0, bMax = 0;
                    if (GetMonitorBrightness(arr[0].hPhysicalMonitor, &bMin, &bCur, &bMax)) {
                        dev.minBrightness = bMin;
                        dev.maxBrightness = bMax;
                        dev.currBrightness = bCur;
                    }
                    DWORD cMin = 0, cCur = 0, cMax = 0;
                    if (GetMonitorContrast(arr[0].hPhysicalMonitor, &cMin, &cCur, &cMax)) {
                        dev.minContrast = cMin;
                        dev.maxContrast = cMax;
                        dev.currContrast = cCur;
                    }
                    DestroyPhysicalMonitors(count, arr);
                }
                delete[] arr;
            }
        }
    }
};

