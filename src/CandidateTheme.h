#pragma once

#ifndef CANDIDATE_THEME_H
#define CANDIDATE_THEME_H

#include <windows.h>
#include "SettingsManager.h"

struct CandidateThemePalette {
    COLORREF windowBackground;
    COLORREF containerBackground;
    COLORREF containerBorder;
    COLORREF divider;
    COLORREF selectedBackground;
    COLORREF selectedBorder;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF shadowColor;
};

namespace CandidateTheme {
    bool IsWindowsAppDarkTheme();
    bool ResolveDarkMode(SettingsManager::ThemeMode mode);
    CandidateThemePalette GetPalette(bool darkMode);
    COLORREF BlendColor(COLORREF color, COLORREF background, int alpha);
}

#endif // CANDIDATE_THEME_H
