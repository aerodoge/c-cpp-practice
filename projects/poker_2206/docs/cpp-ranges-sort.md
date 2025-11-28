# C++20 std::ranges::sort 详解

## 概述

`std::ranges::sort` 是 C++20 引入的 Ranges 库中的排序算法，是传统 `std::sort` 的现代化升级版本。它提供了更简洁的语法、更强的类型安全性，以及对投影（projections）的原生支持。

## 为什么要升级到 ranges::sort？

### 传统 std::sort 的问题

```cpp
std::vector<int> vec = {3, 1, 4, 1, 5};

// 需要写 .begin() 和 .end()
std::sort(vec.begin(), vec.end());

// 自定义比较器
std::sort(vec.begin(), vec.end(), std::greater<int>());

// 按对象的某个成员排序（需要完整 lambda）
std::vector<Card> cards;
std::sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
    return a.get_rank() < b.get_rank();
});
```

**问题**：
- 样板代码多（`.begin()`, `.end()`）
- 容易写错（混用不同容器的迭代器）
- 按成员排序需要完整的 lambda

### ranges::sort 的优势

```cpp
std::vector<int> vec = {3, 1, 4, 1, 5};

// ✓ 直接传容器
std::ranges::sort(vec);

// ✓ 使用 ranges 版本的比较器
std::ranges::sort(vec, std::ranges::greater{});

// ✓ 使用投影，无需 lambda
std::vector<Card> cards;
std::ranges::sort(cards, {}, &Card::get_rank);
```

**优势**：
- ✅ 更简洁的语法
- ✅ 更强的类型安全
- ✅ 支持投影（projections）
- ✅ 返回有用的迭代器
- ✅ 与 Ranges Views 完美配合
- ✅ 性能完全相同（零开销抽象）

## 项目中的改进

本项目已全面升级到 `std::ranges::sort`，共改进 **12 处**代码。

### 改进统计

| 文件 | 改进数量 | 主要改进 |
|------|---------|---------|
| Game.cpp | 2处 | 降序排序索引 |
| HandEvaluator.cpp | 5处 | 牌型评估中的排序 |
| Hand.cpp | 1处 | **使用投影优化** |
| Player.cpp | 4处 | AI决策中的排序 |

### 改进示例

#### 示例1：移除 .begin()/.end()（Game.cpp:43）

```cpp
// 改进前
std::sort(aiCardsToReplace.begin(), aiCardsToReplace.end(), std::greater<size_t>());

// 改进后
std::ranges::sort(aiCardsToReplace, std::ranges::greater{});
```

**收益**：
- 代码更短
- 不会错误混用迭代器
- 使用 `std::ranges::greater{}` 而不是 `std::greater<size_t>()`

#### 示例2：使用投影（Hand.cpp:43）✨

这是最大的改进！

```cpp
// 改进前：需要完整的 lambda
std::sort(cards_.begin(), cards_.end(), [](const Card& a, const Card& b) {
    return a.get_rank() < b.get_rank();
});

// 改进后：使用投影，超级简洁！
std::ranges::sort(cards_, {}, &Card::get_rank);
```

**说明**：
- 第一个参数：容器 `cards_`
- 第二个参数：比较器（`{}` 表示默认 `<` 比较）
- 第三个参数：投影函数（按 `Card::get_rank` 排序）

**收益**：
- 从 3 行减少到 1 行
- 意图更清晰：明确表示"按 rank 排序"
- 性能相同（编译器优化后代码一样）

#### 示例3：升序排序（HandEvaluator.cpp:46）

```cpp
// 改进前
std::vector<Rank> sortedRanks = ranks;
std::sort(sortedRanks.begin(), sortedRanks.end());

// 改进后
std::vector<Rank> sortedRanks = ranks;
std::ranges::sort(sortedRanks);
```

**收益**：更简洁，默认升序排序。

#### 示例4：降序排序（HandEvaluator.cpp:127）

```cpp
// 改进前
std::sort(kickers.begin(), kickers.end(), std::greater<Rank>());

// 改进后
std::ranges::sort(kickers, std::ranges::greater{});
```

**收益**：使用 `std::ranges::greater{}`，类型自动推导。

## ranges::sort 完整语法

### 基本形式

```cpp
std::ranges::sort(range);                    // 升序
std::ranges::sort(range, comparator);        // 自定义比较器
std::ranges::sort(range, comp, projection);  // 带投影
std::ranges::sort(begin, end);               // 仍支持迭代器对
std::ranges::sort(begin, end, comp, proj);   // 完整形式
```

### 参数说明

1. **range**：可排序的范围（容器、数组、子范围等）
2. **comparator**：比较器（可选，默认 `<`）
3. **projection**：投影函数（可选，用于提取比较的键）

## 使用场景

### 1. 基本排序

```cpp
std::vector<int> vec = {3, 1, 4, 1, 5};

// 升序
std::ranges::sort(vec);  // {1, 1, 3, 4, 5}

// 降序
std::ranges::sort(vec, std::ranges::greater{});  // {5, 4, 3, 1, 1}
```

### 2. 排序容器的一部分

```cpp
std::vector<int> vec = {5, 2, 8, 1, 9};

// 只排序前3个元素
std::ranges::sort(vec.begin(), vec.begin() + 3);  // {2, 5, 8, 1, 9}

// 或使用 subrange
auto subrange = std::ranges::subrange(vec.begin(), vec.begin() + 3);
std::ranges::sort(subrange);
```

### 3. 使用投影排序对象

```cpp
struct Player {
    std::string name;
    int score;
};

std::vector<Player> players = {
    {"Alice", 100},
    {"Bob", 200},
    {"Charlie", 150}
};

// 按分数排序
std::ranges::sort(players, {}, &Player::score);
// 结果：Alice(100), Charlie(150), Bob(200)

// 按分数降序排序
std::ranges::sort(players, std::ranges::greater{}, &Player::score);
// 结果：Bob(200), Charlie(150), Alice(100)

// 按名字排序
std::ranges::sort(players, {}, &Player::name);
// 结果：Alice, Bob, Charlie
```

### 4. 自定义比较器

```cpp
std::vector<int> vec = {1, 2, 3, 4, 5};

// 按绝对值排序
std::ranges::sort(vec, [](int a, int b) {
    return std::abs(a) < std::abs(b);
});

// 按奇偶性排序（奇数在前）
std::ranges::sort(vec, [](int a, int b) {
    return (a % 2) > (b % 2);
});
```

### 5. 结合投影和比较器

```cpp
struct Card {
    Suit suit;
    Rank rank;
};

std::vector<Card> cards;

// 按 rank 降序排序
std::ranges::sort(cards, std::ranges::greater{}, &Card::rank);

// 先按 suit 排序，再按 rank 排序（需要自定义比较器）
std::ranges::sort(cards, [](const Card& a, const Card& b) {
    if (a.suit != b.suit) return a.suit < b.suit;
    return a.rank < b.rank;
});
```

### 6. 返回值的使用

```cpp
std::vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};

// ranges::sort 返回指向排序后范围末尾的迭代器
auto end = std::ranges::sort(vec);

// 可以继续操作
std::cout << "Sorted " << std::distance(vec.begin(), end) << " elements\n";

// 结合其他算法
auto [first_dup, last] = std::ranges::unique(vec);  // 去重
vec.erase(first_dup, last);
```

## 投影（Projections）详解

投影是 Ranges 库最强大的特性之一。

### 什么是投影？

投影是一个函数，用于从对象中"提取"用于比较的键。

```cpp
struct Person {
    std::string name;
    int age;
};

std::vector<Person> people = {
    {"Alice", 30},
    {"Bob", 25},
    {"Charlie", 35}
};

// 投影：从 Person 对象中提取 age 字段进行比较
std::ranges::sort(people, {}, &Person::age);

// 等价于传统写法：
std::sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
    return a.age < b.age;
});
```

### 投影的类型

```cpp
// 1. 成员指针
std::ranges::sort(people, {}, &Person::age);

// 2. Lambda 投影
std::ranges::sort(people, {}, [](const Person& p) { return p.age; });

// 3. 成员函数指针
std::ranges::sort(cards, {}, &Card::get_rank);

// 4. 自定义投影函数
auto get_length = [](const std::string& s) { return s.length(); };
std::ranges::sort(strings, {}, get_length);
```

### 投影 + 比较器

```cpp
struct Person {
    std::string name;
    int age;
};

std::vector<Person> people = {
    {"Alice", 30},
    {"Bob", 25},
    {"Charlie", 35}
};

// 按年龄降序排序
std::ranges::sort(people, std::ranges::greater{}, &Person::age);
//                        ↑                      ↑
//                    比较器（降序）         投影（提取age）

// 结果：Charlie(35), Alice(30), Bob(25)
```

### 复杂投影示例

```cpp
struct Product {
    std::string name;
    double price;
    int stock;
};

std::vector<Product> products;

// 按价格和库存的乘积排序（总价值）
std::ranges::sort(products, {}, [](const Product& p) {
    return p.price * p.stock;
});

// 按名字长度排序
std::ranges::sort(products, {}, [](const Product& p) {
    return p.name.length();
});
```

## 比较器

### ranges 版本的比较器

C++20 提供了 `std::ranges` 命名空间下的比较器：

```cpp
std::ranges::sort(vec, std::ranges::greater{});     // 降序
std::ranges::sort(vec, std::ranges::less{});        // 升序（默认）
std::ranges::sort(vec, std::ranges::greater_equal{});
std::ranges::sort(vec, std::ranges::less_equal{});
```

**优势**：
- 不需要指定类型参数（自动推导）
- 支持异质比较（不同但兼容的类型）

### 传统比较器仍然有效

```cpp
// 仍然可以使用传统比较器
std::ranges::sort(vec, std::greater<int>());
std::ranges::sort(vec, std::less<int>());

// 但 ranges 版本更简洁
std::ranges::sort(vec, std::ranges::greater{});  // 推荐
```

### 自定义比较器

```cpp
// Lambda
std::ranges::sort(vec, [](int a, int b) { return a > b; });

// 函数对象
struct CompareByAbs {
    bool operator()(int a, int b) const {
        return std::abs(a) < std::abs(b);
    }
};
std::ranges::sort(vec, CompareByAbs{});

// 函数指针
bool compare_desc(int a, int b) { return a > b; }
std::ranges::sort(vec, compare_desc);
```

## 与 Views 结合

Ranges 的真正威力在于与 Views 的组合。

### 示例1：过滤后排序

```cpp
std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 提取偶数，复制到新容器，然后排序
auto evens = vec
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::ranges::to<std::vector>();
std::ranges::sort(evens, std::ranges::greater{});
// 结果：{10, 8, 6, 4, 2}
```

### 示例2：变换后排序

```cpp
std::vector<std::string> words = {"apple", "banana", "cherry"};

// 获取长度并排序
std::vector<size_t> lengths;
for (const auto& word : words) {
    lengths.push_back(word.length());
}
std::ranges::sort(lengths);
// 结果：{5, 6, 6}

// 或者使用 transform + sort（更优雅）
auto length_view = words | std::views::transform([](const auto& s) {
    return s.length();
});
std::vector<size_t> lengths2(length_view.begin(), length_view.end());
std::ranges::sort(lengths2);
```

## 性能对比

### std::sort vs std::ranges::sort

| 特性 | std::sort | std::ranges::sort |
|------|-----------|-------------------|
| 时间复杂度 | O(n log n) | O(n log n) |
| 编译后代码 | 相同 | 相同 |
| 运行时性能 | 相同 | 相同 |
| 编译时间 | 稍快 | 稍慢（模板复杂） |
| 代码可读性 | 较差 | 更好 |
| 类型安全 | 一般 | 更强 |

**结论**：`std::ranges::sort` 是零开销抽象，运行时性能完全相同，但代码更简洁安全。

### 性能测试

```cpp
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>

void benchmark() {
    std::vector<int> data(1'000'000);
    for (int i = 0; i < data.size(); ++i) {
        data[i] = rand();
    }

    auto data1 = data;
    auto data2 = data;

    // 测试 std::sort
    auto start1 = std::chrono::high_resolution_clock::now();
    std::sort(data1.begin(), data1.end());
    auto end1 = std::chrono::high_resolution_clock::now();
    auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);

    // 测试 std::ranges::sort
    auto start2 = std::chrono::high_resolution_clock::now();
    std::ranges::sort(data2);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);

    std::cout << "std::sort:         " << time1.count() << " ms\n";
    std::cout << "std::ranges::sort: " << time2.count() << " ms\n";
}
```

**典型结果**：两者性能几乎完全相同（误差 < 1%）。

## 常见模式对比

### 模式1：基本排序

```cpp
std::vector<int> vec;

// 传统
std::sort(vec.begin(), vec.end());

// Ranges
std::ranges::sort(vec);
```

### 模式2：降序排序

```cpp
// 传统
std::sort(vec.begin(), vec.end(), std::greater<int>());

// Ranges
std::ranges::sort(vec, std::ranges::greater{});
```

### 模式3：部分排序

```cpp
// 传统
std::sort(vec.begin(), vec.begin() + 5);

// Ranges（两种写法都支持）
std::ranges::sort(vec.begin(), vec.begin() + 5);
std::ranges::sort(std::ranges::subrange(vec.begin(), vec.begin() + 5));
```

### 模式4：按对象成员排序

```cpp
struct Person { std::string name; int age; };
std::vector<Person> people;

// 传统
std::sort(people.begin(), people.end(),
    [](const Person& a, const Person& b) { return a.age < b.age; });

// Ranges（使用投影）
std::ranges::sort(people, {}, &Person::age);
```

### 模式5：稳定排序

```cpp
// 传统
std::stable_sort(vec.begin(), vec.end());

// Ranges
std::ranges::stable_sort(vec);
```

## 编译器支持

| 编译器 | 版本要求 | 支持情况 |
|--------|---------|---------|
| GCC | 10+ | ✅ 完全支持 |
| Clang | 13+ | ✅ 完全支持 |
| MSVC | 2019 16.10+ | ✅ 完全支持 |
| Apple Clang | 13+ | ✅ 完全支持 |

### CMakeLists.txt 配置

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_project)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

本项目已正确配置 C++20。

## 其他 Ranges 算法

除了 `sort`，Ranges 库还提供了许多其他算法的现代版本：

### 查找算法

```cpp
std::ranges::find(vec, value);
std::ranges::find_if(vec, predicate);
std::ranges::binary_search(vec, value);
std::ranges::lower_bound(vec, value);
std::ranges::upper_bound(vec, value);
```

### 修改算法

```cpp
std::ranges::copy(src, dest);
std::ranges::fill(vec, value);
std::ranges::transform(vec, dest, func);
std::ranges::reverse(vec);
std::ranges::rotate(vec, middle);
```

### 其他排序算法

```cpp
std::ranges::stable_sort(vec);              // 稳定排序
std::ranges::partial_sort(vec, middle);     // 部分排序
std::ranges::nth_element(vec, nth);         // 第n个元素
std::ranges::is_sorted(vec);                // 检查是否已排序
```

### 集合算法

```cpp
std::ranges::unique(vec);                   // 去重
std::ranges::merge(vec1, vec2, dest);       // 合并
std::ranges::set_union(set1, set2, dest);   // 并集
std::ranges::set_intersection(s1, s2, dest);// 交集
```

## 最佳实践

### 1. 优先使用 ranges::sort

```cpp
// ✗ 不推荐（旧风格）
std::sort(vec.begin(), vec.end());

// ✓ 推荐（现代 C++20）
std::ranges::sort(vec);
```

### 2. 使用投影代替 lambda

```cpp
struct Person { std::string name; int age; };
std::vector<Person> people;

// ✗ 不推荐（冗长）
std::ranges::sort(people, [](const Person& a, const Person& b) {
    return a.age < b.age;
});

// ✓ 推荐（简洁）
std::ranges::sort(people, {}, &Person::age);
```

### 3. 使用 ranges 版本的比较器

```cpp
// ✗ 不推荐
std::ranges::sort(vec, std::greater<int>());

// ✓ 推荐（类型自动推导）
std::ranges::sort(vec, std::ranges::greater{});
```

### 4. 保持一致性

如果项目使用 C++20，全面使用 Ranges 库保持风格一致。

```cpp
// ✓ 好的做法：全部使用 ranges
std::ranges::sort(vec);
std::ranges::find(vec, value);
std::ranges::copy(src, dest);

// ✗ 不好：混用
std::ranges::sort(vec);
std::find(vec.begin(), vec.end(), value);  // 不一致
std::copy(src.begin(), src.end(), dest);   // 不一致
```

## 迁移指南

### 从 std::sort 迁移到 ranges::sort

#### 步骤1：基本替换

```cpp
// 替换
std::sort(container.begin(), container.end());
// 为
std::ranges::sort(container);
```

#### 步骤2：更新比较器

```cpp
// 替换
std::sort(vec.begin(), vec.end(), std::greater<T>());
// 为
std::ranges::sort(vec, std::ranges::greater{});
```

#### 步骤3：使用投影优化

```cpp
// 替换
std::sort(objects.begin(), objects.end(),
    [](const T& a, const T& b) { return a.member < b.member; });
// 为
std::ranges::sort(objects, {}, &T::member);
```

#### 步骤4：测试

确保编译通过，运行单元测试验证正确性。

## 常见问题

### Q1：ranges::sort 比 std::sort 慢吗？

**A**：不会。编译器会将两者优化成相同的机器代码，运行时性能完全一致。

### Q2：能在 C++17 项目中使用吗？

**A**：不能。`std::ranges` 是 C++20 特性。如果必须使用 C++17，可以使用第三方库如 range-v3。

### Q3：投影的性能开销？

**A**：零开销。编译器会内联投影函数，不会产生额外的函数调用。

### Q4：可以同时使用比较器和投影吗？

**A**：可以。

```cpp
// 按 age 降序排序
std::ranges::sort(people, std::ranges::greater{}, &Person::age);
```

### Q5：ranges::sort 支持并行吗？

**A**：C++20 标准的 ranges::sort 不直接支持并行，但可以结合执行策略使用传统 std::sort：

```cpp
#include <execution>
std::sort(std::execution::par, vec.begin(), vec.end());
```

未来 C++ 标准可能会添加 ranges 的并行版本。

## 总结

### ranges::sort 的核心优势

1. **更简洁**：无需 `.begin()` 和 `.end()`
2. **更安全**：类型检查更严格
3. **更强大**：原生支持投影
4. **更现代**：体现 C++20 最佳实践
5. **零开销**：性能与传统 `std::sort` 完全相同

### 本项目改进总结

- ✅ **12 处**代码已升级到 `std::ranges::sort`
- ✅ 编译通过，无错误
- ✅ 性能不变，代码更清晰
- ✅ 特别优化了 `Hand.cpp` 的投影使用

### 快速参考

```cpp
// 基本用法
std::ranges::sort(container);                        // 升序
std::ranges::sort(container, std::ranges::greater{});// 降序

// 投影
std::ranges::sort(objects, {}, &Object::member);     // 按成员排序

// 迭代器
std::ranges::sort(begin, end);                       // 仍然支持

// 组合
std::ranges::sort(objects, std::ranges::greater{}, &Object::score);
```

### 相关文档

- [cppreference: std::ranges::sort](https://en.cppreference.com/w/cpp/algorithm/ranges/sort)
- [C++20 Ranges](https://en.cppreference.com/w/cpp/ranges)
- 项目改进文件：
  - `src/Game.cpp`
  - `src/HandEvaluator.cpp`
  - `src/Hand.cpp`
  - `src/Player.cpp`

---

**现在开始使用 `std::ranges::sort`，让你的 C++ 代码更现代、更简洁！**
