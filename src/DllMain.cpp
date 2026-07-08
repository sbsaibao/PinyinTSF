#include "Globals.h"
#include "ClassFactory.h"
#include "SettingsManager.h"
#include "TrayIcon.h"

// ================================================================
// DllMain - DLL entry point
// ================================================================
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInstance;
            DisableThreadLibraryCalls(hInstance);
            SettingsManager::Instance().Load();
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

// ================================================================
// DllGetClassObject - Return class factory for the requested CLSID
// ================================================================
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    // Only our CLSID is supported
    if (!IsEqualCLSID(rclsid, CLSID_PinyinTextService))
        return CLASS_E_CLASSNOTAVAILABLE;

    CClassFactory* pClassFactory = new CClassFactory();
    if (!pClassFactory)
        return E_OUTOFMEMORY;

    HRESULT hr = pClassFactory->QueryInterface(riid, ppvObj);
    pClassFactory->Release();

    return hr;
}

// ================================================================
// DllCanUnloadNow - TSF calls this to check if DLL can be unloaded
// ================================================================
STDAPI DllCanUnloadNow() {
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}

// ================================================================
// DllRegisterServer - Called by regsvr32 to register the input method
//
// Performs three registrations:
//   1. COM server (registry CLSID -> InProcServer32)
//   2. TSF profile (ITfInputProcessorProfiles)
//   3. TSF category (ITfCategoryMgr -> GUID_TFCAT_TIP_KEYBOARD)
// ================================================================
STDAPI DllRegisterServer() {
    // Step 1: Register COM server
    if (!RegisterServer()) {
        return SELFREG_E_CLASS;
    }

    // Step 2: Register TSF profiles
    if (!RegisterProfiles()) {
        UnregisterServer();
        return SELFREG_E_CLASS;
    }

    // Step 3: Register TSF categories
    if (!RegisterCategories()) {
        UnregisterProfiles();
        UnregisterServer();
        return SELFREG_E_CLASS;
    }

    return S_OK;
}

// ================================================================
// DllUnregisterServer - Called by regsvr32 /u to unregister
// ================================================================
STDAPI DllUnregisterServer() {
    TrayIcon::RequestShutdownExisting();
    UnregisterCategories();
    UnregisterProfiles();
    UnregisterServer();
    return S_OK;
}
