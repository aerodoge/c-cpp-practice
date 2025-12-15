/**
 * @file sml_vm.c
 * @brief SML 虚拟机实现
 *
 * 实现经典的 取指-解码-执行 (Fetch-Decode-Execute) 循环。
 *
 * 指令格式: XXYY
 *   - XX: 操作码 (10-43)
 *   - YY: 操作数/内存地址 (00-99)
 *
 * 支持的指令:
 *   I/O:     READ(10), WRITE(11), NEWLINE(12), WRITES(13)
 *   存储:    LOAD(20), STORE(21)
 *   算术:    ADD(30), SUB(31), DIV(32), MUL(33), MOD(34)
 *   控制:    JMP(40), JMPNEG(41), JMPZERO(42), HALT(43)
 */

#include "sml_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 最大执行周期数 (防止无限循环) */
#define MAX_CYCLES 100000

/**
 * 初始化虚拟机
 */
void sml_vm_init(SML_VM *vm) {
    memset(vm, 0, sizeof(SML_VM));
    vm->running = 0;
}

/**
 * 加载程序到内存
 */
void sml_vm_load(SML_VM *vm, const int *memory) {
    memcpy(vm->memory, memory, sizeof(vm->memory));
    vm->instruction_counter = 0;
    vm->accumulator = 0;
    vm->running = 1;
    vm->cycle_count = 0;
    vm->error_message[0] = '\0';
}

/**
 * 从文件加载SML程序
 */
int sml_vm_load_file(SML_VM *vm, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Cannot open file: %s", filename);
        return 0;
    }

    sml_vm_init(vm);

    int address = 0;
    int instruction;
    while (fscanf(file, "%d", &instruction) == 1 && address < MEMORY_SIZE) {
        vm->memory[address++] = instruction;
    }

    fclose(file);
    vm->running = 1;
    return 1;
}

/**
 * 单步执行一条指令
 */
int sml_vm_step(SML_VM *vm) {
    if (!vm->running) {
        return 0;
    }

    // 检查指令计数器范围
    if (vm->instruction_counter < 0 || vm->instruction_counter >= MEMORY_SIZE) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Invalid instruction counter: %d", vm->instruction_counter);
        vm->running = 0;
        return 0;
    }

    // 取指 (Fetch)
    vm->instruction_register = vm->memory[vm->instruction_counter];

    // 解码 (Decode)
    vm->opcode = vm->instruction_register / 100;
    vm->operand = vm->instruction_register % 100;

    // 检查操作数范围
    if (vm->operand < 0 || vm->operand >= MEMORY_SIZE) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Invalid operand: %d at PC=%d", vm->operand, vm->instruction_counter);
        vm->running = 0;
        return 0;
    }

    // 执行 (Execute)
    int next_pc = vm->instruction_counter + 1;

    switch (vm->opcode) {
        case SML_READ:  // 从键盘读取
            printf("? ");
            fflush(stdout);
            if (scanf("%d", &vm->memory[vm->operand]) != 1) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Invalid input");
                vm->running = 0;
                return 0;
            }
            break;

        case SML_WRITE:  // 输出整数
            printf("%d", vm->memory[vm->operand]);
            break;

        case SML_NEWLINE:  // 输出换行
            printf("\n");
            break;

        case SML_WRITES:  // 输出字符串
            {
                // 字符串格式: 第一个单元存长度，后续单元存字符
                int str_loc = vm->operand;
                int len = vm->memory[str_loc];
                for (int i = 0; i < len; i++) {
                    int ch = vm->memory[str_loc - 1 - i];
                    if (ch >= 0 && ch < 256) {
                        putchar(ch);
                    }
                }
            }
            break;

        case SML_LOAD:  // 加载到累加器
            vm->accumulator = vm->memory[vm->operand];
            break;

        case SML_STORE:  // 存储累加器
            vm->memory[vm->operand] = vm->accumulator;
            break;

        case SML_ADD:  // 加法
            vm->accumulator += vm->memory[vm->operand];
            break;

        case SML_SUBTRACT:  // 减法
            vm->accumulator -= vm->memory[vm->operand];
            break;

        case SML_DIVIDE:  // 除法
            if (vm->memory[vm->operand] == 0) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Division by zero at PC=%d", vm->instruction_counter);
                vm->running = 0;
                return 0;
            }
            vm->accumulator /= vm->memory[vm->operand];
            break;

        case SML_MULTIPLY:  // 乘法
            vm->accumulator *= vm->memory[vm->operand];
            break;

        case SML_MOD:  // 取模
            if (vm->memory[vm->operand] == 0) {
                snprintf(vm->error_message, sizeof(vm->error_message),
                         "Modulo by zero at PC=%d", vm->instruction_counter);
                vm->running = 0;
                return 0;
            }
            vm->accumulator %= vm->memory[vm->operand];
            break;

        case SML_BRANCH:  // 无条件跳转
            next_pc = vm->operand;
            break;

        case SML_BRANCHNEG:  // 负数跳转
            if (vm->accumulator < 0) {
                next_pc = vm->operand;
            }
            break;

        case SML_BRANCHZERO:  // 零跳转
            if (vm->accumulator == 0) {
                next_pc = vm->operand;
            }
            break;

        case SML_HALT:  // 停机
            vm->running = 0;
            return 0;

        default:
            snprintf(vm->error_message, sizeof(vm->error_message),
                     "Unknown opcode %d at PC=%d", vm->opcode, vm->instruction_counter);
            vm->running = 0;
            return 0;
    }

    // 更新指令计数器
    vm->instruction_counter = next_pc;
    vm->cycle_count++;

    // 检查是否超过最大周期数
    if (vm->cycle_count >= MAX_CYCLES) {
        snprintf(vm->error_message, sizeof(vm->error_message),
                 "Exceeded maximum cycles (%d), possible infinite loop", MAX_CYCLES);
        vm->running = 0;
        return 0;
    }

    return 1;
}

/**
 * 执行程序直到停机或错误
 */
int sml_vm_run(SML_VM *vm) {
    while (vm->running) {
        if (!sml_vm_step(vm)) {
            break;
        }
    }

    // 如果有错误信息，返回失败
    if (vm->error_message[0] != '\0') {
        return 0;
    }

    return 1;
}

/**
 * 打印寄存器状态
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
 * 打印内存内容
 */
void sml_vm_dump_memory(SML_VM *vm) {
    printf("=== Memory ===\n");
    printf("       0      1      2      3      4      5      6      7      8      9\n");
    for (int i = 0; i < MEMORY_SIZE; i += 10) {
        printf("%2d ", i);
        for (int j = 0; j < 10; j++) {
            printf("%+05d  ", vm->memory[i + j]);
        }
        printf("\n");
    }
}

/**
 * 获取错误信息
 */
const char* sml_vm_get_error(SML_VM *vm) {
    return vm->error_message;
}
