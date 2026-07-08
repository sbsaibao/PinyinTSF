#include "PinyinTextService.h"
#include "EditSession.h"
#include "KeyEventLogic.h"

namespace {
bool IsKeyDown(int vkey) {
    return (GetKeyState(vkey) & 0x8000) != 0;
}
}

// ================================================================
// _InitKeyEventSink - Register as key event sink with TSF
// ================================================================
BOOL CPinyinTextService::_InitKeyEventSink() {
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    HRESULT hr = _pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);
    if (FAILED(hr)) return FALSE;

    hr = pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, (ITfKeyEventSink*)this, TRUE);
    pKeystrokeMgr->Release();

    return SUCCEEDED(hr);
}

// ================================================================
// _UninitKeyEventSink - Unregister key event sink
// ================================================================
void CPinyinTextService::_UninitKeyEventSink() {
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    if (_pThreadMgr &&
        SUCCEEDED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr))) {
        pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);
        pKeystrokeMgr->Release();
    }
}

// ================================================================
// ITfKeyEventSink::OnSetFocus
// ================================================================
STDMETHODIMP CPinyinTextService::OnSetFocus(BOOL fForeground) {
    return S_OK;
}

// ================================================================
// ITfKeyEventSink::OnTestKeyDown
//
// Pre-test: tell TSF whether we want to eat this key.
// Return pfEaten=TRUE to handle the key in OnKeyDown.
// ================================================================
STDMETHODIMP CPinyinTextService::OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;

    bool ctrlDown = IsKeyDown(VK_CONTROL);
    bool altDown = IsKeyDown(VK_MENU);
    bool winDown = IsKeyDown(VK_LWIN) || IsKeyDown(VK_RWIN);

    *pfEaten = KeyEventLogic::ShouldEatKeyDown(
        wParam,
        _pComposition != nullptr,
        !_candidates.empty(),
        static_cast<int>(_candidates.size()),
        ctrlDown,
        altDown,
        winDown) ? TRUE : FALSE;

    return S_OK;
}

// ================================================================
// ITfKeyEventSink::OnKeyDown
//
// Actual key handling. Called only if OnTestKeyDown set pfEaten=TRUE.
// ================================================================
STDMETHODIMP CPinyinTextService::OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    bool ctrlDown = IsKeyDown(VK_CONTROL);
    bool altDown = IsKeyDown(VK_MENU);
    bool winDown = IsKeyDown(VK_LWIN) || IsKeyDown(VK_RWIN);

    if (!KeyEventLogic::ShouldEatKeyDown(
            wParam,
            _pComposition != nullptr,
            !_candidates.empty(),
            static_cast<int>(_candidates.size()),
            ctrlDown,
            altDown,
            winDown)) {
        return S_OK;
    }

    // ---- Letter keys: add to pinyin buffer ----
    if (wParam >= 'A' && wParam <= 'Z') {
        wchar_t ch = (wchar_t)(wParam - 'A' + 'a');  // to lowercase
        _HandleCharInput(pContext, ch);
        *pfEaten = TRUE;
        return S_OK;
    }

    // ---- Keys that require active composition ----
    if (_pComposition) {
        // Number keys 1-5: select candidate
        if (wParam >= '1' && wParam <= '5' && !_candidates.empty()) {
            int index = (int)(wParam - '1');
            if (index < (int)_candidates.size()) {
                _HandleCandidateSelection(pContext, index);
                *pfEaten = TRUE;
                return S_OK;
            }
        }

        // Backspace
        if (wParam == VK_BACK) {
            _HandleBackspace(pContext);
            *pfEaten = TRUE;
            return S_OK;
        }

        // Escape: cancel
        if (wParam == VK_ESCAPE) {
            _HandleEscape(pContext);
            *pfEaten = TRUE;
            return S_OK;
        }

        // Enter: commit raw pinyin
        if (wParam == VK_RETURN) {
            _HandleEnter(pContext);
            *pfEaten = TRUE;
            return S_OK;
        }

        // Space: commit the currently highlighted candidate
        if (wParam == VK_SPACE && !_candidates.empty()) {
            int index = _selectedIndex;
            if (_pCandidateWindow) {
                index = _pCandidateWindow->GetSelection();
                _selectedIndex = index;
            }
            _HandleCandidateSelection(pContext, index);
            *pfEaten = TRUE;
            return S_OK;
        }

        // Arrow keys: navigate candidates
        if (wParam == VK_RIGHT || wParam == VK_DOWN) {
            if (_pCandidateWindow && !_candidates.empty()) {
                _pCandidateWindow->MoveSelectionNext();
                _selectedIndex = _pCandidateWindow->GetSelection();
            }
            *pfEaten = TRUE;
            return S_OK;
        }

        if (wParam == VK_LEFT || wParam == VK_UP) {
            if (_pCandidateWindow && !_candidates.empty()) {
                _pCandidateWindow->MoveSelectionPrev();
                _selectedIndex = _pCandidateWindow->GetSelection();
            }
            *pfEaten = TRUE;
            return S_OK;
        }
    }

    return S_OK;
}

// ================================================================
// ITfKeyEventSink::OnTestKeyUp / OnKeyUp
// (Not used - we don't process key up events)
// ================================================================
STDMETHODIMP CPinyinTextService::OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP CPinyinTextService::OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

// ================================================================
// ITfKeyEventSink::OnPreservedKey (not used)
// ================================================================
STDMETHODIMP CPinyinTextService::OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten) {
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

// ================================================================
// _HandleCharInput - User typed a letter key
// ================================================================
void CPinyinTextService::_HandleCharInput(ITfContext* pContext, wchar_t ch) {
    _compositionState.AppendChar(ch);

    if (!_pComposition) {
        // Start a new composition
        _StartComposition(pContext);
    }

    _compositionState.Rebuild(*_pPinyinEngine);
    _compositionBuffer = _compositionState.BuildDisplayText();

    // Update the inline composition text
    _UpdateComposition(pContext);

    // Regenerate candidates
    _UpdateCandidates();

    // Show/hide candidate window
    _UpdateCandidateWindow(pContext);
}

// ================================================================
// _HandleCandidateSelection - User selected a candidate (1-5)
// ================================================================
void CPinyinTextService::_HandleCandidateSelection(ITfContext* pContext, int index) {
    if (index >= 0 && index < (int)_candidates.size()) {
        if (_compositionState.HasPendingSyllable()) {
            bool complete = _compositionState.SelectCurrentSyllable(_candidates[index]);
            _compositionBuffer = _compositionState.BuildDisplayText();

            if (complete) {
                _CommitText(pContext, _compositionState.BuildCommittedText());
            } else {
                _UpdateComposition(pContext);
                _UpdateCandidates();
                _UpdateCandidateWindow(pContext);
            }
        } else {
            _CommitText(pContext, _candidates[index]);
        }
    }
}

// ================================================================
// _HandleBackspace - Delete last character from buffer
// ================================================================
void CPinyinTextService::_HandleBackspace(ITfContext* pContext) {
    if (!_compositionState.HasInput()) return;

    _compositionState.Backspace();

    if (!_compositionState.HasInput()) {
        // Buffer empty -> cancel composition
        _CommitText(pContext, L"");
    } else {
        // Update composition text and candidates
        _compositionState.Rebuild(*_pPinyinEngine);
        _compositionBuffer = _compositionState.BuildDisplayText();
        _UpdateComposition(pContext);
        _UpdateCandidates();
        _UpdateCandidateWindow(pContext);
    }
}

// ================================================================
// _HandleEscape - Cancel composition entirely
// ================================================================
void CPinyinTextService::_HandleEscape(ITfContext* pContext) {
    _CommitText(pContext, L"");
}

// ================================================================
// _HandleEnter - Commit the raw pinyin text as-is
// ================================================================
void CPinyinTextService::_HandleEnter(ITfContext* pContext) {
    _CommitText(pContext, _compositionState.HasInput()
        ? _compositionState.BuildCommittedText()
        : _compositionBuffer);
}
