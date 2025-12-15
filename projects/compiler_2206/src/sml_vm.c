/**
 * @file sml_vm.c
 * @brief SML (Simpletron Machine Language) 虚拟机实现
 *
 * ============================================================================
 *                              SML 虚拟机架构
 * ============================================================================
 *
 * 本虚拟机实现了经典的冯·诺依曼架构和累加器机器模型。
 *
 * 冯·诺依曼架构特点:
 *   - 指令和数据共享同一内存空间 (统一存储)
 *   - 程序存储在内存中，可以被修改 (存储程序概念)
 *   - 顺序执行指令，通过跳转改变执行流
 *
 * 累加器架构特点:
 *   - 只有一个主寄存器 (累加器 AC)
 *   - 所有运算都通过累加器进行
 *   - 指令格式: 操作码 + 内存地址
 *
 * ============================================================================
 *                              CPU 执行周期
 * ============================================================================
 *
 * 实现经典的 取指-解码-执行 (Fetch-Decode-Execute) 循环:
 *
 * ┌───────────────────────────────────────────────────────────────────────────┐
 * │                                                                           │
 * │   1. 取指 (Fetch)                                                         │
 * │      ┌─────────────────────────────────────────┐                         │
 * │      │ IR = Memory[PC]                         │                         │
 * │      │ 从 PC 指向的内存地址读取指令到指令寄存器    │                         │
 * │      └─────────────────────────────────────────┘                         │
 * │                         ↓                                                 │
 * │   2. 解码 (Decode)                                                        │
 * │      ┌─────────────────────────────────────────┐                         │
 * │      │ opcode  = IR / 100  (操作码)             │                         │
 * │      │ operand = IR % 100  (操作数/地址)        │                         │
 * │      │ 解析指令：XXYY → XX=操作码, YY=地址       │                         │
 * │      └─────────────────────────────────────────┘                         │
 * │                         ↓                                                 │
 * │   3. 执行 (Execute)                                                       │
 * │      ┌─────────────────────────────────────────┐                         │
 * │      │ 根据 opcode 执行对应操作:                 │                         │
 * │      │ - I/O: 读写数据                          │                         │
 * │      │ - 数据传输: 加载/存储                     │                         │
 * │      │ - 算术运算: 加减乘除模                    │                         │
 * │      │ - 控制流: 跳转/条件跳转/停机              │                         │
 * │      └─────────────────────────────────────────┘                         │
 * │                         ↓                                                 │
 * │   4. 更新 PC                                                              │
 * │      ┌─────────────────────────────────────────┐                         │
 * │      │ 顺序执行: PC++                           │                         │
 * │      │ 跳转指令: PC = operand                   │                         │
 * │      │ 停机指令: running = 0                    │                         │
 * │      └─────────────────────────────────────────┘                         │
 * │                         ↓                                                 │
 * │      ─────────────── 返回步骤 1 ───────────────                           │
 * │                                                                           │
 * └───────────────────────────────────────────────────────────────────────────┘
 *
 * ============================================================================
 *                              SML 指令集
 * ============================================================================
 *
 * 指令格式: ±XXYY (5位带符号整数)
 *   - XX: 操作码 (10-43)
 *   - YY: 操作数 (00-99，通常是内存地址)
 *
 * I/O 指令:
 *   10 READ     读取整数到 Memory[YY]
 *   11 WRITE    输出 Memory[YY] 的值
 *   12 NEWLINE  输出换行符
 *   13 WRITES   输出字符串 (从 Memory[YY] 开始)
 *
 * 数据传输指令:
 *   20 LOAD     AC = Memory[YY]      (加载内存到累加器)
 *   21 STORE    Memory[YY] = AC      (存储累加器到内存)
 *
 * 算术指令:
 *   30 ADD      AC = AC + Memory[YY]
 *   31 SUB      AC = AC - Memory[YY]
 *   32 DIV      AC = AC / Memory[YY] (整数除法，除零错误)
 *   33 MUL      AC = AC * Memory[YY]
 *   34 MOD      AC = AC % Memory[YY] (取模，除零错误)
 *
 * 控制流指令:
 *   40 JMP      PC = YY              (无条件跳转)
 *   41 JMPNEG   if (AC < 0) PC = YY  (负数跳转)
 *   42 JMPZERO  if (AC == 0) PC = YY (零跳转)
 *   43 HALT     停止执行
 *
 * ============================================================================
 *                              内存模型
 * ============================================================================
 *
 *   地址     用途
 *   ─────────────────────
 *   00-??    程序代码
 *   ??-99    数据区
 *   ─────────────────────
 *
 * 所有内存单元都可以存储指令或数据，这是冯诺依曼架构的核心特点。
 */

#include "sml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** 最大执行周期数 (防止无限循环) */
#define MAX_CYCLES 100000

/* ============================================================================
 *                              虚拟机初始化与加载
 * ============================================================================ */

/**
 * @brief 初始化虚拟机
 *
 * 将所有寄存器和内存清零，为执行新程序做准备。
 *
 * @param vm 虚拟机指针
 *
 * 初始状态:
 *   - 所有内存单元 = 0
 *   - AC (累加器) = 0
 *   - PC (程序计数器) = 0
 *   - IR (指令寄存器) = 0
 *   - running = 0 (未运行)
 */
void sml_vm_init(SML_VM *vm) {
    /* memset 将整个结构体清零
     * 包括 memory[100], accumulator, instruction_counter 等 */
    memset(vm, 0, sizeof(SML_VM));
    vm->running = 0;
}

/**
 * @brief 从数组加载程序到虚拟机内存
 *
 * 将编译器生成的 SML 代码复制到虚拟机内存，准备执行。
 *
 * @param vm     虚拟机指针
 * @param memory 源内存数组 (MEMORY_SIZE=100 个整数)
 *
 * 使用场景:
 *   - 从编译器直接加载: sml_vm_load(&vm, compiler_get_memory(&comp));
 */
void sml_vm_load(SML_VM *vm, const int *memory) {
    /* 复制整个内存映像 */
    memcpy(vm->memory, memory, sizeof(vm->memory));

    /* 重置执行状态 */
    vm->instruction_counter = 0;   /* PC 从 0 开始 */
    vm->accumulator = 0;           /* AC 清零 */
    vm->running = 1;               /* 设置为运行状态 */
    vm->cycle_count = 0;           /* 清零周期计数 */
    vm->error_message[0] = '\0';   /* 清空错误信息 */
}

/**
 * @brief 从文件加载 SML 程序
 *
 * 读取 .sml 文件，每行一个整数 (±XXYY 格式)。
 *
 * @param vm       虚拟机指针
 * @param filename SML 文件路径
 * @return 成功返回 1，失败返回 0
 *
 * 文件格式示例:
 *   +1099     (READ 99: 读取输入到地址 99)
 *   +2099     (LOAD 99: 加载地址 99 到累加器)
 *   +1199     (WRITE 99: 输出地址 99 的值)
 *   +4300     (HALT: 停机)
 */
int sml_vm_load_file(SML_VM *vm, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Cannot open file: %s", filename);
        return 0;
    }

    sml_vm_init(vm);  /* 先初始化 */

    /* 逐行读取指令，存入内存 */
    int address = 0;
    int instruction;
    while (fscanf(file, "%d", &instruction) == 1 && address < MEMORY_SIZE) {
        vm->memory[address++] = instruction;
    }

    fclose(file);
    vm->running = 1;
    return 1;
}

/* ============================================================================
 *                              执行引擎
 * ============================================================================ */

/**
 * @brief 单步执行一条指令 (Fetch-Decode-Execute)
 *
 * 这是虚拟机的核心函数，实现了完整的 CPU 执行周期。
 *
 * @param vm 虚拟机指针
 * @return 继续执行返回 1，停机或错误返回 0
 *
 * 执行流程:
 *   1. 检查 PC 是否有效
 *   2. 取指: IR = Memory[PC]
 *   3. 解码: opcode = IR/100, operand = IR%100
 *   4. 执行: 根据 opcode 执行操作
 *   5. 更新 PC (顺序执行或跳转)
 *   6. 检查是否超过最大周期数
 */
int sml_vm_step(SML_VM *vm) {
    /* 检查虚拟机是否在运行状态 */
    if (!vm->running) {
        return 0;
    }

    /* ========== 步骤 1: 验证 PC 范围 ========== */
    if (vm->instruction_counter < 0 || vm->instruction_counter >= MEMORY_SIZE) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Invalid instruction counter: %d", vm->instruction_counter);
        vm->running = 0;
        return 0;
    }

    /* ========== 步骤 2: 取指 (Fetch) ========== */
    /* 从 PC 指向的内存地址读取指令到指令寄存器 */
    vm->instruction_register = vm->memory[vm->instruction_counter];

    /* ========== 步骤 3: 解码 (Decode) ========== */
    /* SML 指令格式: XXYY
     * - XX (高两位): 操作码
     * - YY (低两位): 操作数/内存地址 */
    vm->opcode = vm->instruction_register / 100;   /* 整数除法取高位 */
    vm->operand = vm->instruction_register % 100;  /* 取模得低位 */

    /* 验证操作数范围 */
    if (vm->operand < 0 || vm->operand >= MEMORY_SIZE) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Invalid operand: %d at PC=%d", vm->operand, vm->instruction_counter);
        vm->running = 0;
        return 0;
    }

    /* ========== 步骤 4: 执行 (Execute) ========== */
    /* 默认下一条指令是顺序执行 (PC+1) */
    int next_pc = vm->instruction_counter + 1;

    switch (vm->opcode) {
        /* ========== I/O 指令 ========== */

        case SML_READ:      /* 10: 从键盘读取整数 */
            printf("? ");   /* 显示输入提示符 */
            fflush(stdout); /* 确保提示符立即显示 */
            if (scanf("%d", &vm->memory[vm->operand]) != 1) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Invalid input");
                vm->running = 0;
                return 0;
            }
            break;

        case SML_WRITE:     /* 11: 输出整数 */
            printf("%d", vm->memory[vm->operand]);
            break;

        case SML_NEWLINE:   /* 12: 输出换行符 */
            printf("\n");
            break;

        case SML_WRITES:    /* 13: 输出字符串 */
            {
                /* 字符串存储格式: [长度][字符1][字符2]...
                 * 长度在基地址，字符在递减的地址中 */
                int str_loc = vm->operand;
                int len = vm->memory[str_loc];  /* 读取字符串长度 */
                for (int i = 0; i < len; i++) {
                    int ch = vm->memory[str_loc - 1 - i];  /* 字符在低地址 */
                    if (ch >= 0 && ch < 256) {
                        putchar(ch);  /* 输出 ASCII 字符 */
                    }
                }
            }
            break;

        /* ========== 数据传输指令 ========== */

        case SML_LOAD:      /* 20: 加载内存到累加器 */
            /* AC = Memory[operand] */
            vm->accumulator = vm->memory[vm->operand];
            break;

        case SML_STORE:     /* 21: 存储累加器到内存 */
            /* Memory[operand] = AC */
            vm->memory[vm->operand] = vm->accumulator;
            break;

        /* ========== 算术指令 ========== */

        case SML_ADD:       /* 30: 加法 */
            /* AC = AC + Memory[operand] */
            vm->accumulator += vm->memory[vm->operand];
            break;

        case SML_SUBTRACT:  /* 31: 减法 */
            /* AC = AC - Memory[operand] */
            vm->accumulator -= vm->memory[vm->operand];
            break;

        case SML_DIVIDE:    /* 32: 除法 */
            /* AC = AC / Memory[operand] */
            if (vm->memory[vm->operand] == 0) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Division by zero at PC=%d", vm->instruction_counter);
                vm->running = 0;
                return 0;
            }
            vm->accumulator /= vm->memory[vm->operand];
            break;

        case SML_MULTIPLY:  /* 33: 乘法 */
            /* AC = AC * Memory[operand] */
            vm->accumulator *= vm->memory[vm->operand];
            break;

        case SML_MOD:       /* 34: 取模 */
            /* AC = AC % Memory[operand] */
            if (vm->memory[vm->operand] == 0) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Modulo by zero at PC=%d", vm->instruction_counter);
                vm->running = 0;
                return 0;
            }
            vm->accumulator %= vm->memory[vm->operand];
            break;

        /* ========== 控制流指令 ========== */

        case SML_BRANCH:     /* 40: 无条件跳转 */
            /* PC = operand (跳转到指定地址) */
            next_pc = vm->operand;
            break;

        case SML_BRANCHNEG:  /* 41: 负数条件跳转 */
            /* if (AC < 0) PC = operand */
            if (vm->accumulator < 0) {
                next_pc = vm->operand;
            }
            break;

        case SML_BRANCHZERO: /* 42: 零条件跳转 */
            /* if (AC == 0) PC = operand */
            if (vm->accumulator == 0) {
                next_pc = vm->operand;
            }
            break;

        case SML_HALT:       /* 43: 停机 */
            vm->running = 0;
            return 0;  /* 正常停机 */

        /* ========== 未知指令 ========== */
        default:
            snprintf(vm->error_message, sizeof(vm->error_message),
                     "Unknown opcode %d at PC=%d", vm->opcode, vm->instruction_counter);
            vm->running = 0;
            return 0;
    }

    /* ========== 步骤 5: 更新 PC ========== */
    vm->instruction_counter = next_pc;
    vm->cycle_count++;

    /* ========== 步骤 6: 检查无限循环保护 ========== */
    if (vm->cycle_count >= MAX_CYCLES) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Exceeded maximum cycles (%d), possible infinite loop", MAX_CYCLES);
        vm->running = 0;
        return 0;
    }

    return 1;  /* 继续执行 */
}

/**
 * @brief 执行程序直到停机或错误
 *
 * 循环调用 sml_vm_step() 直到程序结束。
 *
 * @param vm 虚拟机指针
 * @return 成功返回 1，错误返回 0
 */
int sml_vm_run(SML_VM *vm) {
    /* 主执行循环 */
    while (vm->running) {
        if (!sml_vm_step(vm)) {
            break;  /* 停机或错误 */
        }
    }

    /* 如果有错误信息，表示异常终止 */
    if (vm->error_message[0] != '\0') {
        return 0;
    }

    return 1;  /* 正常结束 */
}

/* ============================================================================
 *                              调试与诊断
 * ============================================================================ */

/**
 * @brief 打印寄存器状态
 *
 * 显示所有 CPU 寄存器的当前值，用于调试。
 *
 * @param vm 虚拟机指针
 *
 * 输出格式:
 *   === Registers ===
 *     Accumulator:          +0000
 *     Instruction Counter:  00
 *     Instruction Register: +0000
 *     Opcode:               00
 *     Operand:              00
 *     Cycle Count:          0
 */
void sml_vm_dump_registers(SML_VM *vm) {
    printf("=== Registers ===\n");
    printf("  Accumulator:          %+05d\n", vm->accumulator);
    printf("  Instruction Counter:  %02d\n", vm->instruction_counter);
    printf("  Instruction Register: %+05d\n", vm->instruction_register);
    printf("  Opcode:               %02d\n", vm->opcode);
    printf("  Operand:              %02d\n", vm->operand);
    printf("  Cycle Count:          %d\n", vm->cycle_count);
}

/**
 * @brief 打印内存内容
 *
 * 以 10x10 网格格式显示全部 100 个内存单元。
 *
 * @param vm 虚拟机指针
 *
 * 输出格式:
 *   === Memory ===
 *        0      1      2    ...
 *    0 +0000  +0000  +0000  ...
 *   10 +0000  +0000  +0000  ...
 *   ...
 */
void sml_vm_dump_memory(SML_VM *vm) {
    printf("=== Memory ===\n");
    /* 列标题 */
    printf("       0      1      2      3      4      5      6      7      8      9\n");

    /* 每 10 个单元一行 */
    for (int i = 0; i < MEMORY_SIZE; i += 10) {
        printf("%2d ", i);  /* 行标题 (起始地址) */
        for (int j = 0; j < 10; j++) {
            printf("%+05d  ", vm->memory[i + j]);
        }
        printf("\n");
    }
}

/**
 * @brief 获取错误信息
 *
 * @param vm 虚拟机指针
 * @return 错误信息字符串；如果无错误，返回空字符串
 */
const char* sml_vm_get_error(SML_VM *vm) {
    return vm->error_message;
}
