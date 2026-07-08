#include "TrayIcon.h"
#include "SettingsDialog.h"
#include "resource.h"

HINSTANCE TrayIcon::_hInst = nullptr;
HWND      TrayIcon::_hwndHost = nullptr;
HANDLE    TrayIcon::_hThread = nullptr;
HANDLE    TrayIcon::_hMutex = nullptr;
DWORD     TrayIcon::_dwThreadId = 0;
BOOL      TrayIcon::_initialized = FALSE;

void TrayIcon::_ResetStateFromTrayThread() {
    _hwndHost = nullptr;
    _dwThreadId = 0;
    _initialized = FALSE;
    if (_hMutex) {
        CloseHandle(_hMutex);
        _hMutex = nullptr;
    }
}

DWORD WINAPI TrayIcon::_TrayThread(LPVOID lpParam) {
    HINSTANCE hInst = (HINSTANCE)lpParam;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = _WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PinyinTSFTrayHost";
    RegisterClassExW(&wc);

    _hwndHost = CreateWindowExW(0, L"PinyinTSFTrayHost", L"",
                                 WS_OVERLAPPED, 0, 0, 0, 0,
                                 nullptr, nullptr, hInst, nullptr);
    if (!_hwndHost) {
        _ResetStateFromTrayThread();
        return 0;
    }

    NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
    nid.hWnd = _hwndHost;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    // Load tray-specific icon next to the DLL
    WCHAR iconPath[MAX_PATH];
    GetModuleFileNameW(hInst, iconPath, MAX_PATH);
    // Replace filename with tray_icon.ico
    WCHAR* slash = wcsrchr(iconPath, L'\\');
    if (slash) {
        lstrcpyW(slash + 1, L"tray_icon.ico");
    }
    nid.hIcon = (HICON)LoadImageW(nullptr, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!nid.hIcon) {
        nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    lstrcpyW(nid.szTip, L"\x62FC\x97F3\x58F0\x8C03\x8F93\x5165\x6CD5");
    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        WCHAR msg[128];
        wsprintfW(msg, L"PinyinTSF: Shell_NotifyIconW(NIM_ADD) failed, error=%lu\n", GetLastError());
        OutputDebugStringW(msg);
        if (nid.hIcon) {
            DestroyIcon(nid.hIcon);
        }
        DestroyWindow(_hwndHost);
        _ResetStateFromTrayThread();
        return 0;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    NOTIFYICONDATAW nidDel = { sizeof(NOTIFYICONDATAW) };
    nidDel.hWnd = _hwndHost;
    nidDel.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nidDel);
    if (nid.hIcon) {
        DestroyIcon(nid.hIcon);
    }
    DestroyWindow(_hwndHost);
    _ResetStateFromTrayThread();

    return 0;
}

void TrayIcon::Initialize(HINSTANCE hInst) {
    if (_initialized) return;
    _hInst = hInst;

    _hMutex = CreateMutexW(nullptr, FALSE, L"Global\\PinyinTSFTray");
    if (!_hMutex) return;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(_hMutex);
        _hMutex = nullptr;
        return;
    }

    _hThread = CreateThread(nullptr, 0, _TrayThread, hInst, 0, &_dwThreadId);
    if (!_hThread) {
        CloseHandle(_hMutex);
        _hMutex = nullptr;
        return;
    }

    _initialized = TRUE;
}

void TrayIcon::Uninitialize() {
    if (!_initialized) return;

    if (_hwndHost) {
        PostMessageW(_hwndHost, WM_CLOSE, 0, 0);
    } else if (_dwThreadId) {
        PostThreadMessageW(_dwThreadId, WM_QUIT, 0, 0);
    }
    if (_hThread) {
        DWORD waitResult = WaitForSingleObject(_hThread, 2000);
        if (waitResult == WAIT_OBJECT_0) {
            CloseHandle(_hThread);
            _hThread = nullptr;
        } else {
            OutputDebugStringW(L"PinyinTSF: tray thread did not exit within timeout\n");
            return;
        }
    }

    if (_hMutex) {
        CloseHandle(_hMutex);
        _hMutex = nullptr;
    }

    _initialized = FALSE;
    _dwThreadId = 0;
    _hwndHost = nullptr;
}

LRESULT CALLBACK TrayIcon::_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_LBUTTONUP) {
            _ShowSettingsDialog(hwnd);
            return 0;
        }
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"\x8BBE\x7F6E");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"\x9000\x51FA");
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                     pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
            if (cmd == IDM_SETTINGS) {
                _ShowSettingsDialog(hwnd);
            } else if (cmd == IDM_EXIT) {
                PostQuitMessage(0);
            }
            return 0;
        }
    }
    if (msg == WM_CLOSE) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void TrayIcon::_ShowSettingsDialog(HWND hwndOwner) {
    SettingsDialog::Show(_hInst, hwndOwner);
}
