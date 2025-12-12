# 虚拟机实现原理

本文档详细介绍 SML 虚拟机的内部实现原理，包括架构设计、核心数据结构、执行周期和设计模式应用。

## 目录

1. [架构概述](#1-架构概述)
2. [核心数据结构](#2-核心数据结构)
3. [取指-解码-执行循环](#3-取指-解码-执行循环)
4. [指令系统实现](#4-指令系统实现)
5. [设计模式应用](#5-设计模式应用)
6. [C++20 特性应用](#6-c20-特性应用)

---

## 1. 架构概述

### 1.1 冯诺依曼架构

本虚拟机采用经典的**冯诺依曼架构**：

```
┌─────────────────────────────────────────────────────────────┐
│                      虚拟机 (VirtualMachine)                 │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                  VMContext (上下文)                   │    │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────────┐    │    │
│  │  │ 累加器 AC │  │ 程序计数器│  │ 指令寄存器 IR │    │    │
│  │  │  (int)    │  │  PC (int) │  │    (int)      │    │    │
│  │  └───────────┘  └───────────┘  └───────────────┘    │    │
│  │  ┌─────────────────────────────────────────────┐    │    │
│  │  │              内存 Memory[100]                │    │    │
│  │  │  ┌────┬────┬────┬────┬─────┬────┬────┐     │    │    │
│  │  │  │ 00 │ 01 │ 02 │... │ ... │ 98 │ 99 │     │    │    │
│  │  │  └────┴────┴────┴────┴─────┴────┴────┘     │    │    │
│  │  │    ↑ 指令区              数据区 ↑          │    │    │
│  │  └─────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │           InstructionFactory (指令工厂)              │    │
│  │  ┌─────────────────────────────────────────────┐    │    │
│  │  │  OpCode → IInstruction* 映射                 │    │    │
│  │  │  READ(10)  → ReadInstruction                 │    │    │
│  │  │  WRITE(11) → WriteInstruction                │    │    │
│  │  │  LOAD(20)  → LoadInstruction                 │    │    │
│  │  │  ...                                         │    │    │
│  │  └─────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 核心特点

| 特点 | 说明 |
|------|------|
| **存储程序** | 指令和数据存储在同一内存中 |
| **累加器架构** | 所有运算通过单一累加器进行 |
| **定长指令** | 每条指令占用一个内存单元 |
| **顺序执行** | 除非跳转，否则按地址顺序执行 |

### 1.3 组件职责

| 组件 | 职责 |
|------|------|
| `VirtualMachine` | 主控制器，协调执行流程 |
| `VMContext` | 状态容器，管理寄存器和内存 |
| `InstructionFactory` | 指令对象工厂，根据操作码获取指令 |
| `IInstruction` | 指令接口，定义统一执行方式 |
| `XxxInstruction` | 具体指令实现类 |

---

## 2. 核心数据结构

### 2.1 VMContext - 虚拟机上下文

```cpp
struct VMContext {
    static constexpr size_t MEMORY_SIZE = 100;

    std::array<int, MEMORY_SIZE> memory{};  // 内存 (初始化为0)
    int accumulator = 0;                     // 累加器
    int instructionCounter = 0;              // 程序计数器 (PC)
    int instructionRegister = 0;             // 指令寄存器 (IR)
    bool running = false;                    // 运行状态

    void reset();                            // 重置所有状态
    int getMemory(int address) const;        // 安全读取内存
    void setMemory(int address, int value);  // 安全写入内存
};
```

**设计要点**：
- 使用 `std::array` 而非原始数组，提供边界检查
- `constexpr` 常量确保编译期确定内存大小
- 成员函数封装内存访问，保证安全性

### 2.2 指令格式

```
指令 = ±XXYY (4位十进制数)
       ││└┴── 操作数 (00-99): 内存地址
       └┴──── 操作码 (10-43): 指令类型

示例:
  +2099 → LOAD 99   (opcode=20, operand=99)
  +3008 → ADD 08    (opcode=30, operand=08)
  +4300 → HALT 00   (opcode=43, operand=00)
```

### 2.3 操作码枚举

```cpp
enum class OpCode : int {
    // I/O 指令
    READ      = 10,   // 读取输入
    WRITE     = 11,   // 输出数据
    NEWLINE   = 12,   // 输出换行 (扩展)
    WRITES    = 13,   // 输出字符串 (扩展)

    // 数据传输
    LOAD      = 20,   // 内存 → 累加器
    STORE     = 21,   // 累加器 → 内存

    // 算术运算
    ADD       = 30,   // 加法
    SUB       = 31,   // 减法
    DIV       = 32,   // 除法
    MUL       = 33,   // 乘法
    MOD       = 34,   // 取模 (扩展)

    // 控制流
    JMP       = 40,   // 无条件跳转
    JMPNEG    = 41,   // 负数跳转
    JMPZERO   = 42,   // 零跳转
    HALT      = 43    // 停机
};
```

**使用强类型枚举的优势**：
- 类型安全，不会与普通 int 混淆
- 作用域限定，避免命名冲突
- 可指定底层类型 (int)

---

## 3. 取指-解码-执行循环

### 3.1 执行流程图

```
          ┌──────────────────┐
          │   execute()      │
          │  启动虚拟机      │
          └────────┬─────────┘
                   ↓
          ┌──────────────────┐
          │ running == true? │──否──→ 退出
          └────────┬─────────┘
                   │ 是
                   ↓
    ┌──────────────────────────────────┐
    │  executeSingleInstruction()      │
    │  ┌────────────────────────────┐  │
    │  │ 1. 取指 (Fetch)            │  │
    │  │    IR = Memory[PC]         │  │
    │  └────────────┬───────────────┘  │
    │               ↓                  │
    │  ┌────────────────────────────┐  │
    │  │ 2. 解码 (Decode)           │  │
    │  │    opcode = IR / 100       │  │
    │  │    operand = IR % 100      │  │
    │  └────────────┬───────────────┘  │
    │               ↓                  │
    │  ┌────────────────────────────┐  │
    │  │ 3. 获取指令对象            │  │
    │  │    instruction = factory.  │  │
    │  │      getInstruction(opcode)│  │
    │  └────────────┬───────────────┘  │
    │               ↓                  │
    │  ┌────────────────────────────┐  │
    │  │ 4. 执行 (Execute)          │  │
    │  │    instruction->execute(   │  │
    │  │      context, operand)     │  │
    │  └────────────┬───────────────┘  │
    │               ↓                  │
    │  ┌────────────────────────────┐  │
    │  │ 5. 更新 PC                 │  │
    │  │    if (!changesPC())       │  │
    │  │      PC++                  │  │
    │  └────────────────────────────┘  │
    └──────────────┬───────────────────┘
                   │
                   └──────→ (回到 running 检查)
```

### 3.2 核心代码实现

```cpp
// VirtualMachine.cpp

void VirtualMachine::execute() {
    context_.running = true;
    context_.instructionCounter = 0;

    while (context_.running) {
        try {
            executeSingleInstruction();
        } catch (const std::exception& e) {
            std::cerr << "运行时错误: " << e.what() << std::endl;
            context_.running = false;
        }
    }
}

void VirtualMachine::executeSingleInstruction() {
    // 1. 取指 (Fetch)
    context_.instructionRegister = context_.memory[context_.instructionCounter];

    // 2. 解码 (Decode)
    const int opcode = context_.instructionRegister / 100;
    const int operand = context_.instructionRegister % 100;

    // 3. 获取指令对象
    auto instructionOpt = factory_.getInstruction(static_cast<OpCode>(opcode));
    if (!instructionOpt.has_value()) {
        throw std::runtime_error("未知的操作码: " + std::to_string(opcode));
    }
    IInstruction* instruction = instructionOpt.value();

    // 4. 执行 (Execute)
    instruction->execute(context_, operand);

    // 5. 更新 PC
    if (!instruction->changesPC()) {
        context_.instructionCounter++;
    }
}
```

### 3.3 各阶段详解

#### 取指 (Fetch)

```cpp
context_.instructionRegister = context_.memory[context_.instructionCounter];
```

- 从内存中读取 PC 指向的内容
- 存入指令寄存器 IR
- 此时 IR 包含完整的指令编码 (如 +2099)

#### 解码 (Decode)

```cpp
const int opcode = context_.instructionRegister / 100;   // 20
const int operand = context_.instructionRegister % 100;  // 99
```

- 整除 100 得到操作码
- 取模 100 得到操作数
- 例如: 2099 → opcode=20 (LOAD), operand=99

#### 执行 (Execute)

```cpp
instruction->execute(context_, operand);
```

- 调用指令对象的 execute 方法
- 传入上下文和操作数
- 指令自行完成具体操作

#### 更新 PC

```cpp
if (!instruction->changesPC()) {
    context_.instructionCounter++;
}
```

- 普通指令: PC 自动加 1
- 跳转指令: 在 execute 中已设置 PC，不再自增
- 通过 `changesPC()` 方法判断

---

## 4. 指令系统实现

### 4.1 指令接口

```cpp
class IInstruction {
public:
    virtual ~IInstruction() = default;

    // 执行指令
    virtual void execute(VMContext& context, int operand) = 0;

    // 获取指令名称 (调试用)
    [[nodiscard]] virtual std::string getName() const = 0;

    // 指令是否改变 PC (默认不改变)
    [[nodiscard]] virtual bool changesPC() const { return false; }
};
```

### 4.2 指令类层次

```
IInstruction (接口)
├── ReadInstruction        (READ)
├── WriteInstruction       (WRITE)
├── LoadInstruction        (LOAD)
├── StoreInstruction       (STORE)
├── ArithmeticInstruction  (抽象基类)
│   ├── AddInstruction     (ADD)
│   ├── SubtractInstruction(SUB)
│   ├── MultiplyInstruction(MUL)
│   └── DivideInstruction  (DIV)
└── ControlFlowInstruction (抽象基类)
    ├── JumpInstruction    (JMP)
    ├── JumpNegInstruction (JMPNEG)
    ├── JumpZeroInstruction(JMPZERO)
    └── HaltInstruction    (HALT)
```

### 4.3 算术指令 - Template Method 模式

```cpp
// 基类定义算法骨架
class ArithmeticInstruction : public IInstruction {
protected:
    // 子类实现具体运算
    virtual int compute(int accumulator, int operand) const = 0;

public:
    void execute(VMContext& context, int operand) override {
        int value = context.getMemory(operand);  // 1. 读取内存
        context.accumulator = compute(context.accumulator, value);  // 2. 运算
    }
};

// 子类只需实现 compute
class AddInstruction : public ArithmeticInstruction {
protected:
    int compute(int accumulator, int operand) const override {
        return accumulator + operand;
    }
};

class DivideInstruction : public ArithmeticInstruction {
protected:
    int compute(int accumulator, int operand) const override {
        if (operand == 0) {
            throw std::runtime_error("除数为零");
        }
        return accumulator / operand;
    }
};
```

### 4.4 控制流指令

```cpp
// 控制流基类
class ControlFlowInstruction : public IInstruction {
public:
    [[nodiscard]] bool changesPC() const override {
        return true;  // 控制流指令都改变 PC
    }
};

// 无条件跳转
class JumpInstruction : public ControlFlowInstruction {
public:
    void execute(VMContext& context, int operand) override {
        context.instructionCounter = operand;
    }
};

// 条件跳转 (负数)
class JumpNegInstruction : public ControlFlowInstruction {
public:
    void execute(VMContext& context, int operand) override {
        if (context.accumulator < 0) {
            context.instructionCounter = operand;
        } else {
            context.instructionCounter++;
        }
    }
};

// 停机
class HaltInstruction : public ControlFlowInstruction {
public:
    void execute(VMContext& context, int operand) override {
        context.running = false;
    }
};
```

---

## 5. 设计模式应用

### 5.1 Command 模式

**应用**: 每个指令都是一个命令对象

```
┌─────────────┐     ┌───────────────┐
│VirtualMachine│────→│ IInstruction  │
│  (Invoker)  │     │  (Command)    │
└─────────────┘     └───────┬───────┘
                            │
        ┌───────────────────┼───────────────────┐
        ↓                   ↓                   ↓
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│LoadInstruction│   │ AddInstruction│   │HaltInstruction│
│  (Concrete)   │   │  (Concrete)   │   │  (Concrete)   │
└───────────────┘   └───────────────┘   └───────────────┘
```

**优势**:
- 指令与执行器解耦
- 易于添加新指令
- 支持指令的撤销/重做（如果需要）

### 5.2 Factory 模式

**应用**: InstructionFactory 创建指令对象

```cpp
class InstructionFactory {
private:
    std::map<OpCode, std::unique_ptr<IInstruction>> instructions_;

public:
    InstructionFactory() {
        instructions_.emplace(OpCode::READ, std::make_unique<ReadInstruction>());
        instructions_.emplace(OpCode::WRITE, std::make_unique<WriteInstruction>());
        // ...
    }

    std::optional<IInstruction*> getInstruction(OpCode opcode) const {
        auto it = instructions_.find(opcode);
        if (it != instructions_.end()) {
            return it->second.get();
        }
        return std::nullopt;
    }
};
```

### 5.3 Singleton 模式

**应用**: InstructionFactory 全局唯一

```cpp
class InstructionFactory {
public:
    static InstructionFactory& getInstance() {
        static InstructionFactory instance;  // 线程安全的局部静态变量
        return instance;
    }

private:
    InstructionFactory();  // 私有构造函数
    InstructionFactory(const InstructionFactory&) = delete;
    InstructionFactory& operator=(const InstructionFactory&) = delete;
};
```

### 5.4 Template Method 模式

**应用**: ArithmeticInstruction 定义算法骨架

```
ArithmeticInstruction::execute()
    │
    ├── 1. getMemory(operand)     // 固定步骤
    │
    ├── 2. compute(acc, value)    // 子类实现
    │       │
    │       ├── AddInstruction::compute() → acc + value
    │       ├── SubInstruction::compute() → acc - value
    │       ├── MulInstruction::compute() → acc * value
    │       └── DivInstruction::compute() → acc / value
    │
    └── 3. 存入 accumulator       // 固定步骤
```

---

## 6. C++20 特性应用

### 6.1 强类型枚举 (enum class)

```cpp
enum class OpCode : int {
    READ = 10,
    WRITE = 11,
    // ...
};

// 使用时必须显式转换
int opcode = 10;
auto op = static_cast<OpCode>(opcode);
```

### 6.2 std::optional

```cpp
std::optional<IInstruction*> getInstruction(OpCode opcode) const {
    auto it = instructions_.find(opcode);
    if (it != instructions_.end()) {
        return it->second.get();
    }
    return std::nullopt;  // 表示"无值"
}

// 使用
auto opt = factory.getInstruction(opcode);
if (opt.has_value()) {
    opt.value()->execute(context, operand);
}
```

### 6.3 [[nodiscard]] 属性

```cpp
[[nodiscard]] virtual std::string getName() const = 0;
[[nodiscard]] virtual bool changesPC() const { return false; }
```

- 返回值必须被使用，否则编译器警告
- 防止忽略重要返回值

### 6.4 std::array

```cpp
std::array<int, MEMORY_SIZE> memory{};
```

- 替代原始数组 `int memory[100]`
- 提供 `size()`, `at()` 等方法
- 支持范围 for 循环
- 列表初始化为零

### 6.5 智能指针

```cpp
std::map<OpCode, std::unique_ptr<IInstruction>> instructions_;
```

- 自动管理指令对象生命周期
- 无需手动 delete
- 异常安全

---

## 7. 执行示例

### 程序: 计算 5 + 3

```
内存:
  00: +2099  LOAD 99    ; 加载 A
  01: +3098  ADD 98     ; 加 B
  02: +2197  STORE 97   ; 存结果
  03: +1197  WRITE 97   ; 输出
  04: +4300  HALT       ; 停机
  ...
  97: +0000  (结果)
  98: +0003  (B = 3)
  99: +0005  (A = 5)
```

### 执行轨迹

| 周期 | PC | IR | opcode | operand | 操作 | AC | Mem[97] |
|-----|----|----|--------|---------|------|-----|---------|
| 1 | 0 | +2099 | 20 | 99 | LOAD 99 | 5 | 0 |
| 2 | 1 | +3098 | 30 | 98 | ADD 98 | 8 | 0 |
| 3 | 2 | +2197 | 21 | 97 | STORE 97 | 8 | 8 |
| 4 | 3 | +1197 | 11 | 97 | WRITE 97 | 8 | 8 |
| 5 | 4 | +4300 | 43 | 00 | HALT | 8 | 8 |

**输出**: `8`

---

## 参考资料

- 《计算机组成与设计》- 冯诺依曼架构
- 《设计模式》- Command, Factory, Singleton, Template Method
- 《C++ Primer》- 现代 C++ 特性
- 《Effective Modern C++》- 智能指针、enum class
