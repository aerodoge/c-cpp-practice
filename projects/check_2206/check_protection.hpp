#ifndef CHECK_PROTECTION_HPP
#define CHECK_PROTECTION_HPP

#include <string>
#include <string_view>
#include <optional>
#include <stdexcept>

namespace check {

// 常量定义
constexpr size_t MAX_DISPLAY_WIDTH = 9;      // 总显示宽度（包含小数点）
constexpr size_t MAX_INTEGER_DIGITS = 5;     // 整数部分最大位数：99999
constexpr size_t MAX_DECIMAL_DIGITS = 2;     // 小数部分最大位数：.99
constexpr char FILL_CHAR = '*';              // 保护填充字符

/**
 * InvalidAmountError - 无效金额异常
 *
 * 当输入的金额格式无效时抛出此异常
 */
class InvalidAmountError : public std::runtime_error {
public:
    explicit InvalidAmountError(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * CheckAmount - 支票金额处理类
 *
 * 支持：
 * - 解析金额字符串（支持逗号分隔）
 * - 验证字符合法性（数字、逗号、小数点）
 * - 去除前导零
 * - 生成带星号的保护格式
 *
 * 示例：99.87 -> ****99.87
 */
class CheckAmount {
public:
    // 从字符串输入构造（如："99,999.99", "1234.56"）
    explicit CheckAmount(std::string_view input);

    // 获取保护格式字符串（如："****99.87"）
    [[nodiscard]] std::string getProtectedFormat() const;

    // 获取整数部分字符串
    [[nodiscard]] const std::string& getIntegerPart() const { return integer_part_; }

    // 获取小数部分字符串
    [[nodiscard]] const std::string& getDecimalPart() const { return decimal_part_; }

    // 检查是否有小数部分
    [[nodiscard]] bool hasDecimal() const { return !decimal_part_.empty(); }

private:
    std::string integer_part_;   // 整数部分（不含逗号）
    std::string decimal_part_;   // 小数部分（不含小数点）

    // 解析输入字符串
    void parseInput(std::string_view input);

    // 验证字符是否合法（数字、逗号或小数点）
    [[nodiscard]] static bool isValidChar(char c);

    // 移除整数部分的前导零
    void removeLeadingZeros();

    // 验证解析后的金额
    void validate() const;
};

/**
 * InputHandler - 输入处理类
 *
 * 处理用户输入的读取和验证
 */
class InputHandler {
public:
    // 从用户输入读取有效金额
    [[nodiscard]] static std::optional<CheckAmount> readAmount();

    // 显示格式化后的支票金额
    static void displayProtected(const CheckAmount& amount);

    // 显示输入提示信息
    static void showPrompt();
};

} // namespace check

#endif // CHECK_PROTECTION_HPP
