#include "SettingsManager.h"

SettingsManager::SettingsManager()
    : _scale(DEFAULT_SCALE)
    , _fontSize(DEFAULT_FONT_SIZE)
    , _themeMode(ThemeMode::FollowSystem)
{
}

SettingsManager& SettingsManager::Instance() {
    static SettingsManager inst;
    return inst;
}

void SettingsManager::SetScale(float s) {
    if (s < MIN_SCALE) s = MIN_SCALE;
    if (s > MAX_SCALE) s = MAX_SCALE;
    _scale = s;
}

void SettingsManager::SetFontSize(int fs) {
    if (fs < MIN_FONT_SIZE) fs = MIN_FONT_SIZE;
    if (fs > MAX_FONT_SIZE) fs = MAX_FONT_SIZE;
    _fontSize = fs;
}

void SettingsManager::Load() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, PINYINTSF_REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwVal = 0;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = 0;

        if (RegQueryValueExW(hKey, L"Scale", nullptr, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS
            && dwType == REG_DWORD) {
            float s = dwVal / 1000.0f;
            if (s >= MIN_SCALE && s <= MAX_SCALE) _scale = s;
        }

        dwSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"FontSize", nullptr, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS
            && dwType == REG_DWORD) {
            if ((int)dwVal >= MIN_FONT_SIZE && (int)dwVal <= MAX_FONT_SIZE) _fontSize = (int)dwVal;
        }

        dwSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ThemeMode", nullptr, &dwType, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS
            && dwType == REG_DWORD) {
            _themeMode = NormalizeThemeModeValue(dwVal);
        }

        RegCloseKey(hKey);
    }
}

void SettingsManager::Save() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, PINYINTSF_REG_PATH, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD dwScale = (DWORD)(_scale * 1000.0f + 0.5f);
        RegSetValueExW(hKey, L"Scale", 0, REG_DWORD, (const BYTE*)&dwScale, sizeof(DWORD));

        DWORD dwFont = (DWORD)_fontSize;
        RegSetValueExW(hKey, L"FontSize", 0, REG_DWORD, (const BYTE*)&dwFont, sizeof(DWORD));

        DWORD dwThemeMode = (DWORD)_themeMode;
        RegSetValueExW(hKey, L"ThemeMode", 0, REG_DWORD, (const BYTE*)&dwThemeMode, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}
