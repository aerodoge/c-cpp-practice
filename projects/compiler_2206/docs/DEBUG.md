# 调试指南

本文档记录 Simple 编译器/解释器的调试技巧、常见错误及解决方案。

## 目录

1. [调试工具](#1-调试工具)
2. [常见错误](#2-常见错误)
3. [调试技巧](#3-调试技巧)
4. [内部调试函数](#4-内部调试函数)
5. [GDB 调试示例](#5-gdb-调试示例)

---

## 1. 调试工具

### 1.1 编译调试版本

```bash
# 使用 Debug 模式编译 (包含调试符号，禁用优化)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 或手动指定编译选项
gcc -g -O0 -Wall -Wextra src/*.c -I include -o simple -lm
```

### 1.2 使用 GDB

```bash
# 启动 GDB
gdb ./build/simple

# 常用命令
(gdb) break main              # 设置断点
(gdb) break compiler.c:100    # 在特定行设置断点
(gdb) run examples/sum.simple # 运行程序
(gdb) next                    # 单步执行 (不进入函数)
(gdb) step                    # 单步执行 (进入函数)
(gdb) print variable          # 打印变量值
(gdb) backtrace               # 查看调用栈
(gdb) continue                # 继续执行
```

### 1.3 使用 LLDB (macOS)

```bash
# 启动 LLDB
lldb ./build/simple

# 常用命令
(lldb) breakpoint set -n main
(lldb) run examples/sum.simple
(lldb) thread step-over       # 单步
(lldb) thread step-in         # 进入函数
(lldb) frame variable         # 显示局部变量
(lldb) bt                     # 调用栈
```

### 1.4 使用 Valgrind (内存检查)

```bash
# 检查内存泄漏
valgrind --leak-check=full ./build/simple examples/sum.simple

# 检查未初始化内存
valgrind --track-origins=yes ./build/simple examples/sum.simple
```

### 1.5 使用 AddressSanitizer

```bash
# 编译时启用 AddressSanitizer
gcc -g -fsanitize=address -fno-omit-frame-pointer src/*.c -I include -o simple -lm

# 运行会自动检测内存错误
./simple examples/sum.simple
```

---

## 2. 常见错误

### 2.1 词法分析错误

#### 错误: 未识别的字符

```
Error: Unexpected character '$' at line 10
```

**原因**: Simple 语言只支持特定字符集

**解决**: 检查源代码是否包含非法字符

#### 错误: 字符串未闭合

```
Error: Unterminated string at line 10
```

**原因**: 字符串缺少结束引号

**解决**:
```basic
10 print "hello    // 错误
10 print "hello"   // 正确
```

### 2.2 语法错误

#### 错误: 缺少行号

```
Error: Expected line number
```

**原因**: Simple 程序每行必须以数字开头

**解决**:
```basic
print x        // 错误
10 print x     // 正确
```

#### 错误: 表达式语法错误

```
Error: Expected expression at line 10
```

**原因**: 表达式不完整或格式错误

**解决**:
```basic
10 let x = * 5     // 错误: 缺少左操作数
10 let x = 3 * 5   // 正确
```

#### 错误: for/next 不匹配

```
Error: 'next' without matching 'for'
```

**原因**: `next` 语句的变量与 `for` 不匹配

**解决**:
```basic
10 for i = 1 to 5
20 next j          // 错误: 应该是 next i
```

### 2.3 运行时错误

#### 错误: 除零

```
Runtime Error: Division by zero at address 05
```

**原因**: 除法或取模运算的除数为零

**解决**: 在除法前检查除数

```basic
10 input x
20 if x == 0 goto 50
30 let y = 100 / x
40 print y
50 end
```

#### 错误: 未定义变量

```
Error: Undefined variable 'x' at line 10
```

**原因**: 使用了未初始化的变量

**解决**: 确保变量在使用前被赋值

#### 错误: 数组越界

```
Error: Array index out of bounds
```

**原因**: 数组索引超出 0-99 范围

**解决**: 检查数组索引范围

#### 错误: 跳转目标不存在

```
Error: Line 100 not found
```

**原因**: `goto` 或 `if` 的目标行号不存在

**解决**: 确保目标行号在程序中定义

### 2.4 编译错误

#### 错误: 内存溢出

```
Error: Out of memory (code/data collision)
```

**原因**: 程序太大，指令区和数据区冲突

**解决**:
- 减少变量数量
- 减少常量数量 (相同常量只需定义一次)
- 简化程序逻辑

#### 错误: 符号表溢出

```
Error: Symbol table overflow
```

**原因**: 符号数量超过 MAX_SYMBOLS (100)

**解决**: 减少变量和常量数量

---

## 3. 调试技巧

### 3.1 使用编译模式查看生成代码

```bash
# 使用 -c 选项查看编译结果
./simple -c examples/sum.simple
```

输出示例:
```
=== Compiling examples/sum.simple ===
Compilation successful!

=== Symbol Table ===
  LINE    10 -> loc 00
  LINE    20 -> loc 02
  VAR     's' -> loc 99
  CONST   0 -> loc 98
  ...

=== SML Program ===
Instructions (0-15):
  00: +2098  LOAD     98
  01: +2199  STORE    99
  ...
```

### 3.2 单步执行 SML 程序

在代码中添加单步执行:

```c
SML_VM vm;
sml_vm_load_file(&vm, "program.sml");

while (vm.running) {
    sml_vm_dump_registers(&vm);  // 打印寄存器状态
    sml_vm_step(&vm);            // 执行一条指令
    getchar();                   // 等待用户按键
}
```

### 3.3 添加调试输出

在源代码关键位置添加调试信息:

```c
// compiler.c 中
#define DEBUG_COMPILE 1

#if DEBUG_COMPILE
    printf("[DEBUG] Compiling line %d: %s\n", line_number, statement);
#endif
```

### 3.4 检查符号表

编译后检查符号表是否正确:

```bash
./simple -c program.simple 2>&1 | grep "Symbol Table" -A 20
```

### 3.5 检查前向引用

前向引用问题是编译器常见 bug。检查方法:

1. 使用 `-c` 模式编译
2. 查看符号表中的行号映射
3. 检查跳转指令的目标地址是否正确

---

## 4. 内部调试函数

### 4.1 编译器调试函数

```c
// 打印符号表
compiler_dump_symbols(&comp);

// 打印 SML 程序
compiler_dump(&comp);

// 获取错误信息
printf("Error: %s\n", compiler_get_error(&comp));
```

### 4.2 虚拟机调试函数

```c
// 打印寄存器状态
sml_vm_dump_registers(&vm);
/*
输出:
REGISTERS:
  accumulator          +0042
  instruction_counter     05
  instruction_register +2099
  opcode                  20
  operand                 99
*/

// 打印内存内容
sml_vm_dump_memory(&vm);
/*
输出:
MEMORY:
       0     1     2     3     4     5     6     7     8     9
 0  +2098 +2199 +2097 ...
10  ...
*/

// 单步执行
sml_vm_step(&vm);

// 获取错误信息
printf("Error: %s\n", sml_vm_get_error(&vm));
```

### 4.3 解释器调试

```c
// 获取错误信息
printf("Error: %s\n", interpreter_get_error(&interp));
```

---

## 5. GDB 调试示例

### 5.1 调试词法分析器

```gdb
(gdb) break lexer_next_token
(gdb) run examples/sum.simple
(gdb) print lexer->current
(gdb) print lexer->line
(gdb) continue
```

### 5.2 调试编译器

```gdb
# 断点设置在编译表达式
(gdb) break compile_expression
(gdb) run -c examples/sum.simple

# 查看当前 token
(gdb) print comp->current_token

# 查看符号表
(gdb) print comp->symbols[0]
(gdb) print comp->symbol_count

# 查看生成的指令
(gdb) print comp->memory[0]
(gdb) print comp->instruction_counter
```

### 5.3 调试虚拟机

```gdb
# 断点设置在执行循环
(gdb) break sml_vm_step
(gdb) run -r examples/sum.simple

# 查看寄存器
(gdb) print vm->accumulator
(gdb) print vm->instruction_counter
(gdb) print vm->instruction_register

# 查看内存
(gdb) print vm->memory[0]@10    # 查看前10个内存单元
(gdb) print vm->memory[90]@10   # 查看最后10个内存单元
```

### 5.4 条件断点

```gdb
# 当行号为 50 时断点
(gdb) break compiler.c:200 if comp->current_line_number == 50

# 当累加器为负时断点
(gdb) break sml_vm.c:100 if vm->accumulator < 0

# 当执行特定指令时断点
(gdb) break sml_vm.c:100 if vm->opcode == 43  # HALT
```

---

## 6. 性能分析

### 6.1 使用 gprof

```bash
# 编译时启用分析
gcc -pg src/*.c -I include -o simple -lm

# 运行程序
./simple examples/sum.simple

# 生成分析报告
gprof simple gmon.out > analysis.txt
```

### 6.2 使用 perf (Linux)

```bash
# 记录性能数据
perf record ./simple examples/sum.simple

# 查看报告
perf report
```

---

## 7. 常见调试场景

### 场景 1: 程序无输出

**检查步骤**:
1. 确认程序有 `end` 语句
2. 确认 `print` 语句语法正确
3. 使用 `-c` 查看是否生成了 WRITE 指令

### 场景 2: 无限循环

**检查步骤**:
1. 检查 `for` 循环的边界条件
2. 检查 `step` 值的正负是否正确
3. 使用单步执行定位循环位置

### 场景 3: 计算结果错误

**检查步骤**:
1. 使用 `-c` 查看生成的 SML 代码
2. 检查运算符优先级
3. 检查变量是否正确初始化

### 场景 4: 段错误 (Segmentation Fault)

**检查步骤**:
1. 使用 Valgrind 检查内存访问
2. 使用 GDB 获取崩溃位置的 backtrace
3. 检查数组索引和指针操作

---

## 8. 测试建议

### 8.1 单元测试

```bash
# 运行所有测试
cd build && ctest

# 运行特定测试
./test_lexer
./test_compiler
./test_sml_vm
```

### 8.2 边界测试用例

创建测试极端情况的程序:

```basic
// 最大行号
9999 end

// 嵌套循环
10 for i = 1 to 3
20   for j = 1 to 3
30     print i
40   next j
50 next i
60 end

// 复杂表达式
10 let x = 1 + 2 * 3 - 4 / 2 % 3 ^ 2
20 print x
30 end
```

### 8.3 回归测试

保存已知正确的测试用例和期望输出:

```bash
# 运行并保存输出
./simple examples/sum.simple > expected/sum.out

# 验证输出
./simple examples/sum.simple | diff - expected/sum.out
```
