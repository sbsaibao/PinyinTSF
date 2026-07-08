#include "PinyinEngine.h"
#include <algorithm>
#include <cctype>

// ================================================================
// Tone map: a, o, e, i, u, u-diaeresis
// Each row: tone1(flat), tone2(rising), tone3(dipping), tone4(falling), tone5(neutral)
// ================================================================
const wchar_t CPinyinEngine::TONE_MAP[6][5] = {
    { 0x0101, 0x00E1, 0x01CE, 0x00E0, L'a' },  // a
    { 0x014D, 0x00F3, 0x01D2, 0x00F2, L'o' },  // o
    { 0x0113, 0x00E9, 0x011B, 0x00E8, L'e' },  // e
    { 0x012B, 0x00ED, 0x01D0, 0x00EC, L'i' },  // i
    { 0x016B, 0x00FA, 0x01D4, 0x00F9, L'u' },  // u
    { 0x01D6, 0x01D8, 0x01DA, 0x01DC, U_DIAERESIS },  // u-diaeresis
};

static const wchar_t* KNOWN_SYLLABLES[] = {
    L"a", L"ai", L"an", L"ang", L"ao",
    L"ba", L"bai", L"ban", L"bang", L"bao", L"bei", L"ben", L"beng", L"bi", L"bian", L"biao", L"bie", L"bin", L"bing", L"bo", L"bu",
    L"ca", L"cai", L"can", L"cang", L"cao", L"ce", L"cen", L"ceng", L"cha", L"chai", L"chan", L"chang", L"chao", L"che", L"chen", L"cheng", L"chi", L"chong", L"chou", L"chu", L"chuai", L"chuan", L"chuang", L"chui", L"chun", L"chuo", L"ci", L"cong", L"cou", L"cu", L"cuan", L"cui", L"cun", L"cuo",
    L"da", L"dai", L"dan", L"dang", L"dao", L"de", L"dei", L"den", L"deng", L"di", L"dia", L"dian", L"diao", L"die", L"ding", L"diu", L"dong", L"dou", L"du", L"duan", L"dui", L"dun", L"duo",
    L"e", L"ei", L"en", L"eng", L"er",
    L"fa", L"fan", L"fang", L"fei", L"fen", L"feng", L"fo", L"fou", L"fu",
    L"ga", L"gai", L"gan", L"gang", L"gao", L"ge", L"gei", L"gen", L"geng", L"gong", L"gou", L"gu", L"gua", L"guai", L"guan", L"guang", L"gui", L"gun", L"guo",
    L"ha", L"hai", L"han", L"hang", L"hao", L"he", L"hei", L"hen", L"heng", L"hong", L"hou", L"hu", L"hua", L"huai", L"huan", L"huang", L"hui", L"hun", L"huo",
    L"ji", L"jia", L"jian", L"jiang", L"jiao", L"jie", L"jin", L"jing", L"jiong", L"jiu", L"ju", L"juan", L"jue", L"jun",
    L"ka", L"kai", L"kan", L"kang", L"kao", L"ke", L"kei", L"ken", L"keng", L"kong", L"kou", L"ku", L"kua", L"kuai", L"kuan", L"kuang", L"kui", L"kun", L"kuo",
    L"la", L"lai", L"lan", L"lang", L"lao", L"le", L"lei", L"leng", L"li", L"lia", L"lian", L"liang", L"liao", L"lie", L"lin", L"ling", L"liu", L"lo", L"long", L"lou", L"lu", L"l" L"\x00FC", L"luan", L"l" L"\x00FC" L"e", L"lun", L"luo",
    L"ma", L"mai", L"man", L"mang", L"mao", L"me", L"mei", L"men", L"meng", L"mi", L"mian", L"miao", L"mie", L"min", L"ming", L"miu", L"mo", L"mou", L"mu",
    L"na", L"nai", L"nan", L"nang", L"nao", L"ne", L"nei", L"nen", L"neng", L"ni", L"nian", L"niang", L"niao", L"nie", L"nin", L"ning", L"niu", L"nong", L"nou", L"nu", L"n" L"\x00FC", L"nuan", L"n" L"\x00FC" L"e", L"nun", L"nuo",
    L"o", L"ou",
    L"pa", L"pai", L"pan", L"pang", L"pao", L"pei", L"pen", L"peng", L"pi", L"pian", L"piao", L"pie", L"pin", L"ping", L"po", L"pou", L"pu",
    L"qi", L"qia", L"qian", L"qiang", L"qiao", L"qie", L"qin", L"qing", L"qiong", L"qiu", L"qu", L"quan", L"que", L"qun",
    L"ran", L"rang", L"rao", L"re", L"ren", L"reng", L"ri", L"rong", L"rou", L"ru", L"ruan", L"rui", L"run", L"ruo",
    L"sa", L"sai", L"san", L"sang", L"sao", L"se", L"sen", L"seng", L"sha", L"shai", L"shan", L"shang", L"shao", L"she", L"shen", L"sheng", L"shi", L"shou", L"shu", L"shua", L"shuai", L"shuan", L"shuang", L"shui", L"shun", L"shuo", L"si", L"song", L"sou", L"su", L"suan", L"sui", L"sun", L"suo",
    L"ta", L"tai", L"tan", L"tang", L"tao", L"te", L"teng", L"ti", L"tian", L"tiao", L"tie", L"ting", L"tong", L"tou", L"tu", L"tuan", L"tui", L"tun", L"tuo",
    L"wa", L"wai", L"wan", L"wang", L"wei", L"wen", L"weng", L"wo", L"wu",
    L"xi", L"xia", L"xian", L"xiang", L"xiao", L"xie", L"xin", L"xing", L"xiong", L"xiu", L"xu", L"xuan", L"xue", L"xun",
    L"ya", L"yan", L"yang", L"yao", L"ye", L"yi", L"yin", L"ying", L"yo", L"yong", L"you", L"yu", L"yuan", L"yue", L"yun",
    L"za", L"zai", L"zan", L"zang", L"zao", L"ze", L"zei", L"zen", L"zeng", L"zha", L"zhai", L"zhan", L"zhang", L"zhao", L"zhe", L"zhen", L"zheng", L"zhi", L"zhong", L"zhou", L"zhu", L"zhua", L"zhuai", L"zhuan", L"zhuang", L"zhui", L"zhun", L"zhuo", L"zi", L"zong", L"zou", L"zu", L"zuan", L"zui", L"zun", L"zuo"
};

CPinyinEngine::CPinyinEngine() {}
CPinyinEngine::~CPinyinEngine() {}

// ================================================================
// Check if character is a vowel (a, o, e, i, u, u-diaeresis)
// ================================================================
bool CPinyinEngine::_IsVowel(wchar_t ch) {
    return ch == L'a' || ch == L'o' || ch == L'e' ||
           ch == L'i' || ch == L'u' || ch == U_DIAERESIS;
}

// ================================================================
// Map vowel character to index in TONE_MAP
// ================================================================
int CPinyinEngine::_VowelToIndex(wchar_t ch) {
    switch (ch) {
        case L'a':    return 0;
        case L'o':    return 1;
        case L'e':    return 2;
        case L'i':    return 3;
        case L'u':    return 4;
        case U_DIAERESIS:  return 5;  // u-diaeresis
        default:      return -1;
    }
}

bool CPinyinEngine::_IsKnownSyllable(const std::wstring& syllable) {
    for (const wchar_t* known : KNOWN_SYLLABLES) {
        if (syllable == known) {
            return true;
        }
    }
    return false;
}

// ================================================================
// Check if input contains any vowel
// ================================================================
bool CPinyinEngine::HasVowel(const std::wstring& input) {
    std::wstring processed = _Preprocess(input);
    for (wchar_t ch : processed) {
        if (_IsVowel(ch)) return true;
    }
    return false;
}

// ================================================================
// Check if input is a single syllable (only one vowel group)
// ================================================================
bool CPinyinEngine::_IsSingleSyllable(const std::wstring& pinyin) {
    int vowelGroupCount = 0;
    bool inVowelGroup = false;

    for (wchar_t ch : pinyin) {
        if (_IsVowel(ch)) {
            if (!inVowelGroup) {
                vowelGroupCount++;
                inVowelGroup = true;
            }
        } else {
            inVowelGroup = false;
        }
    }

    // Single syllable has only one vowel group
    return vowelGroupCount <= 1;
}

// ================================================================
// Preprocess input: lowercase, v->u-diaeresis, colon notation
// ================================================================
std::wstring CPinyinEngine::_Preprocess(const std::wstring& input) {
    if (input.empty()) return L"";

    std::wstring result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); i++) {
        wchar_t ch = input[i];

        // Convert to lowercase
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }

        // Handle u: -> u-diaeresis (colon notation, only after 'l' or 'n')
        if (ch == L'u' && i + 1 < input.size() && input[i + 1] == L':') {
            if (!result.empty() && (result.back() == L'l' || result.back() == L'n')) {
                result += U_DIAERESIS;
                i++;  // skip the ':'
                continue;
            }
        }

        // Handle v -> u-diaeresis conversion (only after 'l' or 'n')
        if (ch == L'v') {
            if (!result.empty() && (result.back() == L'l' || result.back() == L'n')) {
                result += U_DIAERESIS;
                continue;
            }
        }

        result += ch;
    }

    return result;
}

// ================================================================
// Find the character position where tone mark should be placed
//
// Rules (Standard Hanyu Pinyin):
//   1. If contains 'a', tone goes on 'a'
//   2. If contains 'e' (and not "ue" after u-diaeresis), tone goes on 'e'
//   3. Special: "u-diaeresis + e" -> tone goes on u-diaeresis
//   4. If contains "ou", tone goes on 'o'
//   5. Otherwise, tone goes on the last vowel
// ================================================================
int CPinyinEngine::_FindTonePosition(const std::wstring& pinyin) {
    // Rule 1: If contains 'a', tone goes on 'a'
    for (int i = 0; i < static_cast<int>(pinyin.size()); i++) {
        if (pinyin[i] == L'a') return i;
    }

    // Rule 2 & 3: If contains 'e'
    for (int i = 0; i < static_cast<int>(pinyin.size()); i++) {
        if (pinyin[i] == L'e') {
            // Check if preceded by u-diaeresis ("ue" combination)
            if (i > 0 && pinyin[i - 1] == U_DIAERESIS) {
                return i - 1;  // tone on u-diaeresis, not e
            }
            return i;
        }
    }

    // Rule 4: If contains "ou", tone goes on 'o'
    for (int i = 0; i + 1 < static_cast<int>(pinyin.size()); i++) {
        if (pinyin[i] == L'o' && pinyin[i + 1] == L'u') {
            return i;
        }
    }

    // Rule 5: Otherwise, tone on the last vowel
    int lastVowelPos = -1;
    for (int i = 0; i < static_cast<int>(pinyin.size()); i++) {
        if (_IsVowel(pinyin[i])) {
            lastVowelPos = i;
        }
    }

    return lastVowelPos;
}

// ================================================================
// Replace vowel at tonePos with the toned Unicode character
// ================================================================
std::wstring CPinyinEngine::_ApplyTone(const std::wstring& pinyin, int tonePos, int tone) {
    if (tonePos < 0 || tonePos >= static_cast<int>(pinyin.size())) return pinyin;
    if (tone < 0 || tone > 4) return pinyin;

    int vowelIdx = _VowelToIndex(pinyin[tonePos]);
    if (vowelIdx < 0) return pinyin;

    std::wstring result = pinyin;
    result[tonePos] = TONE_MAP[vowelIdx][tone];
    return result;
}

// ================================================================
// Segment continuous pinyin using longest known syllable match
// ================================================================
std::vector<std::wstring> CPinyinEngine::SegmentInput(const std::wstring& input) {
    std::vector<std::wstring> segments;
    std::wstring pinyin = _Preprocess(input);
    if (pinyin.empty()) return segments;

    size_t pos = 0;
    while (pos < pinyin.size()) {
        size_t maxLen = std::min<size_t>(6, pinyin.size() - pos);
        std::wstring best;

        for (size_t len = maxLen; len >= 1; --len) {
            std::wstring candidate = pinyin.substr(pos, len);
            if (_IsKnownSyllable(candidate)) {
                best = candidate;
                break;
            }
            if (len == 1) break;
        }

        if (!best.empty()) {
            segments.push_back(best);
            pos += best.size();
        } else {
            segments.push_back(pinyin.substr(pos, 1));
            ++pos;
        }
    }

    return segments;
}

// ================================================================
// Generate candidates for a single syllable
// ================================================================
std::vector<std::wstring> CPinyinEngine::GetSyllableCandidates(const std::wstring& syllable) {
    std::vector<std::wstring> candidates;
    if (syllable.empty()) return candidates;

    std::wstring pinyin = _Preprocess(syllable);
    int tonePos = _FindTonePosition(pinyin);

    if (tonePos < 0) {
        candidates.push_back(pinyin);
        return candidates;
    }

    for (int tone = 0; tone < 5; tone++) {
        candidates.push_back(_ApplyTone(pinyin, tonePos, tone));
    }

    return candidates;
}

// ================================================================
// Main API: Generate candidates from raw pinyin
//
// Single syllable (e.g., "ma"): generate 5 toned variants (4 tones + neutral)
// Multi-syllable (e.g., "nihao"): return original input only
// ================================================================
std::vector<std::wstring> CPinyinEngine::GetCandidates(const std::wstring& input) {
    if (input.empty()) return std::vector<std::wstring>();

    std::vector<std::wstring> segments = SegmentInput(input);
    if (segments.size() == 1) {
        return GetSyllableCandidates(segments[0]);
    }

    return std::vector<std::wstring>{ input };
}
