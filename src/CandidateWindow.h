#pragma once

#ifndef CANDIDATE_WINDOW_H
#define CANDIDATE_WINDOW_H

#include <windows.h>
#include <string>
#include <vector>
#include "CandidateTheme.h"
#include "SettingsManager.h"

// ================================================================
// CCandidateWindow - Win32 popup candidate window
// ================================================================
class CCandidateWindow {
public:
    CCandidateWindow();
    ~CCandidateWindow();

    // Lifecycle
    BOOL Create(HINSTANCE hInst);
    void Destroy();

    // Show/hide
    void Show(POINT pt);
    void Hide();
    BOOL IsVisible();

    // Data
    void SetPinyinText(const std::wstring& pinyin);
    void SetCandidates(const std::vector<std::wstring>& candidates);
    void SetSelection(int index);
    int  GetSelection();
    int  GetCandidateCount();
    std::wstring GetSelectedText();

    // Navigation
    void MoveSelectionNext();
    void MoveSelectionPrev();

    // Settings
    void ReloadSettings();

private:
    // Window procedure
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Drawing
    void _OnPaint(HDC hdc);
    void _DrawPinyinText(HDC hdc, int x, int y);
    void _DrawDivider(HDC hdc, int x, int y, int width);
    void _DrawItemBackground(HDC hdc, int x, int y, bool selected);
    void _DrawIndex(HDC hdc, int x, int y, int index);
    void _DrawCandidates(HDC hdc, int x, int y);

    // Layout
    void _CalculateWindowSize();
    void _AdjustWindowPosition(POINT& pt);
    void _Invalidate();
    void _UpdateWindowRgn();
    void _CreateFonts();
    void _RefreshThemeResources();
    void _ReleaseThemeResources();
    int  _GetShadowPadding() const;

    // Members
    HWND        _hwnd;
    HINSTANCE   _hInst;
    HFONT       _hFontBadge;
    HFONT       _hFontTitle;
    HFONT       _hFontCandidate;
    HFONT       _hFontCandidateSelected;

    // Cached GDI objects
    HPEN        _hContainerBorderPen;
    HPEN        _hDividerPen;
    HPEN        _hSelectedBorderPen;
    HBRUSH      _hBgBrush;
    HBRUSH      _hContainerBrush;
    HBRUSH      _hSelectedBrush;
    CandidateThemePalette _palette;
    bool        _darkMode;

    std::wstring                _pinyinText;
    std::vector<std::wstring>   _candidates;
    int                         _selectedIndex;

    int         _width;
    int         _height;

    static const wchar_t* _className;
    static BOOL _classRegistered;
    static const wchar_t* _chineseNumbers[];
};

#endif // CANDIDATE_WINDOW_H
