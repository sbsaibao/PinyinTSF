#pragma once

#ifndef PINYIN_TEXT_SERVICE_H
#define PINYIN_TEXT_SERVICE_H

#include "Globals.h"
#include "CandidateWindow.h"
#include "PinyinEngine.h"
#include "PinyinCompositionState.h"

// ================================================================
// CPinyinTextService - Main TSF text input processor
//
// Implements:
//   ITfTextInputProcessorEx  - Activation/deactivation
//   ITfKeyEventSink          - Keyboard input handling
//   ITfCompositionSink       - Composition termination events
//   ITfThreadMgrEventSink    - Focus change events
// ================================================================
class CPinyinTextService : public ITfTextInputProcessorEx,
                           public ITfKeyEventSink,
                           public ITfCompositionSink,
                           public ITfThreadMgrEventSink
{
public:
    CPinyinTextService();
    ~CPinyinTextService();

    // ==================== IUnknown ====================
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ==================== ITfTextInputProcessor ====================
    STDMETHODIMP Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId);

    // ==================== ITfTextInputProcessorEx ====================
    STDMETHODIMP ActivateEx(ITfThreadMgr* pThreadMgr, TfClientId tfClientId, DWORD dwFlags);
    STDMETHODIMP Deactivate();

    // ==================== ITfKeyEventSink ====================
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext* pContext, REFGUID rguid, BOOL* pfEaten);

    // ==================== ITfCompositionSink ====================
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition);

    // ==================== ITfThreadMgrEventSink ====================
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDocMgr);
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr);
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus);
    STDMETHODIMP OnPushContext(ITfContext* pContext);
    STDMETHODIMP OnPopContext(ITfContext* pContext);

    // ==================== Accessors ====================
    ITfThreadMgr*       GetThreadMgr()          { return _pThreadMgr; }
    TfClientId          GetClientId()           { return _tfClientId; }
    ITfComposition*     GetComposition()        { return _pComposition; }
    CCandidateWindow*   GetCandidateWindow()    { return _pCandidateWindow; }
    CPinyinEngine*      GetPinyinEngine()       { return _pPinyinEngine; }
    std::wstring&               GetCompositionBuffer()  { return _compositionBuffer; }
    std::vector<std::wstring>&  GetCandidates()         { return _candidates; }
    int&                        GetSelectedIndex()       { return _selectedIndex; }
    PinyinCompositionState&      GetCompositionState()    { return _compositionState; }

    void SetComposition(ITfComposition* pComp);

    // ==================== Internal Operations ====================

    // Key input handlers (KeyEventSink.cpp)
    void _HandleCharInput(ITfContext* pContext, wchar_t ch);
    void _HandleCandidateSelection(ITfContext* pContext, int index);
    void _HandleBackspace(ITfContext* pContext);
    void _HandleEscape(ITfContext* pContext);
    void _HandleEnter(ITfContext* pContext);

    // Composition operations (Composition.cpp)
    void _StartComposition(ITfContext* pContext);
    void _UpdateComposition(ITfContext* pContext);
    void _CommitText(ITfContext* pContext, const std::wstring& text);
    void _ShowCandidateWindow(ITfContext* pContext);

    // State management (PinyinTextService.cpp)
    void _EndComposition();
    void _UpdateCandidates();
    void _UpdateCandidateWindow(ITfContext* pContext);

private:
    // KeyEventSink init/uninit (KeyEventSink.cpp)
    BOOL _InitKeyEventSink();
    void _UninitKeyEventSink();

    // ThreadMgrEventSink init/uninit (ThreadMgrEventSink.cpp)
    BOOL _InitThreadMgrEventSink();
    void _UninitThreadMgrEventSink();

    // ==================== Member variables ====================
    LONG                _refCount;
    ITfThreadMgr*       _pThreadMgr;
    TfClientId          _tfClientId;
    DWORD               _dwActivateFlags;

    // ThreadMgrEventSink cookie
    DWORD               _dwThreadMgrEventSinkCookie;

    // Composition state
    ITfComposition*     _pComposition;
    std::wstring        _compositionBuffer;
    PinyinCompositionState _compositionState;

    // Candidate window
    CCandidateWindow*   _pCandidateWindow;

    // Pinyin engine
    CPinyinEngine*      _pPinyinEngine;

    // Current candidates
    std::vector<std::wstring> _candidates;
    int                 _selectedIndex;
};

#endif // PINYIN_TEXT_SERVICE_H
