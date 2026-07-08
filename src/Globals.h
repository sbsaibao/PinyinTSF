#pragma once

#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include <ole2.h>
#include <olectl.h>
#include <msctf.h>
#include <string>
#include <vector>

// ================================================================
// CLSID for PinyinTextService
// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
// ================================================================
extern const CLSID CLSID_PinyinTextService;

// ================================================================
// Profile GUID
// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
// ================================================================
extern const GUID GUID_Profile;

// ================================================================
// Language configuration
// ================================================================
#define TEXTSERVICE_LANGID      MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define TEXTSERVICE_ICON_INDEX  0
#define TEXTSERVICE_DESC        L"\x62FC\x97F3\x58F0\x8C03\x8F93\x5165\x6CD5"
// "鎷奸煶澹拌皟杈撳叆娉? in escaped Unicode

// ================================================================
// Global variables
// ================================================================
extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

// ================================================================
// DLL reference counting
// ================================================================
void DllAddRef();
void DllRelease();

// ================================================================
// Registration functions (implemented in Register.cpp)
// ================================================================
BOOL RegisterServer();
void UnregisterServer();
BOOL RegisterProfiles();
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();

#endif // GLOBALS_H
