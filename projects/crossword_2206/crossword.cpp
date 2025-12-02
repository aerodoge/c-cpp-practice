/**
 * crossword.cpp
 * 填字谜生成器 - 主程序
 *
 * 用法:
 *   ./crossword                      交互式输入单词
 *   ./crossword words.txt            从文件读取单词
 *   ./crossword words.txt output.txt 输出到文件
 */

#include "board.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

// 验证单词是否有效（只包含字母）
bool isValidWord(const std::string& word) {
    if (word.empty() || word.length() > Board::SIZE) {
        return false;
    }
    return std::all_of(word.begin(), word.end(),
                       [](char c) { return std::isalpha(static_cast<unsigned char>(c)); });
}

// 从标准输入读取单词
std::vector<std::string> readWordsFromInput() {
    std::vector<std::string> words;

    std::cout << "===== 填字谜生成器 (Crossword Generator) =====\n\n";
    std::cout << "请输入单词（每行一个，最多 " << Board::MAX_WORDS << " 个）:\n";
    std::cout << "输入 '.' 或按 Ctrl+D 结束输入\n\n";

    std::string word;
    while (words.size() < Board::MAX_WORDS && std::cin >> word) {
        if (word == ".") break;

        if (!isValidWord(word)) {
            std::cerr << "警告: '" << word << "' 无效（只能包含字母且长度不超过 "
                      << Board::SIZE << "），已跳过\n";
            continue;
        }

        words.push_back(word);
        std::cout << "  已添加: " << word << " (" << words.size() << "/" << Board::MAX_WORDS << ")\n";
    }

    return words;
}

// 从文件读取单词
std::vector<std::string> readWordsFromFile(const std::string& filename) {
    std::vector<std::string> words;
    std::ifstream infile(filename);

    if (!infile) {
        std::cerr << "错误: 无法打开文件 '" << filename << "'\n";
        return words;
    }

    std::string word;
    while (words.size() < Board::MAX_WORDS && infile >> word) {
        if (!isValidWord(word)) {
            std::cerr << "警告: '" << word << "' 无效，已跳过\n";
            continue;
        }
        words.push_back(word);
    }

    std::cout << "从文件 '" << filename << "' 读取了 " << words.size() << " 个单词\n";
    return words;
}

// 输出结果
void outputResults(Board& board, std::ostream& out, bool isFile = false) {
    // 保存和重定向 cout
    std::streambuf* coutBuf = nullptr;
    if (isFile) {
        coutBuf = std::cout.rdbuf();
        std::cout.rdbuf(out.rdbuf());
    }

    if (board.getPlacedCount() == 0) {
        std::cout << "\n没有成功放置任何单词。\n";
    } else {
        std::cout << "\n成功放置了 " << board.getPlacedCount() << " 个单词\n";
        board.printSolution();
        board.printPuzzle();
        board.printClues();
    }

    // 恢复 cout
    if (isFile && coutBuf) {
        std::cout.rdbuf(coutBuf);
    }
}

void showUsage(const char* progName) {
    std::cout << "用法:\n";
    std::cout << "  " << progName << "                      交互式输入单词\n";
    std::cout << "  " << progName << " <输入文件>           从文件读取单词\n";
    std::cout << "  " << progName << " <输入文件> <输出文件> 输出结果到文件\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> words;

    // 根据参数决定输入方式
    if (argc == 1) {
        // 交互式输入
        words = readWordsFromInput();
    } else if (argc == 2) {
        // 从文件读取
        words = readWordsFromFile(argv[1]);
    } else if (argc == 3) {
        // 从文件读取，输出到文件
        words = readWordsFromFile(argv[1]);
    } else {
        showUsage(argv[0]);
        return 1;
    }

    if (words.empty()) {
        std::cerr << "错误: 没有有效的单词可以处理\n";
        return 1;
    }

    // 创建棋盘并放置单词
    Board board;
    board.placeWords(words);

    // 输出结果
    if (argc == 3) {
        std::ofstream outfile(argv[2]);
        if (!outfile) {
            std::cerr << "错误: 无法创建输出文件 '" << argv[2] << "'\n";
            return 1;
        }
        outputResults(board, outfile, true);
        std::cout << "结果已保存到 '" << argv[2] << "'\n";
    } else {
        outputResults(board, std::cout);
    }

    return 0;
}
