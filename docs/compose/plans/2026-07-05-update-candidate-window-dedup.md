# Update Candidate Window Dedup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Confirm and record completion of the candidate-window show/hide deduplication item.

**Architecture:** `KeyEventSink.cpp` should delegate candidate-window visibility decisions to `CPinyinTextService::_UpdateCandidateWindow`. `PinyinTextService.cpp` owns the shared show/hide decision so character input and backspace paths do not duplicate it.

**Tech Stack:** C++17, Win32 TSF COM component, Visual Studio 2022 `msbuild`.

## Global Constraints

Use Chinese for user-facing status.
Keep changes surgical and do not refactor adjacent code.
Do not manipulate TSF text directly outside edit sessions.
Verify with source search and Release x64 build because the project has no tests or linters.

---

### Task 1: Candidate Window Dedup Completion

**Covers:** todo item 1, `_UpdateCandidateWindow` duplicate show/hide logic.

**Files:**
- Modify: `docs/todo list.md:12`
- Inspect: `src/KeyEventSink.cpp:196-239`
- Inspect: `src/PinyinTextService.cpp:209-218`

**Interfaces:**
- Consumes: `void CPinyinTextService::_UpdateCandidateWindow(ITfContext* pContext)` declared in `src/PinyinTextService.h`.
- Produces: Completed todo entry showing the code already delegates visibility decisions through `_UpdateCandidateWindow`.

- [x] **Step 1: Plan**

Confirm the target state:

```cpp
void CPinyinTextService::_HandleCharInput(ITfContext* pContext, wchar_t ch) {
    _compositionBuffer += ch;

    if (!_pComposition) {
        _StartComposition(pContext);
    }

    _UpdateComposition(pContext);
    _UpdateCandidates();
    _UpdateCandidateWindow(pContext);
}

void CPinyinTextService::_HandleBackspace(ITfContext* pContext) {
    if (_compositionBuffer.empty()) return;

    _compositionBuffer.pop_back();

    if (_compositionBuffer.empty()) {
        _CommitText(pContext, L"");
    } else {
        _UpdateComposition(pContext);
        _UpdateCandidates();
        _UpdateCandidateWindow(pContext);
    }
}
```

- [x] **Step 2: Develop**

No production code change is needed because both paths already call the shared helper. Update the todo item to checked.

- [x] **Step 3: Review**

Check that `_ShowCandidateWindow(pContext)` and `_pCandidateWindow->Hide()` are not duplicated inside `_HandleCharInput` or `_HandleBackspace`.

- [ ] **Step 4: Test**

Run: `msbuild PinyinTSF.sln /p:Configuration=Release /p:Platform=x64`
Expected: build succeeds with `0 Error(s)`.

- [ ] **Step 5: Debug**

If the build fails, inspect the first compiler or linker error, make the smallest fix, and rerun the same build command until it succeeds.
