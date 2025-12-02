/**
 * board.h
 * 填字谜生成器 - Board 类声明
 *
 * 功能：管理15x15的填字谜棋盘，支持单词放置和显示
 */

#pragma once

#include <string>
#include <vector>
#include <array>

class Board {
public:
    static constexpr int SIZE = 15;
    static constexpr int MAX_WORDS = 20;

    // 单词方向
    enum class Direction { Horizontal, Vertical };

    // 已放置单词的信息
    struct PlacedWord {
        std::string word;
        int row;
        int col;
        Direction dir;
    };

    Board();

    // 获取指定位置的字符
    [[nodiscard]] char getSpot(int row, int col) const;

    // 设置指定位置的字符
    void setSpot(int row, int col, char c);

    // 放置单词列表（自动排列）
    void placeWords(std::vector<std::string>& words);

    // 打印完整解答
    void printSolution() const;

    // 打印空白谜题（答案用 # 表示）
    void printPuzzle() const;

    // 打印字谜线索
    void printClues() const;

    // 获取已放置的单词数量
    [[nodiscard]] int getPlacedCount() const { return placedCount_; }

    // 获取已放置的单词列表
    [[nodiscard]] const std::vector<PlacedWord>& getPlacedWords() const { return placedWords_; }

private:
    // 15x15 棋盘
    std::array<std::array<char, SIZE>, SIZE> grid_;

    // 已放置的单词信息
    std::vector<PlacedWord> placedWords_;
    int placedCount_ = 0;

    // 尝试水平放置单词，返回最佳位置的得分
    int tryPlaceHorizontal(const std::string& word, int& bestRow, int& bestCol) const;

    // 尝试垂直放置单词，返回最佳位置的得分
    int tryPlaceVertical(const std::string& word, int& bestRow, int& bestCol) const;

    // 检查单词是否可以放置在指定位置
    [[nodiscard]] int evaluatePosition(const std::string& word, int row, int col, Direction dir) const;

    // 在指定位置放置单词
    void placeWord(const std::string& word, int row, int col, Direction dir);

    // 生成字谜（打乱字母顺序）
    static std::string scramble(const std::string& word);

    // 获取方向字符串
    static std::string directionStr(Direction dir);
};
