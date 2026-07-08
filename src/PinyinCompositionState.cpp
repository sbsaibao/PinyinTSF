#include "PinyinCompositionState.h"

void PinyinCompositionState::Clear() {
    _rawInput.clear();
    _syllables.clear();
    _selectedTexts.clear();
    _currentSyllableIndex = 0;
}

void PinyinCompositionState::AppendChar(wchar_t ch) {
    _rawInput += ch;
}

void PinyinCompositionState::Backspace() {
    if (!_rawInput.empty()) {
        _rawInput.pop_back();
    }
}

void PinyinCompositionState::Rebuild(CPinyinEngine& engine) {
    _syllables = engine.SegmentInput(_rawInput);

    if (_selectedTexts.size() > _syllables.size()) {
        _selectedTexts.resize(_syllables.size());
    }

    if (_currentSyllableIndex > _selectedTexts.size()) {
        _currentSyllableIndex = _selectedTexts.size();
    }

    if (_currentSyllableIndex >= _syllables.size() && !_syllables.empty()) {
        _currentSyllableIndex = _syllables.size() - 1;
    }
}

bool PinyinCompositionState::HasInput() const {
    return !_rawInput.empty();
}

bool PinyinCompositionState::HasPendingSyllable() const {
    return _currentSyllableIndex < _syllables.size();
}

bool PinyinCompositionState::SelectCurrentSyllable(const std::wstring& selectedText) {
    if (!HasPendingSyllable()) {
        return false;
    }

    if (_selectedTexts.size() > _currentSyllableIndex) {
        _selectedTexts[_currentSyllableIndex] = selectedText;
    } else {
        _selectedTexts.push_back(selectedText);
    }

    _currentSyllableIndex = _selectedTexts.size();
    return _currentSyllableIndex >= _syllables.size();
}

std::wstring PinyinCompositionState::GetCurrentSyllable() const {
    if (!HasPendingSyllable()) {
        return L"";
    }
    return _syllables[_currentSyllableIndex];
}

std::wstring PinyinCompositionState::BuildDisplayText(bool insertSyllableSpaces) const {
    std::wstring result;

    if (insertSyllableSpaces && !_selectedTexts.empty()) {
        for (size_t i = 0; i < _selectedTexts.size(); ++i) {
            if (!result.empty()) {
                result += L' ';
            }
            result += _selectedTexts[i];
        }

        for (size_t i = _selectedTexts.size(); i < _syllables.size(); ++i) {
            if (!result.empty()) {
                result += L' ';
            }
            result += _syllables[i];
        }
    } else {
        for (const std::wstring& selected : _selectedTexts) {
            result += selected;
        }

        for (size_t i = _selectedTexts.size(); i < _syllables.size(); ++i) {
            result += _syllables[i];
        }
    }

    return result.empty() ? _rawInput : result;
}

std::wstring PinyinCompositionState::BuildCommittedText(bool insertSyllableSpaces) const {
    return BuildDisplayText(insertSyllableSpaces);
}
