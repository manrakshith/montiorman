# MonitorMan

A super lightweight, modern, native Windows C++ desktop utility to control external monitor brightness and contrast using **DDC/CI** (Display Data Channel Command Interface).

---

## ✨ Features

- **⚡ Super Lightweight**:
  - Pure native Win32 C++ (No Qt, No Electron, No .NET, Zero external runtime DLLs).
  - Standalone executable (~970 KB statically linked).
  - Uses < 5 MB RAM and launches instantly (< 50 ms).
- **🪟 Rock-Solid Native Window**:
  - Regular native Windows application window with dark title bar, minimize, and close.
  - Stays open reliably—no auto-closing or focus issues.
  - Closing minimizes to the System Tray; clicking the tray icon restores it.
- **📐 3 View Size Modes (Right-Click Settings)**:
  - **Sleek (Slim & Compact)**: ultra-minimal, thin profile (`300px × 76px`), ideal for tight spaces.
  - **Default (Standard)**: balanced Windows 11 Fluent size (`360px × 96px`).
  - **Large (Expanded)**: wider track and larger touch targets (`440px × 126px`).
  - **Persistent**: your chosen size mode is automatically saved in the Windows Registry (`HKCU\Software\MonitorMan`).
- **⚙️ Right-Click Settings**:
  - Right-click anywhere inside the window to switch Size Modes, refresh displays, or toggle Windows startup.
- **🔢 Editable Number Badges**:
  - Direct numeric value badge (`0`–`100`) next to each slider track.
  - **Click to edit**: type any exact number directly and press `Enter` to commit.
- **🖱️ Velocity-Sensitive Mouse Wheel Control**:
  - **Fine Control**: nudge the wheel slowly for strictly **1 unit (1%)** adjustments per notch.
  - **Dynamic Acceleration**: flick the wheel quickly to jump by 3, 6, or more units.
- **⌨️ Global Shortcuts & System Tray**:
  - Press **`Ctrl + Alt + B`** or **`Win + Alt + B`** to summon or minimize the window.
- **🖥️ Hardware DDC/CI Support**:
  - Non-blocking background worker thread debounces hardware I2C writes to ensure silky smooth UI dragging with zero lag.

---

## 🛠️ Building from Source

### One-Click Build Script (MinGW)
```cmd
build.bat
```

### CMake (MSVC or MinGW)
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
