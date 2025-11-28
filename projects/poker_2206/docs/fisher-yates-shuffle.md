# Fisher-Yates 洗牌算法详解

## 概述

Fisher-Yates 洗牌算法（也称为 Knuth shuffle）是一个**公平、高效**的随机洗牌算法，由 Ronald Fisher 和 Frank Yates 在 1938 年提出，后由 Donald Knuth 在《计算机程序设计艺术》中推广。

## 算法原理

### 核心思想

从后往前遍历数组，每次将当前位置的元素与**它之前的随机位置**（包括自己）的元素交换。

### 为什么从后往前？

- 确保每个元素都有机会被移动到任意位置
- 保证每种排列出现的概率完全相同（完美随机）

## 项目实现

在本项目的 `src/Deck.cpp:21-29` 中实现：

```cpp
void Deck::shuffle() {
    // Fisher-Yates 洗牌算法 - O(n) 时间复杂度，性能优异
    for (size_t i = DECK_SIZE - 1; i > 0; --i) {
        std::uniform_int_distribution<size_t> dist(0, i);  // 生成 [0, i] 的随机数
        size_t j = dist(rng_);                              // 随机选择一个位置
        std::swap(cards_[i], cards_[j]);                    // 交换两张牌
    }
    currentIndex_ = 0;
}
```

### 关键组件

**随机数生成器**（`Deck.h:44`）：
```cpp
std::mt19937 rng_;  // Mersenne Twister 19937 随机数引擎
```

**初始化**（`Deck.cpp:8`）：
```cpp
rng_(std::chrono::steady_clock::now().time_since_epoch().count())
```

使用当前时间作为种子，确保每次运行产生不同的洗牌结果。

## 算法演示

假设有 5 张牌 `[A, B, C, D, E]`，下标 `[0, 1, 2, 3, 4]`：

```
初始状态: [A, B, C, D, E]
         ↑  ↑  ↑  ↑  ↑
         0  1  2  3  4

第 1 轮: i = 4
  - 从 [0, 4] 中随机选位置 → 假设选到 j = 1
  - 交换 cards[4] 和 cards[1]
  - 结果: [A, E, C, D, B]
          　  ↑        ↑
          　  └────────┘

第 2 轮: i = 3
  - 从 [0, 3] 中随机选位置 → 假设选到 j = 3
  - 交换 cards[3] 和 cards[3]（自己）
  - 结果: [A, E, C, D, B]

第 3 轮: i = 2
  - 从 [0, 2] 中随机选位置 → 假设选到 j = 0
  - 交换 cards[2] 和 cards[0]
  - 结果: [C, E, A, D, B]
          ↑     ↑
          └─────┘

第 4 轮: i = 1
  - 从 [0, 1] 中随机选位置 → 假设选到 j = 1
  - 交换 cards[1] 和 cards[1]（自己）
  - 结果: [C, E, A, D, B]

结束: i = 0 时循环停止
最终结果: [C, E, A, D, B] ✓ 洗牌完成
```

## 算法特性

### 1. 时间复杂度：O(n)

- 只需遍历一次数组
- 每次迭代只做一次交换操作
- 对于 52 张牌，只需 51 次操作

### 2. 空间复杂度：O(1)

- 原地交换，不需要额外的数组
- 只使用几个临时变量

### 3. 完美随机性

每种排列出现的概率完全相同：

- n 个元素的排列总数：`n!`
- 每种排列的概率：`1/n!`
- 对于 52 张扑克牌：
  - 总排列数：`52! ≈ 8.07 × 10⁶⁷`
  - 每种排列概率：`1/52!`

**数学证明**：
- 第 i 个位置选中某个特定元素的概率是 `1/(i+1)`
- 所有位置的独立概率相乘：`1/n × 1/(n-1) × ... × 1/2 × 1/1 = 1/n!`

### 4. 实现简洁

- 核心代码只有 5 行
- 逻辑清晰易懂
- 不易出错

## 常见错误实现

### ❌ 错误做法 1：完全随机交换

```cpp
// 错误！不是均匀分布
for (size_t i = 0; i < DECK_SIZE; ++i) {
    size_t j = random(0, DECK_SIZE - 1);  // 从整个数组随机选
    std::swap(cards_[i], cards_[j]);
}
```

**问题**：
- 某些排列出现的概率 > 其他排列
- 不公平！偏向某些特定排列

**原因**：
- 每个位置都从全部范围选择
- 总共产生 `n^n` 种可能，但只有 `n!` 种排列
- `n^n` 不能被 `n!` 整除 → 分布不均

### ❌ 错误做法 2：排序 + 随机键

```cpp
// 效率低！O(n log n)
for (auto& card : cards_) {
    card.random_key = random();
}
std::sort(cards_.begin(), cards_.end(), [](auto& a, auto& b) {
    return a.random_key < b.random_key;
});
```

**问题**：
- 时间复杂度：`O(n log n)`，比 Fisher-Yates 的 `O(n)` 慢
- 需要额外空间存储随机键
- 代码更复杂

### ❌ 错误做法 3：多次随机交换

```cpp
// 错误！不是均匀分布
for (int i = 0; i < 1000; ++i) {
    size_t a = random(0, DECK_SIZE - 1);
    size_t b = random(0, DECK_SIZE - 1);
    std::swap(cards_[a], cards_[b]);
}
```

**问题**：
- 仍然不是均匀分布
- 效率低（需要多次迭代）
- 无法保证公平性

## C++ 实现要点

### 1. 使用现代随机数生成器

```cpp
#include <random>

// ✓ 好的做法
std::mt19937 rng(seed);                    // Mersenne Twister
std::uniform_int_distribution<size_t> dist(0, i);
size_t j = dist(rng);

// ✗ 不好的做法
size_t j = rand() % (i + 1);               // rand() 质量差，分布不均
```

### 2. std::mt19937 的优势

- **高质量**：周期长达 2^19937-1
- **快速**：性能优于许多老式 RNG
- **可移植**：C++11 标准库，跨平台一致
- **可重现**：相同种子产生相同序列（便于测试）

### 3. 使用 std::uniform_int_distribution

- 确保整数均匀分布在 `[a, b]` 范围
- 避免模运算导致的偏差（`rand() % n` 的问题）

### 4. 循环边界

```cpp
for (size_t i = DECK_SIZE - 1; i > 0; --i)  // 注意：i > 0，不是 i >= 0
```

- 从最后一个元素开始（`i = n-1`）
- 到第二个元素结束（`i = 1`）
- 第一个元素（`i = 0`）不需要交换

## 应用场景

### 1. 卡牌游戏
- 扑克牌洗牌（本项目）
- 桌游、纸牌游戏

### 2. 随机化算法
- 随机抽样
- Monte Carlo 模拟
- 随机化测试

### 3. 数据处理
- 数据集打乱（机器学习）
- 随机化顺序避免偏差

### 4. 其他场景
- 音乐播放器的随机播放
- 抽奖系统
- 任何需要公平随机排列的场景

## 性能对比

| 算法 | 时间复杂度 | 空间复杂度 | 随机性 | 实现难度 |
|------|-----------|-----------|--------|---------|
| Fisher-Yates | O(n) | O(1) | 完美 | 简单 |
| 排序+随机键 | O(n log n) | O(n) | 完美 | 中等 |
| 多次随机交换 | O(n²) | O(1) | 不均匀 | 简单 |
| 错误的随机交换 | O(n) | O(1) | 不均匀 | 简单 |

## 扩展：反向 Fisher-Yates

也可以从前往后遍历：

```cpp
void shuffle_forward() {
    for (size_t i = 0; i < DECK_SIZE - 1; ++i) {
        std::uniform_int_distribution<size_t> dist(i, DECK_SIZE - 1);
        size_t j = dist(rng_);
        std::swap(cards_[i], cards_[j]);
    }
}
```

- 从 `[i, n-1]` 中随机选择位置 j
- 与标准版本等价，只是方向相反
- 本项目使用的是**从后往前**的经典版本

## 历史与命名

- **1938**：Ronald Fisher 和 Frank Yates 首次发表（纸笔版本）
- **1964**：Richard Durstenfeld 改进为计算机版本
- **1969**：Donald Knuth 在《计算机程序设计艺术》卷 2 中推广
- **别名**：Knuth shuffle、Durstenfeld shuffle

## 总结

Fisher-Yates 算法是**业界标准**的洗牌算法，原因是：

- ✅ **快速**：O(n) 线性时间
- ✅ **省空间**：O(1) 原地操作
- ✅ **公平**：完美均匀分布
- ✅ **简单**：代码量少，逻辑清晰
- ✅ **健壮**：不易出错
- ✅ **可证明**：数学上可证明其正确性

几乎所有需要随机洗牌的生产环境代码都应该使用这个算法！

## 参考资料

- Donald E. Knuth, *The Art of Computer Programming*, Volume 2: Seminumerical Algorithms, Section 3.4.2
- [Wikipedia: Fisher-Yates shuffle](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle)
- 本项目实现：`src/Deck.cpp:21-29`
