#ifndef MORSE_CODE_HPP
#define MORSE_CODE_HPP

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>

namespace morse {

/**
 * MorseCode - 摩尔斯电码编码器/解码器
 *
 * 支持：
 * - 字母 A-Z (大小写不敏感)
 * - 数字 0-9
 * - 英文短语编码/解码
 *
 * 编码规则：
 * - 字母之间用单个空格分隔
 * - 单词之间用三个空格分隔
 */
class MorseCode {
public:
    MorseCode();

    // 将文本编码为摩尔斯电码
    [[nodiscard]] std::string encode(std::string_view text) const;

    // 将摩尔斯电码解码为文本
    [[nodiscard]] std::string decode(std::string_view morse) const;

    // 检查字符是否可编码
    [[nodiscard]] bool isEncodable(char c) const;

    // 检查摩尔斯电码是否有效
    [[nodiscard]] bool isValidMorse(std::string_view morse) const;

    // 获取单个字符的摩尔斯电码
    [[nodiscard]] std::optional<std::string> charToMorse(char c) const;

    // 获取摩尔斯电码对应的字符
    [[nodiscard]] std::optional<char> morseToChar(std::string_view morse) const;

private:
    std::unordered_map<char, std::string> char_to_morse_;
    std::unordered_map<std::string, char> morse_to_char_;

    void initializeMaps();

    // 分割字符串
    [[nodiscard]] std::vector<std::string> split(std::string_view str,
                                                  std::string_view delimiter) const;
};

} // namespace morse

#endif // MORSE_CODE_HPP
