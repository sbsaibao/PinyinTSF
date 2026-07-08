#include "PinyinTextService.h"
#include "EditSession.h"

// ================================================================
// _StartComposition
//
// Request a synchronous edit session to create a new composition
// at the current insertion point.
// ================================================================
void CPinyinTextService::_StartComposition(ITfContext* pContext) {
    CStartCompositionEditSession* pEditSession = new CStartCompositionEditSession(this, pContext);

    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                TF_ES_READWRITE | TF_ES_SYNC, &hr);
    pEditSession->Release();
}

// ================================================================
// _UpdateComposition
//
// Request a synchronous edit session to update the composition
// range text with the current pinyin buffer.
// ================================================================
void CPinyinTextService::_UpdateComposition(ITfContext* pContext) {
    CUpdateCompositionEditSession* pEditSession =
        new CUpdateCompositionEditSession(this, pContext, _compositionBuffer);

    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                TF_ES_READWRITE | TF_ES_SYNC, &hr);
    pEditSession->Release();
}

// ================================================================
// _CommitText
//
// Request a synchronous edit session to commit the final text
// and end the composition.
// ================================================================
void CPinyinTextService::_CommitText(ITfContext* pContext, const std::wstring& text) {
    CCommitTextEditSession* pEditSession =
        new CCommitTextEditSession(this, pContext, text);

    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                TF_ES_READWRITE | TF_ES_SYNC, &hr);
    pEditSession->Release();
}

// ================================================================
// _ShowCandidateWindow
//
// Request a read-only edit session to get the cursor screen
// position, then show the candidate window at that location.
// ================================================================
void CPinyinTextService::_ShowCandidateWindow(ITfContext* pContext) {
    if (!_pCandidateWindow) return;

    POINT pt = { 0, 0 };
    CGetPositionEditSession* pEditSession =
        new CGetPositionEditSession(this, pContext, &pt);

    HRESULT hr;
    pContext->RequestEditSession(_tfClientId, pEditSession,
                                TF_ES_READ | TF_ES_SYNC, &hr);
    pEditSession->Release();

    // Show window at the retrieved position
    if (pt.x != 0 || pt.y != 0) {
        _pCandidateWindow->Show(pt);
    }
}
