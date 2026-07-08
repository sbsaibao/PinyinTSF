#pragma once

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <windows.h>

class SettingsDialog {
public:
    static void Show(HINSTANCE hInst, HWND hwndParent);

private:
    static INT_PTR CALLBACK _DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static void _UpdateLabels(HWND hDlg);
    static void _DrawPreview(HWND hDlg, DRAWITEMSTRUCT* dis);

    static HWND _hDlgOpen;
};

#endif // SETTINGS_DIALOG_H
