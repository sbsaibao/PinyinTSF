#include "CandidateTheme.h"

namespace {
    const wchar_t* kPersonalizeRegPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    const wchar_t* kAppsUseLightThemeValue = L"AppsUseLightTheme";
}

namespace CandidateTheme {

bool IsWindowsAppDarkTheme() {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kPersonalizeRegPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 1;
    DWORD size = sizeof(DWORD);
    DWORD type = 0;
    bool isDark = false;

    if (RegQueryValueExW(hKey, kAppsUseLightThemeValue, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS
        && type == REG_DWORD) {
        isDark = (value == 0);
    }

    RegCloseKey(hKey);
    return isDark;
}

bool ResolveDarkMode(SettingsManager::ThemeMode mode) {
    return SettingsManager::ResolveEffectiveDarkMode(mode, IsWindowsAppDarkTheme());
}

CandidateThemePalette GetPalette(bool darkMode) {
    CandidateThemePalette palette = {};

    if (darkMode) {
        palette.windowBackground = RGB(24, 24, 27);
        palette.containerBackground = RGB(32, 32, 36);
        palette.containerBorder = RGB(56, 56, 62);
        palette.divider = RGB(56, 56, 62);
        palette.selectedBackground = RGB(38, 78, 120);
        palette.selectedBorder = RGB(38, 78, 120);
        palette.textPrimary = RGB(244, 244, 245);
        palette.textSecondary = RGB(161, 161, 170);
        palette.shadowColor = RGB(0, 0, 0);
    } else {
        palette.windowBackground = RGB(255, 255, 255);
        palette.containerBackground = RGB(255, 255, 255);
        palette.containerBorder = RGB(235, 235, 235);
        palette.divider = RGB(235, 235, 235);
        palette.selectedBackground = RGB(232, 244, 255);
        palette.selectedBorder = RGB(232, 244, 255);
        palette.textPrimary = RGB(26, 26, 26);
        palette.textSecondary = RGB(153, 153, 153);
        palette.shadowColor = RGB(0, 0, 0);
    }

    return palette;
}

COLORREF BlendColor(COLORREF color, COLORREF background, int alpha) {
    const int invAlpha = 255 - alpha;
    const int r = (GetRValue(color) * alpha + GetRValue(background) * invAlpha + 127) / 255;
    const int g = (GetGValue(color) * alpha + GetGValue(background) * invAlpha + 127) / 255;
    const int b = (GetBValue(color) * alpha + GetBValue(background) * invAlpha + 127) / 255;
    return RGB(r, g, b);
}

}
