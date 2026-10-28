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

    static std::wstring GetMonitorNameForDevice(const wchar_t* adapterDeviceName) {
        if (!adapterDeviceName || adapterDeviceName[0] == L'\0') return L"";

        DISPLAY_DEVICEW ddMon{sizeof(ddMon)};
        for (DWORD j = 0; EnumDisplayDevicesW(adapterDeviceName, j, &ddMon, 1); ++j) {
            std::wstring devId = ddMon.DeviceID;
            // Example: \\?\DISPLAY#BNQ7964#4&1e0ce86e&0&UID214595#{...}
            size_t p1 = devId.find(L"DISPLAY#");
            if (p1 != std::wstring::npos) {
                p1 += 8;
                size_t p2 = devId.find(L'#', p1);
                if (p2 != std::wstring::npos) {
                    std::wstring modelId = devId.substr(p1, p2 - p1);
                    size_t p3 = devId.find(L'#', p2 + 1);
                    std::wstring instanceId = (p3 != std::wstring::npos) ? devId.substr(p2 + 1, p3 - (p2 + 1)) : devId.substr(p2 + 1);

                    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\" + modelId + L"\\" + instanceId + L"\\Device Parameters";
                    HKEY hKey = nullptr;
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                        BYTE edid[512] = {0};
                        DWORD edidSize = sizeof(edid);
                        if (RegQueryValueExW(hKey, L"EDID", nullptr, nullptr, edid, &edidSize) == ERROR_SUCCESS && edidSize >= 128) {
                            for (int blk = 54; blk <= 108; blk += 18) {
                                if (edid[blk] == 0x00 && edid[blk + 1] == 0x00 && edid[blk + 2] == 0x00 && edid[blk + 3] == 0xFC) {
                                    std::string name;
                                    for (int c = blk + 5; c < blk + 18; ++c) {
                                        if (edid[c] == 0x0A || edid[c] == 0x00) break;
                                        name += (char)edid[c];
                                    }
                                    while (!name.empty() && name.back() == ' ') name.pop_back();
                                    if (!name.empty()) {
                                        RegCloseKey(hKey);
                                        return std::wstring(name.begin(), name.end());
                                    }
                                }
                            }
                        }
                        RegCloseKey(hKey);
                    }
                }
            }

            // Fallback to DeviceString if non-generic
            if (ddMon.DeviceString[0] != L'\0' && 
                wcscmp(ddMon.DeviceString, L"Generic PnP Monitor") != 0 &&
                wcscmp(ddMon.DeviceString, L"Generic Monitor") != 0) {
                return ddMon.DeviceString;
            }
        }
        return L"";
    }

    static BOOL CALLBACK EnumCallback(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
        auto* self = reinterpret_cast<MonitorManager*>(lParam);

        std::wstring specificName;
        MONITORINFOEXW mi{sizeof(mi)};
        if (GetMonitorInfoW(hMon, &mi)) {
            specificName = GetMonitorNameForDevice(mi.szDevice);
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
                    if (!specificName.empty()) {
                        dName = specificName;
                    } else if (dName.empty() || dName == L"Generic PnP Monitor" || dName == L"Generic Monitor") {
                        dName = L"Display " + std::to_wstring(self->m_devices.size() + 1);
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
                            std::wstring specificName;
                            MONITORINFOEXW mi{sizeof(mi)};
                            if (GetMonitorInfoW(hPrimary, &mi)) {
                                specificName = GetMonitorNameForDevice(mi.szDevice);
                            }
                            dev.name = !specificName.empty() ? specificName : L"Primary Display";
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

