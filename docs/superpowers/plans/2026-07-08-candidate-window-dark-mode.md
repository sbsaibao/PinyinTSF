# Candidate Window Dark Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add candidate window dark mode that follows the Windows app theme by default, and add a manual theme override control directly into the existing tray settings dialog.

**Architecture:** Persist a three-state theme mode in `SettingsManager`, resolve the effective candidate window theme from either the saved override or the current Windows app theme, and drive both runtime painting and the settings preview from the same light/dark palette logic. Keep layout and TSF behavior unchanged; only theme state, resource dialog controls, preview painting, and candidate window color resources change.

**Tech Stack:** Win32 dialog resources, GDI drawing, Windows registry, TSF COM DLL, C++17, MSBuild, existing `tests\PinyinEngineTests.vcxproj`.

---

## Task 1: Add Failing Tests For Theme Mode Resolution

**Files:**
- Modify: `D:\PinyinTSF\tests\PinyinEngineTests.cpp`
- Modify: `D:\PinyinTSF\src\SettingsManager.h`

- [ ] **Step 1: Write the failing test calls**

In `D:\PinyinTSF\tests\PinyinEngineTests.cpp`, after the existing key handling assertions, add:

```cpp
    ExpectBool(L"follow system resolves to dark when system dark",
        SettingsManager::ResolveEffectiveDarkMode(SettingsManager::ThemeMode::FollowSystem, true), true);
    ExpectBool(L"follow system resolves to light when system light",
        SettingsManager::ResolveEffectiveDarkMode(SettingsManager::ThemeMode::FollowSystem, false), false);
    ExpectBool(L"light override resolves to light",
        SettingsManager::ResolveEffectiveDarkMode(SettingsManager::ThemeMode::Light, true), false);
    ExpectBool(L"dark override resolves to dark",
        SettingsManager::ResolveEffectiveDarkMode(SettingsManager::ThemeMode::Dark, false), true);
    ExpectBool(L"invalid stored theme falls back to follow system",
        SettingsManager::NormalizeThemeModeValue(99) == SettingsManager::ThemeMode::FollowSystem, true);
```

- [ ] **Step 2: Add the settings header include**

At the top of `D:\PinyinTSF\tests\PinyinEngineTests.cpp`, ensure this include exists:

```cpp
#include "SettingsManager.h"
```

- [ ] **Step 3: Run the Release test build to verify RED**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Release /p:Platform=x64
```

Expected before implementation:

```text
error C2039: 'ResolveEffectiveDarkMode': is not a member of 'SettingsManager'
```

## Task 2: Implement Theme Mode Persistence In SettingsManager

**Files:**
- Modify: `D:\PinyinTSF\src\SettingsManager.h`
- Modify: `D:\PinyinTSF\src\SettingsManager.cpp`

- [ ] **Step 1: Add the theme mode enum and helpers**

In `D:\PinyinTSF\src\SettingsManager.h`, add:

```cpp
    enum class ThemeMode : DWORD {
        FollowSystem = 0,
        Light = 1,
        Dark = 2,
    };
```

and add these public APIs:

```cpp
    ThemeMode GetThemeMode() const { return _themeMode; }
    void SetThemeMode(ThemeMode mode) { _themeMode = NormalizeThemeModeValue((DWORD)mode); }

    static ThemeMode NormalizeThemeModeValue(DWORD value);
    static bool ResolveEffectiveDarkMode(ThemeMode mode, bool systemDark);
```

and add this private member:

```cpp
    ThemeMode _themeMode;
```

- [ ] **Step 2: Initialize, load, and save the theme mode**

In `D:\PinyinTSF\src\SettingsManager.cpp`, initialize `_themeMode` to `ThemeMode::FollowSystem`, then add:

```cpp
SettingsManager::ThemeMode SettingsManager::NormalizeThemeModeValue(DWORD value) {
    switch (value) {
    case (DWORD)ThemeMode::FollowSystem:
        return ThemeMode::FollowSystem;
    case (DWORD)ThemeMode::Light:
        return ThemeMode::Light;
    case (DWORD)ThemeMode::Dark:
        return ThemeMode::Dark;
    default:
        return ThemeMode::FollowSystem;
    }
}

bool SettingsManager::ResolveEffectiveDarkMode(ThemeMode mode, bool systemDark) {
    switch (mode) {
    case ThemeMode::Light:
        return false;
    case ThemeMode::Dark:
        return true;
    case ThemeMode::FollowSystem:
    default:
        return systemDark;
    }
}
```

Also update `Load()` and `Save()` to read and write a `ThemeMode` DWORD value.

- [ ] **Step 3: Run the Release test build and test executable to verify GREEN**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Release /p:Platform=x64
tests\bin\Release\PinyinEngineTests.exe
```

Expected:

```text
PinyinEngineTests passed
```

## Task 3: Add Theme Controls To The Existing Settings Dialog

**Files:**
- Modify: `D:\PinyinTSF\src\resource.h`
- Modify: `D:\PinyinTSF\PinyinTSF.rc`
- Modify: `D:\PinyinTSF\src\SettingsDialog.cpp`

- [ ] **Step 1: Add control IDs**

In `D:\PinyinTSF\src\resource.h`, add:

```cpp
#define IDC_THEME_SYSTEM        1008
#define IDC_THEME_LIGHT         1009
#define IDC_THEME_DARK          1010
```

- [ ] **Step 2: Add the radio buttons to the existing dialog resource**

In `D:\PinyinTSF\PinyinTSF.rc`, add a theme section above preview:

```rc
    GROUPBOX        L"主题", -1, 14, 74, 270, 42
    CONTROL         L"跟随系统", IDC_THEME_SYSTEM, L"Button", BS_AUTORADIOBUTTON | WS_TABSTOP, 24, 88, 70, 12
    CONTROL         L"浅色", IDC_THEME_LIGHT, L"Button", BS_AUTORADIOBUTTON, 110, 88, 40, 12
    CONTROL         L"深色", IDC_THEME_DARK, L"Button", BS_AUTORADIOBUTTON, 166, 88, 40, 12
```

and move the preview group and buttons downward enough to avoid overlap.

- [ ] **Step 3: Initialize the radio buttons from settings**

In `D:\PinyinTSF\src\SettingsDialog.cpp`, inside `WM_INITDIALOG`, set the checked radio based on `sm.GetThemeMode()`.

- [ ] **Step 4: Save the selected theme mode**

In the `IDC_BTN_SAVE` branch, read the checked radio and call:

```cpp
            if (IsDlgButtonChecked(hDlg, IDC_THEME_LIGHT) == BST_CHECKED) {
                sm.SetThemeMode(SettingsManager::ThemeMode::Light);
            } else if (IsDlgButtonChecked(hDlg, IDC_THEME_DARK) == BST_CHECKED) {
                sm.SetThemeMode(SettingsManager::ThemeMode::Dark);
            } else {
                sm.SetThemeMode(SettingsManager::ThemeMode::FollowSystem);
            }
```

- [ ] **Step 5: Reset should return theme mode to follow system**

In the `IDC_BTN_RESET` branch, re-check:

```cpp
            CheckRadioButton(hDlg, IDC_THEME_SYSTEM, IDC_THEME_DARK, IDC_THEME_SYSTEM);
```

and invalidate the preview.

## Task 4: Make The Settings Preview Use The Selected Theme

**Files:**
- Modify: `D:\PinyinTSF\src\SettingsDialog.cpp`

- [ ] **Step 1: Add a small helper to resolve preview theme**

Inside `D:\PinyinTSF\src\SettingsDialog.cpp`, add a helper that reads the currently checked radio buttons and resolves whether preview should render dark, using `SettingsManager::ResolveEffectiveDarkMode(...)`.

- [ ] **Step 2: Paint both preview themes from one code path**

In `_DrawPreview(...)`, replace the hard-coded light colors with theme-dependent colors, at minimum for:

```cpp
background
container
container border
divider
selected background
primary text
secondary text
```

- [ ] **Step 3: Refresh preview on theme radio changes**

In `WM_COMMAND`, when `IDC_THEME_SYSTEM`, `IDC_THEME_LIGHT`, or `IDC_THEME_DARK` is clicked, call:

```cpp
            InvalidateRect(GetDlgItem(hDlg, IDC_PREVIEW), nullptr, FALSE);
```

## Task 5: Make The Runtime Candidate Window Use The Same Theme Resolution

**Files:**
- Modify: `D:\PinyinTSF\src\CandidateWindow.h`
- Modify: `D:\PinyinTSF\src\CandidateWindow.cpp`

- [ ] **Step 1: Add runtime palette state**

In `D:\PinyinTSF\src\CandidateWindow.h`, add a palette struct and helpers for:

```cpp
_ResolveEffectiveDarkMode()
_RefreshThemeResources()
_ReleaseThemeResources()
```

- [ ] **Step 2: Replace hard-coded theme macros with runtime palette values**

Stop using the current `CW_COLOR_*` macros for drawing decisions. Use palette values for:

```cpp
window background
container fill
container border
divider
selected fill
selected border
primary text
secondary text
```

- [ ] **Step 3: Make shadow blending background-aware**

Replace the white-only shadow blend helper with one that blends shadow color toward the current background color.

- [ ] **Step 4: Refresh resources in create/show/reload**

Ensure `Create()` builds theme resources once, `Show()` refreshes them before painting, and `ReloadSettings()` refreshes theme resources after `SettingsManager::Load()`.

## Task 6: Verification

**Files:**
- Verify: `D:\PinyinTSF\src\CandidateWindow.cpp`
- Verify: `D:\PinyinTSF\src\SettingsDialog.cpp`
- Verify: `D:\PinyinTSF\bin\Release\PinyinTSF.dll`

- [ ] **Step 1: Run test project**

Run:

```powershell
msbuild tests\PinyinEngineTests.vcxproj /p:Configuration=Release /p:Platform=x64
tests\bin\Release\PinyinEngineTests.exe
```

Expected:

```text
PinyinEngineTests passed
```

- [ ] **Step 2: Build the DLL**

Run:

```powershell
msbuild PinyinTSF.vcxproj /p:Configuration=Release /p:Platform=x64
```

If the output DLL is locked, run:

```powershell
msbuild PinyinTSF.vcxproj /p:Configuration=Release /p:Platform=x64 /p:OutDir="C:\Users\xukun\AppData\Local\Temp\opencode\PinyinTSF-build\"
```

- [ ] **Step 3: Manual smoke test**

Check these exact behaviors:

```text
1. 托盘设置里可以看到“跟随系统 / 浅色 / 深色”。
2. 预览区切换三种选项时立即变化。
3. 跟随系统 + Windows 浅色主题：候选框是浅色。
4. 跟随系统 + Windows 深色主题：候选框是深色。
5. 手动浅色覆盖：即使系统深色，候选框仍是浅色。
6. 手动深色覆盖：即使系统浅色，候选框仍是深色。
7. 数字选词、回车提交、Esc 取消和方向键移动没有回归。
```

## Self-Review

Spec coverage:

- 自动跟随系统主题：Task 2 + Task 5。
- 原设置页加手动覆盖按钮：Task 3。
- 设置预览同步主题：Task 4。
- 运行时候选框真正变深色：Task 5。
- 构建与手动验证：Task 6。

Placeholder scan:

- 无 `TBD`、`TODO`、`稍后处理` 类占位项。

Type consistency:

- 统一使用 `SettingsManager::ThemeMode`。
- 统一使用 `ResolveEffectiveDarkMode(...)` 解析有效深色状态。
