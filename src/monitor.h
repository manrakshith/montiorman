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
    HANDLE hPhysicalMonitor = nullptr;
    
    bool supportsBrightness = false;
    DWORD minBrightness = 0;
    DWORD maxBrightness = 100;
    DWORD currBrightness = 50;
    
    bool supportsContrast = false;
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

    // Asynchronous hardware write queue to prevent UI stutter
    struct PendingCommand {
        HANDLE hPhysical = nullptr;
        bool setBrightness = false;
        DWORD brightnessVal = 0;
        bool setContrast = false;
        DWORD contrastVal = 0;
    };

    std::mutex m_workerMutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
    std::thread m_workerThread;
    PendingCommand m_pendingCmd;
    bool m_hasPending = false;

    static BOOL CALLBACK EnumCallback(HMONITOR hMon, HDC, LPRECT, LPARAM lParam) {
        auto* self = reinterpret_cast<MonitorManager*>(lParam);
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
                    dev.name = pPhys[i].szPhysicalMonitorDescription;
                    if (dev.name.empty()) {
                        dev.name = L"Display " + std::to_wstring(self->m_devices.size() + 1);
                    }
                    dev.hPhysicalMonitor = pPhys[i].hPhysicalMonitor;

                    // Query Brightness
                    DWORD bMin = 0, bCur = 0, bMax = 0;
                    if (GetMonitorBrightness(dev.hPhysicalMonitor, &bMin, &bCur, &bMax)) {
                        dev.supportsBrightness = true;
                        dev.minBrightness = bMin;
                        dev.maxBrightness = bMax;
                        dev.currBrightness = bCur;
                    }

                    // Query Contrast
                    DWORD cMin = 0, cCur = 0, cMax = 0;
                    if (GetMonitorContrast(dev.hPhysicalMonitor, &cMin, &cCur, &cMax)) {
                        dev.supportsContrast = true;
                        dev.minContrast = cMin;
                        dev.maxContrast = cMax;
                        dev.currContrast = cCur;
                    }

                    // Add device if it supports brightness or contrast
                    if (dev.supportsBrightness || dev.supportsContrast) {
                        self->m_devices.push_back(dev);
                    }
                }
            } else {
                delete[] pPhys;
            }
        }
        return TRUE;
    }

    void WorkerLoop() {
        while (m_running) {
            PendingCommand cmd;
            {
                std::unique_lock<std::mutex> lock(m_workerMutex);
                m_cv.wait(lock, [this]() { return !m_running || m_hasPending; });
                if (!m_running) break;
                cmd = m_pendingCmd;
                m_hasPending = false;
            }

            if (cmd.hPhysical) {
                if (cmd.setBrightness) {
                    SetMonitorBrightness(cmd.hPhysical, cmd.brightnessVal);
                }
                if (cmd.setContrast) {
                    SetMonitorContrast(cmd.hPhysical, cmd.contrastVal);
                }
            }
        }
    }

public:
    MonitorManager() {
        m_running = true;
        m_workerThread = std::thread(&MonitorManager::WorkerLoop, this);
        Refresh();
    }

    ~MonitorManager() {
        m_running = false;
        m_cv.notify_all();
        if (m_workerThread.joinable()) {
            m_workerThread.join();
        }
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

    void SetBrightnessAsync(size_t index, DWORD val) {
        if (index >= m_devices.size()) return;
        auto& dev = m_devices[index];
        if (!dev.supportsBrightness) return;
        dev.currBrightness = val;

        {
            std::lock_guard<std::mutex> lock(m_workerMutex);
            if (m_pendingCmd.hPhysical != dev.hPhysicalMonitor) {
                m_pendingCmd.hPhysical = dev.hPhysicalMonitor;
                m_pendingCmd.setContrast = false;
            }
            m_pendingCmd.setBrightness = true;
            m_pendingCmd.brightnessVal = val;
            m_hasPending = true;
        }
        m_cv.notify_one();
    }

    void SetContrastAsync(size_t index, DWORD val) {
        if (index >= m_devices.size()) return;
        auto& dev = m_devices[index];
        if (!dev.supportsContrast) return;
        dev.currContrast = val;

        {
            std::lock_guard<std::mutex> lock(m_workerMutex);
            if (m_pendingCmd.hPhysical != dev.hPhysicalMonitor) {
                m_pendingCmd.hPhysical = dev.hPhysicalMonitor;
                m_pendingCmd.setBrightness = false;
            }
            m_pendingCmd.setContrast = true;
            m_pendingCmd.contrastVal = val;
            m_hasPending = true;
        }
        m_cv.notify_one();
    }
};

