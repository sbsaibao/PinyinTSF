#pragma once

#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON  (WM_APP + 1)

class TrayIcon {
public:
    static void Initialize(HINSTANCE hInst);
    static void Uninitialize();
    static void RequestShutdownExisting();

private:
    static DWORD WINAPI _TrayThread(LPVOID lpParam);
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void _ShowSettingsDialog(HWND hwndOwner);
    static void _ResetStateFromTrayThread();

    static HINSTANCE _hInst;
    static HWND      _hwndHost;
    static HANDLE    _hThread;
    static HANDLE    _hMutex;
    static DWORD     _dwThreadId;
    static BOOL      _initialized;
};

inline void TrayIcon::RequestShutdownExisting() {
    HWND hwnd = FindWindowW(L"PinyinTSFTrayHost", nullptr);
    if (!hwnd) {
        hwnd = FindWindowExW(HWND_MESSAGE, nullptr, L"PinyinTSFTrayHost", nullptr);
    }
    if (!hwnd) return;

    DWORD_PTR result = 0;
    SendMessageTimeoutW(hwnd, WM_CLOSE, 0, 0, SMTO_ABORTIFHUNG, 1000, &result);
}

#endif // TRAY_ICON_H
