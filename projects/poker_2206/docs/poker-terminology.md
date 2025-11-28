# 扑克游戏术语详解

## 概述

本文档详细介绍扑克游戏中的常用术语，特别是本项目实现的五张牌扑克（Five-Card Draw）游戏中涉及的概念。

## 核心术语

### 1. Showdown（摊牌）

**定义**：扑克游戏的最后阶段，所有玩家亮出手牌，比较大小，决定胜负。

#### 词源
- **Show**：展示、亮出
- **Down**：向下翻开
- **Showdown**：把面朝下的牌翻开展示

#### 游戏流程中的位置

```
┌─────────────┐
│  1. Shuffle │  洗牌
└──────┬──────┘
       ↓
┌─────────────┐
│   2. Deal   │  发牌
└──────┬──────┘
       ↓
┌─────────────┐
│ 3. Replace  │  换牌（可选）
└──────┬──────┘
       ↓
┌─────────────┐
│ 4. Showdown │  摊牌 ✨ ← 决定胜负的时刻
└──────┬──────┘
       ↓
┌─────────────┐
│  5. Result  │  宣布结果
└─────────────┘
```

#### 项目中的实现

在 `Game.cpp:87-125` 中实现：

```cpp
void Game::showdown() {
    print_separator();
    std::cout << "SHOWDOWN\n";  // 显示"摊牌"标题
    print_separator();

    // 步骤1：显示双方手牌
    human_player_->show_hand();
    const HandEvaluation humanEval = HandEvaluator::evaluate(human_player_->get_hand());
    std::cout << "Hand: " << humanEval.to_string() << "\n\n";

    ai_player_->show_hand();
    const HandEvaluation aiEval = HandEvaluator::evaluate(ai_player_->get_hand());
    std::cout << "Hand: " << aiEval.to_string() << "\n\n";

    // 步骤2：比较牌型
    const ComparisonResult result = HandComparator::compare(
        human_player_->get_hand(),
        ai_player_->get_hand()
    );

    // 步骤3：宣布结果
    print_separator();
    std::cout << "结果: ";
    switch (result) {
        case ComparisonResult::Hand1Wins:
            std::cout << human_player_->get_name() << " 胜!\n";
            human_wins_++;
            break;
        case ComparisonResult::Hand2Wins:
            std::cout << ai_player_->get_name() << " 胜!\n";
            ai_wins_++;
            break;
        case ComparisonResult::Tie:
            std::cout << "平局!\n";
            ties_++;
            break;
    }
    print_separator();
}
```

#### 真实游戏示例

**德州扑克的 Showdown**：

```
场景：最后一轮下注结束，两名玩家进入 showdown

公共牌：♠K ♥Q ♦J ♣10 ♠9

玩家A（亮牌）：♥A ♠8
  → 最佳手牌：♥A ♠K ♥Q ♦J ♣10（顺子：A高）

玩家B（亮牌）：♣A ♦8
  → 最佳手牌：♣A ♠K ♥Q ♦J ♣10（顺子：A高）

结果：平局！（相同的顺子）
```

**本项目的 Showdown**（5张牌扑克）：

```
换牌结束后，进入 showdown

玩家（亮牌）：♠A ♠K ♠Q ♠J ♠10
  → 牌型：同花顺（皇家同花顺）

庄家（亮牌）：♥A ♥A ♥A ♦K ♣K
  → 牌型：葫芦（三条A + 一对K）

结果：玩家胜！（同花顺 > 葫芦）
```

#### 为什么 Showdown 很重要？

1. **最紧张的时刻**：决定输赢
2. **信息揭露**：终于看到对手的牌
3. **策略验证**：检验之前的决策是否正确
4. **学习机会**：观察对手的打法

#### 相关术语

| 术语 | 说明 |
|------|------|
| **Muck** | 弃牌不亮（认输） |
| **Show cards** | 亮牌 |
| **Turn over** | 翻开牌 |
| **Reveal** | 揭示手牌 |

### 2. Shuffle（洗牌）

**定义**：随机打乱一副牌的顺序。

#### 项目实现

使用 Fisher-Yates 算法（`Deck.cpp:21-29`）：

```cpp
void Deck::shuffle() {
    // Fisher-Yates 洗牌算法 - O(n) 时间复杂度，性能优异
    for (size_t i = DECK_SIZE - 1; i > 0; --i) {
        std::uniform_int_distribution<size_t> dist(0, i);
        const size_t j = dist(rng_);
        std::swap(cards_[i], cards_[j]);
    }
    current_index_ = 0;
}
```

#### 相关术语

| 术语 | 说明 |
|------|------|
| **Riffle shuffle** | 鸽尾式洗牌（现实中最常见） |
| **Overhand shuffle** | 上手洗牌 |
| **Cut** | 切牌 |

### 3. Deal（发牌）

**定义**：从牌堆中给玩家分发手牌。

#### 项目实现

```cpp
void Game::deal_cards() {
    deck_.reset();
    deck_.shuffle();
    human_player_->get_hand().clear();
    ai_player_->get_hand().clear();

    std::cout << "发牌...\n";
    // 发5张牌给每个玩家
    for (int i = 0; i < Hand::HAND_SIZE; ++i) {
        if (auto card = deck_.deal_card()) {
            human_player_->get_hand().add_card(*card);
        }
        if (auto card = deck_.deal_card()) {
            ai_player_->get_hand().add_card(*card);
        }
    }
}
```

#### 发牌顺序

**轮流发牌**（本项目）：
```
玩家1 → 玩家2 → 玩家1 → 玩家2 → ...
```

**德州扑克**：
```
小盲 → 大盲 → 庄家后第3位 → ... （顺时针）
```

### 4. Draw / Replace（换牌）

**定义**：丢弃部分手牌，从牌堆抽取新牌。

#### 项目实现

在 `Game::replace_cards_phase()` 中：

```cpp
void Game::replace_cards_phase() {
    // AI 换牌
    auto aiCardsToReplace = ai_player_->decide_cards_to_replace();
    std::ranges::sort(aiCardsToReplace, std::ranges::greater{});

    for (const size_t index : aiCardsToReplace) {
        ai_player_->get_hand().remove_card(index);
    }

    for (size_t i = 0; i < aiCardsToReplace.size(); ++i) {
        if (auto card = deck_.deal_card()) {
            ai_player_->get_hand().add_card(*card);
        }
    }

    // 人类玩家换牌
    if (auto humanCardsToReplace = human_player_->decide_cards_to_replace();
        humanCardsToReplace.empty()) {
        std::cout << "你选择不换牌.\n";
    } else {
        // ... 换牌逻辑
    }
}
```

#### 策略考虑

**换牌数量暗示牌力**：
- **换0张**：可能有好牌（顺子、同花、葫芦等）
- **换1张**：可能在听同花或顺子
- **换2张**：可能有三条
- **换3张**：可能有一对
- **换4-5张**：牌很差

## 牌型术语

### 1. High Card（高牌）

**中文**：单牌、散牌、高牌

**定义**：没有任何组合的手牌，只能比最大的单张牌。

**通俗理解**：就像考试没考好，只能说"我这题最起码还答对了"，高牌就是"我这牌最起码有张 A"。

#### 示例

**示例1：典型高牌**
```
手牌：♠A ♥K ♦10 ♣7 ♠3

分析：
- 没有对子（没有两张相同）
- 没有顺子（不连续）
- 没有同花（花色不同）
- 没有任何组合

牌型：高牌 A
```

**示例2：另一个高牌**
```
手牌：♥Q ♦J ♣9 ♠6 ♥2
牌型：高牌 Q
```

#### 比较规则

当两个玩家都是高牌时，按以下顺序比较：

**1. 比最大的牌**
```
玩家A：♠A ♥K ♦10 ♣7 ♠3  （最大：A）
玩家B：♣K ♥Q ♦J ♠9 ♣8   （最大：K）
结果：玩家A 胜！（A > K）
```

**2. 最大牌相同，比第二大**
```
玩家A：♠A ♥K ♦10 ♣7 ♠3  （A, K, 10, 7, 3）
玩家B：♣A ♦Q ♠J ♥9 ♣8   （A, Q, J, 9, 8）
比较：A = A，K > Q
结果：玩家A 胜！
```

**3. 依次比较直到分出胜负**
```
玩家A：♠A ♥K ♦J ♣7 ♠3
玩家B：♣A ♥K ♦J ♠6 ♣2
比较：A = A，K = K，J = J，7 > 6
结果：玩家A 胜！
```

**4. 点数都相同，比花色（本项目规则）**
```
玩家A：♠A ♥K ♦J ♣7 ♠3  （A 是黑桃♠）
玩家B：♣A ♦K ♥J ♠7 ♣3  （A 是梅花♣）
花色大小：♥ < ♦ < ♣ < ♠
结果：玩家A 胜！（♠ > ♣）
```

#### 概率统计

在5张牌中：

| 牌型 | 概率 | 说明 |
|------|------|------|
| **高牌** | **50.12%** | **最常见！** |
| 一对 | 42.26% | 第二常见 |
| 两对 | 4.75% | - |
| 三条 | 2.11% | - |
| 顺子 | 0.39% | - |
| 同花 | 0.20% | - |
| 葫芦 | 0.14% | - |
| 四条 | 0.02% | - |
| 同花顺 | 0.001% | 最稀有 |

**结论**：约有一半的手牌都是高牌！

#### 项目代码中的实现

**枚举定义**（Card.h）
```cpp
enum class HandRank {
    HighCard = 0,        // 高牌（最弱）
    OnePair = 1,
    TwoPair = 2,
    ThreeOfKind = 3,
    Straight = 4,
    Flush = 5,
    FullHouse = 6,
    FourOfKind = 7,
    StraightFlush = 8    // 同花顺（最强）
};
```

**评估逻辑**（HandEvaluator.cpp:228）
```cpp
HandEvaluation HandEvaluator::evaluate(const Hand& hand) {
    // ... 检查所有其他牌型 ...

    // 如果没有任何组合，就是高牌
    return HandEvaluation(HandRank::HighCard, kickers);
}
```

#### 实际游戏场景

**场景1：两个玩家都是高牌**
```
========================================
SHOWDOWN
========================================

玩家的手牌:
[1] ♠A  [2] ♥K  [3] ♦10  [4] ♣7  [5] ♠3
Hand: High Card

庄家的手牌:
[1] ♣K  [2] ♥Q  [3] ♦J  [4] ♠9  [5] ♣8
Hand: High Card

========================================
结果: 玩家 胜!
========================================

原因：玩家有 A，庄家最大只有 K
```

**场景2：高牌输给一对**
```
玩家：♠A ♥K ♦Q ♣J ♠9  （高牌 A）
庄家：♣2 ♦2 ♥5 ♠7 ♣9  （一对 2）

结果：庄家胜！

原因：一对 > 高牌（即使一对2也比高牌A强）
```

#### 策略建议

**拿到高牌怎么办？**

**1. 查看点数**
- **A, K, Q** → 还有希望，换掉小牌试试
- **J, 10, 9** → 考虑换3-4张
- **8 以下** → 基本没戏，换牌碰运气

**2. 查看是否接近组合**

项目中 AI 的决策逻辑（Player.cpp）：
```cpp
// 接近同花？（4张同花色）
if (is_almost_flush()) {
    // 换掉那张不同花色的牌
}

// 接近顺子？（4张连续）
else if (is_almost_straight()) {
    // 换掉不连续的牌
}

// 纯高牌
else {
    // 换掉最小的3张牌
    std::vector<std::pair<Rank, size_t>> ranksWithPos;
    for (size_t i = 0; i < hand_.get_cards().size(); ++i) {
        ranksWithPos.push_back({hand_.get_cards()[i].get_rank(), i});
    }
    std::ranges::sort(ranksWithPos);

    // 换掉最小的3张
    for (size_t i = 0; i < 3; ++i) {
        cardsToReplace.push_back(ranksWithPos[i].second);
    }
}
```

#### 相关术语

| 英文 | 中文 | 说明 |
|------|------|------|
| High Card | 高牌 | 没有任何组合 |
| Ace High | A 高 | 最大牌是 A 的高牌 |
| King High | K 高 | 最大牌是 K 的高牌 |
| Kicker | 踢脚牌 | 用于打破平局的牌 |

#### 口语表达

```
"我只有高牌"          → 牌很差
"高牌 A"             → 最大的牌是 A
"我的 A 比你的 K 大"  → 高牌对决
"连个对子都没有"      → 高牌
```

#### 记忆口诀

> **"什么都没有，只有高牌"**
> **"高牌比大小，从头比到尾"**
> **"A 最大，2 最小，花色分高低"**

### 2. One Pair（一对）

**中文**：对子

**定义**：两张相同点数的牌。

**示例**：
```
♠A ♥A ♦K ♣9 ♠3
一对：A
```

### 3. Two Pair（两对）

**中文**：两对

**定义**：两个不同的对子。

**示例**：
```
♠A ♥A ♦K ♣K ♠3
两对：A 和 K
```

### 4. Three of a Kind（三条）

**中文**：三条、三张

**别名**：Trips, Set

**定义**：三张相同点数的牌。

**示例**：
```
♠A ♥A ♦A ♣K ♠3
三条：A
```

### 5. Straight（顺子）

**中文**：顺子

**定义**：五张连续点数的牌（花色不限）。

**示例**：
```
♠10 ♥9 ♦8 ♣7 ♠6
顺子：10高
```

**特殊顺子**：
```
♠A ♥2 ♦3 ♣4 ♠5  （A-2-3-4-5，最小的顺子）
♠A ♥K ♦Q ♣J ♠10 （A-K-Q-J-10，最大的顺子）
```

### 6. Flush（同花）

**中文**：同花

**定义**：五张相同花色的牌（点数不连续）。

**示例**：
```
♥A ♥K ♥9 ♥7 ♥3
同花（红心）
```

### 7. Full House（葫芦）

**中文**：葫芦、满堂红

**别名**：Boat, Full Boat

**定义**：三条 + 一对。

**示例**：
```
♠A ♥A ♦A ♣K ♠K
葫芦：A带K（三条A + 一对K）
```

### 8. Four of a Kind（四条）

**中文**：四条、铁支

**别名**：Quads

**定义**：四张相同点数的牌。

**示例**：
```
♠A ♥A ♦A ♣A ♠K
四条：A
```

### 9. Straight Flush（同花顺）

**中文**：同花顺

**定义**：五张连续且同花色的牌。

**示例**：
```
♠10 ♠9 ♠8 ♠7 ♠6
同花顺（黑桃，10高）
```

**皇家同花顺（Royal Flush）**：
```
♠A ♠K ♠Q ♠J ♠10
皇家同花顺（最大的同花顺）
```

## 下注术语

### 1. Bet（下注）

**定义**：第一个押注的行为。

**示例**：
```
玩家A: "我下注 100"
```

### 2. Call（跟注）

**定义**：跟上前面玩家的赌注。

**示例**：
```
玩家A 下注 100
玩家B: "跟注"（也押 100）
```

### 3. Raise（加注）

**定义**：在别人下注的基础上增加赌注。

**示例**：
```
玩家A 下注 100
玩家B: "加注到 200"
```

### 4. Fold（弃牌）

**定义**：放弃这一局，不继续跟注。

**示例**：
```
玩家A 下注 100
玩家B: "弃牌"（退出本局）
```

### 5. Check（过牌）

**定义**：不下注，把行动权交给下一位玩家。

**示例**：
```
玩家A: "过牌"（不下注）
玩家B: "下注 50"
```

### 6. All-in（全押）

**定义**：把所有筹码都押上。

**示例**：
```
玩家A（剩余 500 筹码）: "All-in!"
```

## 位置术语

### 1. Dealer（庄家）

**定义**：负责发牌的位置。

在赌场中，有专门的荷官发牌，庄家位置用"按钮"（Button）标记。

### 2. Small Blind（小盲注）

**定义**：庄家左边第一位，必须下小盲注。

**示例**：小盲注 10 元。

### 3. Big Blind（大盲注）

**定义**：庄家左边第二位，必须下大盲注。

**示例**：大盲注 20 元（通常是小盲注的两倍）。

### 4. Under the Gun（枪口位）

**定义**：大盲注左边第一位，第一个行动，位置最差。

**缩写**：UTG

## 游戏阶段术语

### 德州扑克的阶段

#### 1. Pre-flop（翻牌前）

发完底牌，公共牌尚未发出。

#### 2. Flop（翻牌）

发出前三张公共牌。

```
公共牌：♠K ♥Q ♦J
```

#### 3. Turn（转牌）

发出第四张公共牌。

```
公共牌：♠K ♥Q ♦J ♣10
```

#### 4. River（河牌）

发出第五张公共牌（最后一张）。

```
公共牌：♠K ♥Q ♦J ♣10 ♠9
```

#### 5. Showdown（摊牌）

亮牌比大小。

### 五张牌扑克的阶段（本项目）

```
1. Deal（发牌）      → deal_cards()
2. Replace（换牌）   → replace_cards_phase()
3. Showdown（摊牌）  → showdown()
```

## 其他常用术语

### 1. Pot（底池）

**定义**：本局所有玩家下注的总和。

**示例**：
```
玩家A 下注 100
玩家B 跟注 100
底池：200
```

### 2. Ante（前注）

**定义**：每个玩家在发牌前必须押的小额赌注。

**示例**：
```
前注 5 元，5 个玩家 = 底池 25 元
```

### 3. Kicker（踢脚牌）

**定义**：在牌型相同时，用于比较大小的其他牌。

**示例**：
```
玩家A：♠A ♥A ♦K ♣9 ♠3（一对A，K踢脚）
玩家B：♣A ♦A ♥Q ♠8 ♦2（一对A，Q踢脚）

结果：玩家A 胜（K > Q）
```

### 4. Nuts（坚果牌）

**定义**：当前可能的最好牌型。

**示例**：
```
公共牌：♠K ♥Q ♦J ♣10 ♠9
你的手牌：♥A ♦8

你有：♥A ♠K ♥Q ♦J ♣10（顺子，A高）
这是 Nuts（最好的牌）！
```

### 5. Drawing（听牌）

**定义**：差一张牌就能组成好牌。

**类型**：
- **Flush Draw**：听同花（差一张同花色）
- **Straight Draw**：听顺子（差一张连续牌）
- **Gutshot**：中张听牌（顺子中间缺一张）

**示例**：
```
手牌：♥A ♥K ♥Q ♥J ♦3
听同花：再来一张红心就是同花
```

### 6. Bluff（诈唬）

**定义**：手里牌不好，但假装牌很好，通过下注逼对手弃牌。

**示例**：
```
你的牌：♠7 ♥3（很差）
你大额下注，假装有好牌
对手以为你牌很好，弃牌
你赢了！（虽然牌很差）
```

### 7. Tilt（上头）

**定义**：因为输钱或情绪失控，做出不理性的决策。

**示例**：
```
连续输了几局 → 情绪失控 → 疯狂加注 → 输更多
```

## 项目术语映射

### 代码中的术语

| 代码 | 中文 | 英文术语 |
|------|------|---------|
| `shuffle()` | 洗牌 | Shuffle |
| `deal_card()` | 发牌 | Deal |
| `replace_cards_phase()` | 换牌阶段 | Draw/Replace |
| `showdown()` | 摊牌 | Showdown |
| `HandEvaluation` | 牌型评估 | Hand Evaluation |
| `HandComparator` | 牌型比较 | Hand Comparison |
| `HandRank::HighCard` | 高牌 | High Card |
| `HandRank::OnePair` | 一对 | One Pair |
| `HandRank::TwoPair` | 两对 | Two Pair |
| `HandRank::ThreeOfKind` | 三条 | Three of a Kind |
| `HandRank::Straight` | 顺子 | Straight |
| `HandRank::Flush` | 同花 | Flush |
| `HandRank::FullHouse` | 葫芦 | Full House |
| `HandRank::FourOfKind` | 四条 | Four of a Kind |
| `HandRank::StraightFlush` | 同花顺 | Straight Flush |

## 花色和点数

### 花色（Suits）

| 英文 | 符号 | 中文 | 项目代码 |
|------|------|------|---------|
| Hearts | ♥ | 红心 | `Suit::Hearts` |
| Diamonds | ♦ | 方块 | `Suit::Diamonds` |
| Clubs | ♣ | 梅花 | `Suit::Clubs` |
| Spades | ♠ | 黑桃 | `Suit::Spades` |

**花色大小**（本项目）：
```
红心 < 方块 < 梅花 < 黑桃
♥    < ♦     < ♣    < ♠
```

### 点数（Ranks）

| 英文 | 符号 | 中文 | 项目代码 |
|------|------|------|---------|
| Ace | A | A / 尖 | `Rank::Ace` |
| King | K | K / 老K | `Rank::King` |
| Queen | Q | Q / 娘娘 | `Rank::Queen` |
| Jack | J | J / 勾 | `Rank::Jack` |
| Ten | 10 | 10 | `Rank::Ten` |
| Nine | 9 | 9 | `Rank::Nine` |
| Eight | 8 | 8 | `Rank::Eight` |
| Seven | 7 | 7 | `Rank::Seven` |
| Six | 6 | 6 | `Rank::Six` |
| Five | 5 | 5 | `Rank::Five` |
| Four | 4 | 4 | `Rank::Four` |
| Three | 3 | 3 | `Rank::Three` |
| Two | 2 | 2 | `Rank::Two` |

**点数大小**：
```
A > K > Q > J > 10 > 9 > 8 > 7 > 6 > 5 > 4 > 3 > 2
```

**注意**：A 在顺子中可以作为最小（A-2-3-4-5）或最大（10-J-Q-K-A）。

## 常见口语表达

### 英文口语

| 表达 | 含义 |
|------|------|
| "I'm all in!" | 我全押！ |
| "I fold." | 我弃牌。 |
| "I call." | 我跟注。 |
| "Raise to 100." | 加注到 100。 |
| "Check." | 过牌。 |
| "Show your cards!" | 亮牌！ |
| "Good hand!" | 好牌！ |
| "Bad beat!" | 被爆冷门！ |
| "I'm on tilt." | 我上头了。 |
| "Pocket aces!" | 底牌两个A！ |

### 中文口语

| 表达 | 含义 |
|------|------|
| "梭哈！" | All-in（全押） |
| "我不跟了" | 弃牌 |
| "跟" | 跟注 |
| "加" | 加注 |
| "让" | 过牌 |
| "亮牌" | Showdown |
| "炸弹" | 四条 |
| "三带二" | 葫芦 |
| "顺子" | Straight |
| "同花" | Flush |

## 游戏规则参考

### 本项目规则（Five-Card Draw）

1. **发牌**：每人发 5 张牌
2. **第一轮**：查看手牌，决定换牌
3. **换牌**：可换 0-5 张牌
4. **摊牌**：亮牌比大小
5. **比较规则**：
   - 先比牌型等级
   - 牌型相同比关键牌点数
   - 点数相同比花色

### 其他流行扑克变体

#### 德州扑克（Texas Hold'em）
- 每人 2 张底牌 + 5 张公共牌
- 4 轮下注（翻牌前、翻牌、转牌、河牌）
- 用最好的 5 张牌组合

#### 奥马哈（Omaha）
- 每人 4 张底牌 + 5 张公共牌
- 必须用 2 张底牌 + 3 张公共牌

#### 七张梭哈（Seven-Card Stud）
- 每人 7 张牌（3 暗 4 明）
- 无公共牌
- 选最好的 5 张组合

## 总结

### 核心术语速查

| 术语 | 含义 | 项目中的体现 |
|------|------|-------------|
| **Showdown** | 摊牌、亮牌比大小 | `Game::showdown()` |
| **Shuffle** | 洗牌 | `Deck::shuffle()` |
| **Deal** | 发牌 | `Deck::deal_card()` |
| **Draw/Replace** | 换牌 | `replace_cards_phase()` |
| **Hand** | 手牌 | `Hand` 类 |
| **Rank** | 牌型等级 | `HandRank` 枚举 |
| **Kicker** | 踢脚牌 | `HandEvaluation::kickers` |

### 学习资源

- **书籍**：《The Theory of Poker》- David Sklansky
- **网站**：PokerStars School、888poker Learn
- **视频**：YouTube 搜索 "poker tutorial"
- **练习**：本项目！运行游戏体验真实扑克

---

**现在你已经掌握了扑克术语，可以像专业玩家一样讨论游戏了！** 🃏
