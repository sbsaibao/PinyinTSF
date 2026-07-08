# Shortcut Key Passthrough Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix PinyinTSF swallowing application/system shortcuts like Ctrl+C, Ctrl+V, and Alt+letter while preserving normal pinyin input behavior.

**Architecture:** Extract pure key-eating rules into `KeyEventLogic` so tests can cover shortcut passthrough without TSF COM setup. `KeyEventSink.cpp` will use the same helper from `OnTestKeyDown()` and defensively from `OnKeyDown()`.

**Tech Stack:** Windows TSF, C++17, MSBuild, Visual Studio v143, x64, existing console test project `tests\PinyinEngineTests.vcxproj`.

---

## File Structure

- Create: `src\KeyEventLogic.h` declares pure key decision helper.
- Create: `src\KeyEventLogic.cpp` implements pure key decision helper.
- Modify: `src\KeyEventSink.cpp` uses helper and reads Ctrl/Alt/Win state.
- Modify: `PinyinTSF.vcxproj` compiles new helper in the DLL.
- Modify: `PinyinTSF.vcxproj.filters` shows new helper in Visual Studio.
- Modify: `tests\PinyinEngineTests.cpp` adds shortcut passthrough unit tests.
- Modify: `tests\PinyinEngineTests.vcxproj` compiles new helper into tests.

## Task 1: Add RED Tests

**Files:**
- Modify: `tests\PinyinEngineTests.cpp`
- Modify: `tests\PinyinEngineTests.vcxproj`
- Create: `src\KeyEventLogic.h`
- Create: `src\KeyEventLogic.cpp`

- [ ] Add `#include "KeyEventLogic.h"` to `tests\PinyinEngineTests.cpp`.
- [ ] Add this helper after `g_failures`:

```cpp
static void ExpectBool(const std::wstring& name, bool actual, bool expected) {
    if (actual != expected) {
        std::wcerr << L"FAIL " << name << L": expected "
                   << (expected ? L"true" : L"false")
                   << L", got "
                   << (actual ? L"true" : L"false")
                   << L"\n";
        ++g_failures;
    }
}
```

- [ ] Add tests in `wmain()` after `TrayIcon::RequestShutdownExisting();`:

```cpp
    ExpectBool(L"plain letter is eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, false, false, false), true);
    ExpectBool(L"shift letter is still eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, false, false, false), true);
    ExpectBool(L"ctrl c is not eaten",
        KeyEventLogic::ShouldEatKeyDown('C', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl v is not eaten",
        KeyEventLogic::ShouldEatKeyDown('V', false, false, 0, true, false, false), false);
    ExpectBool(L"alt f is not eaten",
        KeyEventLogic::ShouldEatKeyDown('F', false, false, 0, false, true, false), false);
    ExpectBool(L"win l is not eaten",
        KeyEventLogic::ShouldEatKeyDown('L', false, false, 0, false, false, true), false);
    ExpectBool(L"number candidate is eaten during composition",
        KeyEventLogic::ShouldEatKeyDown('1', true, true, 5, false, false, false), true);
    ExpectBool(L"ctrl number is not eaten during composition",
        KeyEventLogic::ShouldEatKeyDown('1', true, true, 5, true, false, false), false);
    ExpectBool(L"backspace is eaten during composition",
        KeyEventLogic::ShouldEatKeyDown(VK_BACK, true, false, 0, false, false, false), true);
    ExpectBool(L"alt backspace is not eaten during composition",
        KeyEventLogic::ShouldEatKeyDown(VK_BACK, true, false, 0, false, true, false), false);
```

- [ ] Create `src\KeyEventLogic.h`:

```cpp
#pragma once

#ifndef KEY_EVENT_LOGIC_H
#define KEY_EVENT_LOGIC_H

#include <windows.h>

namespace KeyEventLogic {
    bool ShouldEatKeyDown(
        WPARAM key,
        bool hasComposition,
        bool hasCandidates,
        int candidateCount,
        bool ctrlDown,
        bool altDown,
        bool winDown);
}

#endif // KEY_EVENT_LOGIC_H
```

- [ ] Create `src\KeyEventLogic.cpp` with only `#include "KeyEventLogic.h"`.
- [ ] Add `..\src\KeyEventLogic.cpp` and `..\src\KeyEventLogic.h` to `tests\PinyinEngineTests.vcxproj`.
- [ ] Run `msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Debug /p:Platform=x64`; expected RED: unresolved external for `KeyEventLogic::ShouldEatKeyDown`.

## Task 2: Implement Pure Logic

**Files:**
- Modify: `src\KeyEventLogic.cpp`
- Test: `tests\PinyinEngineTests.cpp`

- [ ] Replace `src\KeyEventLogic.cpp` with:

```cpp
#include "KeyEventLogic.h"

namespace KeyEventLogic {

bool ShouldEatKeyDown(
    WPARAM key,
    bool hasComposition,
    bool hasCandidates,
    int candidateCount,
    bool ctrlDown,
    bool altDown,
    bool winDown) {
    if (ctrlDown || altDown || winDown) {
        return false;
    }

    if (key >= 'A' && key <= 'Z') {
        return true;
    }

    if (!hasComposition) {
        return false;
    }

    if (key >= '1' && key <= '5' && hasCandidates) {
        int index = static_cast<int>(key - '1');
        return index < candidateCount;
    }

    if (key == VK_BACK || key == VK_ESCAPE || key == VK_RETURN) {
        return true;
    }

    if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN) {
        return hasCandidates;
    }

    return false;
}

}
```

- [ ] Run `msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Debug /p:Platform=x64`; expected PASS build.
- [ ] Run `D:\PinyinTSF\tests\bin\Debug\PinyinEngineTests.exe`; expected `PinyinEngineTests passed`.

## Task 3: Use Logic in TSF Key Sink

**Files:**
- Modify: `src\KeyEventSink.cpp`
- Modify: `PinyinTSF.vcxproj`
- Modify: `PinyinTSF.vcxproj.filters`

- [ ] Include `KeyEventLogic.h` in `src\KeyEventSink.cpp`.
- [ ] Add local helpers `IsKeyDown`, `IsCtrlDown`, `IsAltDown`, `IsWinDown` using `GetKeyState()`.
- [ ] Replace `OnTestKeyDown()` body with a call to `KeyEventLogic::ShouldEatKeyDown(...)`.
- [ ] Add the same helper call at the start of `OnKeyDown()` and return `S_OK` when it says not to eat the key.
- [ ] Add `src\KeyEventLogic.cpp` and `src\KeyEventLogic.h` to `PinyinTSF.vcxproj`.
- [ ] Add `src\KeyEventLogic.cpp` and `src\KeyEventLogic.h` to `PinyinTSF.vcxproj.filters`.
- [ ] Run `msbuild PinyinTSF.sln /p:Configuration=Debug /p:Platform=x64`; expected PASS build.

## Task 4: Add Composition Shortcut Coverage

**Files:**
- Modify: `tests\PinyinEngineTests.cpp`

- [ ] Add tests for Ctrl+C, Ctrl+V, Alt+F, Ctrl+Right, and Alt+Enter during active composition.
- [ ] Run Debug test build and executable; expected `PinyinEngineTests passed`.

## Task 5: Release Verification

**Files:**
- Build: `PinyinTSF.sln`
- Build: `tests\PinyinEngineTests.vcxproj`

- [ ] Run `msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Release /p:Platform=x64`; expected PASS.
- [ ] Run `D:\PinyinTSF\tests\bin\Release\PinyinEngineTests.exe`; expected `PinyinEngineTests passed`.
- [ ] Run `msbuild PinyinTSF.sln /p:Configuration=Release /p:Platform=x64`; expected PASS. If DLL is locked, use `ReleaseCheck` output directory.

## Task 6: Regenerate Installer

**Files:**
- Build: `installer\PinyinTSF.iss`
- Output: `dist\PinyinTSF-Setup-x64.exe`

- [ ] Run `& "D:\Program Files (x86)\Inno Setup 6\ISCC.exe" "D:\PinyinTSF\installer\PinyinTSF.iss"`; expected `Successful compile`.
- [ ] Confirm `D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe` exists.

## Manual Verification

- [ ] Reinstall the input method with administrator rights.
- [ ] Verify Ctrl+C, Ctrl+V, Ctrl+A, Ctrl+Z, Alt+F, Alt+Space, and Win+Space are not converted into pinyin letters.
- [ ] Verify plain `nihao`, candidate selection, Backspace, Escape, Enter, and arrow navigation still work without Ctrl/Alt/Win.

## Commit Strategy

Current workspace is not a git repository. If moved into git later, use these commits:

```bash
git add src/KeyEventLogic.h src/KeyEventLogic.cpp tests/PinyinEngineTests.cpp tests/PinyinEngineTests.vcxproj
git commit -m "test: cover shortcut key passthrough"
git add src/KeyEventSink.cpp PinyinTSF.vcxproj PinyinTSF.vcxproj.filters
git commit -m "fix: pass through shortcut key combinations"
git add installer/PinyinTSF.iss dist/PinyinTSF-Setup-x64.exe
git commit -m "build: regenerate installer"
```
