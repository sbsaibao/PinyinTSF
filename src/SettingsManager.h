#pragma once

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <windows.h>
#include <cmath>

#define PINYINTSF_REG_PATH  L"Software\\PinyinTSF"
#define WM_PINYINTSF_SETTINGS_CHANGED  RegisterWindowMessageW(L"PinyinTSF_SettingsChanged")

// Base layout values from the Figma "候选框浅色" design (scale = 1.0)
#define BASE_WINDOW_HEIGHT      72.0f
#define BASE_MIN_WIDTH          480.0f
#define BASE_SHADOW_PADDING     20.0f
#define BASE_WINDOW_RADIUS      8.0f
#define BASE_CONTAINER_TOP      0.0f
#define BASE_CONTAINER_HEIGHT   72.0f
#define BASE_PADDING_LEFT       4.0f
#define BASE_ITEM_WIDTH         94.4f
#define BASE_ITEM_HEIGHT        36.0f
#define BASE_ITEM_RADIUS        8.0f
#define BASE_ITEM_Y             32.0f
#define BASE_NUMBER_WIDTH       12.0f
#define BASE_NUMBER_TEXT_GAP    12.0f
#define BASE_TITLE_LEFT         12.0f
#define BASE_TITLE_Y            8.0f
#define BASE_DIVIDER_LEFT       12.0f
#define BASE_DIVIDER_RIGHT      12.0f
#define BASE_DIVIDER_Y          28.0f

#define DEFAULT_SCALE           1.0f
#define DEFAULT_FONT_SIZE       13
#define DEFAULT_INSERT_SYLLABLE_SPACES true
#define MIN_SCALE               0.5f
#define MAX_SCALE               3.0f
#define MIN_FONT_SIZE           10
#define MAX_FONT_SIZE           28

class SettingsManager {
public:
    enum class ThemeMode : DWORD {
        FollowSystem = 0,
        Light = 1,
        Dark = 2,
    };

    static SettingsManager& Instance();

    void Load();
    void Save();

    float GetScale() const { return _scale; }
    void  SetScale(float s);
    int   GetFontSize() const { return _fontSize; }
    void  SetFontSize(int fs);
    ThemeMode GetThemeMode() const { return _themeMode; }
    void SetThemeMode(ThemeMode mode) { _themeMode = NormalizeThemeModeValue((DWORD)mode); }
    bool GetInsertSyllableSpaces() const { return _insertSyllableSpaces; }
    void SetInsertSyllableSpaces(bool enabled) { _insertSyllableSpaces = enabled; }

    static ThemeMode NormalizeThemeModeValue(DWORD value) {
        switch (value) {
        case (DWORD)ThemeMode::FollowSystem:
            return ThemeMode::FollowSystem;
        case (DWORD)ThemeMode::Light:
            return ThemeMode::Light;
        case (DWORD)ThemeMode::Dark:
            return ThemeMode::Dark;
        default:
            return ThemeMode::FollowSystem;
        }
    }

    static bool ResolveEffectiveDarkMode(ThemeMode mode, bool systemDark) {
        switch (mode) {
        case ThemeMode::Light:
            return false;
        case ThemeMode::Dark:
            return true;
        case ThemeMode::FollowSystem:
        default:
            return systemDark;
        }
    }

    static bool NormalizeInsertSyllableSpacesValue(DWORD value) {
        return value != 0;
    }

    // Computed layout values
    int WindowHeight()    const { return _round(BASE_WINDOW_HEIGHT * _scale); }
    int MinWidth()        const { return _round(BASE_MIN_WIDTH * _scale); }
    int ShadowPadding()   const { return _round(BASE_SHADOW_PADDING * _scale); }
    int OuterHeight()     const { return WindowHeight() + ShadowPadding() * 2; }
    int OuterMinWidth()   const { return MinWidth() + ShadowPadding() * 2; }
    int WindowRadius()    const { return _round(BASE_WINDOW_RADIUS * _scale); }
    int ContainerTop()    const { return _round(BASE_CONTAINER_TOP * _scale); }
    int ContainerHeight() const { return _round(BASE_CONTAINER_HEIGHT * _scale); }
    int PaddingLeft()     const { return _round(BASE_PADDING_LEFT * _scale); }
    int ItemWidth()       const { return _round(BASE_ITEM_WIDTH * _scale); }
    int ItemHeight()      const { return _round(BASE_ITEM_HEIGHT * _scale); }
    int ItemRadius()      const { return _round(BASE_ITEM_RADIUS * _scale); }
    int ItemY()           const { return _round(BASE_ITEM_Y * _scale); }
    int NumberWidth()     const { return _round(BASE_NUMBER_WIDTH * _scale); }
    int NumberTextGap()   const { return _round(BASE_NUMBER_TEXT_GAP * _scale); }
    int TitleLeft()       const { return _round(BASE_TITLE_LEFT * _scale); }
    int TitleY()          const { return _round(BASE_TITLE_Y * _scale); }
    int DividerLeft()     const { return _round(BASE_DIVIDER_LEFT * _scale); }
    int DividerRight()    const { return _round(BASE_DIVIDER_RIGHT * _scale); }
    int DividerY()        const { return _round(BASE_DIVIDER_Y * _scale); }
    int BadgeFontSize()   const { return _fontSize; }
    int CandidateFontSize() const { return _fontSize; }

private:
    SettingsManager();
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    static int _round(float v) { return (int)(v + 0.5f); }

    float _scale;
    int   _fontSize;
    ThemeMode _themeMode;
    bool _insertSyllableSpaces;
};

#endif // SETTINGS_MANAGER_H
