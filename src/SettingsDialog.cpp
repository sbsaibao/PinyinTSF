#include "SettingsDialog.h"
#include "CandidateTheme.h"
#include "SettingsManager.h"
#include "resource.h"
#include <commctrl.h>
#include <stdio.h>

#pragma comment(lib, "comctl32.lib")

HWND SettingsDialog::_hDlgOpen = nullptr;

namespace {
    SettingsManager::ThemeMode GetDialogThemeMode(HWND hDlg) {
        if (IsDlgButtonChecked(hDlg, IDC_THEME_LIGHT) == BST_CHECKED) {
            return SettingsManager::ThemeMode::Light;
        }
        if (IsDlgButtonChecked(hDlg, IDC_THEME_DARK) == BST_CHECKED) {
            return SettingsManager::ThemeMode::Dark;
        }
        return SettingsManager::ThemeMode::FollowSystem;
    }

    void SetDialogThemeMode(HWND hDlg, SettingsManager::ThemeMode mode) {
        int controlId = IDC_THEME_SYSTEM;
        if (mode == SettingsManager::ThemeMode::Light) {
            controlId = IDC_THEME_LIGHT;
        } else if (mode == SettingsManager::ThemeMode::Dark) {
            controlId = IDC_THEME_DARK;
        }
        CheckRadioButton(hDlg, IDC_THEME_SYSTEM, IDC_THEME_DARK, controlId);
    }
}

void SettingsDialog::Show(HINSTANCE hInst, HWND hwndParent) {
    // Only one settings dialog at a time
    if (_hDlgOpen && IsWindow(_hDlgOpen)) {
        SetForegroundWindow(_hDlgOpen);
        return;
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_SETTINGS), hwndParent,
                    _DlgProc, 0);
}

void SettingsDialog::_UpdateLabels(HWND hDlg) {
    SettingsManager& sm = SettingsManager::Instance();

    int scalePos = (int)SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_GETPOS, 0, 0);
    float scale = scalePos / 100.0f;

    int fontPos = (int)SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_GETPOS, 0, 0);

    wchar_t buf[32];
    swprintf_s(buf, 32, L"%.1fx", scale);
    SetDlgItemTextW(hDlg, IDC_SCALE_LABEL, buf);

    swprintf_s(buf, 32, L"%dpx", fontPos);
    SetDlgItemTextW(hDlg, IDC_FONT_LABEL, buf);
}

void SettingsDialog::_DrawPreview(HWND hDlg, DRAWITEMSTRUCT* dis) {
    int scalePos = (int)SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_GETPOS, 0, 0);
    float scale = scalePos / 100.0f;
    int fontPos = (int)SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_GETPOS, 0, 0);
    bool darkMode = CandidateTheme::ResolveDarkMode(GetDialogThemeMode(hDlg));
    CandidateThemePalette palette = CandidateTheme::GetPalette(darkMode);

    int barHeight = (int)(BASE_WINDOW_HEIGHT * scale + 0.5f);

    RECT rcPreview = dis->rcItem;
    int pw = rcPreview.right - rcPreview.left;
    int ph = rcPreview.bottom - rcPreview.top;

    // Fit scale to preview area
    float fitScale = 1.0f;
    if (barHeight > ph - 4) fitScale = (ph - 4.0f) / barHeight;
    int drawBarH = (int)(barHeight * fitScale + 0.5f);
    int offsetY = rcPreview.top + (ph - drawBarH) / 2;
    int offsetX = rcPreview.left + 10;

    HDC hdc = dis->hDC;
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBmp = CreateCompatibleBitmap(hdc, pw, ph);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hBmp);

    // Background
    RECT rcBg = { 0, 0, pw, ph };
    FillRect(memDC, &rcBg, (HBRUSH)(COLOR_BTNFACE + 1));

    // Scale factor for drawing
    float ds = fitScale * scale;

    int dwBarH = (int)(BASE_WINDOW_HEIGHT * ds + 0.5f);
    int dwItemW = (int)(BASE_ITEM_WIDTH * ds + 0.5f);
    int dwItemH = (int)(BASE_ITEM_HEIGHT * ds + 0.5f);
    int dwItemR = max(1, (int)(BASE_ITEM_RADIUS * ds + 0.5f));
    int dwItemY = (int)(BASE_ITEM_Y * ds + 0.5f);
    int dwNumberW = (int)(BASE_NUMBER_WIDTH * ds + 0.5f);
    int dwGap = (int)(BASE_NUMBER_TEXT_GAP * ds + 0.5f);
    int dwContainerT = (int)(BASE_CONTAINER_TOP * ds + 0.5f);
    int dwContainerH = (int)(BASE_CONTAINER_HEIGHT * ds + 0.5f);
    int dwPadLeft = (int)(BASE_PADDING_LEFT * ds + 0.5f);
    int dwWindowR = max(1, (int)(BASE_WINDOW_RADIUS * ds + 0.5f));
    int dwTitleX = (int)(BASE_TITLE_LEFT * ds + 0.5f);
    int dwTitleY = (int)(BASE_TITLE_Y * ds + 0.5f);
    int dwDividerX = (int)(BASE_DIVIDER_LEFT * ds + 0.5f);
    int dwDividerY = (int)(BASE_DIVIDER_Y * ds + 0.5f);
    int dwDividerRight = (int)(BASE_DIVIDER_RIGHT * ds + 0.5f);

    int dx = offsetX - rcPreview.left;
    int dy = offsetY - rcPreview.top;

    // White background
    HBRUSH hBgBrush = CreateSolidBrush(palette.windowBackground);
    RECT rcBar = { dx, dy, dx + pw, dy + dwBarH };
    FillRect(memDC, &rcBar, hBgBrush);
    DeleteObject(hBgBrush);

    HBRUSH hContainerBrush = CreateSolidBrush(palette.containerBackground);
    HPEN hContainerPen = CreatePen(PS_SOLID, 1, palette.containerBorder);
    HPEN hOldPen = (HPEN)SelectObject(memDC, hContainerPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, hContainerBrush);
    RoundRect(memDC, dx, dy + dwContainerT, dx + pw, dy + dwContainerT + dwContainerH,
              dwWindowR * 2, dwWindowR * 2);
    SelectObject(memDC, hOldPen);
    SelectObject(memDC, hOldBrush);
    DeleteObject(hContainerPen);
    DeleteObject(hContainerBrush);

    HFONT hBadgeFont = CreateFontW(-max(8, (int)(fontPos * fitScale + 0.5f)),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    HFONT hTitleFont = CreateFontW(-max(8, (int)(fontPos * fitScale + 0.5f)),
        0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    HFONT hCandidateFont = CreateFontW(-max(8, (int)(fontPos * fitScale + 0.5f)),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    HFONT hCandidateSelectedFont = CreateFontW(-max(8, (int)(fontPos * fitScale + 0.5f)),
        0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");

    const wchar_t* nums[] = { L"\x4E00", L"\x4E8C", L"\x4E09", L"\x56DB", L"\x4E94" };
    const wchar_t* texts[] = { L"m\x0101", L"m\x00E1", L"m\x01CE", L"m\x00E0", L"ma" };

    HFONT hOldFont = (HFONT)SelectObject(memDC, hTitleFont);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, palette.textPrimary);
    TextOutW(memDC, dx + dwTitleX, dy + dwTitleY, L"ma", 2);
    SelectObject(memDC, hOldFont);

    HPEN hDividerPen = CreatePen(PS_SOLID, 1, palette.divider);
    hOldPen = (HPEN)SelectObject(memDC, hDividerPen);
    MoveToEx(memDC, dx + dwDividerX, dy + dwDividerY, nullptr);
    LineTo(memDC, dx + pw - dwDividerRight, dy + dwDividerY);
    SelectObject(memDC, hOldPen);
    DeleteObject(hDividerPen);

    int cx = dx + dwPadLeft;
    for (int i = 0; i < 5; i++) {
        int bx = cx;
        int by = dy + dwItemY;

        if (i == 0) {
            HBRUSH hSelectedBrush = CreateSolidBrush(palette.selectedBackground);
            HPEN hSelectedPen = CreatePen(PS_SOLID, 1, palette.selectedBorder);
            hOldBrush = (HBRUSH)SelectObject(memDC, hSelectedBrush);
            hOldPen = (HPEN)SelectObject(memDC, hSelectedPen);
            RoundRect(memDC, bx, by, bx + dwItemW, by + dwItemH,
                      dwItemR * 2, dwItemR * 2);
            SelectObject(memDC, hOldPen);
            SelectObject(memDC, hOldBrush);
            DeleteObject(hSelectedPen);
            DeleteObject(hSelectedBrush);
        }

        // Index text
        hOldFont = (HFONT)SelectObject(memDC, hBadgeFont);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, palette.textSecondary);
        SIZE numSz;
        GetTextExtentPoint32W(memDC, nums[i], 1, &numSz);
        TextOutW(memDC, bx + dwGap + (dwNumberW - numSz.cx) / 2,
                 by + (dwItemH - numSz.cy) / 2, nums[i], 1);
        SelectObject(memDC, hOldFont);

        // Candidate text
        hOldFont = (HFONT)SelectObject(memDC, (i == 0) ? hCandidateSelectedFont : hCandidateFont);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, palette.textPrimary);
        SIZE txtSz;
        GetTextExtentPoint32W(memDC, texts[i], (int)wcslen(texts[i]), &txtSz);
        TextOutW(memDC, bx + dwGap + dwNumberW + dwGap,
                 by + (dwItemH - txtSz.cy) / 2, texts[i], (int)wcslen(texts[i]));
        SelectObject(memDC, hOldFont);

        cx += dwItemW;
    }

    // Blit
    BitBlt(hdc, rcPreview.left, rcPreview.top, pw, ph, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    DeleteObject(hBadgeFont);
    DeleteObject(hTitleFont);
    DeleteObject(hCandidateFont);
    DeleteObject(hCandidateSelectedFont);
}

INT_PTR CALLBACK SettingsDialog::_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsManager& sm = SettingsManager::Instance();

    switch (msg) {
    case WM_INITDIALOG: {
        _hDlgOpen = hDlg;

        // Scale trackbar: 50 = 0.5x, 300 = 3.0x
        SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_SETRANGE, TRUE, MAKELONG(50, 300));
        SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_SETPOS, TRUE,
                            (LPARAM)(int)(sm.GetScale() * 100.0f + 0.5f));

        // Font trackbar: 10-28
        SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_SETRANGE, TRUE, MAKELONG(MIN_FONT_SIZE, MAX_FONT_SIZE));
        SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_SETPOS, TRUE, (LPARAM)sm.GetFontSize());
        SetDialogThemeMode(hDlg, sm.GetThemeMode());
        CheckDlgButton(hDlg, IDC_INSERT_SPACES, sm.GetInsertSyllableSpaces() ? BST_CHECKED : BST_UNCHECKED);

        _UpdateLabels(hDlg);
        return TRUE;
    }

    case WM_HSCROLL:
        if (LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_THUMBPOSITION
            || LOWORD(wParam) == TB_LINEUP || LOWORD(wParam) == TB_LINEDOWN
            || LOWORD(wParam) == TB_PAGEUP || LOWORD(wParam) == TB_PAGEDOWN) {
            _UpdateLabels(hDlg);
            InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW), nullptr, FALSE);
        }
        return TRUE;

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == IDC_PREVIEW) {
            _DrawPreview(hDlg, dis);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_THEME_SYSTEM:
        case IDC_THEME_LIGHT:
        case IDC_THEME_DARK:
            InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW), nullptr, FALSE);
            return TRUE;
        case IDC_BTN_SAVE: {
            int scalePos = (int)SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_GETPOS, 0, 0);
            int fontPos = (int)SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_GETPOS, 0, 0);
            sm.SetScale(scalePos / 100.0f);
            sm.SetFontSize(fontPos);
            sm.SetThemeMode(GetDialogThemeMode(hDlg));
            sm.SetInsertSyllableSpaces(IsDlgButtonChecked(hDlg, IDC_INSERT_SPACES) == BST_CHECKED);
            sm.Save();

            // Broadcast change to all CandidateWindow instances
            PostMessageW(HWND_BROADCAST, WM_PINYINTSF_SETTINGS_CHANGED, 0, 0);

            _hDlgOpen = nullptr;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDC_BTN_RESET:
            SendDlgItemMessageW(hDlg, IDC_SCALE_TRACKBAR, TBM_SETPOS, TRUE, 100);
            SendDlgItemMessageW(hDlg, IDC_FONT_TRACKBAR, TBM_SETPOS, TRUE, DEFAULT_FONT_SIZE);
            SetDialogThemeMode(hDlg, SettingsManager::ThemeMode::FollowSystem);
            CheckDlgButton(hDlg, IDC_INSERT_SPACES, DEFAULT_INSERT_SYLLABLE_SPACES ? BST_CHECKED : BST_UNCHECKED);
            _UpdateLabels(hDlg);
            InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW), nullptr, FALSE);
            return TRUE;
        case IDCANCEL:
            _hDlgOpen = nullptr;
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        _hDlgOpen = nullptr;
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}
