#include "ClassFactory.h"
#include "PinyinTextService.h"
#include "Globals.h"

// ================================================================
// Constructor / Destructor
// ================================================================

CClassFactory::CClassFactory()
    : _refCount(1)
{
    DllAddRef();
}

CClassFactory::~CClassFactory() {
    DllRelease();
}

// ================================================================
// IUnknown
// ================================================================

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObj = (IClassFactory*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef() {
    return InterlockedIncrement(&_refCount);
}

STDMETHODIMP_(ULONG) CClassFactory::Release() {
    LONG cr = InterlockedDecrement(&_refCount);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

// ================================================================
// IClassFactory::CreateInstance
//
// Creates a new CPinyinTextService instance and returns the
// requested interface.
// ================================================================
STDMETHODIMP CClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    // We don't support COM aggregation
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    CPinyinTextService* pTextService = new CPinyinTextService();
    if (!pTextService) return E_OUTOFMEMORY;

    HRESULT hr = pTextService->QueryInterface(riid, ppvObj);
    pTextService->Release();  // QI AddRef'd if successful

    return hr;
}

// ================================================================
// IClassFactory::LockServer
// ================================================================
STDMETHODIMP CClassFactory::LockServer(BOOL fLock) {
    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }
    return S_OK;
}
