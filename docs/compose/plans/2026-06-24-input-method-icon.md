# Input Method Icon Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace all UI-visible icons in the TSF input method with `icon.png` so the system tray and the Windows input-switch UI both display the new brand icon.

**Architecture:** Keep a single source image (`icon.png`) at the project root. At build time, generate an `.ico` from it and copy the icon next to the DLL so runtime code can load it. In source, update every site that currently references `app_icon.ico`:
- TSF profile registration (`Register.cpp`) tells Windows which icon file/index to show in Win+Space and language settings.
- System tray (`TrayIcon.cpp`) loads an icon file at runtime via `LoadImageW`.
This is purely asset wiring; no new COM interfaces or runtime behavior changes are needed.

**Tech Stack:** Windows TSF APIs, Win32 resource/tooling, MSBuild copy step, ICO generation tool (e.g. ImageMagick or equivalent).

---

### Task 1: Add icon source asset and build-friendly ICO

**Covers:** (none)

**Files:**
- Modify: `PinyinTSF.vcxproj`
- Modify: `PinyinTSF.rc` (optional if embedding)
- Modify: `res/app_icon.ico` (regenerated)

- [ ] **Step 1: Decide generation strategy**

Two viable approaches:
- Pre-generate `res/app_icon.ico` once and commit it.
- Add a build script that converts `icon.png` to `res/app_icon.ico` on every build.

If repository size and reproducibility matter, prefer committing the generated ICO and keeping `icon.png` as the canonical source (check `res/app_icon.ico` into Git).

- [ ] **Step 2: Generate the ICO**

From a developer shell, convert the PNG to multi-size ICO (at least 16/32/48; optionally 256). Use an available tool:
```bash
magick icon.png -resize 256x256 -define icon:auto-resize="256,48,32,16" res/app_icon.ico
```
If ImageMagick is unavailable, use another local ICO generator that supports multiple sizes.

- [ ] **Step 3: Verify the generated file**

Open `res/app_icon.ico` in Explorer or an ICO viewer and confirm all required sizes exist.

- [ ] **Step 4: Keep vcxproj copy rule**

`PinyinTSF.vcxproj` already copies `res/app_icon.ico` to the output directory as `app_icon.ico`. Ensure this rule remains (currently at `PinyinTSF.vcxproj:130-133`).

---

### Task 2: Update TSF profile icon registration

**Covers:** (none)

**Files:**
- Modify: `src/Register.cpp`

- [ ] **Step 1: Confirm current profile icon path**

`RegisterProfiles()` constructs `achIconFile` as `<DLL dir>\app_icon.ico` (`src/Register.cpp:110-115`).

- [ ] **Step 2: Keep same filename**

Because the build already copies the regenerated ICO as `app_icon.ico`, no filename change is required. If you prefer the output file to be `pinyintfs.ico`, update both:
- `Register.cpp` path construction
- `PinyinTSF.vcxproj` `<Link>` name

- [ ] **Step 3: Re-run registration**

After installing the new build:
```bash
install.bat
```
and reboot explorer or re-login. Verify the icon in:
- Win+Space input switcher
- Settings → Time & language → Language → Input method list

If the icon still shows the old image, run:
```bash
uninstall.bat
install.bat
```
and confirm again.

---

### Task 3: Update system tray icon loading

**Covers:** (none)

**Files:**
- Modify: `src/TrayIcon.cpp`

- [ ] **Step 1: Confirm tray loads from file**

`TrayIcon.cpp:31-42` constructs a path to `app_icon.ico` next to the DLL and calls `LoadImageW` with `LR_LOADFROMFILE`.

- [ ] **Step 2: Keep filename consistent**

Because we regenerate `res/app_icon.ico` into the same output filename, no code change is required unless you renamed the output ICO.

- [ ] **Step 3: Verify tray icon**

Run the input method, right-click or left-click the tray icon, and confirm the new icon appears. If it doesn’t:
1. Confirm the DLL directory contains `app_icon.ico` (from build output).
2. Restart `explorer.exe` or sign out/in.

---

### Task 4: Optional hardening (recommended)

- [ ] **Step 1: Consider embedding the icon as a resource**

Loading from a loose `.ico` file next to the DLL works, but is sensitive to deployment file layout. If desired, embed the icon in the DLL using the resource script and load it via `MAKEINTRESOURCE`. This is optional but reduces runtime path assumptions.

- [ ] **Step 2: Document icon-source workflow**

Add a short note in `AGENTS.md` or a new `docs/` file describing how to regenerate the ICO from `icon.png` so future contributors don’t manually create mismatched sizes.

---

### Verification

1. `msbuild PinyinTSF.sln /p:Configuration=Release /p:Platform=x64` succeeds.
2. `bin/Release/app_icon.ico` exists and contains the expected sizes.
3. `install.bat` completes successfully (admin prompt).
4. Win+Space and Language settings show the new icon.
5. System tray shows the new icon.
