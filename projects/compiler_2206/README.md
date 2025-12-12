# Simple 语言编译器与解释器

一个完整的Simple语言工具链，包括解释器、编译器和SML虚拟机。

## 项目概述

本项目实现了一个类BASIC的简单编程语言 (Simple)，提供两种执行方式：

1. **解释器**: 直接解析执行源代码，功能完整
2. **编译器**: 将源代码编译为SML(Simpletron Machine Language) 机器码

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  .simple    │ ──→ │  Compiler   │ ──→ │    .sml     │
│             │     │             │     │             │
└─────────────┘     └─────────────┘     └─────────────┘
       │                                       │
       │                                       ↓
       │            ┌─────────────┐     ┌─────────────┐
       └──────────→ │   解释器     │     │  SML 虚拟机  │
                    │ interpreter │     │   sml_vm    │
                    └─────────────┘     └─────────────┘
```

## 构建

```bash
cmake -B build -S .
cmake --build build
```

## 使用方法

```bash
# 解释执行 (默认)
./build/simple program.simple

# 编译为 SML (生成 .sml 文件)
./build/simple -c program.simple

# 编译并运行
./build/simple -r program.simple

# 执行 SML 文件
./build/simple -x program.sml

# 交互模式 (REPL)
./build/simple
```

## Simple 语言语法

### 程序结构

每行以行号开始，行号决定执行顺序：

```basic
10 rem 这是注释
20 let x = 10
30 print x
40 end
```

### 语句类型

| 语句    | 语法                         | 说明          |
|-------|----------------------------|-------------|
| rem   | `rem 注释内容`                 | 注释，被忽略      |
| input | `input x`                  | 从键盘读取值到变量 x |
| print | `print x` 或 `print "text"` | 输出变量值或字符串   |
| let   | `let x = 表达式`              | 赋值语句        |
| goto  | `goto 行号`                  | 无条件跳转       |
| if    | `if 条件 goto 行号`            | 条件跳转        |
| for   | `for i = 1 to 10 step 2`   | 循环开始        |
| next  | `next i`                   | 循环结束        |
| end   | `end`                      | 程序结束        |

### 表达式

支持的运算符：

- 算术: `+`, `-`, `*`, `/`, `%` (取模), `^` (幂)
- 关系: `==`, `!=`, `<`, `>`, `<=`, `>=`

### 变量与数组

- 变量名: 单个小写字母 `a-z`
- 数组: `a(0)`, `a(1)`, ...
    - 解释器: 支持动态索引 `a(i)`
    - 编译器: 仅支持常量索引 `a(0)`

## 示例程序

### 计算 1 到 n 的和

```basic
10 rem 计算 1+2+...+n
20 input n
30 let s = 0
40 for i = 1 to n
50   let s = s + i
60 next i
70 print s
80 end
```

### 倒计时 (负步长)

```basic
10 rem 倒计时
20 for i = 5 to 1 step -1
30   print i
40 next i
50 end
```

### 数组使用

```basic
10 rem 数组测试
20 let a(0) = 10
30 let a(1) = 20
40 let s = a(0) + a(1)
50 print s
60 end
```

### 阶乘计算

```basic
10 rem 计算 n!
20 input n
30 let f = 1
40 for i = 1 to n
50   let f = f * i
60 next i
70 print f
80 end
```

## 架构说明

### 模块结构

```
compiler_2206/
├── include/              # 头文件
│   ├── token.h           # Token 类型定义
│   ├── lexer.h           # 词法分析器接口
│   ├── interpreter.h     # 解释器接口
│   ├── compiler.h        # 编译器接口
│   └── sml_vm.h          # SML 虚拟机接口
├── src/                  # 源文件
│   ├── main.c            # 主程序 (CLI)
│   ├── lexer.c           # 词法分析器实现
│   ├── interpreter.c     # 解释器实现
│   ├── compiler.c        # 编译器实现
│   └── sml_vm.c          # SML 虚拟机实现
├── docs/
│   └── SIMPLE_LANGUAGE.md  # 语言规范
├── examples/             # 示例程序
├── CMakeLists.txt        # 构建配置
└── README.md
```

### 编译过程 (Two-Pass)

编译器采用**两遍扫描**算法：

**第一遍 (Pass 1)**:

1. 逐行解析源代码
2. 建立符号表 (行号、变量、常量、数组、字符串)
3. 生成 SML 指令，前向引用 (如`goto 100`) 留空

**第二遍 (Pass 2)**:

1. 查找符号表中的所有行号
2. 填充未解决的跳转地址

### SML 虚拟机架构

采用**冯·诺依曼架构** + **累加器架构**：

```
内存布局 (100 单元):
┌─────────────────────────────────────┐
│  0: 指令区 (向高地址增长 →)         │
│  ...                                │
│  instruction_counter                │
│  ─────── 空闲区 ───────             │
│  data_counter                       │
│  ...                                │
│ 99: 数据区 (向低地址增长 ←)         │
└─────────────────────────────────────┘
```

**执行周期 (Fetch-Decode-Execute)**:

1. 取指 (Fetch): `IR = Memory[PC]`
2. 解码 (Decode): `opcode = IR/100`, `operand = IR%100`
3. 执行 (Execute): 根据 opcode 执行操作
4. `PC++` (除非是跳转指令)

**指令格式**: `±XXYY`

- XX: 操作码 (10-43)
- YY: 操作数 (内存地址 00-99)

### SML 指令集

| 操作码 | 助记符     | 说明           |
|-----|---------|--------------|
| 10  | READ    | 读取输入到内存      |
| 11  | WRITE   | 输出内存值        |
| 12  | NEWLINE | 输出换行符        |
| 13  | WRITES  | 输出字符串        |
| 20  | LOAD    | 内存 → 累加器     |
| 21  | STORE   | 累加器 → 内存     |
| 30  | ADD     | 累加器 += 内存    |
| 31  | SUB     | 累加器 -= 内存    |
| 32  | DIV     | 累加器 /= 内存    |
| 33  | MUL     | 累加器 *= 内存    |
| 34  | MOD     | 累加器 %= 内存    |
| 40  | JMP     | 无条件跳转        |
| 41  | JMPNEG  | 累加器 < 0 时跳转  |
| 42  | JMPZERO | 累加器 == 0 时跳转 |
| 43  | HALT    | 停机           |

## 解释器 vs 编译器

| 特性     | 解释器      | 编译器        |
|--------|----------|------------|
| 执行方式   | 边解析边执行   | 生成SML码后执行  |
| 浮点运算   | ✓ 支持     | ✗ 仅整数      |
| 动态数组索引 | ✓ `a(i)` | ✗ 仅 `a(0)` |
| 内存限制   | 无        | 100 单元     |
| 执行速度   | 较慢       | 较快         |
| 适用场景   | 调试、学习    | 部署、性能      |

## 限制说明

1. **内存限制**: SML虚拟机只有100个内存单元 (指令+数据共享)
2. **数组索引**: 编译器仅支持常量索引 (SML 无间接寻址指令)
3. **变量数量**: 最多26个变量 (a-z)
4. **整数限制**: SML使用int存储，范围 ±9999

## 文档

| 文档                                            | 说明              |
|-----------------------------------------------|-----------------|
| [SIMPLE_LANGUAGE.md](docs/SIMPLE_LANGUAGE.md) | Simple语言完整语法规范  |
| [IMPLEMENTATION.md](docs/IMPLEMENTATION.md)   | 编译器/解释器/虚拟机实现原理 |

## 参考资料

- 《编译原理》(龙书) - 词法分析、语法分析
- 《C How to Program》- Simpletron Machine Language
- 《计算机组成原理》- 冯诺依曼架构
- vm_2206: 配套的C++ SML虚拟机实现
