#include "PinyinTextService.h"

// ================================================================
// _InitThreadMgrEventSink
//
// Register as a thread manager event sink to receive focus
// change notifications.
// ================================================================
BOOL CPinyinTextService::_InitThreadMgrEventSink() {
    ITfSource* pSource = nullptr;
    HRESULT hr = _pThreadMgr->QueryInterface(IID_ITfSource, (void**)&pSource);
    if (FAILED(hr)) return FALSE;

    hr = pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                             (ITfThreadMgrEventSink*)this,
                             &_dwThreadMgrEventSinkCookie);
    pSource->Release();

    return SUCCEEDED(hr);
}

// ================================================================
// _UninitThreadMgrEventSink
// ================================================================
void CPinyinTextService::_UninitThreadMgrEventSink() {
    if (_dwThreadMgrEventSinkCookie == TF_INVALID_COOKIE) return;

    ITfSource* pSource = nullptr;
    if (_pThreadMgr &&
        SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&pSource))) {
        pSource->UnadviseSink(_dwThreadMgrEventSinkCookie);
        pSource->Release();
    }
    _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;
}

// ================================================================
// ITfThreadMgrEventSink::OnInitDocumentMgr (not used)
// ================================================================
STDMETHODIMP CPinyinTextService::OnInitDocumentMgr(ITfDocumentMgr* pDocMgr) {
    return S_OK;
}

// ================================================================
// ITfThreadMgrEventSink::OnUninitDocumentMgr (not used)
// ================================================================
STDMETHODIMP CPinyinTextService::OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) {
    return S_OK;
}

// ================================================================
// ITfThreadMgrEventSink::OnSetFocus
//
// Called when focus changes to a different document.
// We end any active composition to avoid leaving stale state.
// ================================================================
STDMETHODIMP CPinyinTextService::OnSetFocus(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus) {
    // When focus changes, clean up any active composition
    if (_pComposition) {
        _EndComposition();
    }
    return S_OK;
}

// ================================================================
// ITfThreadMgrEventSink::OnPushContext / OnPopContext (not used)
// ================================================================
STDMETHODIMP CPinyinTextService::OnPushContext(ITfContext* pContext) {
    return S_OK;
}

STDMETHODIMP CPinyinTextService::OnPopContext(ITfContext* pContext) {
    return S_OK;
}
