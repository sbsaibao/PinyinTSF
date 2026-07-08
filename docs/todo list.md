# PinyinTSF 重构待办清单

## 已完成

- [x] 删除词组联想功能 - 恢复到简单的4声调拼音候选模式（2026-07-04）
- [x] 简化拼音输入逻辑 - 单音节显示4个声调，多音节只显示原始拼音（2026-07-04）
- [x] 添加轻声候选 - 单音节显示5个候选（4个声调+轻声）（2026-07-04）
- [x] 合并重复的 `_EndComposition` - `OnCompositionTerminated` 已直接调用 `_EndComposition()`（2026-07-04）

## 代码重复

- [x] `_UpdateCandidateWindow` 模式：候选窗口显示/隐藏逻辑已统一到 `_UpdateCandidateWindow`，`_HandleCharInput` 和 `_HandleBackspace` 均调用该方法（2026-07-05）
- [x] 候选文本格式化：`swprintf_s(L"%d.", i+1) + candidate` 重复格式化已不存在，当前序号徽章和候选文本分开处理（2026-07-05）

## 死代码清理

- [x] `new` 永远不会返回 NULL，`Composition.cpp` 中的4个 `if (!pEditSession) return;` 检查已移除（2026-07-05）

## 状态管理

- [x] `CCommitTextEditSession` 提交后已清除 `_candidates` 和 `_selectedIndex`（2026-07-06）

## 硬编码值

- [x] `0x00FC` (ü) 在 `PinyinEngine.cpp` 中已统一使用 `U_DIAERESIS`，仅在 `PinyinEngine.h` 中作为命名常量定义（2026-07-06）

## 代码风格

- [x] `Register.cpp` 中 GUID 转字符串已使用 `StringFromGUID2`（2026-07-06）
- [x] `PinyinEngine.cpp` 中 C 风格类型转换已改为 C++ 风格 `static_cast`（2026-07-06）

## 优先级建议

| 优先级 | 项目 | 原因 |
|--------|------|------|
| 高 | 提取 `ü` 常量 | 消除魔法数字，提高可读性 |
| 中 | 清理死代码检查 | 代码简洁性 |
| 中 | 提取候选窗口显示/隐藏逻辑 | 减少重复代码 |
| 中 | 清理提交后的状态 | 避免状态不一致 |
| 低 | 统一类型转换风格 | 代码风格一致性 |
| 低 | 使用 `StringFromGUID2` | 代码简洁性 |
