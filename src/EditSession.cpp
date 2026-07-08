#include "EditSession.h"
#include "PinyinTextService.h"

// ================================================================
// CEditSessionBase - Base class implementation
// ================================================================

CEditSessionBase::CEditSessionBase(CPinyinTextService* pTextService, ITfContext* pContext)
    : _pTextService(pTextService)
    , _pContext(pContext)
    , _refCount(1)
{
    _pContext->AddRef();
}

CEditSessionBase::~CEditSessionBase() {
    if (_pContext) {
        _pContext->Release();
    }
}

STDMETHODIMP CEditSessionBase::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
        *ppvObj = (ITfEditSession*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEditSessionBase::AddRef() {
    return InterlockedIncrement(&_refCount);
}

STDMETHODIMP_(ULONG) CEditSessionBase::Release() {
    LONG cr = InterlockedDecrement(&_refCount);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

// ================================================================
// CStartCompositionEditSession
//
// Creates a new composition at the current insertion point.
// Uses ITfInsertAtSelection to find the position, then
// ITfContextComposition to start the composition.
// ================================================================

CStartCompositionEditSession::CStartCompositionEditSession(
    CPinyinTextService* pTextService, ITfContext* pContext)
    : CEditSessionBase(pTextService, pContext)
{
}

STDMETHODIMP CStartCompositionEditSession::DoEditSession(TfEditCookie ec) {
    // Get the insertion point
    ITfInsertAtSelection* pInsertAtSelection = nullptr;
    HRESULT hr = _pContext->QueryInterface(IID_ITfInsertAtSelection, (void**)&pInsertAtSelection);
    if (FAILED(hr)) return hr;

    // Get a range at the current selection (query only, don't insert yet)
    ITfRange* pRange = nullptr;
    hr = pInsertAtSelection->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &pRange);
    pInsertAtSelection->Release();

    if (FAILED(hr) || !pRange) return E_FAIL;

    // Start composition on this range
    ITfContextComposition* pContextComposition = nullptr;
    hr = _pContext->QueryInterface(IID_ITfContextComposition, (void**)&pContextComposition);
    if (FAILED(hr)) {
        pRange->Release();
        return hr;
    }

    ITfComposition* pComposition = nullptr;
    hr = pContextComposition->StartComposition(ec, pRange,
        (ITfCompositionSink*)_pTextService, &pComposition);
    pContextComposition->Release();
    pRange->Release();

    if (SUCCEEDED(hr) && pComposition) {
        _pTextService->SetComposition(pComposition);
        pComposition->Release();  // SetComposition AddRef'd it
    }

    return S_OK;
}

// ================================================================
// CUpdateCompositionEditSession
//
// Updates the composition range text with the current pinyin buffer.
// ================================================================

CUpdateCompositionEditSession::CUpdateCompositionEditSession(
    CPinyinTextService* pTextService, ITfContext* pContext, const std::wstring& text)
    : CEditSessionBase(pTextService, pContext)
    , _text(text)
{
}

STDMETHODIMP CUpdateCompositionEditSession::DoEditSession(TfEditCookie ec) {
    ITfComposition* pComposition = _pTextService->GetComposition();
    if (!pComposition) return S_OK;

    // Get the composition range
    ITfRange* pRange = nullptr;
    HRESULT hr = pComposition->GetRange(&pRange);
    if (FAILED(hr) || !pRange) return E_FAIL;

    // Update the text in the range
    hr = pRange->SetText(ec, 0, _text.c_str(), (LONG)_text.size());
    pRange->Release();

    return hr;
}

// ================================================================
// CCommitTextEditSession
//
// Sets the final text in the composition range, then ends the
// composition. Also moves the cursor to after the committed text.
// If commitText is empty, it effectively cancels the composition.
// ================================================================

CCommitTextEditSession::CCommitTextEditSession(
    CPinyinTextService* pTextService, ITfContext* pContext, const std::wstring& commitText)
    : CEditSessionBase(pTextService, pContext)
    , _commitText(commitText)
{
}

STDMETHODIMP CCommitTextEditSession::DoEditSession(TfEditCookie ec) {
    ITfComposition* pComposition = _pTextService->GetComposition();
    if (!pComposition) return S_OK;

    // Get the composition range
    ITfRange* pRange = nullptr;
    HRESULT hr = pComposition->GetRange(&pRange);
    if (FAILED(hr) || !pRange) return E_FAIL;

    // Set the final text (could be candidate text, raw pinyin, or empty for cancel)
    hr = pRange->SetText(ec, 0, _commitText.c_str(), (LONG)_commitText.size());

    if (SUCCEEDED(hr) && !_commitText.empty()) {
        // Move cursor to end of committed text
        pRange->Collapse(ec, TF_ANCHOR_END);

        TF_SELECTION sel;
        sel.range = pRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        _pContext->SetSelection(ec, 1, &sel);
    }

    pRange->Release();

    // End the composition
    pComposition->EndComposition(ec);

    // Clear the text service's composition state
    _pTextService->SetComposition(nullptr);
    _pTextService->GetCompositionBuffer().clear();
    _pTextService->GetCompositionState().Clear();
    _pTextService->GetCandidates().clear();
    _pTextService->GetSelectedIndex() = 0;

    // Hide candidate window
    CCandidateWindow* pCandWnd = _pTextService->GetCandidateWindow();
    if (pCandWnd) pCandWnd->Hide();

    return S_OK;
}

// ================================================================
// CGetPositionEditSession
//
// Gets the screen coordinates of the composition range end,
// used for positioning the candidate window.
// ================================================================

CGetPositionEditSession::CGetPositionEditSession(
    CPinyinTextService* pTextService, ITfContext* pContext, POINT* ppt)
    : CEditSessionBase(pTextService, pContext)
    , _ppt(ppt)
{
}

STDMETHODIMP CGetPositionEditSession::DoEditSession(TfEditCookie ec) {
    if (!_ppt) return E_INVALIDARG;

    _ppt->x = 0;
    _ppt->y = 0;

    ITfComposition* pComposition = _pTextService->GetComposition();
    if (!pComposition) return E_FAIL;

    // Get the composition range
    ITfRange* pRange = nullptr;
    HRESULT hr = pComposition->GetRange(&pRange);
    if (FAILED(hr) || !pRange) return E_FAIL;

    // Get the active view
    ITfContextView* pView = nullptr;
    hr = _pContext->GetActiveView(&pView);
    if (FAILED(hr) || !pView) {
        pRange->Release();
        return E_FAIL;
    }

    // Get the screen rectangle of the composition text
    RECT rc;
    BOOL fClipped;
    hr = pView->GetTextExt(ec, pRange, &rc, &fClipped);
    pView->Release();
    pRange->Release();

    if (SUCCEEDED(hr)) {
        // Position candidate window below the composition text
        _ppt->x = rc.left;
        _ppt->y = rc.bottom + 2;
    }

    return hr;
}
