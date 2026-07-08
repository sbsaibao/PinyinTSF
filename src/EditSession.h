#pragma once

#ifndef EDIT_SESSION_H
#define EDIT_SESSION_H

#include <msctf.h>
#include <string>

class CPinyinTextService;  // Forward declaration

// ================================================================
// Base edit session class with IUnknown implementation
// ================================================================
class CEditSessionBase : public ITfEditSession {
public:
    CEditSessionBase(CPinyinTextService* pTextService, ITfContext* pContext);
    virtual ~CEditSessionBase();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ITfEditSession (pure virtual)
    STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

protected:
    CPinyinTextService* _pTextService;
    ITfContext*          _pContext;
    LONG                _refCount;
};

// ================================================================
// Start a new composition at current selection
// ================================================================
class CStartCompositionEditSession : public CEditSessionBase {
public:
    CStartCompositionEditSession(CPinyinTextService* pTextService, ITfContext* pContext);
    STDMETHODIMP DoEditSession(TfEditCookie ec);
};

// ================================================================
// Update composition text (inline display)
// ================================================================
class CUpdateCompositionEditSession : public CEditSessionBase {
public:
    CUpdateCompositionEditSession(CPinyinTextService* pTextService, ITfContext* pContext,
                                  const std::wstring& text);
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    std::wstring _text;
};

// ================================================================
// Commit final text and end composition
// ================================================================
class CCommitTextEditSession : public CEditSessionBase {
public:
    CCommitTextEditSession(CPinyinTextService* pTextService, ITfContext* pContext,
                           const std::wstring& commitText);
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    std::wstring _commitText;
};

// ================================================================
// Get text cursor screen position for candidate window placement
// ================================================================
class CGetPositionEditSession : public CEditSessionBase {
public:
    CGetPositionEditSession(CPinyinTextService* pTextService, ITfContext* pContext, POINT* ppt);
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    POINT* _ppt;
};

#endif // EDIT_SESSION_H
