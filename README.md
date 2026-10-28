# MonitorMan

### MonitorMan is a lightweight, native Windows desktop utility designed to control external monitor brightness and contrast via VESA DDC/CI (Display Data Channel Command Interface).

---

## Key Features

- **Native Win32 Implementation**:
  - Pure C++17 utilizing the Windows API and High-Level Monitor Configuration API (`dxva2.lib`).
  - Completely standalone — zero external runtime dependencies (no Qt, Electron, or .NET runtimes).
  - Minimal resource usage: compact executable (~290 KB) with tiny idle memory footprint (< 5 MB RAM).

- **Windows 11 Fluent Flyout**:
  - Sleek, borderless flyout window featuring DWM rounded corners and native dark/light theme detection.
  - Automatically docks above the system tray taskbar and supports instantaneous summoning.

- **Solar Circadian Daylight Tracking**: (under development)
  - **Dynamic Sun-Tracking Luminance**: Automatically adjusts monitor brightness throughout the day to counteract sunlight glare during peak daylight hours and dim gently in the evening to reduce eye strain.
  - **Two Tracking Engines**:
    - **Smooth Solar Bell Curve**: Sinusoidal mathematical curve following the sun from sunrise (06:30 AM) to solar noon (12:30 PM peak) to sunset (06:30 PM), with nighttime floor rest.
    - **4-Point Custom Waypoints**: Fully customizable schedule targeting specific brightness levels for Morning (07:00), Midday Solar Peak (13:00), Evening Dusk (18:30), and Bedtime (23:00).
  - **Full Manual Override Freedom**: Adjusting the brightness slider in the flyout never disables solar tracking. Instead, MonitorMan preserves your personal preference offset across the entire daylight curve!
  - **Hardware I2C Safety**: Luminance changes are applied via smooth 1% micro-steps to prevent display flicker and avoid I2C bus saturation.

- **Global Shortcuts & System Tray**:
  - Hotkeys: `Win + Alt + B` or `Ctrl + Alt + B` to toggle the flyout from any application or game.
  - High-resolution dynamic system tray icon reflecting your monitor's display state.
  - Windows autostart integration via Run key (`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`).

---

## Requirements

- **Operating System**: Windows 10 (version 1809 or higher) or Windows 11
- **Display Hardware**: External monitor with VESA DDC/CI enabled in its on-screen display (OSD) settings. Connected via DisplayPort, HDMI, USB-C (DisplayPort Alt Mode), or DVI.

---

## Building from Source

### Prerequisites
- Modern MinGW-w64 toolchain (GCC 11+ recommended) or Microsoft Visual C++ (MSVC)
- Windows SDK (for standard headers and `dxva2.lib`)

### Quick Build (MinGW)
Run the included build script to compile the standalone binary:
```cmd
build.bat
```
The compiled binary will be generated at `bin\monitorman.exe`.

### Building the Installer (NSIS)
To generate the single-file setup wizard (`bin\MonitorMan_Setup.exe`):
```cmd
build_installer.bat
```
This packages the executable, high-DPI application icons, Start Menu shortcuts, Desktop shortcut, and a clean Windows uninstaller into a single setup executable.

---

## Keyboard Shortcuts

| Shortcut | Description |
| :--- | :--- |
| `Win + Alt + B` | Summon or dismiss the MonitorMan flyout |
| `Ctrl + Alt + B` | Alternative global hotkey to toggle the flyout |
| `Esc` | Close flyout or settings window |
| `Enter` | Commit direct numeric entry in slider text box |

---

## License

MIT License. See [LICENSE](LICENSE) for details.
