#include "PinyinTextService.h"
#include "SettingsManager.h"
#include "TrayIcon.h"

// ================================================================
// Constructor
// ================================================================
CPinyinTextService::CPinyinTextService()
    : _refCount(1)
    , _pThreadMgr(nullptr)
    , _tfClientId(TF_CLIENTID_NULL)
    , _dwActivateFlags(0)
    , _dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE)
    , _pComposition(nullptr)
    , _pCandidateWindow(nullptr)
    , _pPinyinEngine(nullptr)
    , _selectedIndex(0)
{
    DllAddRef();
}

// ================================================================
// Destructor
// ================================================================
CPinyinTextService::~CPinyinTextService() {
    DllRelease();
}

// ================================================================
// IUnknown::QueryInterface
// ================================================================
STDMETHODIMP CPinyinTextService::QueryInterface(REFIID riid, void** ppvObj) {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppvObj = (ITfTextInputProcessorEx*)this;
    } else if (IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = (ITfTextInputProcessor*)this;
    } else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
        *ppvObj = (ITfTextInputProcessorEx*)this;
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = (ITfKeyEventSink*)this;
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObj = (ITfCompositionSink*)this;
    } else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) {
        *ppvObj = (ITfThreadMgrEventSink*)this;
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// ================================================================
// IUnknown::AddRef / Release
// ================================================================
STDMETHODIMP_(ULONG) CPinyinTextService::AddRef() {
    return InterlockedIncrement(&_refCount);
}

STDMETHODIMP_(ULONG) CPinyinTextService::Release() {
    LONG cr = InterlockedDecrement(&_refCount);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

// ================================================================
// ITfTextInputProcessor::Activate (legacy, delegates to ActivateEx)
// ================================================================
STDMETHODIMP CPinyinTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) {
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

// ================================================================
// ITfTextInputProcessorEx::ActivateEx
//
// Called by TSF when the input method is activated.
// Initializes all subsystems: key sink, thread mgr sink,
// candidate window, and pinyin engine.
// ================================================================
STDMETHODIMP CPinyinTextService::ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags) {
    _pThreadMgr = pThreadMgr;
    _pThreadMgr->AddRef();
    _tfClientId = tfClientId;
    _dwActivateFlags = dwFlags;

    TrayIcon::Initialize(g_hInst);

    // Initialize key event sink
    if (!_InitKeyEventSink()) {
        Deactivate();
        return E_FAIL;
    }

    // Initialize thread manager event sink
    if (!_InitThreadMgrEventSink()) {
        Deactivate();
        return E_FAIL;
    }

    // Create candidate window
    _pCandidateWindow = new CCandidateWindow();
    if (!_pCandidateWindow->Create(g_hInst)) {
        Deactivate();
        return E_FAIL;
    }

    // Create pinyin engine
    _pPinyinEngine = new CPinyinEngine();

    return S_OK;
}

// ================================================================
// ITfTextInputProcessorEx::Deactivate
//
// Called by TSF when the input method is deactivated.
// Cleans up all resources.
// ================================================================
STDMETHODIMP CPinyinTextService::Deactivate() {
    // End any active composition
    _EndComposition();

    // Cleanup key event sink
    _UninitKeyEventSink();

    // Cleanup thread manager event sink
    _UninitThreadMgrEventSink();

    // Destroy candidate window
    if (_pCandidateWindow) {
        _pCandidateWindow->Destroy();
        delete _pCandidateWindow;
        _pCandidateWindow = nullptr;
    }

    // Destroy pinyin engine
    if (_pPinyinEngine) {
        delete _pPinyinEngine;
        _pPinyinEngine = nullptr;
    }

    // Tray settings entry should exist only while this TIP is active.
    TrayIcon::Uninitialize();

    // Release thread manager
    if (_pThreadMgr) {
        _pThreadMgr->Release();
        _pThreadMgr = nullptr;
    }

    _tfClientId = TF_CLIENTID_NULL;
    return S_OK;
}

// ================================================================
// SetComposition - safely swap composition pointer with AddRef/Release
// ================================================================
void CPinyinTextService::SetComposition(ITfComposition* pComp) {
    if (_pComposition == pComp) return;

    if (_pComposition) {
        _pComposition->Release();
    }
    _pComposition = pComp;
    if (_pComposition) {
        _pComposition->AddRef();
    }
}

// ================================================================
// _EndComposition - cleanup composition state without edit session
//
// Called from Deactivate() and OnSetFocus() where we don't have
// an edit cookie. Just releases references and resets state.
// ================================================================
void CPinyinTextService::_EndComposition() {
    if (_pComposition) {
        _pComposition->Release();
        _pComposition = nullptr;
    }
    _compositionBuffer.clear();
    _compositionState.Clear();
    _candidates.clear();
    _selectedIndex = 0;
    if (_pCandidateWindow) {
        _pCandidateWindow->Hide();
    }
}

// ================================================================
// _UpdateCandidates - regenerate candidates from current buffer
// ================================================================
void CPinyinTextService::_UpdateCandidates() {
    if (!_pPinyinEngine) return;
    const bool insertSyllableSpaces = SettingsManager::Instance().GetInsertSyllableSpaces();

    std::wstring syllable = _compositionState.GetCurrentSyllable();
    if (!syllable.empty()) {
        _candidates = _pPinyinEngine->GetSyllableCandidates(syllable);
    } else {
        _candidates = _pPinyinEngine->GetCandidates(_compositionBuffer);
    }
    _selectedIndex = 0;

    if (_pCandidateWindow) {
        _pCandidateWindow->SetPinyinText(_compositionState.HasInput()
            ? _compositionState.BuildDisplayText(insertSyllableSpaces)
            : _compositionBuffer);
        _pCandidateWindow->SetCandidates(_candidates);
        _pCandidateWindow->SetSelection(0);
    }
}

// ================================================================
// _UpdateCandidateWindow - show or hide candidate window
// ================================================================
void CPinyinTextService::_UpdateCandidateWindow(ITfContext* pContext) {
    // Show candidate window as long as we're in composition state
    // This provides immediate visual feedback even when typing initial consonants
    if (_pComposition) {
        _ShowCandidateWindow(pContext);
    } else if (_pCandidateWindow) {
        _pCandidateWindow->Hide();
    }
}

// ================================================================
// ITfCompositionSink::OnCompositionTerminated
//
// Called when the composition is terminated externally
// (e.g., application closes, focus changes).
// ================================================================
STDMETHODIMP CPinyinTextService::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) {
    _EndComposition();
    return S_OK;
}
