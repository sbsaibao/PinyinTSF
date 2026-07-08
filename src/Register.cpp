#include "Globals.h"

// CLSID registry string length: "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
#define CLSID_STRLEN 38

static const TCHAR c_szInfoKeyPrefix[] = TEXT("CLSID\\");
static const TCHAR c_szInProcSvr32[]   = TEXT("InProcServer32");
static const TCHAR c_szModelName[]     = TEXT("ThreadingModel");

// ================================================================
// RegisterServer - Register COM server in registry
//
// Creates:
//   HKCR\CLSID\{clsid}                    = "Description"
//   HKCR\CLSID\{clsid}\InProcServer32     = "path\to\PinyinTSF.dll"
//   HKCR\CLSID\{clsid}\InProcServer32\ThreadingModel = "Apartment"
// ================================================================
BOOL RegisterServer() {
    TCHAR achClsid[CLSID_STRLEN + 1];
    TCHAR achFileName[MAX_PATH];

    StringFromGUID2(CLSID_PinyinTextService, achClsid, CLSID_STRLEN + 1);

    // Build key paths for rollback
    TCHAR achClsidKey[256];
    TCHAR achInProcKey[256];
    wsprintf(achClsidKey, TEXT("%s%s"), c_szInfoKeyPrefix, achClsid);
    wsprintf(achInProcKey, TEXT("%s\\%s"), achClsidKey, c_szInProcSvr32);

    // Create CLSID key
    HKEY hKey;
    DWORD dwDisposition;
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, achClsidKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, &dwDisposition) != ERROR_SUCCESS) {
        return FALSE;
    }

    // Set description value
    RegSetValueEx(hKey, nullptr, 0, REG_SZ,
                  (const BYTE*)TEXTSERVICE_DESC,
                  (DWORD)((wcslen(TEXTSERVICE_DESC) + 1) * sizeof(TCHAR)));
    RegCloseKey(hKey);

    // Create InProcServer32 subkey
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, achInProcKey, 0, nullptr, REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, nullptr, &hKey, &dwDisposition) != ERROR_SUCCESS) {
        RegDeleteKey(HKEY_CLASSES_ROOT, achClsidKey);
        return FALSE;
    }

    // Set DLL path
    GetModuleFileName(g_hInst, achFileName, MAX_PATH);
    LONG res = RegSetValueEx(hKey, nullptr, 0, REG_SZ,
                             (const BYTE*)achFileName,
                             (DWORD)((wcslen(achFileName) + 1) * sizeof(TCHAR)));
    if (res != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        RegDeleteKey(HKEY_CLASSES_ROOT, achInProcKey);
        RegDeleteKey(HKEY_CLASSES_ROOT, achClsidKey);
        return FALSE;
    }

    // Set threading model
    res = RegSetValueEx(hKey, c_szModelName, 0, REG_SZ,
                        (const BYTE*)TEXT("Apartment"),
                        (DWORD)((wcslen(TEXT("Apartment")) + 1) * sizeof(TCHAR)));
    RegCloseKey(hKey);
    if (res != ERROR_SUCCESS) {
        RegDeleteKey(HKEY_CLASSES_ROOT, achInProcKey);
        RegDeleteKey(HKEY_CLASSES_ROOT, achClsidKey);
        return FALSE;
    }

    return TRUE;
}

// ================================================================
// UnregisterServer - Remove COM registration from registry
// ================================================================
void UnregisterServer() {
    TCHAR achClsid[CLSID_STRLEN + 1];
    TCHAR achIMEKey[256];

    StringFromGUID2(CLSID_PinyinTextService, achClsid, CLSID_STRLEN + 1);

    // Delete InProcServer32 subkey
    wsprintf(achIMEKey, TEXT("%s%s\\%s"), c_szInfoKeyPrefix, achClsid, c_szInProcSvr32);
    RegDeleteKey(HKEY_CLASSES_ROOT, achIMEKey);

    // Delete CLSID key
    wsprintf(achIMEKey, TEXT("%s%s"), c_szInfoKeyPrefix, achClsid);
    RegDeleteKey(HKEY_CLASSES_ROOT, achIMEKey);
}

// ================================================================
// RegisterProfiles - Register input method profile with TSF
//
// Uses ITfInputProcessorProfiles to register the text service
// and add a Chinese language profile.
// ================================================================
BOOL RegisterProfiles() {
    ITfInputProcessorProfiles* pInputProcessProfiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfiles, (void**)&pInputProcessProfiles);
    if (FAILED(hr)) return FALSE;

    // Register the text service CLSID
    hr = pInputProcessProfiles->Register(CLSID_PinyinTextService);
    if (SUCCEEDED(hr)) {
        // Get TSF profile icon file path next to the DLL
        WCHAR achIconFile[MAX_PATH];
        GetModuleFileNameW(g_hInst, achIconFile, MAX_PATH);
        WCHAR* slash = wcsrchr(achIconFile, L'\\');
        if (slash) {
            lstrcpyW(slash + 1, L"profile_icon.ico");
        }

        // Add language profile (Chinese Simplified)
        hr = pInputProcessProfiles->AddLanguageProfile(
            CLSID_PinyinTextService,        // CLSID
            TEXTSERVICE_LANGID,             // Language ID (0x0804)
            GUID_Profile,                    // Profile GUID
            TEXTSERVICE_DESC,                // Description
            (ULONG)wcslen(TEXTSERVICE_DESC), // Description length
            achIconFile,                     // Icon file
            (ULONG)wcslen(achIconFile),      // Icon file length
            TEXTSERVICE_ICON_INDEX           // Icon index
        );
    }

    pInputProcessProfiles->Release();
    return SUCCEEDED(hr);
}

// ================================================================
// UnregisterProfiles - Remove TSF profile registration
// ================================================================
void UnregisterProfiles() {
    ITfInputProcessorProfiles* pInputProcessProfiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfiles, (void**)&pInputProcessProfiles);
    if (FAILED(hr)) return;

    pInputProcessProfiles->Unregister(CLSID_PinyinTextService);
    pInputProcessProfiles->Release();
}

// ================================================================
// RegisterCategories - Register as keyboard text service category
//
// This tells TSF that our text service is a keyboard input method.
// ================================================================
BOOL RegisterCategories() {
    ITfCategoryMgr* pCategoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (FAILED(hr)) return FALSE;

    hr = pCategoryMgr->RegisterCategory(
        CLSID_PinyinTextService,    // CLSID of the text service
        GUID_TFCAT_TIP_KEYBOARD,    // Category: keyboard
        CLSID_PinyinTextService     // CLSID to register
    );

    pCategoryMgr->Release();
    return SUCCEEDED(hr);
}

// ================================================================
// UnregisterCategories - Remove category registration
// ================================================================
void UnregisterCategories() {
    ITfCategoryMgr* pCategoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (FAILED(hr)) return;

    pCategoryMgr->UnregisterCategory(
        CLSID_PinyinTextService,
        GUID_TFCAT_TIP_KEYBOARD,
        CLSID_PinyinTextService
    );

    pCategoryMgr->Release();
}
