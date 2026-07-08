# PinyinTSF 项目理解报告

## 项目定位

`PinyinTSF` 是一个 Windows TSF 输入法 COM DLL，不是普通桌面应用。

它实现的是极简拼音声调输入：输入拼音字母后生成带声调候选，并通过候选窗口提交到当前 TSF 文本上下文。

它不是完整拼音输入法：没有词库、没有组词、没有上下文预测，多音节输入目前只返回原始输入。

## 构建与安装边界

关键文件：

- `PinyinTSF.sln`
- `PinyinTSF.vcxproj`
- `install.bat`
- `uninstall.bat`
- `src/PinyinTSF.def`

构建约束：

- Visual Studio 2022 / v143 toolset。
- C++17。
- Unicode。
- 仅配置 `Debug|x64` 和 `Release|x64`。
- 输出 DLL 为 `bin/Debug/PinyinTSF.dll` 或 `bin/Release/PinyinTSF.dll`。

安装方式：

- `install.bat` 需要管理员权限。
- 脚本优先注册 Release DLL，找不到时回退 Debug DLL。
- 实际注册命令是 `regsvr32 /s "<dll>"`。
- 卸载使用 `regsvr32 /u /s "<dll>"`。

## 核心架构

中心类是 `CPinyinTextService`，声明在 `src/PinyinTextService.h`。

它同时实现四组 TSF 接口：

- `ITfTextInputProcessorEx`：激活和反激活。
- `ITfKeyEventSink`：键盘输入处理。
- `ITfCompositionSink`：composition 外部终止通知。
- `ITfThreadMgrEventSink`：焦点切换清理。

核心成员：

- `_pThreadMgr` 和 `_tfClientId`：TSF 线程管理和客户端 ID。
- `_pComposition`：当前活动 composition。
- `_compositionBuffer`：当前拼音输入缓冲。
- `_pCandidateWindow`：自绘 Win32 候选窗口。
- `_pPinyinEngine`：拼音声调候选生成器。
- `_candidates` 和 `_selectedIndex`：当前候选列表和选择项。

## 文件职责

关键源码职责：

- `src/PinyinTextService.cpp`：主 COM 对象生命周期、激活/反激活、composition 状态清理、候选同步。
- `src/KeyEventSink.cpp`：吃键判断、按键分发、字母输入、候选选择、Backspace、Escape、Enter、方向键。
- `src/Composition.cpp`：创建 edit session 并调用 `RequestEditSession` 的薄封装。
- `src/EditSession.cpp`：实际执行 TSF 文本操作，包括启动 composition、更新文本、提交文本、读取候选窗位置。
- `src/CandidateWindow.cpp`：Win32 popup 候选窗创建、显示、隐藏、绘制、尺寸计算、屏幕边界修正。
- `src/PinyinEngine.cpp`：拼音预处理、声调位置规则、带声调候选生成。
- `src/Register.cpp`：COM CLSID 注册、TSF profile 注册、TSF keyboard category 注册。
- `src/DllMain.cpp`：DLL 入口、COM 导出函数、regsvr32 注册/注销入口。
- `src/ClassFactory.cpp`：COM class factory，创建 `CPinyinTextService` 实例。
- `src/SettingsManager.cpp`、`src/TrayIcon.cpp`、`src/SettingsDialog.cpp`：候选窗外观设置、托盘入口和设置对话框。

## 输入流程

按键入口分两步：

- `OnTestKeyDown` 决定 TSF 是否吃掉该按键。
- `OnKeyDown` 只在按键被吃掉后执行实际处理。

字母输入链路：

1. `OnTestKeyDown` 对 `A-Z` 总是返回 eaten。
2. `OnKeyDown` 把字母转为小写。
3. `_HandleCharInput` 追加到 `_compositionBuffer`。
4. 若当前没有 `_pComposition`，调用 `_StartComposition`。
5. 调用 `_UpdateComposition` 更新内联 composition 文本。
6. 调用 `_UpdateCandidates` 重新生成候选。
7. 调用 `_UpdateCandidateWindow` 显示或刷新候选窗口。

候选选择链路：

1. composition 活动时，数字 `1-5` 可选择候选。
2. `_HandleCandidateSelection` 调用 `_CommitText`。
3. `CCommitTextEditSession::DoEditSession` 写入最终文本、结束 composition、清理本地状态、隐藏候选窗。

其他按键：

- `Backspace`：删除一个拼音字符；为空则取消 composition。
- `Escape`：提交空字符串，相当于取消 composition。
- `Enter`：提交原始拼音字符串。
- 方向键：只在候选列表内循环移动选择，不做文本编辑和翻页。

## TSF Edit Session 约束

所有 TSF 文本修改必须通过 `ITfContext::RequestEditSession` 执行。

`Composition.cpp` 不直接修改文本，它只创建 edit session 并请求 TSF 调度。

真正修改文本的位置在 `EditSession.cpp`：

- `CStartCompositionEditSession::DoEditSession`：用 `ITfInsertAtSelection` 获取插入点，并用 `ITfContextComposition::StartComposition` 启动 composition。
- `CUpdateCompositionEditSession::DoEditSession`：获取 composition range 并 `SetText` 为当前拼音 buffer。
- `CCommitTextEditSession::DoEditSession`：写入最终文本、移动光标、`EndComposition`、清理状态。
- `CGetPositionEditSession::DoEditSession`：用 `ITfContextView::GetTextExt` 获取屏幕位置，供候选窗定位。

这是项目最重要的工程约束：不要在普通按键处理函数中直接操作 TSF range 或 composition。

## 拼音引擎能力

`CPinyinEngine` 是纯逻辑模块。

当前行为：

- 输入会先转小写。
- `lv`、`nv`、`lu:`、`nu:` 会转换为 `lü`、`nü` 形式。
- 单音节输入生成 5 个候选：一声、二声、三声、四声、轻声/原元音形式。
- 无元音输入返回原输入。
- 多音节输入返回原输入。

声调位置规则：

- 有 `a` 标 `a`。
- 否则有 `e` 标 `e`。
- `üe` 特殊标在 `ü`。
- `ou` 标在 `o`。
- 其他情况标最后一个元音。

## 候选窗口

候选窗口不是 TSF 原生候选 UI，而是项目自建的 Win32 popup。

关键点：

- 创建在 `CCandidateWindow::Create`。
- 使用 `WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` 和 `WS_POPUP`。
- 绘制使用 GDI 双缓冲。
- 显示位置来自 TSF composition range 的屏幕矩形。
- 窗口会根据候选文本计算大小，并做工作区边界修正。
- 设置变化通过自定义消息刷新缩放和字体大小。

## COM 与 TSF 注册

注册分三层，缺一不可：

1. `RegisterServer` 写入 `HKCR\CLSID\{...}\InProcServer32`，并设置 `ThreadingModel=Apartment`。
2. `RegisterProfiles` 使用 `ITfInputProcessorProfiles` 注册 TSF 文本服务和中文语言 profile。
3. `RegisterCategories` 使用 `ITfCategoryMgr` 注册为 `GUID_TFCAT_TIP_KEYBOARD` 键盘类 TIP。

DLL 导出函数由 `src/PinyinTSF.def` 声明：

- `DllGetClassObject`
- `DllCanUnloadNow`
- `DllRegisterServer`
- `DllUnregisterServer`

## 常见修改入口

- 改按键行为：`src/KeyEventSink.cpp`。
- 改 composition 启动、更新、提交：`src/Composition.cpp` 和 `src/EditSession.cpp`。
- 改候选生成规则：`src/PinyinEngine.cpp` 和 `src/PinyinEngine.h`。
- 改候选窗 UI：`src/CandidateWindow.cpp` 和 `src/CandidateWindow.h`。
- 改安装、注册、输入法 profile：`src/Register.cpp`、`src/DllMain.cpp`、`src/Globals.cpp`、`src/Globals.h`。
- 改设置项：`src/SettingsManager.cpp`、`src/SettingsDialog.cpp`、`src/TrayIcon.cpp`。

## 风险点

- COM 引用计数需要严格匹配 `AddRef`/`Release`。
- composition 生命周期容易因焦点切换、外部终止、取消提交而状态不一致。
- `OnTestKeyDown` 吃键范围过宽会影响宿主应用正常输入，过窄会导致输入法收不到按键。
- 候选窗是普通 Win32 popup，DPI、多显示器、前台窗口、焦点切换都可能影响表现。
- 注册依赖管理员权限、注册表状态、TSF profile 状态和 DLL 路径。
- 当前仓库没有自动化测试或 lint 配置。

## 验证建议

静态理解阶段未运行构建、测试、安装或注册，因此以上结论来自源码阅读，不等同于运行时验证。

后续改动或接手验证建议：

1. 运行 Debug x64 构建：`msbuild PinyinTSF.sln /p:Configuration=Debug /p:Platform=x64`。
2. 运行 Release x64 构建：`msbuild PinyinTSF.sln /p:Configuration=Release /p:Platform=x64`。
3. 以管理员权限运行 `install.bat`。
4. 在输入法列表中启用 `Pinyin Tone Input`。
5. 手动验证字母输入、候选选择、Enter 原文提交、Escape 取消、Backspace 删除、方向键切换候选。
6. 手动验证焦点切换后 composition 和候选窗不会残留。
7. 如修改注册逻辑，卸载后重新安装，并确认 TSF profile 不残留旧 DLL 路径。
