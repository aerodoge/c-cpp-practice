/**
 * board.cpp
 * 填字谜生成器 - Board 类实现
 */

#include "board.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <iomanip>

Board::Board() : grid_{} {
    // 初始化棋盘为空白
    for (auto& row : grid_) {
        row.fill('.');
    }
}

char Board::getSpot(const int row, const int col) const {
    if (row >= 0 && row < SIZE && col >= 0 && col < SIZE) {
        return grid_[row][col];
    }
    return '\0';
}

void Board::setSpot(const int row, const int col, const char c) {
    if (row >= 0 && row < SIZE && col >= 0 && col < SIZE) {
        grid_[row][col] = c;
    }
}

void Board::placeWords(std::vector<std::string>& words) {
    if (words.empty()) return;

    // 按长度降序排序（最长的先放）
    std::ranges::sort(words.begin(), words.end(),
              [](const std::string& a, const std::string& b) {
                  return a.length() > b.length();
              });

    // 转换为大写
    for (auto& word : words) {
        std::ranges::transform(word.begin(), word.end(), word.begin(), toupper);
    }

    // 放置第一个单词（最长的）在中央水平位置
    const std::string& firstWord = words[0];
    constexpr int startRow = SIZE / 2;
    const int startCol = (SIZE - static_cast<int>(firstWord.length())) / 2;
    placeWord(firstWord, startRow, startCol, Direction::Horizontal);

    // 交替尝试垂直和水平放置剩余单词
    bool tryVerticalFirst = true;

    for (size_t i = 1; i < words.size() && placedCount_ < MAX_WORDS; ++i) {
        const std::string& word = words[i];
        int bestRow = -1, bestCol = -1;
        int bestScore = 0;
        auto bestDir = Direction::Horizontal;

        if (tryVerticalFirst) {
            // 先尝试垂直
            int score = tryPlaceVertical(word, bestRow, bestCol);
            if (score > 0) {
                bestScore = score;
                bestDir = Direction::Vertical;
            }
            // 再尝试水平
            int hRow, hCol;
            score = tryPlaceHorizontal(word, hRow, hCol);
            if (score > bestScore) {
                bestScore = score;
                bestRow = hRow;
                bestCol = hCol;
                bestDir = Direction::Horizontal;
            }
        } else {
            // 先尝试水平
            int score = tryPlaceHorizontal(word, bestRow, bestCol);
            if (score > 0) {
                bestScore = score;
                bestDir = Direction::Horizontal;
            }
            // 再尝试垂直
            int vRow, vCol;
            score = tryPlaceVertical(word, vRow, vCol);
            if (score > bestScore) {
                bestScore = score;
                bestRow = vRow;
                bestCol = vCol;
                bestDir = Direction::Vertical;
            }
        }

        if (bestScore > 0) {
            placeWord(word, bestRow, bestCol, bestDir);
        }

        tryVerticalFirst = !tryVerticalFirst;
    }
}

int Board::tryPlaceHorizontal(const std::string& word, int& bestRow, int& bestCol) const {
    int bestScore = 0;
    const int wordLen = static_cast<int>(word.length());

    for (int row = 0; row < SIZE; ++row) {
        for (int col = 0; col <= SIZE - wordLen; ++col) {
            if (const int score = evaluatePosition(word, row, col, Direction::Horizontal); score > bestScore) {
                bestScore = score;
                bestRow = row;
                bestCol = col;
            }
        }
    }

    return bestScore;
}

int Board::tryPlaceVertical(const std::string& word, int& bestRow, int& bestCol) const {
    int bestScore = 0;
    const int wordLen = static_cast<int>(word.length());

    for (int row = 0; row <= SIZE - wordLen; ++row) {
        for (int col = 0; col < SIZE; ++col) {
            if (const int score = evaluatePosition(word, row, col, Direction::Vertical); score > bestScore) {
                bestScore = score;
                bestRow = row;
                bestCol = col;
            }
        }
    }

    return bestScore;
}

int Board::evaluatePosition(const std::string& word, const int row, const int col, const Direction dir) const {
    const int wordLen = static_cast<int>(word.length());
    int score = 0;
    int matchCount = 0;

    // 检查单词前后是否有空间
    if (dir == Direction::Horizontal) {
        // 检查左边
        if (col > 0 && grid_[row][col - 1] != '.') return 0;
        // 检查右边
        if (col + wordLen < SIZE && grid_[row][col + wordLen] != '.') return 0;
    } else {
        // 检查上边
        if (row > 0 && grid_[row - 1][col] != '.') return 0;
        // 检查下边
        if (row + wordLen < SIZE && grid_[row + wordLen][col] != '.') return 0;
    }

    for (int i = 0; i < wordLen; ++i) {
        const int r = dir == Direction::Horizontal ? row : row + i;
        const int c = dir == Direction::Horizontal ? col + i : col;
        const char boardChar = grid_[r][c];
        const char wordChar = word[i];

        if (boardChar == '.') {
            // 空位置 - 检查相邻位置
            if (dir == Direction::Horizontal) {
                // 水平放置时检查上下
                if (r > 0 && grid_[r - 1][c] != '.') return 0;
                if (r < SIZE - 1 && grid_[r + 1][c] != '.') return 0;
            } else {
                // 垂直放置时检查左右
                if (c > 0 && grid_[r][c - 1] != '.') return 0;
                if (c < SIZE - 1 && grid_[r][c + 1] != '.') return 0;
            }
        } else if (boardChar == wordChar) {
            // 字母匹配 - 交叉点
            matchCount++;
            score += 10;
        } else {
            // 字母冲突
            return 0;
        }
    }

    // 必须有至少一个交叉点（除了第一个单词）
    if (placedCount_ > 0 && matchCount == 0) {
        return 0;
    }

    // 基础分数
    score += 1;

    return score;
}

void Board::placeWord(const std::string& word, const int row, const int col, const Direction dir) {
    const int wordLen = static_cast<int>(word.length());

    for (int i = 0; i < wordLen; ++i) {
        const int r = dir == Direction::Horizontal ? row : row + i;
        const int c = dir == Direction::Horizontal ? col + i : col;
        grid_[r][c] = word[i];
    }

    placedWords_.push_back({word, row + 1, col + 1, dir});  // 转换为1-based索引
    placedCount_++;
}

void Board::printSolution() const {
    std::cout << "\n===== 解答 (Solution) =====\n\n";
    std::cout << "   ";
    for (int c = 1; c <= SIZE; ++c) {
        std::cout << std::setw(2) << c << " ";
    }
    std::cout << "\n";

    for (int r = 0; r < SIZE; ++r) {
        std::cout << std::setw(2) << (r + 1) << " ";
        for (int c = 0; c < SIZE; ++c) {
            std::cout << " " << grid_[r][c] << " ";
        }
        std::cout << "\n";
    }
}

void Board::printPuzzle() const {
    std::cout << "\n===== 谜题 (Puzzle) =====\n\n";
    std::cout << "   ";
    for (int c = 1; c <= SIZE; ++c) {
        std::cout << std::setw(2) << c << " ";
    }
    std::cout << "\n";

    for (int r = 0; r < SIZE; ++r) {
        std::cout << std::setw(2) << (r + 1) << " ";

        for (int c = 0; c < SIZE; ++c) {
            if (const char ch = grid_[r][c];ch == '.') {
                std::cout << " . ";
            } else {
                std::cout << " # ";  // 隐藏答案
            }
        }
        std::cout << "\n";
    }
}

void Board::printClues() const {
    std::cout << "\n===== 字谜线索 (Anagram Clues) =====\n\n";
    std::cout << std::left << std::setw(6) << "编号"
              << std::setw(18) << "线索(Clue)"
              << std::setw(8) << "行(Row)"
              << std::setw(8) << "列(Col)"
              << "方向(Direction)\n";
    std::cout << std::string(50, '-') << "\n";

    int num = 1;
    for (const auto& [word, row, col, dir] : placedWords_) {
        std::cout << std::left << std::setw(6) << num++
                  << std::setw(18) << scramble(word)
                  << std::setw(8) << row
                  << std::setw(8) << col
                  << directionStr(dir) << "\n";
    }
}

std::string Board::scramble(const std::string& word) {
    std::string result = word;
    std::random_device rd;
    std::mt19937 gen(rd());

    // 多次打乱确保充分混合
    for (int i = 0; i < 3; ++i) {
        std::ranges::shuffle(result.begin(), result.end(), gen);
    }

    // 如果打乱后和原词相同，再打乱一次
    if (result == word && word.length() > 1) {
        std::ranges::shuffle(result.begin(), result.end(), gen);
    }

    return result;
}

std::string Board::directionStr(const Direction dir) {
    return dir == Direction::Horizontal ? "横向(Across)" : "纵向(Down)";
}
