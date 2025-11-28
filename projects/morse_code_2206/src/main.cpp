#include "morse_code.hpp"
#include <iostream>
#include <string>

void printSeparator() {
    std::cout << std::string(50, '-') << '\n';
}

void demonstrateEncoding(const morse::MorseCode& mc) {
    std::cout << "=== 文本 -> 摩尔斯电码 ===\n\n";

    std::string examples[] = {
        "HELLO WORLD",
        "SOS",
        "HELLO",
        "2024"
    };

    for (const auto& text : examples) {
        std::string encoded = mc.encode(text);
        std::cout << "原文: " << text << '\n';
        std::cout << "编码: " << encoded << '\n';
        printSeparator();
    }
}

void demonstrateDecoding(const morse::MorseCode& mc) {
    std::cout << "\n=== 摩尔斯电码 -> 文本 ===\n\n";

    std::string morse_examples[] = {
        ".... . .-.. .-.. ---   .-- --- .-. .-.. -..",
        "... --- ...",
        ".- -... -.-."
    };

    for (const auto& morse : morse_examples) {
        std::string decoded = mc.decode(morse);
        std::cout << "电码: " << morse << '\n';
        std::cout << "解码: " << decoded << '\n';
        printSeparator();
    }
}

void interactiveMode(const morse::MorseCode& mc) {
    std::cout << "\n=== 交互模式 ===\n";
    std::cout << "输入 'e' 进入编码模式\n";
    std::cout << "输入 'd' 进入解码模式\n";
    std::cout << "输入 'q' 退出\n\n";

    std::string mode;
    while (true) {
        std::cout << "选择模式 (e/d/q): ";
        std::getline(std::cin, mode);

        if (mode == "q" || mode == "Q") {
            std::cout << "再见!\n";
            break;
        }

        if (mode == "e" || mode == "E") {
            std::cout << "输入要编码的文本: ";
            std::string text;
            std::getline(std::cin, text);
            std::string encoded = mc.encode(text);
            std::cout << "摩尔斯电码: " << encoded << "\n\n";
        } else if (mode == "d" || mode == "D") {
            std::cout << "输入摩尔斯电码 (字符间用空格, 单词间用三个空格): ";
            std::string morse;
            std::getline(std::cin, morse);
            std::string decoded = mc.decode(morse);
            std::cout << "解码结果: " << decoded << "\n\n";
        } else {
            std::cout << "无效选项，请重新输入\n";
        }
    }
}

int main() {
    morse::MorseCode mc;

    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║      摩尔斯电码编码器/解码器         ║\n";
    std::cout << "╚══════════════════════════════════════╝\n\n";

    // 演示编码
    demonstrateEncoding(mc);
    // 演示解码
    demonstrateDecoding(mc);

    // 验证往返转换
    std::cout << "\n=== 往返转换验证 ===\n\n";
    const std::string original = "HELLO WORLD";
    const std::string encoded = mc.encode(original);
    const std::string decoded = mc.decode(encoded);
    std::cout << "原文: " << original << '\n';
    std::cout << "编码: " << encoded << '\n';
    std::cout << "解码: " << decoded << '\n';
    std::cout << "验证: " << (original == decoded ? "通过" : "失败") << '\n';
    printSeparator();

    // 交互模式
    interactiveMode(mc);

    return 0;
}
