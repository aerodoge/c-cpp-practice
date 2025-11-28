#include "morse_code.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace morse {

MorseCode::MorseCode() {
    initializeMaps();
}

void MorseCode::initializeMaps() {
    // 数字 0-9
    char_to_morse_['0'] = "-----";
    char_to_morse_['1'] = ".----";
    char_to_morse_['2'] = "..---";
    char_to_morse_['3'] = "...--";
    char_to_morse_['4'] = "....-";
    char_to_morse_['5'] = ".....";
    char_to_morse_['6'] = "-....";
    char_to_morse_['7'] = "--...";
    char_to_morse_['8'] = "---..";
    char_to_morse_['9'] = "----.";

    // 字母 A-Z
    char_to_morse_['A'] = ".-";
    char_to_morse_['B'] = "-...";
    char_to_morse_['C'] = "-.-.";
    char_to_morse_['D'] = "-..";
    char_to_morse_['E'] = ".";
    char_to_morse_['F'] = "..-.";
    char_to_morse_['G'] = "--.";
    char_to_morse_['H'] = "....";
    char_to_morse_['I'] = "..";
    char_to_morse_['J'] = ".---";
    char_to_morse_['K'] = "-.-";
    char_to_morse_['L'] = ".-..";
    char_to_morse_['M'] = "--";
    char_to_morse_['N'] = "-.";
    char_to_morse_['O'] = "---";
    char_to_morse_['P'] = ".--.";
    char_to_morse_['Q'] = "--.-";
    char_to_morse_['R'] = ".-.";
    char_to_morse_['S'] = "...";
    char_to_morse_['T'] = "-";
    char_to_morse_['U'] = "..-";
    char_to_morse_['V'] = "...-";
    char_to_morse_['W'] = ".--";
    char_to_morse_['X'] = "-..-";
    char_to_morse_['Y'] = "-.--";
    char_to_morse_['Z'] = "--..";

    // 构建反向映射
    for (const auto& [ch, code] : char_to_morse_) {
        morse_to_char_[code] = ch;
    }
}

std::string MorseCode::encode(std::string_view text) const {
    std::ostringstream result;
    bool first_char_in_word = true;
    bool prev_was_space = false;

    for (char c : text) {
        if (c == ' ') {
            if (!prev_was_space && !first_char_in_word) {
                result << "   ";  // 单词间三个空格
            }
            prev_was_space = true;
            first_char_in_word = true;
            continue;
        }

        prev_was_space = false;
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        auto it = char_to_morse_.find(upper);
        if (it != char_to_morse_.end()) {
            if (!first_char_in_word) {
                result << ' ';  // 字符间单个空格
            }
            result << it->second;
            first_char_in_word = false;
        }
        // 忽略不可编码的字符
    }

    return result.str();
}

std::string MorseCode::decode(std::string_view morse) const {
    std::ostringstream result;

    // 按三个空格分割单词
    auto words = split(morse, "   ");

    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) {
            result << ' ';
        }

        // 按单个空格分割字符
        auto codes = split(words[i], " ");

        for (const auto& code : codes) {
            if (code.empty()) continue;

            auto it = morse_to_char_.find(code);
            if (it != morse_to_char_.end()) {
                result << it->second;
            }
            // 忽略无效的摩尔斯电码
        }
    }

    return result.str();
}

bool MorseCode::isEncodable(char c) const {
    if (c == ' ') return true;
    char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return char_to_morse_.find(upper) != char_to_morse_.end();
}

bool MorseCode::isValidMorse(std::string_view morse) const {
    if (morse.empty()) return true;

    auto words = split(morse, "   ");
    for (const auto& word : words) {
        auto codes = split(word, " ");
        for (const auto& code : codes) {
            if (code.empty()) continue;
            // 检查是否只包含点和划
            for (char c : code) {
                if (c != '.' && c != '-') return false;
            }
            // 检查是否是有效的摩尔斯电码
            if (morse_to_char_.find(code) == morse_to_char_.end()) {
                return false;
            }
        }
    }
    return true;
}

std::optional<std::string> MorseCode::charToMorse(char c) const {
    char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (const auto it = char_to_morse_.find(upper); it != char_to_morse_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<char> MorseCode::morseToChar(std::string_view morse) const {
    if (const auto it = morse_to_char_.find(std::string(morse)); it != morse_to_char_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> MorseCode::split(std::string_view str,
                                           std::string_view delimiter) const {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string_view::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.emplace_back(str.substr(start));
    return result;
}

} // namespace morse
