#include "PinyinEngine.h"
#include "PinyinCompositionState.h"
#include "TrayIcon.h"
#include "KeyEventLogic.h"
#include "SettingsManager.h"

#include <iostream>
#include <string>
#include <vector>

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

static void ExpectEqual(const std::wstring& name, const std::wstring& actual, const std::wstring& expected) {
    if (actual != expected) {
        std::wcerr << L"FAIL " << name << L": expected [" << expected << L"], got [" << actual << L"]\n";
        ++g_failures;
    }
}

static void ExpectVectorEqual(
    const std::wstring& name,
    const std::vector<std::wstring>& actual,
    const std::vector<std::wstring>& expected) {
    if (actual.size() != expected.size()) {
        std::wcerr << L"FAIL " << name << L": expected size " << expected.size()
                   << L", got " << actual.size() << L"\n";
        ++g_failures;
        return;
    }

    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) {
            std::wcerr << L"FAIL " << name << L"[" << i << L"]: expected [" << expected[i]
                       << L"], got [" << actual[i] << L"]\n";
            ++g_failures;
        }
    }
}

int wmain() {
    CPinyinEngine engine;

    TrayIcon::RequestShutdownExisting();

    ExpectBool(L"plain letter is eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, false, false, false), true);
    ExpectBool(L"shift letter is still eaten for pinyin input",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, false, false, false), true);
    ExpectBool(L"ctrl c is not eaten",
        KeyEventLogic::ShouldEatKeyDown('C', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl v is not eaten",
        KeyEventLogic::ShouldEatKeyDown('V', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl x is not eaten",
        KeyEventLogic::ShouldEatKeyDown('X', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl a is not eaten",
        KeyEventLogic::ShouldEatKeyDown('A', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl z is not eaten",
        KeyEventLogic::ShouldEatKeyDown('Z', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl y is not eaten",
        KeyEventLogic::ShouldEatKeyDown('Y', false, false, 0, true, false, false), false);
    ExpectBool(L"ctrl s is not eaten",
        KeyEventLogic::ShouldEatKeyDown('S', false, false, 0, true, false, false), false);
    ExpectBool(L"alt f is not eaten",
        KeyEventLogic::ShouldEatKeyDown('F', false, false, 0, false, true, false), false);
    ExpectBool(L"alt tab is not eaten",
        KeyEventLogic::ShouldEatKeyDown(VK_TAB, false, false, 0, false, true, false), false);
    ExpectBool(L"alt space is not eaten",
        KeyEventLogic::ShouldEatKeyDown(VK_SPACE, false, false, 0, false, true, false), false);
    ExpectBool(L"win l is not eaten",
        KeyEventLogic::ShouldEatKeyDown('L', false, false, 0, false, false, true), false);
    ExpectBool(L"win space is not eaten",
        KeyEventLogic::ShouldEatKeyDown(VK_SPACE, false, false, 0, false, false, true), false);
    ExpectBool(L"number candidate is eaten during composition",
        KeyEventLogic::ShouldEatKeyDown('1', true, true, 5, false, false, false), true);
    ExpectBool(L"ctrl number is not eaten during composition",
        KeyEventLogic::ShouldEatKeyDown('1', true, true, 5, true, false, false), false);
    ExpectBool(L"backspace is eaten during composition",
        KeyEventLogic::ShouldEatKeyDown(VK_BACK, true, false, 0, false, false, false), true);
    ExpectBool(L"alt backspace is not eaten during composition",
        KeyEventLogic::ShouldEatKeyDown(VK_BACK, true, false, 0, false, true, false), false);
    ExpectBool(L"space commits selected candidate during composition",
        KeyEventLogic::ShouldEatKeyDown(VK_SPACE, true, true, 5, false, false, false), true);
    ExpectBool(L"space is not eaten without candidates",
        KeyEventLogic::ShouldEatKeyDown(VK_SPACE, true, false, 0, false, false, false), false);
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

    ExpectVectorEqual(L"segments nihao", engine.SegmentInput(L"nihao"), { L"ni", L"hao" });
    ExpectVectorEqual(L"segments zhinengti", engine.SegmentInput(L"zhinengti"), { L"zhi", L"neng", L"ti" });

    std::vector<std::wstring> niCandidates = engine.GetSyllableCandidates(L"ni");
    ExpectVectorEqual(L"ni candidates", niCandidates, { L"n\x012B", L"n\x00ED", L"n\x01D0", L"n\x00EC", L"ni" });

    std::vector<std::wstring> haoCandidates = engine.GetSyllableCandidates(L"hao");
    ExpectVectorEqual(L"hao candidates", haoCandidates, { L"h\x0101o", L"h\x00E1o", L"h\x01CEo", L"h\x00E0o", L"hao" });

    ExpectEqual(L"legacy multi-syllable fallback", engine.GetCandidates(L"nihao")[0], L"nihao");

    PinyinCompositionState state;
    state.AppendChar(L'n');
    state.AppendChar(L'i');
    state.AppendChar(L'h');
    state.AppendChar(L'a');
    state.AppendChar(L'o');
    state.Rebuild(engine);

    ExpectEqual(L"current syllable starts at ni", state.GetCurrentSyllable(), L"ni");
    ExpectEqual(L"initial display is raw input", state.BuildDisplayText(), L"nihao");

    bool complete = state.SelectCurrentSyllable(L"n\x01D0");
    ExpectEqual(L"first selection not complete", complete ? L"true" : L"false", L"false");
    ExpectEqual(L"display after first selection", state.BuildDisplayText(), L"n\x01D0" L"hao");
    ExpectEqual(L"current syllable advances to hao", state.GetCurrentSyllable(), L"hao");

    complete = state.SelectCurrentSyllable(L"h\x01CEo");
    ExpectEqual(L"second selection completes", complete ? L"true" : L"false", L"true");
    ExpectEqual(L"commit after all selections", state.BuildCommittedText(), L"n\x01D0" L"h\x01CEo");

    PinyinCompositionState partial;
    partial.AppendChar(L'z');
    partial.AppendChar(L'h');
    partial.AppendChar(L'i');
    partial.AppendChar(L'n');
    partial.AppendChar(L'e');
    partial.AppendChar(L'n');
    partial.AppendChar(L'g');
    partial.AppendChar(L't');
    partial.AppendChar(L'i');
    partial.Rebuild(engine);
    ExpectEqual(L"zhinengti first syllable", partial.GetCurrentSyllable(), L"zhi");
    partial.SelectCurrentSyllable(L"zh\x00EC");
    ExpectEqual(L"zhinengti display after one selection", partial.BuildDisplayText(), L"zh\x00EC" L"nengti");
    ExpectEqual(L"zhinengti second syllable", partial.GetCurrentSyllable(), L"neng");

    if (g_failures != 0) {
        std::wcerr << g_failures << L" test failure(s)\n";
        return 1;
    }

    std::wcout << L"PinyinEngineTests passed\n";
    return 0;
}
