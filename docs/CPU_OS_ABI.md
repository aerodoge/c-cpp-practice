# CPU、操作系统与 ABI

深入理解 CPU 架构、操作系统和 ABI（应用二进制接口）之间的关系。

---

## 目录

1. [什么是 CPU 架构？](#1-什么是-cpu-架构)
2. [相同 CPU + 不同 OS：编译结果不同](#2-相同-cpu--不同-os编译结果不同)
3. [不同 CPU + 相同 OS：几乎完全不同](#3-不同-cpu--相同-os几乎完全不同)
4. [ABI 详解](#4-abi-详解)
5. [OS 支持多种 ABI](#5-os-支持多种-abi)
6. [总结](#6-总结)
7. [汇编基础：寄存器与内存访问](#7-汇编基础寄存器与内存访问)

---

## 1. 什么是 CPU 架构？

CPU 架构（也叫指令集架构，ISA - Instruction Set Architecture）定义了 CPU 的"语言"。

### 1.1 CPU 架构包含什么？

| 组成部分     | 说明          | 示例                                     |
|----------|-------------|----------------------------------------|
| **指令集**  | CPU 能执行哪些操作 | `add`, `mov`, `jmp`, `syscall`         |
| **寄存器**  | 有哪些寄存器、多少位  | x86_64: rax, rbx... / ARM64: x0, x1... |
| **指令编码** | 每条指令的二进制格式  | `add` 在 x86 和 ARM 编码完全不同               |
| **寻址模式** | 如何访问内存      | 直接寻址、间接寻址、变址寻址等                        |
| **字长**   | 一次处理多少位数据   | 32 位、64 位                              |

### 1.2 常见 CPU 架构

| 架构                  | 类型   | 典型 CPU                | 特点              |
|---------------------|------|-----------------------|-----------------|
| **x86_64** (AMD64)  | CISC | Intel Core, AMD Ryzen | 变长指令，复杂指令集      |
| **ARM64** (AArch64) | RISC | Apple M 系列, 骁龙        | 定长指令，精简指令集      |
| **RISC-V**          | RISC | 开源 CPU                | 开源架构，模块化设计      |
| **x86** (i386)      | CISC | 老 Intel/AMD           | 32 位，x86_64 的前身 |
| **ARM32**           | RISC | 树莓派、老手机               | 32 位 ARM        |

### 1.3 架构 vs 具体 CPU 型号

```
架构 (Architecture)          具体 CPU (Implementation)
─────────────────────────────────────────────────────
x86_64                    →  Intel Core i9-14900K
                          →  AMD Ryzen 9 7950X
                          →  Intel Xeon

ARM64                     →  Apple M3
                          →  高通骁龙 8 Gen 3
                          →  AWS Graviton

RISC-V                    →  SiFive U74
                          →  阿里平头哥玄铁
```

**架构**是规范/标准，**具体 CPU** 是实现。同一架构的不同 CPU 可以运行相同的机器码。

---

## 2. 相同 CPU + 不同 OS：编译结果不同

相同架构的CPU在不同操作系统上编译出来的机器码**通常不同**。

### 2.1 可执行文件格式不同

| OS      | 可执行文件格式 | 文件头魔数                 |
|---------|---------|-----------------------|
| Linux   | ELF     | `0x7F 'E' 'L' 'F'`    |
| macOS   | Mach-O  | `0xFE 0xED 0xFA 0xCE` |
| Windows | PE      | `'M' 'Z'`             |

### 2.2 系统调用不同

同样是打印"Hello World"，不同OS的系统调用完全不同：

```asm
; Linux x86_64 - 使用 syscall
mov rax, 1          ; sys_write (调用号 1)
mov rdi, 1          ; stdout
mov rsi, msg        ; 字符串地址
mov rdx, 13         ; 长度
syscall

; macOS x86_64 - 也用 syscall，但调用号不同
mov rax, 0x2000004  ; sys_write (调用号 0x2000004)
mov rdi, 1
mov rsi, msg
mov rdx, 13
syscall

; Windows x86_64 - 调用 DLL，完全不同的方式
sub rsp, 40
mov rcx, -11        ; STD_OUTPUT_HANDLE
call GetStdHandle   ; 调用 kernel32.dll
mov rcx, rax
lea rdx, [msg]
mov r8, 13
call WriteConsole
```

### 2.3 ABI（应用二进制接口）不同

| 项目    | Linux/macOS (System V)     | Windows (Microsoft x64)     |
|-------|----------------------------|-----------------------------|
| 参数传递  | rdi, rsi, rdx, rcx, r8, r9 | rcx, rdx, r8, r9            |
| 返回值   | rax                        | rax                         |
| 栈对齐   | 16 字节                      | 16 字节                       |
| 调用者保存 | rbx, rbp, r12-r15          | rbx, rbp, rdi, rsi, r12-r15 |

### 2.4 链接的库不同

```
Linux:   libc.so.6 → glibc
macOS:   libSystem.B.dylib
Windows: msvcrt.dll, kernel32.dll
```

### 2.5 哪些部分可能相同？

**纯计算逻辑**的机器码可能相同：

```c
int add(int a, int b) {
    return a + b;
}

// 可能在所有平台上都生成类似的指令：
// lea eax, [rdi + rsi]
// ret
```

但即使是简单函数，参数传递方式可能导致寄存器使用不同。

### 2.6 小结

| 层面      | 相同 CPU + 不同 OS             |
|---------|----------------------------|
| 源代码     | ✅ 相同                       |
| 可执行文件格式 | ❌ 不同 (ELF vs PE vs Mach-O) |
| 系统调用    | ❌ 完全不同                     |
| ABI     | ⚠️ 可能不同 (Linux vs Windows) |
| 链接库     | ❌ 不同                       |
| 纯计算机器码  | ⚠️ 可能相同                    |

---

## 3. 不同 CPU + 相同 OS：几乎完全不同

不同架构的 CPU 在相同 OS 上编译后，**几乎所有底层的东西都不同**。

### 3.1 对比表

| 层面    | x86_64 (Intel/AMD)    | ARM64 (Apple M/骁龙) |
|-------|-----------------------|--------------------|
| 指令集   | CISC，变长指令             | RISC，定长 4 字节       |
| 机器码   | 完全不同                  | 完全不同               |
| 寄存器   | rax, rbx, rdi, rsi... | x0, x1, x2... x30  |
| ABI   | System V AMD64        | AAPCS64            |
| 参数传递  | rdi, rsi, rdx, rcx    | x0, x1, x2, x3     |
| 系统调用号 | 不同                    | 不同                 |
| 可执行文件 | 都是 ELF/Mach-O，但内容不同   | 同左                 |

### 3.2 具体例子

同一个 C 函数：

```c
int add(int a, int b) {
    return a + b;
}
```

**x86_64 编译结果：**

```asm
add:
    lea eax, [rdi + rsi]    ; 参数在 rdi, rsi
    ret
```

机器码：`8D 04 37 C3` (变长)

**ARM64 编译结果：**

```asm
add:
    add w0, w0, w1          ; 参数在 w0, w1
    ret
```

机器码：`0B 01 00 00 C0 03 5F D6` (定长 4 字节每条)

### 3.3 哪些相同？

| 相同的     | 说明                              |
|---------|---------------------------------|
| 源代码     | C/Go/Rust 代码不变                  |
| 文件格式类型  | 都是 ELF (Linux) 或 Mach-O (macOS) |
| 系统调用名称  | 都叫 `write()`、`read()`，但调用号不同    |
| 高级库 API | `printf()`、`malloc()` 接口相同      |

### 3.4 小结

| 层面      | 不同 CPU + 相同 OS |
|---------|----------------|
| 源代码     | ✅ 相同           |
| 可执行文件格式 | ⚠️ 类型相同，内容不同   |
| 指令集     | ❌ 完全不同         |
| 机器码     | ❌ 完全不同         |
| ABI     | ❌ 不同           |
| 系统调用号   | ❌ 不同           |

---

## 4. ABI 详解

### 4.1 什么是 ABI

ABI (Application Binary Interface) 定义了：

- 函数参数放在哪些寄存器
- 返回值放在哪个寄存器
- 哪些寄存器由调用者保存
- 栈的对齐方式
- 数据类型的大小和对齐

### 4.2 ABI 是谁定义的？

**ABI 是跟 CPU 架构绑定的，不是跟 OS 绑定的。**

```
CPU 厂商/标准组织
      │
      │ 制定 ABI 规范文档
      │ (规定参数放哪个寄存器)
      ↓
┌─────────────────────────────────────┐
│ ABI 规范文档                         │
│                                     │
│ "第1个整数参数放 rdi"                 │
│ "第2个整数参数放 rsi"                 │
│ "返回值放 rax"                       │
└─────────────────────────────────────┘
      │
      │ 编译器厂商阅读规范，实现到编译器中
      ↓
┌─────────────────────────────────────┐
│ 编译器 (GCC/Clang/MSVC)              │
│                                     │
│ 把 C 代码翻译成符合 ABI 的机器码       │
└─────────────────────────────────────┘
      │
      │ OS 选择使用哪个 ABI，系统库按此编译
      ↓
┌─────────────────────────────────────┐
│ 操作系统                             │
│                                     │
│ "我的系统库用 System V ABI 编译"      │
│ "你想调用我的库，就得遵守这个 ABI"     │
└─────────────────────────────────────┘
```

### 4.3 各平台的 ABI

| 平台           | ABI 名称                           | 制定者           |
|--------------|----------------------------------|---------------|
| Linux x86_64 | System V AMD64 ABI               | AMD + 开源社区    |
| macOS x86_64 | System V AMD64 ABI (略有修改)        | Apple 采纳 + 修改 |
| Windows x64  | Microsoft x64 calling convention | Microsoft     |
| ARM64        | AAPCS64                          | ARM 公司        |

### 4.4 各角色职责

| 角色           | 职责         | 决定什么     |
|--------------|------------|----------|
| AMD/ARM/标准组织 | 写规范文档      | 参数放哪个寄存器 |
| 编译器          | 读规范，生成代码   | 如何实现这些规则 |
| 操作系统         | 选择采用哪个 ABI | 系统库用哪套规则 |
| 开发者          | 写代码        | 一般不用管    |

### 4.5 为什么ABI不能跨架构统一？

因为 ABI 规定的是**具体用哪个寄存器**，而不同架构的寄存器完全不同：

```
x86_64 有的寄存器：rax, rbx, rcx, rdx, rdi, rsi, r8-r15
ARM64 有的寄存器：x0-x30, sp

x86_64 根本没有 x0 寄存器
ARM64 根本没有 rdi 寄存器
```

不可能说"第一个参数放rdi"然后让ARM64也遵守——ARM64没有rdi。

### 4.6 OS 与 ABI 的关系

```
                    ┌─────────────────────────────────────┐
                    │              Linux                  │
                    └─────────────────────────────────────┘
                           /                    \
                          /                      \
            ┌────────────────────┐      ┌────────────────────┐
            │ Linux on x86_64    │      │ Linux on ARM64     │
            │                    │      │                    │
            │ 采用 System V      │      │ 采用 AAPCS64       │
            │ AMD64 ABI          │      │ ABI                │
            │                    │      │                    │
            │ 参数: rdi,rsi,rdx  │      │ 参数: x0,x1,x2    │
            │ syscall号: 1=write │      │ syscall号: 64=write│
            └────────────────────┘      └────────────────────┘
```

**OS不是BI的制定者，而是ABI的采纳者。**OS为**每种CPU架构**选择一种ABI。

### 4.7 OS能决定什么？

| 项目       | OS 能统一吗？    | 原因                           |
|----------|-------------|------------------------------|
| 参数用哪个寄存器 | ❌ 不能        | 不同架构寄存器不同                    |
| 系统调用号    | ⚠️ 能，但实际没统一 | 历史原因，各架构移植时独立分配（见下文）         |
| 系统调用名称   | ✅ 能         | `write()` 在所有架构上都叫 `write()` |
| libc API | ✅ 能         | `printf()` 接口相同              |

### 4.8 为什么系统调用号各架构不同？

系统调用号**是OS决定的**，但Linux选择了为每个架构分配不同的调用号：

```
Linux x86_64:  write = 1,  read = 0,  open = 2
Linux ARM64:   write = 64, read = 63, open = 56
Linux RISC-V:  write = 64, read = 63, open = 57
```

**原因：历史包袱**

各架构的Linux移植是不同时间、不同团队做的：

```
1991: Linux 诞生于 i386，调用号从 0 开始分配
2003: x86_64 移植，重新设计了调用号（优化了常用调用的编号）
2012: ARM64 移植，又用了不同的编号方案
```

每次移植时，团队可以选择沿用已有架构的调用号，但实际选择了重新设计"更好"的调用号。

**为什么不统一？没必要**

因为用户态程序不直接用调用号，而是调用`libc`：

```c
// 你写的代码
write(1, "hello", 5);

// libc 在 x86_64 上展开为
// mov rax, 1; syscall

// libc 在 ARM64 上展开为
// mov x8, 64; svc #0
```

`libc`针对每个架构编译，内部自动用正确的调用号。程序员写`write(fd, buf, len)`，根本不关心底层是1还是64。

| 问题          | 答案                      |
|-------------|-------------------------|
| 系统调用号是谁决定的？ | OS（Linux 内核）            |
| 为什么各架构不同？   | 历史原因，各架构独立移植时没统一        |
| 能统一吗？       | 理论上能，但没动力（libc 已经屏蔽了差异） |
| 对程序员有影响吗？   | 没有，libc 处理了             |

---

## 5. OS支持多种ABI

**OS可以同时支持多种ABI**，这是很常见的。

### 5.1 Linux支持多种架构的ABI

```
Linux 内核
    │
    ├── x86_64  → System V AMD64 ABI
    ├── ARM64   → AAPCS64
    ├── RISC-V  → RISC-V psABI
    ├── x86_32  → System V i386 ABI
    └── ARM32   → AAPCS
```

同一个Linux内核源码，编译到不同架构时，会使用对应的ABI。

### 5.2 同一台机器上支持多种ABI

**macOS (Apple Silicon)：**

```
macOS on M1/M2
    │
    ├── ARM64原生程序  → AAPCS64 ABI
    │
    └── x86_64程序    → System V AMD64 ABI
        (通过Rosetta2翻译执行)
```

**Linux x86_64运行32位程序：**

```
Linux x86_64
    │
    ├── 64位程序  → System V AMD64 ABI
    │                参数: rdi, rsi, rdx...
    │
    └── 32位程序  → System V i386 ABI
        (兼容层)     参数: 通过栈传递
```

**Windows：**

```
Windows x64
    │
    ├── 64位程序  → Microsoft x64 ABI
    │                参数: rcx, rdx, r8, r9
    │
    └── 32位程序  → Microsoft cdecl/stdcall ABI
        (WoW64)      参数: 通过栈传递
```

### 5.3 内核如何处理多种ABI

内核本身只关心**系统调用接口**，不管用户态程序内部的函数调用 ABI：

```
用户态程序A (x86_64, System V ABI)
         │
         │ syscall (rax=1, rdi=fd, rsi=buf, rdx=count)
         ↓
┌─────────────────────────────────────┐
│            Linux内核                │
│                                     │
│  系统调用入口点检查:                   │
│  - 当前是x86_64 → 用x86_64的调用号     │
│  - 当前是ARM64  → 用ARM64的调用号      │
└─────────────────────────────────────┘
         ↑
         │ svc #0 (x8=64, x0=fd, x1=buf, x2=count)
         │
用户态程序B (ARM64, AAPCS64 ABI)
```

### 5.4 多 ABI 支持方式

| 场景              | OS 支持方式                      |
|-----------------|------------------------------|
| 不同架构的机器         | 内核为每种架构编译不同版本                |
| 同一机器运行不同架构程序    | 翻译层 (Rosetta 2) 或兼容层 (WoW64) |
| 同一架构的 32/64 位程序 | 兼容模式 (x86_64 兼容 x86)         |

---

## 6. 总结

### 6.1 两种跨平台场景对比

```
相同CPU + 不同OS：
  源代码层     →  相同 ✅
  ABI        →  可能不同 (Linux vs Windows) ⚠️
  系统调用    →  完全不同 ❌
  可执行格式   →  不同 (ELF vs PE vs Mach-O) ❌
  机器码      →  核心计算可能相同，系统交互不同 ⚠️

相同OS + 不同CPU：
  源代码层     →  相同 ✅
  ABI        →  不同 (System V vs AAPCS64) ❌
  指令集      →  完全不同 ❌
  机器码      →  完全不同 ❌
  系统调用号   →  不同 ❌
  可执行格式   →  类型相同，内容不同 ⚠️
```

### 6.2 关键结论

1. **ABI是跟CPU架构绑定的**，不是跟OS绑定的
2. **OS是ABI的采纳者**，不是制定者
3. **OS可以支持多种ABI**（不同架构、32/64位兼容、翻译层）
4. **内核只关心系统调用接口**，不管用户态程序内部的函数调用ABI

### 6.3 什么是"平台"？

"平台"是一个组合概念，指**CPU架构 + 操作系统**的组合：

```
平台 = CPU架构 + 操作系统

例如：
linux/amd64   = Linux + x86_64
linux/arm64   = Linux + ARM64
darwin/amd64  = macOS + x86_64
darwin/arm64  = macOS + ARM64 (M系列芯片)
windows/amd64 = Windows + x86_64
```

**为什么要分别编译？** 改变任意一个维度，编译结果都不同：

| 场景     | 需要重新编译？ | 原因           |
|--------|---------|--------------|
| 换CPU架构 | ✅ 需要    | 指令集、ABI都不同   |
| 换操作系统  | ✅ 需要    | 系统调用、可执行格式不同 |
| 两个都换   | ✅ 需要    | 全都不同         |

Go 的交叉编译示例：

```bash
GOOS=linux GOARCH=amd64   # linux/amd64 平台
GOOS=linux GOARCH=arm64   # linux/arm64 平台
GOOS=darwin GOARCH=amd64  # darwin/amd64 平台
GOOS=darwin GOARCH=arm64  # darwin/arm64 平台
GOOS=windows GOARCH=amd64 # windows/amd64 平台
```

"不同平台" = **CPU架构或操作系统有任何一个不同**：

```
平台不同的情况：
├── 只换CPU：    linux/amd64 → linux/arm64
├── 只换OS：     linux/amd64 → windows/amd64
└── 两个都换：    linux/amd64 → darwin/arm64
```

### 6.4 跨平台兼容性详解

#### 6.4.1 macOS Intel版App不能直接在M系列芯片上运行

```
macOS Intel App          →    M系列芯片 Mac
(darwin/amd64)                (darwin/arm64)

OS：       macOS     →    macOS        ✅ 相同
CPU架构：   x86_64   →    ARM64        ❌ 不同
```

**不能运行的原因：**

- 指令集完全不同（x86_64 vs ARM64）
- 机器码完全不同
- ABI不同（System V AMD64 vs AAPCS64）
- 寄存器不同（rax/rdi vs x0/x1）

**为什么Rosetta2能解决？**

- Rosetta2是**指令翻译器**，把x86_64指令实时翻译成ARM64指令
- OS相同，所以系统调用名称、文件格式类型（Mach-O）相同，只需要翻译CPU指令

#### 6.4.2 Linux程序不能直接在Windows上运行

```
Linux 程序               →    Windows
(linux/amd64)                (windows/amd64)

OS：       Linux    →    Windows      ❌ 不同
CPU架构：   x86_64  →    x86_64       ✅ 相同
```

**不能运行的原因：**

- 可执行文件格式不同（ELF vs PE）
- 系统调用完全不同（syscall号不同，调用方式不同）
- ABI不同（System V vs Microsoft x64）
- 链接的库不同（libc.so vs msvcrt.dll）

**为什么WSL能解决？**

- WSL提供了一个Linux内核兼容层
- 能识别ELF格式
- 翻译Linux系统调用到Windows内核
- CPU相同，所以用户态的计算代码可以直接执行

**WSL的工作原理：**

```
Linux 程序在 WSL 上运行：

┌─────────────────────────────────────────┐
│  用户态代码（纯计算）                      │
│  lea eax, [rdi + rsi]                   │  → CPU直接执行
│  add rbx, rcx                           │    （无需翻译）
└─────────────────────────────────────────┘
                    │
                    │ 当程序需要OS服务时
                    ↓
┌─────────────────────────────────────────┐
│  系统调用                                │
│  mov rax, 1; syscall  (Linux write)     │  → WSL 拦截并翻译
└─────────────────────────────────────────┘
                    │
                    ↓
┌─────────────────────────────────────────┐
│  Windows 内核                            │
│  执行对应的 Windows 系统调用              │
└─────────────────────────────────────────┘
```

**关键点：机器码本身不分"和操作系统相关"**

| 部分     | 是否需要翻译 | 原因            |
|--------|--------|---------------|
| 普通计算指令 | ❌ 不需要  | CPU相同，直接执行    |
| 系统调用   | ✅ 需要   | OS不同，WSL 负责翻译 |

CPU只认指令，不认操作系统。`lea eax, [rdi+rsi]` 这条指令在Linux和Windows上完全一样，CPU
直接执行。只有当程序需要操作系统服务（文件、网络、进程等）时，才需要WSL翻译系统调用。

#### 6.4.3 同一个C程序需要为不同平台分别编译

```c
// 源代码（所有平台相同）
int add(int a, int b) {
    return a + b;
}
```

**编译到不同平台：**

| 目标平台          | 机器码       | ABI            | 系统调用                  | 可执行格式  |
|---------------|-----------|----------------|-----------------------|--------|
| linux/amd64   | x86_64 指令 | System V       | Linux syscall         | ELF    |
| linux/arm64   | ARM64 指令  | AAPCS64        | Linux syscall (不同调用号) | ELF    |
| darwin/arm64  | ARM64 指令  | AAPCS64 (略有不同) | macOS syscall         | Mach-O |
| windows/amd64 | x86_64 指令 | Microsoft x64  | Windows API           | PE     |

**即使是最简单的函数，编译结果也不同：**

```asm
; linux/amd64 - 参数在 rdi, rsi
lea eax, [rdi + rsi]
ret

; windows/amd64 - 参数在 rcx, rdx
lea eax, [rcx + rdx]
ret

; linux/arm64 - 参数在 x0, x1
add w0, w0, w1
ret
```

#### 6.4.4 64位系统可以运行32位程序

```
32位程序                  →    64位系统
(linux/386)                   (linux/amd64)

OS：       Linux    →    Linux        ✅ 相同
CPU架构：   x86     →    x86_64       ⚠️ 兼容（x86_64 包含 x86）
```

**能运行的原因：**

- x86_64 CPU**向后兼容**x86指令集
- CPU可以在64位模式和32位兼容模式之间切换
- 内核提供32位系统调用兼容层
- ELF格式相同（只是 32 位 vs 64 位变体）

**兼容层做了什么？**

```
32位程序调用 write()
    ↓
libc32: int 0x80 (旧式 32 位系统调用)
    ↓
内核检测到32位调用，切换到兼容处理
    ↓
执行相同的内核功能
```

#### 6.4.5 总结对比

| 场景                  | OS   | CPU   | 能运行吗？  | 解决方案            |
|---------------------|------|-------|--------|-----------------|
| Intel App → M芯片 Mac | ✅ 相同 | ❌ 不同  | ❌      | Rosetta2 (指令翻译) |
| Linux → Windows     | ❌ 不同 | ✅ 相同  | ❌      | WSL (系统调用翻译)    |
| 源码 → 各平台            | -    | -     | 需要分别编译 | 交叉编译            |
| 32位 → 64位系统         | ✅ 相同 | ⚠️ 兼容 | ✅      | CPU 兼容模式        |

### 6.5 交叉编译（Cross Compilation）

#### 6.5.1 什么是交叉编译？

```
本地编译（Native Compilation）：
  编译平台 = 运行平台
  在 linux/amd64 上编译 → 生成 linux/amd64 程序

交叉编译（Cross Compilation）：
  编译平台 ≠ 运行平台
  在 darwin/arm64 (Mac M芯片) 上编译 → 生成 linux/amd64 程序
```

#### 6.5.2 为什么需要交叉编译？

| 场景    | 说明                                 |
|-------|------------------------------------|
| 嵌入式开发 | 在 PC 上编译 ARM 单片机程序（单片机性能太弱，无法自己编译） |
| 服务器部署 | 在 Mac 上开发，编译出 Linux 服务器程序          |
| 多平台发布 | 一台机器生成所有平台的安装包                     |
| CI/CD | 在一个构建服务器上产出所有平台的二进制                |

#### 6.5.3 Go 的交叉编译

Go 原生支持交叉编译，非常简单：

```bash
# 在任何平台上都可以编译出其他平台的程序
GOOS=linux GOARCH=amd64 go build    # 编译出 Linux x64 程序
GOOS=linux GOARCH=arm64 go build    # 编译出 Linux ARM64 程序
GOOS=windows GOARCH=amd64 go build  # 编译出 Windows x64 程序
GOOS=darwin GOARCH=arm64 go build   # 编译出 macOS ARM64 程序
```

**Go 为什么能这么简单？**

- Go 编译器本身包含所有目标平台的代码生成器
- Go 运行时是纯 Go 实现，不依赖系统 C 库
- 静态链接，不依赖目标系统的动态库

#### 6.5.4 C/C++ 的交叉编译（复杂得多）

```bash
# 需要安装目标平台的工具链
apt install gcc-aarch64-linux-gnu    # 安装 ARM64 交叉编译器

# 使用交叉编译器
aarch64-linux-gnu-gcc -o hello hello.c       # 编译出 ARM64 Linux 程序
x86_64-w64-mingw32-gcc -o hello.exe hello.c  # 编译出 Windows 程序
```

**C/C++ 为什么复杂？**

- 需要安装目标平台的编译器（工具链）
- 需要目标平台的头文件和库
- 可能还需要目标平台的系统调用定义

#### 6.5.5 交叉编译 vs 运行时方案

| 方案         | 时机          | 性能     | 用途    |
|------------|-------------|--------|-------|
| 交叉编译       | 编译时生成目标平台代码 | ✅ 原生性能 | 发布软件  |
| Rosetta 2  | 运行时翻译指令     | ⚠️ 有损耗 | 兼容旧软件 |
| WSL        | 运行时翻译系统调用   | ⚠️ 有损耗 | 开发调试  |
| 模拟器 (QEMU) | 运行时模拟整个 CPU | ❌ 很慢   | 测试/调试 |

**最佳实践：** 如果目标平台明确，优先使用交叉编译，获得原生性能。运行时翻译/模拟主要用于兼容性场景。

### 6.6 条件编译（Conditional Compilation）

#### 6.6.1 什么时候需要条件编译？

**不需要条件编译的情况：纯计算代码**

```go
// 这种代码在所有平台上都一样，不需要条件编译
func Add(a, b int) int {
    return a + b
}

func VerifySignature(hash, sig, pubkey []byte) bool {
    // 纯数学计算，跨平台一致
}
```

**需要条件编译的情况：**

| 情况           | 说明                     |
|--------------|------------------------|
| 系统调用/OS 特定功能 | 文件路径、进程管理、网络等          |
| 平台特定的优化      | 使用 CPU 专用指令（SIMD、NEON） |
| CGO 依赖       | 调用 C 库，不同平台库不同         |
| 硬件相关         | 访问特定硬件、驱动              |

#### 6.6.2 Go 的条件编译语法

**方式一：文件名后缀**

```
mycode_linux.go      # 只在 Linux 编译
mycode_windows.go    # 只在 Windows 编译
mycode_darwin.go     # 只在 macOS 编译
mycode_amd64.go      # 只在 x86_64 编译
mycode_arm64.go      # 只在 ARM64 编译
mycode_linux_amd64.go  # 只在 Linux x86_64 编译
```

**方式二：构建标签（Build Tags）**

```go
// file_linux.go
//go:build linux

package mypackage

func getHomeDir() string {
    return os.Getenv("HOME")
}
```

```go
// file_windows.go
//go:build windows

package mypackage

func getHomeDir() string {
    return os.Getenv("USERPROFILE")
}
```

#### 6.6.3 条件编译示例

**示例1：OS 特定功能**

```go
// path_unix.go
//go:build linux || darwin

func pathSeparator() string {
    return "/"
}

// path_windows.go
//go:build windows

func pathSeparator() string {
    return "\\"
}
```

**示例2：平台特定优化**

```go
// crypto_amd64.go
//go:build amd64

func hashFast(data []byte) []byte {
    // 使用 x86_64 专用的 SIMD 指令
}

// crypto_arm64.go
//go:build arm64

func hashFast(data []byte) []byte {
    // 使用 ARM64 专用的 NEON 指令
}

// crypto_generic.go
//go:build !amd64 && !arm64

func hashFast(data []byte) []byte {
    // 通用实现，性能较低但兼容所有平台
}
```

**示例3：CGO 依赖**

```go
// with_cgo.go
//go:build cgo

/*
#include <openssl/sha.h>
*/
import "C"

func hash(data []byte) []byte {
    // 调用 OpenSSL
}

// no_cgo.go
//go:build !cgo

func hash(data []byte) []byte {
    // 纯 Go 实现
}
```

#### 6.6.4 标准库如何处理平台差异

当你使用 Go 标准库时，平台差异已经在内部处理好了：

```
你的代码
    ↓
crypto/ecdsa (纯 Go 接口，跨平台一致)
    ↓
crypto/elliptic (内部有平台优化)
    ↓
    ├── p256_asm_amd64.s  (x86_64 汇编优化)
    ├── p256_asm_arm64.s  (ARM64 汇编优化)
    └── p256_generic.go   (通用 Go 实现)
```

**你不需要关心这些细节**，直接调用标准库即可。

#### 6.6.5 本项目需要条件编译吗？

对于 `secp256R1-demo` 这类项目：

| 代码类型         | 需要条件编译？ | 原因                 |
|--------------|---------|--------------------|
| P-256 椭圆曲线计算 | ❌ 不需要   | 纯数学，标准库已处理         |
| 签名验证         | ❌ 不需要   | 纯数学                |
| ABI 编码       | ❌ 不需要   | 纯字节操作              |
| 文件读写         | ❌ 不需要   | 标准库 `os` 包已处理      |
| 网络请求         | ❌ 不需要   | 标准库 `net/http` 已处理 |

**结论：** 大多数 Go 项目不需要手写条件编译代码，因为标准库已经封装好了平台差异。只有在以下情况才需要：

- 调用平台特定的系统 API
- 需要极致性能优化（手写汇编）
- 使用 CGO 调用 C 库

---

## 7. 汇编基础：寄存器与内存访问

### 7.1 函数参数传递：值还是地址？

在 System V AMD64 ABI 下，寄存器里存放的是**参数的值**，不是地址：

```c
int add(int a, int b) {
    return a + b;
}

// 调用: add(3, 5)
```

```asm
; rdi = 3    ← 第一个参数的值
; rsi = 5    ← 第二个参数的值

lea eax, [rdi + rsi]   ; eax = 3 + 5 = 8
ret                     ; 返回值在 eax 中
```

### 7.2 什么时候寄存器存地址？

只有当参数本身是指针类型时，寄存器才存放地址：

```c
// 情况1: 值传递
int add(int a, int b);       // rdi=值, rsi=值

// 情况2: 指针传递
int add(int *a, int *b);     // rdi=地址, rsi=地址
                             // 需要 mov 取值: mov eax, [rdi]

// 情况3: 结构体（小于16字节）
struct Point { int x, y; };
void foo(struct Point p);    // rdi=结构体内容直接打包

// 情况4: 大结构体（超过16字节）
struct Big { ... };          // 通过栈传递，或传隐藏指针
```

### 7.3 对比示例

```asm
; int add(int a, int b) - 值在寄存器中
add:
    lea eax, [rdi + rsi]    ; 直接用 rdi、rsi 的值计算
    ret

; int add(int *a, int *b) - 地址在寄存器中
add_ptr:
    mov eax, [rdi]          ; 从 rdi 指向的内存取值
    add eax, [rsi]          ; 从 rsi 指向的内存取值
    ret
```

**简单记忆：C 语言是值传递，寄存器里存的就是值本身。**

### 7.4 汇编中的`[]`语法

`[]`在汇编中表示**内存访问**（解引用），类似C语言的`*`操作符：

| 汇编      | C 语言   | 含义         |
|---------|--------|------------|
| `rdi`   | `ptr`  | 寄存器/变量的值本身 |
| `[rdi]` | `*ptr` | 访问该地址指向的内存 |

```asm
mov rax, rdi          ; rax = rdi 的值（复制寄存器）
mov rax, [rdi]        ; rax = 内存地址 rdi 处的值（解引用）
```

### 7.5 `lea`指令的特殊性

`lea`（Load Effective Address）比较特殊，它**只计算地址，不访问内存**：

```asm
lea eax, [rdi + rsi]   ; eax = rdi + rsi（纯数学运算）
                       ; [] 在这里只是语法，不会访问内存

mov eax, [rdi + rsi]   ; eax = 内存地址(rdi+rsi)处的值
                       ; 这个才会真正访问内存
```

### 7.6 为什么`lea`用`[]`语法？

这是 x86 的**历史设计问题**。

`lea` 的本意是"计算内存地址"，设计给指针运算用的：

```asm
; 原本用途：计算数组元素地址
; int arr[]; 取 arr[i] 的地址
lea rax, [rbx + rcx*4]    ; rax = &arr[i]，不访问内存
mov eax, [rbx + rcx*4]    ; eax = arr[i]，访问内存
```

`[]` 表示"这是一个内存地址表达式"，`lea` 只是**算出这个地址但不访问**。

后来程序员发现 `lea` 可以一条指令完成复杂运算，于是"滥用"它做纯数学运算：

```asm
; 本意：计算地址
lea rax, [rbx + rcx*4 + 8]

; "滥用"：纯数学运算 a + b
lea eax, [rdi + rsi]      ; 没有任何内存概念，纯粹借用语法
```

如果重新设计，可能会更清晰：

```asm
; 假想的更好设计
add3 eax, rdi, rsi        ; eax = rdi + rsi（三操作数加法）
addr rax, [rbx + rcx*4]   ; 计算地址
```

但x86指令集是1978年设计的，历史包袱太重，改不了了。

### 7.7 为什么用`lea`做加法？

对于简单的加法函数：

```c
int add(int a, int b) {
    return a + b;  // 纯数值运算，跟内存地址无关
}
```

编译器生成`lea eax, [rdi + rsi]`，这里**没有"地址"的概念**，只是借用`lea`的计算能力。

**为什么不用`add`指令？**

```asm
; 方法1: 用 add（需要两条指令）
mov eax, edi          ; eax = a
add eax, esi          ; eax = a + b

; 方法2: 用 lea（只需一条指令）
lea eax, [rdi + rsi]  ; eax = a + b
```

`lea` 的优势：

1. **一条指令完成**，更快
2. **不破坏源操作数**，rdi 和 rsi 保持不变
3. **可以同时做更复杂的运算**：`lea eax, [rdi + rsi*4 + 8]`

编译器选择 `lea` 纯粹是因为它**效率更高**，跟"地址"没有任何关系。这是 x86 汇编中常见的优化技巧。

### 7.8 小结

| 概念               | 说明                          |
|------------------|-----------------------------|
| 寄存器存值还是地址        | 取决于 C 语言参数类型：普通类型存值，指针类型存地址 |
| `[]` 语法含义        | 内存访问（解引用），等同于 C 的 `*`       |
| `lea` + `[]`     | 只计算地址表达式的值，不访问内存            |
| 为什么 `lea` 用 `[]` | 历史设计，本意是地址计算，后被"滥用"做纯数学运算   |
| 为什么用 `lea` 做加法   | 一条指令完成运算，效率更高，是常见的编译器优化技巧   |

---

## 参考资料

- [System V AMD64 ABI](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf)
- [ARM64 AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst)
- [Microsoft x64 calling convention](https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention)
- [Linux syscall table](https://syscalls.w3challs.com/)
