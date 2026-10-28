#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <algorithm>
#include <functional>
#include "theme.h"

enum class SizeMode {
    Sleek = 0,
    Default = 1,
    Large = 2
};

class FluentSlider {
public:
    RECT bounds{0, 0, 0, 0};
    RECT trackRect{0, 0, 0, 0};
    RECT iconRect{0, 0, 0, 0};
    RECT numBoxRect{0, 0, 0, 0};
    std::wstring icon;
    int minVal = 0;
    int maxVal = 100;
    int currentVal = 50;
    SizeMode sizeMode = SizeMode::Default;

    bool isHovered = false;
    bool isDragging = false;
    bool isNumBoxHovered = false;
    bool isEditing = false;
    HWND hEdit = nullptr;
    UINT_PTR editControlId = 0;
    DWORD lastWheelTime = 0;

    std::function<void(int)> onValueChanged;

    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        auto* self = reinterpret_cast<FluentSlider*>(dwRefData);
        switch (uMsg) {
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                self->CommitEdit();
                return 0;
            } else if (wParam == VK_ESCAPE) {
                self->CancelEdit();
                return 0;
            }
            break;

        case WM_KILLFOCUS:
            self->CommitEdit();
            return 0;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void SetBounds(int x, int y, int width, int height, SizeMode mode) {
        bounds = {x, y, x + width, y + height};
        sizeMode = mode;
        
        int iconWidth = (mode == SizeMode::Sleek) ? 26 : ((mode == SizeMode::Large) ? 38 : 32);
        int numBoxWidth = (mode == SizeMode::Sleek) ? 40 : ((mode == SizeMode::Large) ? 54 : 46);
        int numBoxHeight = (mode == SizeMode::Sleek) ? 22 : ((mode == SizeMode::Large) ? 30 : 26);
        int trackHeight = (mode == SizeMode::Sleek) ? 3 : ((mode == SizeMode::Large) ? 6 : 4);
        int centerY = y + height / 2;

        iconRect = {x + 2, y, x + iconWidth, y + height};
        numBoxRect = {x + width - numBoxWidth - 4, centerY - numBoxHeight / 2, x + width - 4, centerY + numBoxHeight / 2};
        
        int trackLeft = x + iconWidth + 10;
        int trackRight = numBoxRect.left - 12;
        int trackTop = centerY - trackHeight / 2;
        trackRect = {trackLeft, trackTop, trackRight, trackTop + trackHeight};

        if (hEdit && IsWindow(hEdit)) {
            SetWindowPos(hEdit, nullptr, numBoxRect.left, numBoxRect.top, 
                numBoxRect.right - numBoxRect.left, numBoxRect.bottom - numBoxRect.top, SWP_NOZORDER);
        }
    }

    int ValueFromX(int x) const {
        if (trackRect.right <= trackRect.left) return minVal;
        int clampedX = (std::max)(static_cast<int>(trackRect.left), (std::min)(static_cast<int>(trackRect.right), x));
        double pct = static_cast<double>(clampedX - trackRect.left) / (trackRect.right - trackRect.left);
        return static_cast<int>(minVal + pct * (maxVal - minVal) + 0.5);
    }

    int XFromValue(int val) const {
        if (maxVal <= minVal) return trackRect.left;
        int clampedVal = (std::max)(minVal, (std::min)(maxVal, val));
        double pct = static_cast<double>(clampedVal - minVal) / (maxVal - minVal);
        return static_cast<int>(trackRect.left + pct * (trackRect.right - trackRect.left));
    }

    bool HitTest(int x, int y) const {
        return (x >= bounds.left && x <= bounds.right && y >= bounds.top && y <= bounds.bottom);
    }

    bool HitTestNumBox(int x, int y) const {
        return (x >= numBoxRect.left && x <= numBoxRect.right && y >= numBoxRect.top && y <= numBoxRect.bottom);
    }

    bool OnLButtonDown(int x, int y, HWND hParent, HFONT hFont) {
        if (HitTestNumBox(x, y)) {
            StartEdit(hParent, hFont);
            return true;
        }

        if (HitTest(x, y)) {
            if (isEditing) CommitEdit();
            isDragging = true;
            int newVal = ValueFromX(x);
            if (newVal != currentVal) {
                currentVal = newVal;
                if (onValueChanged) onValueChanged(currentVal);
            }
            return true;
        }
        return false;
    }

    bool OnMouseMove(int x, int y) {
        bool hit = HitTest(x, y);
        bool hitNum = HitTestNumBox(x, y);
        bool needRepaint = false;

        if (hit != isHovered) {
            isHovered = hit;
            needRepaint = true;
        }
        if (hitNum != isNumBoxHovered) {
            isNumBoxHovered = hitNum;
            needRepaint = true;
        }

        if (isDragging) {
            int newVal = ValueFromX(x);
            if (newVal != currentVal) {
                currentVal = newVal;
                if (onValueChanged) onValueChanged(currentVal);
            }
            needRepaint = true;
        }
        return needRepaint;
    }

    bool OnLButtonUp() {
        if (isDragging) {
            isDragging = false;
            return true;
        }
        return false;
    }

    // Velocity-sensitive mouse wheel scrolling
    bool OnMouseWheel(int x, int y, int zDelta) {
        if (HitTest(x, y)) {
            if (isEditing) CommitEdit();

            DWORD now = GetTickCount();
            DWORD dt = (lastWheelTime == 0) ? 999 : (now - lastWheelTime);
            lastWheelTime = now;

            int direction = (zDelta > 0) ? 1 : -1;
            int step = 1; // Fine control (1% per slow notch)

            if (dt < 35) {
                step = 6;  // Fast flick
            } else if (dt < 70) {
                step = 3;  // Brisk scroll
            } else if (dt < 120) {
                step = 2;  // Moderate scroll
            } else {
                step = 1;  // Single slow notch: 1 unit precision!
            }

            int newVal = (std::max)(minVal, (std::min)(maxVal, currentVal + direction * step));
            if (newVal != currentVal) {
                currentVal = newVal;
                if (onValueChanged) onValueChanged(currentVal);
                return true;
            }
        }
        return false;
    }

    void StartEdit(HWND hParent, HFONT hFont) {
        if (hEdit && IsWindow(hEdit)) return;
        isEditing = true;

        std::wstring strVal = std::to_wstring(currentVal);
        hEdit = CreateWindowExW(
            0, L"EDIT", strVal.c_str(),
            WS_CHILD | WS_VISIBLE | ES_CENTER | ES_NUMBER | WS_BORDER,
            numBoxRect.left, numBoxRect.top,
            numBoxRect.right - numBoxRect.left, numBoxRect.bottom - numBoxRect.top,
            hParent, (HMENU)editControlId, GetModuleHandle(nullptr), nullptr
        );

        if (hEdit) {
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetWindowSubclass(hEdit, EditSubclassProc, editControlId, reinterpret_cast<DWORD_PTR>(this));
            SendMessageW(hEdit, EM_SETSEL, 0, -1);
            SetFocus(hEdit);
        }
    }

    void CommitEdit() {
        if (!isEditing || !hEdit || !IsWindow(hEdit)) return;
        wchar_t buf[16] = {0};
        GetWindowTextW(hEdit, buf, 16);
        int val = _wtoi(buf);
        val = (std::max)(minVal, (std::min)(maxVal, val));

        HWND hTemp = hEdit;
        hEdit = nullptr;
        isEditing = false;
        RemoveWindowSubclass(hTemp, EditSubclassProc, editControlId);
        DestroyWindow(hTemp);

        if (val != currentVal) {
            currentVal = val;
            if (onValueChanged) onValueChanged(currentVal);
        }
    }

    void CancelEdit() {
        if (!isEditing || !hEdit || !IsWindow(hEdit)) return;
        HWND hTemp = hEdit;
        hEdit = nullptr;
        isEditing = false;
        RemoveWindowSubclass(hTemp, EditSubclassProc, editControlId);
        DestroyWindow(hTemp);
    }

    void Draw(HDC hdc, const Theme& theme, HFONT hFontIcon, HFONT hFontValue) const {
        SetBkMode(hdc, TRANSPARENT);

        // 1. Draw Icon
        SelectObject(hdc, hFontIcon);
        SetTextColor(hdc, isHovered ? theme.textPrimary : theme.textSecondary);
        RECT ic = iconRect;
        DrawTextW(hdc, icon.c_str(), -1, &ic, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 2. Draw Track background (unfilled)
        HBRUSH hBrushTrackBg = CreateSolidBrush(theme.isDark ? RGB(75, 75, 75) : RGB(215, 215, 215));
        HPEN hNullPen = (HPEN)GetStockObject(NULL_PEN);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hNullPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrushTrackBg);

        int radius = (trackRect.bottom - trackRect.top);
        RoundRect(hdc, trackRect.left, trackRect.top, trackRect.right, trackRect.bottom, radius, radius);

        // 3. Draw Active Track (filled with system accent color)
        int thumbX = XFromValue(currentVal);
        if (thumbX > trackRect.left) {
            HBRUSH hBrushAccent = CreateSolidBrush(theme.accent);
            SelectObject(hdc, hBrushAccent);
            RoundRect(hdc, trackRect.left, trackRect.top, thumbX, trackRect.bottom, radius, radius);
            DeleteObject(hBrushAccent);
        }

        // 4. Draw Thumb Knob (scaled by SizeMode)
        int baseRadius = (sizeMode == SizeMode::Sleek) ? 6 : ((sizeMode == SizeMode::Large) ? 10 : 8);
        int thumbRadius = isDragging ? (baseRadius + 2) : (isHovered ? (baseRadius + 1) : baseRadius);
        int centerY = (trackRect.top + trackRect.bottom) / 2;

        HBRUSH hThumbBrush = CreateSolidBrush(theme.accent);
        HPEN hThumbPen = CreatePen(PS_SOLID, 2, theme.isDark ? RGB(32, 32, 32) : RGB(255, 255, 255));
        SelectObject(hdc, hThumbBrush);
        SelectObject(hdc, hThumbPen);

        Ellipse(hdc, thumbX - thumbRadius, centerY - thumbRadius, thumbX + thumbRadius, centerY + thumbRadius);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrushTrackBg);
        DeleteObject(hThumbBrush);
        DeleteObject(hThumbPen);

        // 5. Draw Editable Number Box badge
        if (!isEditing) {
            COLORREF numBg = isNumBoxHovered 
                ? (theme.isDark ? RGB(54, 54, 54) : RGB(232, 232, 232))
                : (theme.isDark ? RGB(42, 42, 42) : RGB(245, 245, 245));
            COLORREF numBorder = isNumBoxHovered ? theme.accent : theme.border;

            HBRUSH hNumBg = CreateSolidBrush(numBg);
            HPEN hNumPen = CreatePen(PS_SOLID, 1, numBorder);
            HBRUSH oB = (HBRUSH)SelectObject(hdc, hNumBg);
            HPEN oP = (HPEN)SelectObject(hdc, hNumPen);

            RoundRect(hdc, numBoxRect.left, numBoxRect.top, numBoxRect.right, numBoxRect.bottom, 6, 6);

            SelectObject(hdc, oP);
            SelectObject(hdc, oB);
            DeleteObject(hNumBg);
            DeleteObject(hNumPen);

            SelectObject(hdc, hFontValue);
            SetTextColor(hdc, isNumBoxHovered ? theme.textPrimary : theme.textSecondary);
            std::wstring valStr = std::to_wstring(currentVal);
            RECT vr = numBoxRect;
            DrawTextW(hdc, valStr.c_str(), -1, &vr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
};
