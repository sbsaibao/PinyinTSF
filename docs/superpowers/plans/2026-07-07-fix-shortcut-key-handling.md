# Shortcut Key Handling Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let normal Windows/application shortcuts such as `Ctrl+C`, `Ctrl+V`, `Ctrl+A`, `Alt+F`, and Windows-key combinations pass through while PinyinTSF still handles plain pinyin typing and composition keys.

**Architecture:** Move key swallowing rules out of `CPinyinTextService::OnTestKeyDown` into a small pure helper that can be tested without TSF COM setup. `KeyEventSink.cpp` reads modifier state, asks the helper whether the key should be eaten, and only handles keys that the helper allows. This keeps TSF-specific code thin and makes shortcut behavior regression-testable.

**Tech Stack:** Windows TSF `ITfKeyEventSink`, C++17, Visual Studio v143, MSBuild, existing console test project `tests/PinyinEngineTests.vcxproj`, Inno Setup for release packaging.

---

## Root Cause Summary

Current problem is in `D:\PinyinTSF\src\KeyEventSink.cpp`:

```cpp
// Letter keys A-Z: always eat for pinyin input
if (wParam >= 'A' && wParam <= 'Z') {
    *pfEaten = TRUE;
    return S_OK;
}
```

This treats `Ctrl+C`, `Ctrl+V`, `Ctrl+A`, `Alt+F`, and similar shortcuts as ordinary letter input because TSF receives the same `wParam` letter value with modifier keys down. The input method consumes the key before the foreground application sees the shortcut.

## File Structure

Files to create or modify:

- Modify or confirm: `D:\PinyinTSF\src\KeyEventLogic.h`
  - Responsibility: declare pure key handling policy with no COM dependencies.
- Modify or confirm: `D:\PinyinTSF\src\KeyEventLogic.cpp`
  - Responsibility: implement key swallowing policy for plain input vs shortcut combinations.
- Modify: `D:\PinyinTSF\src\KeyEventSink.cpp`
  - Responsibility: read live modifier state and apply `KeyEventLogic::ShouldEatKeyDown()` consistently in `OnTestKeyDown` and `OnKeyDown`.
- Modify: `D:\PinyinTSF\PinyinTSF.vcxproj`
  - Responsibility: compile and include `KeyEventLogic.cpp/.h` in the main DLL project.
- Modify: `D:\PinyinTSF\PinyinTSF.vcxproj.filters`
  - Responsibility: show `KeyEventLogic.cpp/.h` under the TSF filter in Visual Studio.
- Modify: `D:\PinyinTSF\tests\PinyinEngineTests.cpp`
  - Responsibility: add policy tests for shortcut pass-through and existing composition key behavior.
- Modify or confirm: `D:\PinyinTSF\tests\PinyinEngineTests.vcxproj`
  - Responsibility: compile `KeyEventLogic.cpp` into the console test executable.
- Rebuild: `D:\PinyinTSF\installer\PinyinTSF.iss`
  - Responsibility: no script behavior change required; rebuild installer after Release DLL changes.

## Expected Behavior

Shortcut keys that must pass through with `pfEaten = FALSE`:

```text
Ctrl+C
Ctrl+V
Ctrl+X
Ctrl+A
Ctrl+Z
Ctrl+Y
Ctrl+S
Alt+F
Alt+Tab
Alt+Space
Win+L
Win+Space
Ctrl+1 during composition
Alt+Backspace during composition
```

Keys that must still be handled by the input method with no Ctrl/Alt/Win modifier:

```text
A-Z plain letters: start/update pinyin composition
Shift+A-Z: still input pinyin letters, same as lowercase pinyin input
1-5 during active composition with enough candidates: select candidate
Backspace during active composition: delete pinyin input
Escape during active composition: cancel composition
Enter during active composition: commit raw pinyin
Arrow keys during active composition with candidates: move candidate selection
```

## Task 1: Lock Shortcut Policy With Pure Tests

**Files:**

- Modify: `D:\PinyinTSF\tests\PinyinEngineTests.cpp`
- Modify or confirm: `D:\PinyinTSF\tests\PinyinEngineTests.vcxproj`
- Create or confirm: `D:\PinyinTSF\src\KeyEventLogic.h`
- Create or confirm: `D:\PinyinTSF\src\KeyEventLogic.cpp`

- [ ] **Step 1: Ensure the test imports the key policy header**

In `D:\PinyinTSF\tests\PinyinEngineTests.cpp`, the include block must contain:

```cpp
#include "PinyinEngine.h"
#include "PinyinCompositionState.h"
#include "TrayIcon.h"
#include "KeyEventLogic.h"

#include <iostream>
#include <string>
#include <vector>
```

- [ ] **Step 2: Ensure the test file has a boolean assertion helper**

In `D:\PinyinTSF\tests\PinyinEngineTests.cpp`, place this helper near `g_failures`:

```cpp
static int g_failures = 0;

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

- [ ] **Step 3: Write the failing shortcut behavior tests**

In `wmain()` in `D:\PinyinTSF\tests\PinyinEngineTests.cpp`, immediately after `TrayIcon::RequestShutdownExisting();`, insert or confirm this block:

```cpp
    ExpectBool(L"plain letter is eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, false, false, false), true);
    ExpectBool(L"plain v is eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('V', false, false, 0, false, false, false), true);
    ExpectBool(L"ctrl c is not eaten",
        KeyEventLogic::ShouldEatKeyDown('C', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl v is not eaten",
        KeyEventLogic::ShouldEatKeyDown('V', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl a is not eaten",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, true, false, false), false);
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

- [ ] **Step 4: Run the test build to verify it fails before implementation**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Debug /p:Platform=x64
```

Expected before `KeyEventLogic` exists or before it has modifier logic:

```text
error C1083: Cannot open include file: 'KeyEventLogic.h'
```

or:

```text
FAIL ctrl c is not eaten: expected false, got true
```

If the test already passes because `KeyEventLogic` exists, continue to Task 2 and use this test as the regression guard.

- [ ] **Step 5: Commit or record skipped commit**

Run:

```powershell
git status --short
```

Expected in the current workspace:

```text
fatal: not a git repository (or any of the parent directories): .git
```

Because this workspace is currently not a git repo, do not run `git add` or `git commit`. If the project is later initialized as git, commit this task with:

```powershell
git add tests\PinyinEngineTests.cpp tests\PinyinEngineTests.vcxproj src\KeyEventLogic.h src\KeyEventLogic.cpp
git commit -m "test: cover shortcut key pass-through policy"
```

## Task 2: Implement Pure Key Handling Policy

**Files:**

- Create or replace: `D:\PinyinTSF\src\KeyEventLogic.h`
- Create or replace: `D:\PinyinTSF\src\KeyEventLogic.cpp`

- [ ] **Step 1: Write the policy header**

Set `D:\PinyinTSF\src\KeyEventLogic.h` to exactly:

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

- [ ] **Step 2: Write the policy implementation**

Set `D:\PinyinTSF\src\KeyEventLogic.cpp` to exactly:

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

- [ ] **Step 3: Run the policy tests**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Debug /p:Platform=x64
D:\PinyinTSF\tests\bin\Debug\PinyinEngineTests.exe
```

Expected:

```text
PinyinEngineTests passed
```

- [ ] **Step 4: Commit or record skipped commit**

Run:

```powershell
git status --short
```

Expected in the current workspace:

```text
fatal: not a git repository (or any of the parent directories): .git
```

If the project is later initialized as git, commit this task with:

```powershell
git add src\KeyEventLogic.h src\KeyEventLogic.cpp tests\PinyinEngineTests.cpp tests\PinyinEngineTests.vcxproj
git commit -m "feat: add shortcut-aware key handling policy"
```

## Task 3: Wire Policy Into TSF Key Event Sink

**Files:**

- Modify: `D:\PinyinTSF\src\KeyEventSink.cpp`

- [ ] **Step 1: Include the policy header**

At the top of `D:\PinyinTSF\src\KeyEventSink.cpp`, change the include block to:

```cpp
#include "PinyinTextService.h"
#include "EditSession.h"
#include "KeyEventLogic.h"
```

- [ ] **Step 2: Add local modifier helper**

After the include block in `D:\PinyinTSF\src\KeyEventSink.cpp`, add:

```cpp
namespace {
bool IsKeyDown(int vkey) {
    return (GetKeyState(vkey) & 0x8000) != 0;
}
}
```

- [ ] **Step 3: Replace `OnTestKeyDown` with policy-based logic**

Replace the entire `CPinyinTextService::OnTestKeyDown` function in `D:\PinyinTSF\src\KeyEventSink.cpp` with:

```cpp
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
```

- [ ] **Step 4: Add the same policy guard at the top of `OnKeyDown`**

In `CPinyinTextService::OnKeyDown`, immediately after `*pfEaten = FALSE;`, add:

```cpp
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
```

This keeps `OnKeyDown` safe if TSF calls it even when `OnTestKeyDown` returned `FALSE`, or if modifier state changes between the test and actual key event.

- [ ] **Step 5: Do not change composition handlers**

Confirm these functions stay unchanged in `D:\PinyinTSF\src\KeyEventSink.cpp`:

```cpp
void CPinyinTextService::_HandleCharInput(ITfContext* pContext, wchar_t ch)
void CPinyinTextService::_HandleCandidateSelection(ITfContext* pContext, int index)
void CPinyinTextService::_HandleBackspace(ITfContext* pContext)
void CPinyinTextService::_HandleEscape(ITfContext* pContext)
void CPinyinTextService::_HandleEnter(ITfContext* pContext)
```

Reason: this fix is only about deciding whether to consume shortcuts. Composition behavior should not be rewritten in the same task.

- [ ] **Step 6: Build Debug DLL**

Run:

```powershell
msbuild PinyinTSF.sln /p:Configuration=Debug /p:Platform=x64
```

Expected:

```text
已成功生成。
    0 个警告
    0 个错误
```

- [ ] **Step 7: Commit or record skipped commit**

Run:

```powershell
git status --short
```

Expected in the current workspace:

```text
fatal: not a git repository (or any of the parent directories): .git
```

If the project is later initialized as git, commit this task with:

```powershell
git add src\KeyEventSink.cpp
git commit -m "fix: pass through ctrl alt and win shortcuts"
```

## Task 4: Add KeyEventLogic To Main Project

**Files:**

- Modify: `D:\PinyinTSF\PinyinTSF.vcxproj`
- Modify: `D:\PinyinTSF\PinyinTSF.vcxproj.filters`

- [ ] **Step 1: Add source file to main project**

In `D:\PinyinTSF\PinyinTSF.vcxproj`, inside the `<ItemGroup>` that contains `src\KeyEventSink.cpp`, add:

```xml
    <ClCompile Include="src\KeyEventLogic.cpp" />
```

The relevant block should include both files:

```xml
    <ClCompile Include="src\PinyinTextService.cpp" />
    <ClCompile Include="src\KeyEventSink.cpp" />
    <ClCompile Include="src\KeyEventLogic.cpp" />
    <ClCompile Include="src\ThreadMgrEventSink.cpp" />
```

- [ ] **Step 2: Add header file to main project**

In `D:\PinyinTSF\PinyinTSF.vcxproj`, inside the `<ItemGroup>` that contains `src\PinyinTextService.h`, add:

```xml
    <ClInclude Include="src\KeyEventLogic.h" />
```

The relevant block should include:

```xml
    <ClInclude Include="src\PinyinTextService.h" />
    <ClInclude Include="src\KeyEventLogic.h" />
    <ClInclude Include="src\EditSession.h" />
```

- [ ] **Step 3: Add source file to Visual Studio filters**

In `D:\PinyinTSF\PinyinTSF.vcxproj.filters`, inside the source `<ItemGroup>`, add:

```xml
    <ClCompile Include="src\KeyEventLogic.cpp">
      <Filter>Source Files\TSF</Filter>
    </ClCompile>
```

- [ ] **Step 4: Add header file to Visual Studio filters**

In `D:\PinyinTSF\PinyinTSF.vcxproj.filters`, inside the header `<ItemGroup>`, add:

```xml
    <ClInclude Include="src\KeyEventLogic.h">
      <Filter>Header Files</Filter>
    </ClInclude>
```

- [ ] **Step 5: Build Debug DLL**

Run:

```powershell
msbuild PinyinTSF.sln /p:Configuration=Debug /p:Platform=x64
```

Expected:

```text
已成功生成。
    0 个警告
    0 个错误
```

- [ ] **Step 6: Commit or record skipped commit**

Run:

```powershell
git status --short
```

Expected in the current workspace:

```text
fatal: not a git repository (or any of the parent directories): .git
```

If the project is later initialized as git, commit this task with:

```powershell
git add PinyinTSF.vcxproj PinyinTSF.vcxproj.filters
git commit -m "build: include key event policy in main dll"
```

## Task 5: Full Verification And Manual Shortcut Test

**Files:**

- Verify: `D:\PinyinTSF\tests\bin\Debug\PinyinEngineTests.exe`
- Verify: `D:\PinyinTSF\tests\bin\Release\PinyinEngineTests.exe`
- Verify: `D:\PinyinTSF\bin\Debug\PinyinTSF.dll`
- Verify: `D:\PinyinTSF\bin\Release\PinyinTSF.dll`

- [ ] **Step 1: Build and run Debug tests**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Debug /p:Platform=x64
D:\PinyinTSF\tests\bin\Debug\PinyinEngineTests.exe
```

Expected:

```text
PinyinEngineTests passed
```

- [ ] **Step 2: Build and run Release tests**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Release /p:Platform=x64
D:\PinyinTSF\tests\bin\Release\PinyinEngineTests.exe
```

Expected:

```text
PinyinEngineTests passed
```

- [ ] **Step 3: Build Debug DLL**

Run:

```powershell
msbuild PinyinTSF.sln /p:Configuration=Debug /p:Platform=x64
```

Expected:

```text
已成功生成。
    0 个警告
    0 个错误
```

- [ ] **Step 4: Build Release DLL**

Run:

```powershell
msbuild PinyinTSF.sln /p:Configuration=Release /p:Platform=x64
```

Expected:

```text
已成功生成。
    0 个警告
    0 个错误
```

If this fails because `D:\PinyinTSF\bin\Release\PinyinTSF.dll` is locked by `explorer.exe`, run temporary Release verification:

```powershell
msbuild PinyinTSF.vcxproj /p:Configuration=Release /p:Platform=x64 /p:OutDir=D:\PinyinTSF\bin\ReleaseCheck\ /p:IntDir=D:\PinyinTSF\obj\ReleaseCheck\
```

Expected:

```text
已成功生成。
    0 个警告
    0 个错误
```

- [ ] **Step 5: Install or register the fixed DLL for manual testing**

If Release built successfully, run from an elevated terminal:

```cmd
D:\PinyinTSF\uninstall.bat
D:\PinyinTSF\install.bat
```

Expected install output includes:

```text
Installation successful!
```

- [ ] **Step 6: Manually test shortcuts in Notepad**

Open Notepad, switch to PinyinTSF, and run these exact checks:

```text
Type nihao: candidate/composition appears.
Press Ctrl+A: Notepad selects all text, input method does not insert a.
Press Ctrl+C: selected text copies, input method does not insert c.
Press Ctrl+V: clipboard text pastes, input method does not insert v.
Press Alt+F: Notepad File menu opens, input method does not insert f.
Press Alt+Space: window system menu opens.
Press Win+Space: Windows input method switcher opens.
Type nihao again: pinyin input still works.
During nihao composition press 1: first candidate selection still works.
During nihao composition press Ctrl+1: application receives shortcut, candidate is not selected.
```

- [ ] **Step 7: Commit or record skipped commit**

Run:

```powershell
git status --short
```

Expected in the current workspace:

```text
fatal: not a git repository (or any of the parent directories): .git
```

If the project is later initialized as git, commit verification-only changes are not needed. If any verification fixes were made, commit them with:

```powershell
git add src\KeyEventSink.cpp src\KeyEventLogic.h src\KeyEventLogic.cpp PinyinTSF.vcxproj PinyinTSF.vcxproj.filters tests\PinyinEngineTests.cpp tests\PinyinEngineTests.vcxproj
git commit -m "fix: preserve app shortcuts while using pinyin input"
```

## Task 6: Rebuild Installer With Fixed Release DLL

**Files:**

- Verify: `D:\PinyinTSF\installer\PinyinTSF.iss`
- Output: `D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe`

- [ ] **Step 1: Confirm Inno Setup compiler path**

Run:

```powershell
Test-Path -LiteralPath "D:\Program Files (x86)\Inno Setup 6\ISCC.exe"
```

Expected:

```text
True
```

- [ ] **Step 2: Build the installer**

Run:

```powershell
& "D:\Program Files (x86)\Inno Setup 6\ISCC.exe" "D:\PinyinTSF\installer\PinyinTSF.iss"
```

Expected:

```text
Successful compile
D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe
```

- [ ] **Step 3: Confirm installer timestamp**

Run:

```powershell
Get-Item -LiteralPath "D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe" | Format-Table FullName, Length, LastWriteTime -AutoSize
```

Expected:

```text
FullName                                   Length LastWriteTime
--------                                   ------ -------------
D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe <non-zero> <current time>
```

- [ ] **Step 4: Manual installer smoke test**

Run `D:\PinyinTSF\dist\PinyinTSF-Setup-x64.exe` as administrator.

Expected:

```text
Installation completes without error.
PinyinTSF appears in Windows input methods.
Shortcut manual tests from Task 5 still pass.
```

- [ ] **Step 5: Manual uninstaller smoke test**

Run the installed uninstaller:

```text
C:\Program Files\PinyinTSF\unins000.exe
```

Expected:

```text
PinyinTSF is unregistered.
Tray icon exits after uninstall.
HKCU\Software\PinyinTSF is removed for the installing user.
```

## Self-Review

Spec coverage:

- `Ctrl+C` and `Ctrl+V` pass-through: Task 1 tests and Task 3 implementation.
- `Alt+other keys` pass-through: Task 1 tests and Task 3 implementation.
- Existing pinyin typing still works: Task 1 tests plain letters and Task 5 manual `nihao` check.
- Candidate selection still works: Task 1 tests number candidate, Task 5 manual composition check.
- Installer updated after fix: Task 6 rebuilds existing Inno Setup package.

Placeholder scan:

- No `TBD`, `TODO`, `implement later`, or unspecified edge handling remains in this plan.

Type consistency:

- Function name is consistently `KeyEventLogic::ShouldEatKeyDown`.
- Signature is consistently `WPARAM, bool, bool, int, bool, bool, bool`.
- Project file entries consistently use `src\KeyEventLogic.cpp` and `src\KeyEventLogic.h`.
