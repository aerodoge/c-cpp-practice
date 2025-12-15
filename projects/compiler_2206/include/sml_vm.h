/**
 * @file sml_vm.h
 * @brief SML (Simpletron Machine Language) 虚拟机
 *
 * 实现冯·诺依曼架构的简单虚拟机，用于执行编译器生成的 SML 指令。
 *
 * 架构特点:
 * - 冯诺依曼架构: 指令和数据共享内存
 * - 累加器架构: 单一累加器用于所有运算
 * - 定长指令: 每条指令占一个内存单元 (±XXYY)
 *
 * 执行周期 (Fetch-Decode-Execute):
 * 1. 取指 (Fetch): IR = Memory[PC]
 * 2. 解码 (Decode): opcode = IR/100, operand = IR%100
 * 3. 执行 (Execute): 根据 opcode 执行操作
 * 4. PC++（除非是跳转指令）
 *
 * 支持的指令集:
 * - I/O:   READ(10), WRITE(11), NEWLINE(12), WRITES(13)
 * - 数据:  LOAD(20), STORE(21)
 * - 算术:  ADD(30), SUB(31), DIV(32), MUL(33), MOD(34)
 * - 控制:  JMP(40), JMPNEG(41), JMPZERO(42), HALT(43)
 *
 * @see compiler.h SML 操作码定义
 */

#ifndef SML_VM_H
#define SML_VM_H

#include "compiler.h"

/**
 * @struct SML_VM
 * @brief 虚拟机状态
 *
 * 模拟简单计算机的所有寄存器和内存。
 */
typedef struct {
    int memory[MEMORY_SIZE];   /**< 内存 (指令+数据, 100 单元) */
    int accumulator;           /**< 累加器 (AC) - 运算核心 */
    int instruction_counter;   /**< 程序计数器 (PC) */
    int instruction_register;  /**< 指令寄存器 (IR) */
    int opcode;                /**< 当前操作码 (解码后) */
    int operand;               /**< 当前操作数 (解码后) */
    int running;               /**< 运行状态: 1=运行, 0=停止 */
    int cycle_count;           /**< 执行周期计数 (性能分析用) */
    char error_message[256];   /**< 错误信息 */
} SML_VM;

/* ==================== 公共 API ==================== */

/**
 * @brief 初始化虚拟机
 * @param vm 虚拟机指针
 *
 * 清零所有寄存器和内存。
 */
void sml_vm_init(SML_VM *vm);

/**
 * @brief 从内存数组加载程序
 * @param vm 虚拟机指针
 * @param memory 程序内存数组 (MEMORY_SIZE个整数)
 */
void sml_vm_load(SML_VM *vm, const int *memory);

/**
 * @brief 从 .sml 文件加载程序
 * @param vm 虚拟机指针
 * @param filename SML文件路径
 * @return 成功返回1，失败返回0
 *
 * 文件格式: 每行一个整数 (±XXYY 格式的指令或数据)
 */
int sml_vm_load_file(SML_VM *vm, const char *filename);

/**
 * @brief 执行程序直到HALT或出错
 * @param vm 虚拟机指针
 * @return 成功返回1，错误返回0
 */
int sml_vm_run(SML_VM *vm);

/**
 * @brief 单步执行一条指令
 * @param vm 虚拟机指针
 * @return 继续执行返回 1，停机或错误返回 0
 *
 * 用于调试或教学演示。
 */
int sml_vm_step(SML_VM *vm);

/**
 * @brief 打印寄存器状态
 * @param vm 虚拟机指针
 */
void sml_vm_dump_registers(SML_VM *vm);

/**
 * @brief 打印内存内容
 * @param vm 虚拟机指针
 */
void sml_vm_dump_memory(SML_VM *vm);

/**
 * @brief 获取错误信息
 * @param vm 虚拟机指针
 * @return 错误信息字符串
 */
const char* sml_vm_get_error(SML_VM *vm);

#endif /* SML_VM_H */
