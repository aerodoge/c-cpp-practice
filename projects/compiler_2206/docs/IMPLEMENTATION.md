# 实现原理详解

本文档详细介绍 Simple 语言工具链的实现原理，包括词法分析器、解释器、编译器和 SML 虚拟机。

## 目录

1. [词法分析器 (Lexer)](#1-词法分析器-lexer)
2. [解释器 (Interpreter)](#2-解释器-interpreter)
3. [编译器 (Compiler)](#3-编译器-compiler)
4. [SML 虚拟机 (SML VM)](#4-sml-虚拟机-sml-vm)

---

## 1. 词法分析器 (Lexer)

### 1.1 概述

词法分析器是编译/解释过程的第一阶段，负责将源代码字符流转换为 Token（词法单元）序列。

```
源代码字符串 → [Lexer] → Token 序列
```

### 1.2 Token 类型

```c
typedef enum {
    // 特殊标记
    TOKEN_EOF,          // 文件结束
    TOKEN_ERROR,        // 错误
    TOKEN_NEWLINE,      // 换行符

    // 字面量
    TOKEN_NUMBER,       // 整数: 123
    TOKEN_FLOAT,        // 浮点数: 3.14
    TOKEN_STRING,       // 字符串: "hello"
    TOKEN_IDENT,        // 标识符: a-z

    // 关键字
    TOKEN_REM, TOKEN_INPUT, TOKEN_PRINT, TOKEN_LET,
    TOKEN_GOTO, TOKEN_IF, TOKEN_FOR, TOKEN_TO,
    TOKEN_STEP, TOKEN_NEXT, TOKEN_END,

    // 运算符
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
    TOKEN_PERCENT, TOKEN_CARET, TOKEN_ASSIGN,
    TOKEN_EQ, TOKEN_NE, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE,

    // 分隔符
    TOKEN_COMMA, TOKEN_LPAREN, TOKEN_RPAREN,
} TokenType;
```

### 1.3 扫描算法

词法分析器使用**手写递归下降**方法：

```
lexer_next_token():
    1. 跳过空白字符 (空格、制表符)
    2. 记录 Token 起始位置
    3. 读取第一个字符，判断类型：
       - '\n'     → TOKEN_NEWLINE
       - 数字     → scan_number()
       - 字母     → scan_identifier() (可能是关键字)
       - '"'      → scan_string()
       - 运算符   → 返回对应 Token
    4. 返回 Token
```

### 1.4 关键字识别

标识符扫描完成后，与关键字表比对（大小写不敏感）：

```c
static const Keyword keywords[] = {
    {"rem",   TOKEN_REM},
    {"input", TOKEN_INPUT},
    {"print", TOKEN_PRINT},
    // ...
};
```

### 1.5 数字扫描

```
scan_number():
    1. 扫描整数部分: 连续数字
    2. 检查是否有小数点
    3. 如果有，扫描小数部分 → TOKEN_FLOAT
    4. 否则 → TOKEN_NUMBER
```

---

## 2. 解释器 (Interpreter)

### 2.1 概述

解释器采用**边解析边执行**策略，无需生成中间代码。

```
Simple 源码 → [Lexer] → Token → [Parser/Executor] → 直接执行
```

### 2.2 数据结构

```c
typedef struct {
    // 行号索引表 (预处理阶段建立)
    LineInfo lines[MAX_LINES];
    int line_count;

    // 变量存储 (运行时)
    Variable variables[26];    // a-z
    Array arrays[26];          // a()-z()

    // for 循环栈
    ForState for_stack[MAX_FOR_DEPTH];
    int for_depth;

    // 执行状态
    int current_line_index;
    int running;
} Interpreter;
```

### 2.3 执行流程

```
interpreter_run():
    1. 预处理: 扫描源码，建立行号→位置映射
    2. 设置 current_line_index = 0
    3. while (running):
        a. 获取当前行
        b. 初始化词法分析器到该行
        c. 解析并执行语句
        d. 根据语句类型更新 current_line_index
    4. 结束
```

### 2.4 语句执行

每种语句有独立的执行函数：

| 语句 | 执行逻辑 |
|------|---------|
| `rem` | 跳过，什么都不做 |
| `input x` | 读取输入 → `variables[x].value` |
| `print x` | 输出 `variables[x].value` |
| `let x = expr` | 计算表达式 → 赋值 |
| `goto n` | 查找行号 n → 设置 `current_line_index` |
| `if cond goto n` | 计算条件，为真则跳转 |
| `for i = a to b` | 初始化循环变量，压栈 |
| `next i` | 递增变量，检查条件，决定跳转或出栈 |
| `end` | 设置 `running = 0` |

### 2.5 表达式解析 (递归下降)

表达式解析采用**递归下降**方法，按优先级从低到高：

```
expression → term (('+' | '-') term)*
term       → power (('*' | '/' | '%') power)*
power      → unary ('^' unary)*
unary      → '-' unary | primary
primary    → NUMBER | IDENT | IDENT '(' expr ')' | '(' expr ')'
```

实现示例：

```c
double parse_expression(Interpreter *interp) {
    double left = parse_term(interp);
    while (current_token is '+' or '-') {
        char op = current_token;
        advance();
        double right = parse_term(interp);
        left = (op == '+') ? left + right : left - right;
    }
    return left;
}
```

### 2.6 for 循环实现

```c
// for i = 1 to 10 step 2
exec_for():
    1. 解析循环变量 i
    2. 解析起始值 → 赋给 variables[i]
    3. 解析结束值 → 保存到 for_stack
    4. 解析步长 (默认 1) → 保存到 for_stack
    5. 记录循环体起始行 → 保存到 for_stack
    6. for_depth++

// next i
exec_next():
    1. 从 for_stack 获取循环状态
    2. variables[i] += step
    3. 检查是否继续:
       - step > 0: 继续条件 i <= end
       - step < 0: 继续条件 i >= end
    4. 如果继续 → 跳转到循环体起始
    5. 否则 → for_depth--, 继续下一行
```

---

## 3. 编译器 (Compiler)

### 3.1 概述

编译器将 Simple 高级语言翻译为 SML 机器码，采用**两遍扫描 (Two-Pass)** 算法。

```
Simple 源码 → [Pass 1] → SML代码 + 符号表 + 待填充引用
            → [Pass 2] → 填充跳转地址
            → 完整 SML 程序
```

### 3.2 为什么需要两遍？

**前向引用问题**：

```basic
10 goto 50      ; 此时还不知道 50 对应的指令地址
20 print x
...
50 let x = 1    ; 第 50 行在这里
```

第一遍扫描时，遇到 `goto 50` 但还没处理到第 50 行，不知道它对应的机器地址。解决方案：
- 第一遍：生成代码，记录需要填充的位置
- 第二遍：所有行号都已知，填充跳转地址

### 3.3 内存布局

SML 虚拟机只有 100 个内存单元，指令和数据共享：

```
┌─────────────────────────────────────────┐
│ 地址 0:  第一条指令                      │ ← instruction_counter 从这里增长
│ 地址 1:  第二条指令                      │
│ ...                                     │
│ 地址 N:  最后一条指令                    │
│ ─────────── 空闲区 ───────────          │
│ 地址 M:  数据/变量                       │
│ ...                                     │
│ 地址 98: 数据/变量                       │
│ 地址 99: 第一个变量                      │ ← data_counter 从这里递减
└─────────────────────────────────────────┘
```

### 3.4 符号表

符号表记录所有符号及其内存位置：

```c
typedef struct {
    SymbolType type;   // LINE / VARIABLE / CONSTANT / ARRAY / STRING
    int symbol;        // 符号值 (行号/变量索引/常量值)
    int location;      // 内存地址
} Symbol;
```

**符号类型**：

| 类型 | symbol 含义 | location 含义 |
|------|------------|---------------|
| LINE | 行号值 (如 50) | 指令地址 |
| VARIABLE | 变量索引 (a=0) | 数据地址 |
| CONSTANT | 常量值 | 数据地址 |
| ARRAY | 数组标识 | 基地址 |
| STRING | 字符串 ID | 数据起始地址 |

### 3.5 第一遍扫描

```
pass1():
    instruction_counter = 0
    data_counter = 99

    for each line in source:
        1. 解析行号 → 添加到符号表 (LINE, 行号, instruction_counter)
        2. 解析语句类型
        3. 根据语句生成 SML 指令:
           - rem     → 无指令
           - input x → emit(READ, get_or_create_variable(x))
           - print x → emit(LOAD, x), emit(STORE, temp), emit(WRITE, temp)
           - let     → 编译表达式，emit(STORE, 变量)
           - goto n  → emit(BRANCH, 0), 记录待填充位置
           - if      → 编译条件，emit(JMPNEG/JMPZERO, 0), 记录待填充
           - for     → 初始化循环变量，记录循环信息
           - next    → 生成递增和条件跳转代码
           - end     → emit(HALT, 0)
```

### 3.6 指令生成示例

**let x = a + b * 2**

```
编译表达式 a + b * 2:
    1. 编译 a       → LOAD a_loc
    2. 遇到 +，保存左操作数到临时变量
       → STORE temp1
    3. 编译 b * 2:
       a. LOAD b_loc
       b. STORE temp2
       c. LOAD temp2
       d. MULTIPLY const_2_loc
    4. 执行 +:
       → STORE temp3      ; 保存右操作数
       → LOAD temp1       ; 加载左操作数
       → ADD temp3        ; 执行加法
    5. 赋值:
       → STORE x_loc

生成的 SML 代码:
    LOAD     [a]
    STORE    [temp1]
    LOAD     [b]
    MULTIPLY [2]
    STORE    [temp3]
    LOAD     [temp1]
    ADD      [temp3]
    STORE    [x]
```

### 3.7 第二遍扫描

```
pass2():
    for each 待填充引用 in flags:
        1. 获取目标行号
        2. 在符号表中查找该行号的指令地址
        3. 填充到对应指令的操作数部分

示例:
    flags[0] = {instruction: 5, target_line: 50}
    符号表: LINE 50 → address 20
    memory[5] = 4000 → memory[5] = 4020  ; BRANCH 20
```

### 3.8 for 循环编译

```basic
10 for i = 1 to 5 step 2
20   print i
30 next i
```

编译为：

```
; 初始化 (第 10 行)
00: LOAD     [1]        ; 加载起始值
01: STORE    [i]        ; i = 1
02: LOAD     [2]        ; 加载步长
03: STORE    [step]     ; 保存步长

; 循环体 (第 20 行)
04: LOAD     [i]        ; ← loop_start
05: STORE    [temp]
06: WRITE    [temp]
07: NEWLINE  00

; next (第 30 行)
08: LOAD     [i]
09: ADD      [step]     ; i += step
10: STORE    [i]
11: LOAD     [5]        ; 加载结束值
12: SUB      [i]        ; end - i
13: JMPNEG   15         ; 如果 < 0，退出循环
14: JMP      04         ; 否则跳回 loop_start
15: ...                 ; 循环结束后的代码
```

---

## 4. SML 虚拟机 (SML VM)

### 4.1 概述

SML 虚拟机是一个模拟的简单计算机，采用**冯诺依曼架构**和**累加器架构**。

### 4.2 架构特点

- **冯诺依曼架构**: 指令和数据存储在同一内存中
- **累加器架构**: 所有运算通过单一累加器进行
- **定长指令**: 每条指令占用一个内存单元

### 4.3 寄存器

```c
typedef struct {
    int memory[100];           // 内存 (指令+数据)
    int accumulator;           // 累加器 (AC)
    int instruction_counter;   // 程序计数器 (PC)
    int instruction_register;  // 指令寄存器 (IR)
    int opcode;                // 操作码
    int operand;               // 操作数
    int running;               // 运行状态
} SML_VM;
```

### 4.4 指令格式

```
指令 = ±XXYY
  XX: 操作码 (10-43)
  YY: 操作数 (内存地址 00-99)

示例:
  +2099 = LOAD 99   (将地址 99 的值加载到累加器)
  +4020 = JMP 20    (跳转到地址 20)
  +4300 = HALT 00   (停机)
```

### 4.5 指令集

| 操作码 | 助记符 | 操作 |
|--------|--------|------|
| 10 | READ | `Memory[operand] = input()` |
| 11 | WRITE | `print(Memory[operand])` |
| 12 | NEWLINE | `print("\n")` |
| 13 | WRITES | 输出字符串 |
| 20 | LOAD | `AC = Memory[operand]` |
| 21 | STORE | `Memory[operand] = AC` |
| 30 | ADD | `AC = AC + Memory[operand]` |
| 31 | SUB | `AC = AC - Memory[operand]` |
| 32 | DIV | `AC = AC / Memory[operand]` |
| 33 | MUL | `AC = AC * Memory[operand]` |
| 34 | MOD | `AC = AC % Memory[operand]` |
| 40 | JMP | `PC = operand` |
| 41 | JMPNEG | `if (AC < 0) PC = operand` |
| 42 | JMPZERO | `if (AC == 0) PC = operand` |
| 43 | HALT | `running = false` |

### 4.6 执行周期 (Fetch-Decode-Execute)

```c
sml_vm_step():
    // 1. 取指 (Fetch)
    IR = Memory[PC]

    // 2. 解码 (Decode)
    opcode  = IR / 100
    operand = IR % 100

    // 3. 执行 (Execute)
    switch (opcode):
        case LOAD:  AC = Memory[operand]; break
        case STORE: Memory[operand] = AC; break
        case ADD:   AC += Memory[operand]; break
        case JMP:   PC = operand; return   // 不自增 PC
        case HALT:  running = false; return
        // ...

    // 4. PC 自增 (非跳转指令)
    PC++
```

### 4.7 执行流程图

```
          ┌──────────────────┐
          │     开始执行     │
          └────────┬─────────┘
                   ↓
          ┌──────────────────┐
          │  PC < 100 且     │──否──→ 错误退出
          │  running == true │
          └────────┬─────────┘
                   │ 是
                   ↓
          ┌──────────────────┐
          │ 取指: IR=Mem[PC] │
          └────────┬─────────┘
                   ↓
          ┌──────────────────┐
          │ 解码: op=IR/100  │
          │      arg=IR%100  │
          └────────┬─────────┘
                   ↓
          ┌──────────────────┐
          │   执行指令       │
          └────────┬─────────┘
                   ↓
          ┌──────────────────┐
          │ HALT?  ├──是──→ 正常退出
          └────────┬─────────┘
                   │ 否
                   ↓
          ┌──────────────────┐
          │ 跳转指令? │──是──→ (PC已设置)
          └────────┬─────────┘         │
                   │ 否                │
                   ↓                   │
          ┌──────────────────┐         │
          │     PC++         │         │
          └────────┬─────────┘         │
                   │                   │
                   └───────────────────┘
                            ↓
                   (回到循环开始)
```

### 4.8 字符串输出 (WRITES)

字符串在内存中以**长度前缀**格式存储：

```
地址 N:   字符串长度 (如 5)
地址 N-1: 第 1 个字符
地址 N-2: 第 2 个字符
...
地址 N-len: 最后一个字符
```

执行 WRITES 指令时：

```c
case SML_WRITES:
    int str_loc = operand;
    int len = Memory[str_loc];
    for (int i = 0; i < len; i++) {
        putchar(Memory[str_loc - 1 - i]);
    }
    break;
```

### 4.9 安全机制

1. **地址检查**: 所有内存访问检查边界 (0-99)
2. **除零检查**: DIV/MOD 指令检查除数
3. **循环保护**: 最大执行周期限制 (MAX_CYCLES)
4. **输入验证**: READ 指令验证输入格式

---

## 5. 完整编译示例

### 源程序

```basic
10 let x = 5
20 let y = 3
30 let z = x + y
40 print z
50 end
```

### 编译过程

**第一遍**:

```
行 10: let x = 5
  - 符号表添加: LINE 10 → 地址 0
  - 分配变量 x → 地址 99
  - 分配常量 5 → 地址 98, Memory[98] = 5
  - 生成: 00: LOAD 98    (+2098)
          01: STORE 99   (+2199)

行 20: let y = 3
  - 符号表添加: LINE 20 → 地址 2
  - 分配变量 y → 地址 97
  - 分配常量 3 → 地址 96, Memory[96] = 3
  - 生成: 02: LOAD 96    (+2096)
          03: STORE 97   (+2197)

行 30: let z = x + y
  - 符号表添加: LINE 30 → 地址 4
  - 分配变量 z → 地址 95
  - 分配临时变量 → 地址 94
  - 生成: 04: LOAD 99    (+2099) ; 加载 x
          05: STORE 94   (+2194) ; 保存到临时
          06: LOAD 97    (+2097) ; 加载 y
          07: ADD 94     (+3094) ; 加法
          08: STORE 95   (+2195) ; 存储到 z

行 40: print z
  - 符号表添加: LINE 40 → 地址 9
  - 分配临时变量 → 地址 93
  - 生成: 09: LOAD 95    (+2095)
          10: STORE 93   (+2193)
          11: WRITE 93   (+1193)
          12: NEWLINE 0  (+1200)

行 50: end
  - 符号表添加: LINE 50 → 地址 13
  - 生成: 13: HALT 0     (+4300)
```

**最终内存状态**:

```
指令区:
  00: +2098  LOAD 98
  01: +2199  STORE 99
  02: +2096  LOAD 96
  03: +2197  STORE 97
  04: +2099  LOAD 99
  05: +2194  STORE 94
  06: +2097  LOAD 97
  07: +3094  ADD 94
  08: +2195  STORE 95
  09: +2095  LOAD 95
  10: +2193  STORE 93
  11: +1193  WRITE 93
  12: +1200  NEWLINE 00
  13: +4300  HALT 00

数据区:
  93: +0000  (临时变量)
  94: +0000  (临时变量)
  95: +0000  (变量 z)
  96: +0003  (常量 3)
  97: +0000  (变量 y)
  98: +0005  (常量 5)
  99: +0000  (变量 x)
```

### 执行结果

```
PC=0:  LOAD 98   → AC = 5
PC=1:  STORE 99  → Memory[99] = 5  (x = 5)
PC=2:  LOAD 96   → AC = 3
PC=3:  STORE 97  → Memory[97] = 3  (y = 3)
PC=4:  LOAD 99   → AC = 5
PC=5:  STORE 94  → Memory[94] = 5
PC=6:  LOAD 97   → AC = 3
PC=7:  ADD 94    → AC = 3 + 5 = 8
PC=8:  STORE 95  → Memory[95] = 8  (z = 8)
PC=9:  LOAD 95   → AC = 8
PC=10: STORE 93  → Memory[93] = 8
PC=11: WRITE 93  → 输出: 8
PC=12: NEWLINE   → 输出: \n
PC=13: HALT      → 停机

输出: 8
```

---

## 参考资料

- 《编译原理》(龙书) - 词法分析、语法分析
- 《C How to Program》- Simpletron Machine Language
- 《计算机组成原理》- 冯诺依曼架构、指令周期
