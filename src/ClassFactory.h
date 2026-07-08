#pragma once

#ifndef CLASS_FACTORY_H
#define CLASS_FACTORY_H

#include <unknwn.h>

// ================================================================
// CClassFactory - COM class factory for creating CPinyinTextService
// ================================================================
class CClassFactory : public IClassFactory {
public:
    CClassFactory();
    ~CClassFactory();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    LONG _refCount;
};

#endif // CLASS_FACTORY_H
