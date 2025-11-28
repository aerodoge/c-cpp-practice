#include "check_protection.hpp"
#include <iostream>
#include <iomanip>
#include <string>

void printUsage() {
    std::cout << "\n";
    std::cout << "================================\n";
    std::cout << "      支 票 保 护 系 统         \n";
    std::cout << "================================\n";
    std::cout << "\n";
    std::cout << "本系统将货币金额格式化，使用保护星号防止支票被篡改。\n";
    std::cout << "\n";
    std::cout << "输入规则：\n";
    std::cout << "  - 总长度最大9个字符\n";
    std::cout << "  - 整数部分：最多5位数字（99999）\n";
    std::cout << "  - 小数部分：最多2位数字（.99）\n";
    std::cout << "  - 合法字符：数字、逗号、小数点\n";
    std::cout << "\n";
    std::cout << "示例：\n";
    std::cout << "  输入: 99,999.99 -> 输出: *99999.99\n";
    std::cout << "  输入: 99.87     -> 输出: ****99.87\n";
    std::cout << "  输入: 1234      -> 输出: *****1234\n";
    std::cout << "\n";
    std::cout << "输入 'q' 或 'quit' 退出程序。\n";
    std::cout << "================================\n\n";
}

void runDemo() {
    std::cout << "\n=== 演示模式 ===\n\n";

    // 测试用例
    const std::string test_cases[] = {
        "99,999.99",  // 完整9个字符
        "10000.00",   // 无逗号
        "9999.99",    // 7个字符
        "09,999.99",  // 带前导零
        "999.99",     // 6个字符
        "999",        // 仅整数
        "99.99",      // 5个字符
        "99.",        // 带尾部小数点
        "9.99",       // 4个字符
        "0.99",       // 小于1
        ".99"         // 仅小数
    };

    for (const auto& test : test_cases) {
        std::cout << "输入: " << std::setw(12) << std::left << test;
        try {
            check::CheckAmount amount(test);
            std::cout << " -> 输出: " << amount.getProtectedFormat() << "\n";
        } catch (const check::InvalidAmountError& e) {
            std::cout << " -> 错误: " << e.what() << "\n";
        }
    }
    std::cout << "\n";
}

int main() {
    printUsage();

    // 运行演示
    runDemo();

    // 交互模式
    std::cout << "=== 交互模式 ===\n\n";

    while (true) {
        check::InputHandler::showPrompt();

        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }

        // 去除首尾空白字符
        auto start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue;
        }
        auto end = input.find_last_not_of(" \t\r\n");
        input = input.substr(start, end - start + 1);

        // 检查退出命令
        if (input == "q" || input == "quit" || input == "exit") {
            std::cout << "\n再见!\n";
            break;
        }

        // 处理输入
        try {
            check::CheckAmount amount(input);
            check::InputHandler::displayProtected(amount);
        } catch (const check::InvalidAmountError& e) {
            std::cerr << "\n错误: " << e.what() << "\n";
            std::cerr << "请重新输入。\n";
        }

        std::cout << "\n";
    }

    return 0;
}
