# 添加轻声候选设计文档

## 概述

在现有4个声调候选（阴平、阳平、上声、去声）基础上，添加第5个轻声候选，使拼音输入法更符合日常使用习惯。

## 背景

当前拼音输入法对单音节输入（如"ma"）只显示4个声调变体：
- mā (阴平)
- má (阳平)
- mǎ (上声)
- mà (去声)

但在日常使用中，轻声（如"妈妈"中的第二个"妈"）也是常见声调，需要支持。

## 设计决策

### 轻声显示方式

**选择：原始元音字母**

轻声在拼音中没有声调标记，使用原始元音字母显示：
- 输入 "ma" → 第5个候选显示为 "ma"
- 输入 "ba" → 第5个候选显示为 "ba"

**其他方案（已排除）：**
- 使用特殊符号标记（如 mȧ）→ 不符合标准拼音规范

## 技术设计

### 1. 修改 TONE_MAP 数组

**文件：** `src/PinyinEngine.h` 和 `src/PinyinEngine.cpp`

**改动：**
- 将 `TONE_MAP[6][4]`（原始4列：阴平/阳平/上声/去声）扩展为 `TONE_MAP[6][5]`（新增第5列：轻声）
- 第5列（index 4）存储原始元音字母（轻声）

```cpp
// Tone map: a, o, e, i, u, u-diaeresis
// Each row: tone1(flat), tone2(rising), tone3(dipping), tone4(falling), tone5(neutral)
const wchar_t CPinyinEngine::TONE_MAP[6][5] = {
    { 0x0101, 0x00E1, 0x01CE, 0x00E0, L'a' },  // a
    { 0x014D, 0x00F3, 0x01D2, 0x00F2, L'o' },  // o
    { 0x0113, 0x00E9, 0x011B, 0x00E8, L'e' },  // e
    { 0x012B, 0x00ED, 0x01D0, 0x00EC, L'i' },  // i
    { 0x016B, 0x00FA, 0x01D4, 0x00F9, L'u' },  // u
    { 0x01D6, 0x01D8, 0x01DA, 0x01DC, U_DIAERESIS },  // u-diaeresis
};
```

### 2. 修改 GetCandidates() 方法

**文件：** `src/PinyinEngine.cpp`

**改动：**
- 将循环从 `tone < 4` 改为 `tone < 5`
- 生成5个候选（4个声调 + 1个轻声）

```cpp
// Step 5: Generate 5 candidates (4 tones + neutral)
for (int tone = 0; tone < 5; tone++) {
    candidates.push_back(_ApplyTone(pinyin, tonePos, tone));
}
```

### 3. 修改 _ApplyTone() 方法

**文件：** `src/PinyinEngine.cpp`

**改动：**
- 将检查从 `tone > 3` 改为 `tone > 4`

```cpp
if (tone < 0 || tone > 4) return pinyin;
```

### 4. 更新注释

**文件：** `src/PinyinEngine.h`

**改动：**
- 更新方法注释，说明生成5个候选

```cpp
// Generate 5 toned candidates from raw pinyin input (4 tones + neutral)
std::vector<std::wstring> GetCandidates(const std::wstring& input);
```

- 更新 `_ApplyTone` 方法注释，将 tone 范围从 `(0-3)` 改为 `(0-4)`

```cpp
// Apply a specific tone (0-4) to the vowel at tonePos
std::wstring _ApplyTone(const std::wstring& pinyin, int tonePos, int tone);
```

### 5. 修改数字键选择范围

**文件：** `src/KeyEventSink.cpp`

**改动：**
- 将 `OnTestKeyDown` 和 `OnKeyDown` 中数字键范围从 `'1'-'4'` 扩展到 `'1'-'5'`
- 更新相关注释

```cpp
// OnTestKeyDown:
if (wParam >= '1' && wParam <= '5' && !_candidates.empty()) {

// OnKeyDown:
if (wParam >= '1' && wParam <= '5' && !_candidates.empty()) {

// _HandleCandidateSelection 注释:
// _HandleCandidateSelection - User selected a candidate (1-5)
```

> **注意：** 数字键范围检查均伴有 `index < (int)_candidates.size()` 的二次校验，确保候选不足5个时按键不被误吞。

## 显示效果

```
键盘输入: ma
组合缓冲区: ma
候选:
1. mā (阴平)
2. má (阳平)
3. mǎ (上声)
4. mà (去声)
5. ma (轻声)        ← 徽章显示中文数字"五"
```

```
键盘输入: nv（v 在 l/n 后被预处理转换为 ü）
组合缓冲区: nv
候选:
1. nǖ (阴平)
2. nǘ (阳平)
3. nǚ (上声)
4. nǜ (去声)
5. nü (轻声)        ← 徽章显示中文数字"五"
```

> **说明：** 用户无法直接键入 ü 字符，需通过输入 "nv" 来实现。`_Preprocess()` 在 l/n 后将 v 转换为 ü，因此候选显示为带 ü 的变体，但组合缓冲区（内联文本）仍为原始输入 "nv"。

## 边界情况

1. **多音节输入**（如"nihao"）→ 只显示原始拼音，不显示声调候选
2. **无元音输入**（如"zh"）→ 显示原始拼音
3. **空输入** → 返回空数组
4. **候选不足5个时按数字键"5"** → `OnTestKeyDown` 中 `index < _candidates.size()` 检查失败，按键不被"吃掉"，正常传递给应用程序，不会误触发
5. **轻声候选与Enter键提交的功能重叠** → 输入"ma"时，按 '5' 提交 "ma"（轻声候选），按 Enter 也提交 "ma"（原始拼音）。两者结果相同，这是有意设计：按数字键是"选择轻声"的语义，按 Enter 是"放弃声调选择，直接提交原始拼音"的语义
6. **轻声候选与输入缓冲区内容相同** → 对所有单音节输入，第5个候选的文字内容与预处理后的拼音完全相同（因为 `TONE_MAP[vowelIdx][4]` 存储的就是原始元音字符，替换后等于原字符）。这是轻声的本质特性，不影响功能

## 验证方法

1. 输入 "ma" → 应显示5个候选：mā, má, mǎ, mà, ma
2. 输入 "ba" → 应显示5个候选：bā, bá, bǎ, bà, ba
3. 输入 "nihao" → 应只显示原始拼音：nihao
4. 输入 "a" → 应显示5个候选：ā, á, ǎ, à, a
5. 输入 "o" → 应显示5个候选：ō, ó, ǒ, ò, o（单独韵母验证）
6. 输入 "e" → 应显示5个候选：ē, é, ě, è, e（单独韵母验证）
7. 输入 "er" → 应显示5个候选：ēr, ér, ěr, èr, er（儿化音，轻声"er"极常见）
8. 输入 "lv" → 应显示5个候选：lǖ, lǘ, lǚ, lǜ, lü（v→ü 转换路径验证）
9. 输入 "nv" → 应显示5个候选：nǖ, nǘ, nǚ, nǜ, nü（v→ü 转换路径验证）
10. 输入 "zh" → 应只显示原始拼音：zh（无元音输入）
11. 候选不足5个时（如输入"nihao"只有1个候选），按数字键 '5' → 不应触发候选选择，按键应传递给应用程序
12. 输入 "ma" 后按 '5' → 应提交 "ma"（轻声）；输入 "ma" 后按 Enter → 也应提交 "ma"（原始拼音），两者结果相同

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `src/PinyinEngine.h` | 扩展 TONE_MAP 声明，更新注释 |
| `src/PinyinEngine.cpp` | 扩展 TONE_MAP 定义，修改 GetCandidates() 和 _ApplyTone() |
| `src/KeyEventSink.cpp` | 将数字键范围从 `'1'-'4'` 扩展到 `'1'-'5'`，更新相关注释 |

## 无需修改的组件说明

### CandidateWindow（候选窗口）

**无需修改，原因如下：**

1. **动态宽度计算：** `_CalculateWindowSize()` 基于 `_candidates.size()` 遍历计算总宽度，天然支持任意候选数量
2. **屏幕边界处理：** `_AdjustWindowPosition()` 检测右溢出（左移窗口）和底溢出（显示在光标上方），5个候选的更宽窗口不会超出屏幕
3. **中文数字徽章：** `_chineseNumbers` 数组已有9个元素（一到九），第5个候选的徽章自动显示为"五"
4. **箭头键导航：** `MoveSelectionNext()`/`MoveSelectionPrev()` 使用模运算 `(_selectedIndex + 1) % _candidates.size()`，天然支持5个候选的循环导航

**用户体验影响评估：**

从4个候选扩展到5个，候选窗口宽度增加约25%。在默认缩放（scale=1.0）下，4个候选的窗口宽度约为 51*4 + 8*4 + 10*3 + 15 = 281px，5个候选约为 51*5 + 8*5 + 10*4 + 15 = 350px。此宽度在主流显示器上不会造成问题，但在极窄窗口或多显示器布局下需注意。`_AdjustWindowPosition` 已处理此场景。

### SettingsManager（设置管理器）

**无需修改：** 所有布局参数（`BadgeWidth`、`ItemSpacing`、`MinWidth` 等）基于 `SettingsManager` 的常量和缩放因子动态计算，不包含硬编码的候选数量假设。
