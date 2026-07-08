#pragma once

#ifndef PINYIN_COMPOSITION_STATE_H
#define PINYIN_COMPOSITION_STATE_H

#include "PinyinEngine.h"

#include <string>
#include <vector>

class PinyinCompositionState {
public:
    void Clear();
    void AppendChar(wchar_t ch);
    void Backspace();
    void Rebuild(CPinyinEngine& engine);

    bool HasInput() const;
    bool HasPendingSyllable() const;
    bool SelectCurrentSyllable(const std::wstring& selectedText);

    std::wstring GetCurrentSyllable() const;
    std::wstring BuildDisplayText(bool insertSyllableSpaces = true) const;
    std::wstring BuildCommittedText(bool insertSyllableSpaces = true) const;

private:
    std::wstring _rawInput;
    std::vector<std::wstring> _syllables;
    std::vector<std::wstring> _selectedTexts;
    size_t _currentSyllableIndex = 0;
};

#endif // PINYIN_COMPOSITION_STATE_H
