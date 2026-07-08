#pragma once

#ifndef PINYIN_ENGINE_H
#define PINYIN_ENGINE_H

#include <string>
#include <vector>

class CPinyinEngine {
public:
    CPinyinEngine();
    ~CPinyinEngine();

    // Generate 5 toned candidates from raw pinyin input (4 tones + neutral)
    std::vector<std::wstring> GetCandidates(const std::wstring& input);

    // Split continuous pinyin input into syllables using longest-match rules
    std::vector<std::wstring> SegmentInput(const std::wstring& input);

    // Generate 5 toned candidates for a single syllable
    std::vector<std::wstring> GetSyllableCandidates(const std::wstring& syllable);

    // Check if input contains at least one vowel
    bool HasVowel(const std::wstring& input);

private:
    // Preprocess: convert v->u-diaeresis, handle colon notation
    std::wstring _Preprocess(const std::wstring& input);

    // Check if input is a single syllable (only one vowel group)
    bool _IsSingleSyllable(const std::wstring& pinyin);

    // Find the position where tone mark should be placed
    int _FindTonePosition(const std::wstring& pinyin);

    // Apply a specific tone (0-4) to the vowel at tonePos
    std::wstring _ApplyTone(const std::wstring& pinyin, int tonePos, int tone);

    // Check if a character is a vowel
    static bool _IsVowel(wchar_t ch);

    // Get the index of a vowel in the tone map
    static int _VowelToIndex(wchar_t ch);

    // Check whether a preprocessed lowercase string is a known pinyin syllable
    static bool _IsKnownSyllable(const std::wstring& syllable);

    // Tone map: [vowel_index][tone_index]
    static const wchar_t TONE_MAP[6][5];

    // U+00FC = u-diaeresis (眉)
    static const wchar_t U_DIAERESIS = 0x00FC;
};

#endif // PINYIN_ENGINE_H
