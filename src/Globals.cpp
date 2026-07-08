#include "Globals.h"

// Link libraries
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")

// ================================================================
// Global variable instances
// ================================================================
HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

// ================================================================
// GUID instances
// ================================================================

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
const CLSID CLSID_PinyinTextService = {
    0xa1b2c3d4, 0xe5f6, 0x7890,
    { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 }
};

// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
const GUID GUID_Profile = {
    0xb2c3d4e5, 0xf6a7, 0x8901,
    { 0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0x01 }
};

// ================================================================
// DLL reference counting
// ================================================================

void DllAddRef() {
    InterlockedIncrement(&g_cRefDll);
}

void DllRelease() {
    InterlockedDecrement(&g_cRefDll);
}
