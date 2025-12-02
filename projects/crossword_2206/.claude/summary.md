# Crossword Generator 项目总结

## 项目概述

填字谜生成器 (Crossword Generator) - 参考 [scattana/Crossword-Anagram-CPP](https://github.com/scattana/Crossword-Anagram-CPP) 实现。

用户输入单词列表，程序自动在 15x15 棋盘上排列单词并生成字谜线索。

## 文件结构

```
crossword_2206/
├── board.h           # Board 类声明
├── board.cpp         # Board 类实现
├── crossword.cpp     # 主程序
├── Makefile          # 编译脚本
├── CMakeLists.txt    # CMake 配置
├── samplewords.txt   # 示例单词文件
└── samplewords2.txt  # 示例单词文件
```

## 核心类

### Board 类 (`board.h`, `board.cpp`)

- **常量**: `SIZE = 15`, `MAX_WORDS = 20`
- **成员**:
  - `grid_`: 15x15 字符数组 (`std::array<std::array<char, SIZE>, SIZE>>`)
  - `placedWords_`: 已放置单词列表
  - `placedCount_`: 已放置单词计数（头文件中有默认初始化 `= 0`）

- **主要方法**:
  - `placeWords()`: 自动排列单词（最长优先，交替水平/垂直）
  - `printSolution()`: 打印完整解答
  - `printPuzzle()`: 打印空白谜题
  - `printClues()`: 打印字谜线索（打乱字母顺序）

- **放置算法**:
  1. 按长度降序排序
  2. 最长单词居中水平放置
  3. 剩余单词交替尝试垂直/水平放置
  4. 评估位置得分（交叉点加分，冲突返回0）

## 使用方式

```bash
make                              # 编译
./crossword                       # 交互式输入
./crossword samplewords.txt       # 从文件读取
./crossword words.txt output.txt  # 输出到文件
```

## 输出示例

程序输出三部分：
1. **解答 (Solution)**: 显示所有单词的完整棋盘
2. **谜题 (Puzzle)**: 用 `#` 隐藏答案的棋盘
3. **字谜线索 (Clues)**: 打乱字母的单词 + 位置 + 方向

## C++ 特性

- C++20 标准
- `std::array` 用于固定大小棋盘
- `std::ranges::sort`, `std::ranges::shuffle`
- 结构化绑定 (`const auto& [word, row, col, dir]`)
- `[[nodiscard]]` 属性
- 成员初始化列表

## 注意事项

- `grid_` 需要在构造函数初始化列表中显式初始化 (`grid_{}`)，因为 `std::array` 不会自动初始化元素
- `placedCount_` 在头文件中有默认成员初始化器 `= 0`，无需在构造函数中重复初始化
- `std::vector` 默认构造为空，无需显式 `{}`
