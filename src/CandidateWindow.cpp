#include "CandidateWindow.h"
#include <stdio.h>

namespace {
    void DrawShadowRoundRect(HDC hdc, const RECT& rc, int radius, int offsetY,
                             int blurRadius, int maxAlpha, const CandidateThemePalette& palette) {
        for (int i = blurRadius; i >= 1; --i) {
            const int alpha = (maxAlpha * (blurRadius - i + 1)) / (blurRadius * blurRadius);
            COLORREF shadow = CandidateTheme::BlendColor(palette.shadowColor, palette.windowBackground, alpha);
            HPEN hPen = CreatePen(PS_SOLID, 1, shadow);
            HBRUSH hBrush = CreateSolidBrush(shadow);
            HGDIOBJ hOldPen = SelectObject(hdc, hPen);
            HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

            RoundRect(hdc,
                      rc.left - i,
                      rc.top + offsetY - i,
                      rc.right + i,
                      rc.bottom + offsetY + i,
                      (radius + i) * 2,
                      (radius + i) * 2);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrush);
            DeleteObject(hPen);
        }
    }
}

// ================================================================
// Static members
// ================================================================
const wchar_t* CCandidateWindow::_className = L"PinyinTSFCandidateWindow";
BOOL CCandidateWindow::_classRegistered = FALSE;
const wchar_t* CCandidateWindow::_chineseNumbers[] = {
    L"\x4E00",  // one
    L"\x4E8C",  // two
    L"\x4E09",  // three
    L"\x56DB",  // four
    L"\x4E94",  // five
    L"\x516D",  // six
    L"\x4E03",  // seven
    L"\x516B",  // eight
    L"\x4E5D",  // nine
};

// ================================================================
// Constructor / Destructor
// ================================================================

CCandidateWindow::CCandidateWindow()
    : _hwnd(nullptr)
    , _hInst(nullptr)
    , _hFontBadge(nullptr)
    , _hFontTitle(nullptr)
    , _hFontCandidate(nullptr)
    , _hFontCandidateSelected(nullptr)
    , _hContainerBorderPen(nullptr)
    , _hDividerPen(nullptr)
    , _hSelectedBorderPen(nullptr)
    , _hBgBrush(nullptr)
    , _hContainerBrush(nullptr)
    , _hSelectedBrush(nullptr)
    , _darkMode(false)
    , _selectedIndex(0)
    , _width(SettingsManager::Instance().MinWidth())
    , _height(SettingsManager::Instance().WindowHeight())
{
}

CCandidateWindow::~CCandidateWindow() {
    Destroy();
}

// ================================================================
// Create window and fonts
// ================================================================
BOOL CCandidateWindow::Create(HINSTANCE hInst) {
    _hInst = hInst;

    // Register window class (once)
    if (!_classRegistered) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = _WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = _className;

        if (!RegisterClassExW(&wc)) return FALSE;
        _classRegistered = TRUE;
    }

    // Create popup window
    _hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        _className,
        L"",
        WS_POPUP,
        0, 0, _width, _height,
        nullptr, nullptr, hInst, this
    );

    if (!_hwnd) return FALSE;

    // Create fonts from current settings
    _CreateFonts();
    _RefreshThemeResources();

    return TRUE;
}

// ================================================================
// Destroy window and free resources
// ================================================================
void CCandidateWindow::Destroy() {
    if (_hFontBadge) { DeleteObject(_hFontBadge); _hFontBadge = nullptr; }
    if (_hFontTitle) { DeleteObject(_hFontTitle); _hFontTitle = nullptr; }
    if (_hFontCandidate) { DeleteObject(_hFontCandidate); _hFontCandidate = nullptr; }
    if (_hFontCandidateSelected) { DeleteObject(_hFontCandidateSelected); _hFontCandidateSelected = nullptr; }
    _ReleaseThemeResources();
    if (_hwnd) { DestroyWindow(_hwnd); _hwnd = nullptr; }
}

// ================================================================
// Show / Hide / Visibility
// ================================================================

void CCandidateWindow::Show(POINT pt) {
    if (!_hwnd) return;

    _RefreshThemeResources();
    _CalculateWindowSize();
    _UpdateWindowRgn();

    SettingsManager& sm = SettingsManager::Instance();
    int shadowPad = _GetShadowPadding();
    pt.x -= shadowPad;
    pt.y -= shadowPad;
    _AdjustWindowPosition(pt);

    SetWindowPos(_hwnd, HWND_TOPMOST, pt.x, pt.y, _width, _height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    _Invalidate();
}

void CCandidateWindow::Hide() {
    if (_hwnd) {
        ShowWindow(_hwnd, SW_HIDE);
    }
}

BOOL CCandidateWindow::IsVisible() {
    return _hwnd && IsWindowVisible(_hwnd);
}

// ================================================================
// Data setters
// ================================================================

void CCandidateWindow::SetPinyinText(const std::wstring& pinyin) {
    _pinyinText = pinyin;
    _Invalidate();
}

void CCandidateWindow::SetCandidates(const std::vector<std::wstring>& candidates) {
    _candidates = candidates;
    _selectedIndex = 0;
    _Invalidate();
}

void CCandidateWindow::SetSelection(int index) {
    if (index >= 0 && index < (int)_candidates.size()) {
        _selectedIndex = index;
        _Invalidate();
    }
}

int CCandidateWindow::GetSelection() {
    return _selectedIndex;
}

int CCandidateWindow::GetCandidateCount() {
    return (int)_candidates.size();
}

std::wstring CCandidateWindow::GetSelectedText() {
    if (_selectedIndex >= 0 && _selectedIndex < (int)_candidates.size()) {
        return _candidates[_selectedIndex];
    }
    return L"";
}

// ================================================================
// Navigation
// ================================================================

void CCandidateWindow::MoveSelectionNext() {
    if (_candidates.empty()) return;
    _selectedIndex = (_selectedIndex + 1) % (int)_candidates.size();
    _Invalidate();
}

void CCandidateWindow::MoveSelectionPrev() {
    if (_candidates.empty()) return;
    _selectedIndex = (_selectedIndex - 1 + (int)_candidates.size()) % (int)_candidates.size();
    _Invalidate();
}

// ================================================================
// Invalidate (request repaint)
// ================================================================

void CCandidateWindow::_Invalidate() {
    if (_hwnd && IsWindowVisible(_hwnd)) {
        InvalidateRect(_hwnd, nullptr, TRUE);
    }
}

// ================================================================
// Window Procedure (static)
// ================================================================

LRESULT CALLBACK CCandidateWindow::_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CCandidateWindow* pThis = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (CCandidateWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (CCandidateWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (msg) {
        case WM_PAINT: {
            if (pThis) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                pThis->_OnPaint(hdc);
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;  // Prevent flicker
    }

    if (msg == WM_PINYINTSF_SETTINGS_CHANGED) {
        if (pThis) pThis->ReloadSettings();
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ================================================================
// Paint - double buffered
// ================================================================

void CCandidateWindow::_OnPaint(HDC hdc) {
    RECT rcClient;
    GetClientRect(_hwnd, &rcClient);
    int w = rcClient.right;
    int h = rcClient.bottom;

    SettingsManager& sm = SettingsManager::Instance();
    const int shadowPad = _GetShadowPadding();
    const int contentW = w - shadowPad * 2;

    // Create memory DC for double buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hBmp);

    // Layer 1: fill entire window with white background
    HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, _hBgBrush);
    HPEN hOldPen = (HPEN)SelectObject(memDC, (HPEN)GetStockObject(NULL_PEN));
    RECT rcFull = { 0, 0, w, h };
    FillRect(memDC, &rcFull, _hBgBrush);

    RECT rcContainer = {
        shadowPad,
        shadowPad + sm.ContainerTop(),
        shadowPad + contentW,
        shadowPad + sm.ContainerTop() + sm.ContainerHeight()
    };
    if (shadowPad > 0) {
        DrawShadowRoundRect(memDC, rcContainer, sm.WindowRadius(), 2, 12, 20, _palette);
        DrawShadowRoundRect(memDC, rcContainer, sm.WindowRadius(), 6, 20, 10, _palette);
    }

    // Layer 3: draw rounded container
    SelectObject(memDC, _hContainerBrush);
    SelectObject(memDC, _hContainerBorderPen);
    RoundRect(memDC, rcContainer.left, rcContainer.top, rcContainer.right, rcContainer.bottom,
              sm.WindowRadius() * 2, sm.WindowRadius() * 2);
    SelectObject(memDC, hOldPen);
    SelectObject(memDC, hOldBrush);

    _DrawPinyinText(memDC, shadowPad + sm.TitleLeft(), shadowPad + sm.TitleY());
    _DrawDivider(memDC,
                 shadowPad + sm.DividerLeft(),
                 shadowPad + sm.DividerY(),
                 contentW - sm.DividerLeft() - sm.DividerRight());

    // Draw candidates
    _DrawCandidates(memDC, shadowPad + sm.PaddingLeft(), shadowPad + sm.ItemY());

    // Blit to screen
    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(memDC, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
}

// ================================================================
// Draw pinyin text above the candidate row
// ================================================================
void CCandidateWindow::_DrawPinyinText(HDC hdc, int x, int y) {
    if (_pinyinText.empty()) return;

    HFONT hOldFont = (HFONT)SelectObject(hdc, _hFontTitle);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, _palette.textPrimary);
    TextOutW(hdc, x, y, _pinyinText.c_str(), (int)_pinyinText.size());
    SelectObject(hdc, hOldFont);
}

// ================================================================
// Draw the separator between pinyin text and candidates
// ================================================================
void CCandidateWindow::_DrawDivider(HDC hdc, int x, int y, int width) {
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hDividerPen);
    MoveToEx(hdc, x, y, nullptr);
    LineTo(hdc, x + width, y);
    SelectObject(hdc, hOldPen);
}

// ================================================================
// Draw an item background. Only the selected item is highlighted in the
// light Figma design; unselected items sit directly on the white container.
// ================================================================
void CCandidateWindow::_DrawItemBackground(HDC hdc, int x, int y, bool selected) {
    if (!selected) return;

    SettingsManager& sm = SettingsManager::Instance();
    int iw = sm.ItemWidth(), ih = sm.ItemHeight(), ir = sm.ItemRadius();

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, _hSelectedBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, _hSelectedBorderPen);
    RoundRect(hdc, x, y, x + iw, y + ih, ir * 2, ir * 2);
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
}

// ================================================================
// Draw Chinese number index
// ================================================================
void CCandidateWindow::_DrawIndex(HDC hdc, int x, int y, int index) {
    SettingsManager& sm = SettingsManager::Instance();
    int ih = sm.ItemHeight();
    int numberW = sm.NumberWidth();

    HFONT hOldFont = (HFONT)SelectObject(hdc, _hFontBadge);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, _palette.textSecondary);

    const wchar_t* numText = nullptr;
    int numLen = 0;
    wchar_t decText[8];

    if (index < 9) {
        numText = _chineseNumbers[index];
        numLen = 1;
    } else {
        swprintf_s(decText, 8, L"%d", index + 1);
        numText = decText;
        numLen = (int)wcslen(decText);
    }

    SIZE textSize;
    GetTextExtentPoint32W(hdc, numText, numLen, &textSize);
    int textX = x + (numberW - textSize.cx) / 2;
    int textY = y + (ih - textSize.cy) / 2;
    TextOutW(hdc, textX, textY, numText, numLen);

    SelectObject(hdc, hOldFont);
}

// ================================================================
// Draw candidate items with badges
// ================================================================
void CCandidateWindow::_DrawCandidates(HDC hdc, int x, int y) {
    if (_candidates.empty()) return;

    SettingsManager& sm = SettingsManager::Instance();
    int itemW = sm.ItemWidth(), itemH = sm.ItemHeight();
    int numberW = sm.NumberWidth(), gap = sm.NumberTextGap();
    SetBkMode(hdc, TRANSPARENT);

    int currentX = x;

    for (int i = 0; i < (int)_candidates.size(); i++) {
        _DrawItemBackground(hdc, currentX, y, i == _selectedIndex);
        _DrawIndex(hdc, currentX + gap, y, i);

        HFONT hTextFont = (i == _selectedIndex) ? _hFontCandidateSelected : _hFontCandidate;
        HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);
        const std::wstring& text = _candidates[i];
        SetTextColor(hdc, _palette.textPrimary);

        SIZE textSize;
        GetTextExtentPoint32W(hdc, text.c_str(), (int)text.size(), &textSize);
        int textX = currentX + gap + numberW + gap;
        int textY = y + (itemH - textSize.cy) / 2;
        TextOutW(hdc, textX, textY, text.c_str(), (int)text.size());
        SelectObject(hdc, hOldFont);

        currentX += itemW;
    }
}

// ================================================================
// Calculate window size based on content
// ================================================================
void CCandidateWindow::_CalculateWindowSize() {
    if (!_hwnd) return;

    SettingsManager& sm = SettingsManager::Instance();
    int count = (int)_candidates.size();
    if (count <= 0) count = 1;
    int totalWidth = sm.PaddingLeft() * 2 + sm.ItemWidth() * count;

    int contentWidth = (totalWidth > sm.MinWidth()) ? totalWidth : sm.MinWidth();
    int shadowPad = _GetShadowPadding();
    _width = contentWidth + shadowPad * 2;
    _height = sm.WindowHeight() + shadowPad * 2;
}

// ================================================================
// Update window region for rounded corners
// ================================================================
void CCandidateWindow::_UpdateWindowRgn() {
    if (!_hwnd) return;
    SettingsManager& sm = SettingsManager::Instance();
    int shadowPad = _GetShadowPadding();
    HRGN hRgn = CreateRoundRectRgn(0, 0, _width, _height,
                                    (sm.WindowRadius() + shadowPad) * 2,
                                    (sm.WindowRadius() + shadowPad) * 2);
    SetWindowRgn(_hwnd, hRgn, TRUE);
}

// ================================================================
// Adjust position to stay within screen bounds
// ================================================================
void CCandidateWindow::_AdjustWindowPosition(POINT& pt) {
    RECT rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);

    // Right edge overflow -> shift left
    if (pt.x + _width > rcWork.right)
        pt.x = rcWork.right - _width;

    // Bottom overflow -> show above cursor
    if (pt.y + _height > rcWork.bottom)
        pt.y = pt.y - _height - 30;

    // Left edge
    if (pt.x < rcWork.left)
        pt.x = rcWork.left;

    // Top edge
    if (pt.y < rcWork.top)
        pt.y = rcWork.top;
}

// ================================================================
// Create fonts from current settings
// ================================================================
void CCandidateWindow::_CreateFonts() {
    SettingsManager& sm = SettingsManager::Instance();

    if (_hFontBadge) DeleteObject(_hFontBadge);
    if (_hFontTitle) DeleteObject(_hFontTitle);
    if (_hFontCandidate) DeleteObject(_hFontCandidate);
    if (_hFontCandidateSelected) DeleteObject(_hFontCandidateSelected);

    _hFontBadge = CreateFontW(
        -sm.BadgeFontSize(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei"
    );

    _hFontTitle = CreateFontW(
        -sm.CandidateFontSize(), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei"
    );

    _hFontCandidate = CreateFontW(
        -sm.CandidateFontSize(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei"
    );

    _hFontCandidateSelected = CreateFontW(
        -sm.CandidateFontSize(), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei"
    );
}

// ================================================================
// Reload settings (called when settings change via tray icon)
// ================================================================
void CCandidateWindow::ReloadSettings() {
    SettingsManager::Instance().Load();
    _CreateFonts();
    _RefreshThemeResources();

    if (_hwnd && IsWindowVisible(_hwnd)) {
        _CalculateWindowSize();
        _UpdateWindowRgn();
        SetWindowPos(_hwnd, nullptr, 0, 0, _width, _height,
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        _Invalidate();
    }
}

void CCandidateWindow::_RefreshThemeResources() {
    SettingsManager& sm = SettingsManager::Instance();
    _darkMode = CandidateTheme::ResolveDarkMode(sm.GetThemeMode());
    _palette = CandidateTheme::GetPalette(_darkMode);

    _ReleaseThemeResources();

    _hContainerBorderPen = CreatePen(PS_SOLID, 1, _palette.containerBorder);
    _hDividerPen = CreatePen(PS_SOLID, 1, _palette.divider);
    _hSelectedBorderPen = CreatePen(PS_SOLID, 1, _palette.selectedBorder);
    _hBgBrush = CreateSolidBrush(_palette.windowBackground);
    _hContainerBrush = CreateSolidBrush(_palette.containerBackground);
    _hSelectedBrush = CreateSolidBrush(_palette.selectedBackground);
}

void CCandidateWindow::_ReleaseThemeResources() {
    if (_hContainerBorderPen) { DeleteObject(_hContainerBorderPen); _hContainerBorderPen = nullptr; }
    if (_hDividerPen) { DeleteObject(_hDividerPen); _hDividerPen = nullptr; }
    if (_hSelectedBorderPen) { DeleteObject(_hSelectedBorderPen); _hSelectedBorderPen = nullptr; }
    if (_hBgBrush) { DeleteObject(_hBgBrush); _hBgBrush = nullptr; }
    if (_hContainerBrush) { DeleteObject(_hContainerBrush); _hContainerBrush = nullptr; }
    if (_hSelectedBrush) { DeleteObject(_hSelectedBrush); _hSelectedBrush = nullptr; }
}

int CCandidateWindow::_GetShadowPadding() const {
    return _darkMode ? 0 : SettingsManager::Instance().ShadowPadding();
}
