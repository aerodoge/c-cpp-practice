#include "check_protection.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <sstream>

namespace check {

// ============================================================================
// CheckAmount 类实现
// ============================================================================

CheckAmount::CheckAmount(std::string_view input) {
    if (input.empty()) {
        throw InvalidAmountError("输入不能为空");
    }
    parseInput(input);
    removeLeadingZeros();
    validate();
}

bool CheckAmount::isValidChar(char c) {
    // 合法字符：数字、逗号、小数点
    return std::isdigit(static_cast<unsigned char>(c)) || c == ',' || c == '.';
}

void CheckAmount::parseInput(std::string_view input) {
    // 检查所有字符是否合法
    for (char c : input) {
        if (!isValidChar(c)) {
            throw InvalidAmountError("非法字符: '" + std::string(1, c) +
                "'。只允许数字、逗号和小数点。");
        }
    }

    // 查找小数点位置
    auto dot_pos = input.find('.');

    // 检查是否有多个小数点
    if (dot_pos != std::string_view::npos &&
        input.find('.', dot_pos + 1) != std::string_view::npos) {
        throw InvalidAmountError("发现多个小数点");
    }

    // 分割整数部分和小数部分
    std::string_view int_part;
    std::string_view dec_part;

    if (dot_pos != std::string_view::npos) {
        int_part = input.substr(0, dot_pos);
        dec_part = input.substr(dot_pos + 1);
    } else {
        int_part = input;
    }

    // 处理整数部分（移除逗号）
    integer_part_.clear();
    for (char c : int_part) {
        if (c != ',') {
            integer_part_ += c;
        }
    }

    // 处理空整数部分（如：".99"）
    if (integer_part_.empty()) {
        integer_part_ = "0";
    }

    // 处理小数部分
    decimal_part_ = std::string(dec_part);

    // 检查小数部分不包含逗号
    if (decimal_part_.find(',') != std::string::npos) {
        throw InvalidAmountError("小数部分不允许使用逗号");
    }
}

void CheckAmount::removeLeadingZeros() {
    // 移除前导零，但至少保留一位数字
    size_t first_nonzero = integer_part_.find_first_not_of('0');
    if (first_nonzero == std::string::npos) {
        // 全是零，保留一个
        integer_part_ = "0";
    } else if (first_nonzero > 0) {
        integer_part_ = integer_part_.substr(first_nonzero);
    }
}

void CheckAmount::validate() const {
    // 检查整数部分长度（最大5位：99999）
    if (integer_part_.length() > MAX_INTEGER_DIGITS) {
        throw InvalidAmountError("整数部分超过最大" +
            std::to_string(MAX_INTEGER_DIGITS) + "位数字");
    }

    // 检查小数部分长度（最大2位）
    if (decimal_part_.length() > MAX_DECIMAL_DIGITS) {
        throw InvalidAmountError("小数部分超过最大" +
            std::to_string(MAX_DECIMAL_DIGITS) + "位数字");
    }

    // 验证整数和小数部分只包含数字
    for (char c : integer_part_) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            throw InvalidAmountError("整数部分包含非法字符");
        }
    }
    for (char c : decimal_part_) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            throw InvalidAmountError("小数部分包含非法字符");
        }
    }

    // 计算总显示长度
    size_t total_len = integer_part_.length();
    if (!decimal_part_.empty()) {
        total_len += 1 + decimal_part_.length();  // +1 是小数点
    }

    if (total_len > MAX_DISPLAY_WIDTH) {
        throw InvalidAmountError("金额总长度超过" +
            std::to_string(MAX_DISPLAY_WIDTH) + "个字符");
    }
}

std::string CheckAmount::getProtectedFormat() const {
    std::ostringstream oss;

    // 计算实际内容长度
    size_t content_len = integer_part_.length();
    if (!decimal_part_.empty()) {
        content_len += 1 + decimal_part_.length();  // +1 是小数点
    }

    // 计算需要填充的星号数量
    size_t padding = (MAX_DISPLAY_WIDTH > content_len) ?
                     (MAX_DISPLAY_WIDTH - content_len) : 0;

    // 添加保护星号
    oss << std::string(padding, FILL_CHAR);

    // 添加金额
    oss << integer_part_;
    if (!decimal_part_.empty()) {
        oss << '.' << decimal_part_;
    }

    return oss.str();
}

// ============================================================================
// InputHandler 类实现
// ============================================================================

void InputHandler::showPrompt() {
    std::cout << "请输入金额（最多9位，如：99,999.99）：";
}

std::optional<CheckAmount> InputHandler::readAmount() {
    std::string input;
    std::getline(std::cin, input);

    // 移除首尾空白字符
    auto start = input.find_first_not_of(" \t\r\n");
    auto end = input.find_last_not_of(" \t\r\n");

    if (start == std::string::npos) {
        return std::nullopt;
    }

    input = input.substr(start, end - start + 1);

    try {
        return CheckAmount(input);
    } catch (const InvalidAmountError& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return std::nullopt;
    }
}

void InputHandler::displayProtected(const CheckAmount& amount) {
    std::cout << "\n";
    std::cout << "===========================\n";
    std::cout << "    支票保护格式输出       \n";
    std::cout << "===========================\n";
    std::cout << "   " << amount.getProtectedFormat() << "\n";
    std::cout << "   ---------\n";
    std::cout << "   123456789\n";
    std::cout << "===========================\n";

    // 显示解析后的值用于验证
    std::cout << "\n解析结果：\n";
    std::cout << "  整数部分: " << amount.getIntegerPart() << "\n";
    if (amount.hasDecimal()) {
        std::cout << "  小数部分: " << amount.getDecimalPart() << "\n";
    }
}

} // namespace check
