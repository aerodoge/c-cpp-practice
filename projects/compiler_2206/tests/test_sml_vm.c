/**
 * @file test_sml_vm.c
 * @brief SML 虚拟机单元测试
 *
 * 测试覆盖:
 *   - 虚拟机初始化
 *   - 所有 SML 指令
 *   - 程序加载和执行
 *   - 错误处理 (除零、地址越界等)
 *
 * 运行方法:
 *   cd build && ./test_sml_vm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "sml_vm.h"
#include "compiler.h"

/* ============================================================================
 *                              虚拟机初始化测试
 * ============================================================================ */

/**
 * @brief 测试虚拟机初始化
 */
void test_vm_init(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 检查初始状态 */
    ASSERT_EQ(vm.accumulator, 0);
    ASSERT_EQ(vm.instruction_counter, 0);
    ASSERT_EQ(vm.instruction_register, 0);
    ASSERT_EQ(vm.opcode, 0);
    ASSERT_EQ(vm.operand, 0);
    ASSERT_EQ(vm.running, 0);  /* 初始化后为 0, sml_vm_run 时设为 1 */
    ASSERT_EQ(vm.cycle_count, 0);

    /* 内存应该被清零 */
    for (int i = 0; i < MEMORY_SIZE; i++) {
        ASSERT_EQ(vm.memory[i], 0);
    }
}

/**
 * @brief 测试程序加载
 */
void test_vm_load(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 准备测试程序 */
    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 */
    program[1] = 4300;   /* HALT */
    program[99] = 42;    /* 数据 */

    sml_vm_load(&vm, program);

    ASSERT_EQ(vm.memory[0], 2099);
    ASSERT_EQ(vm.memory[1], 4300);
    ASSERT_EQ(vm.memory[99], 42);
}

/* ============================================================================
 *                              数据传输指令测试
 * ============================================================================ */

/**
 * @brief 测试 LOAD 指令
 */
void test_vm_load_instruction(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 */
    program[1] = 4300;   /* HALT */
    program[99] = 123;   /* 数据 */

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 123);
}

/**
 * @brief 测试 STORE 指令
 */
void test_vm_store_instruction(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=50) */
    program[1] = 2198;   /* STORE 98 */
    program[2] = 4300;   /* HALT */
    program[99] = 50;    /* 源数据 */

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.memory[98], 50);
}

/* ============================================================================
 *                              算术指令测试
 * ============================================================================ */

/**
 * @brief 测试 ADD 指令
 */
void test_vm_add(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=10) */
    program[1] = 3098;   /* ADD 98 (值=20) */
    program[2] = 4300;   /* HALT */
    program[99] = 10;
    program[98] = 20;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 30);
}

/**
 * @brief 测试 SUB 指令
 */
void test_vm_subtract(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=50) */
    program[1] = 3198;   /* SUB 98 (值=20) */
    program[2] = 4300;   /* HALT */
    program[99] = 50;
    program[98] = 20;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 30);
}

/**
 * @brief 测试 MUL 指令
 */
void test_vm_multiply(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=6) */
    program[1] = 3398;   /* MUL 98 (值=7) */
    program[2] = 4300;   /* HALT */
    program[99] = 6;
    program[98] = 7;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 42);
}

/**
 * @brief 测试 DIV 指令
 */
void test_vm_divide(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=100) */
    program[1] = 3298;   /* DIV 98 (值=10) */
    program[2] = 4300;   /* HALT */
    program[99] = 100;
    program[98] = 10;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 10);
}

/**
 * @brief 测试 MOD 指令
 */
void test_vm_mod(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=17) */
    program[1] = 3498;   /* MOD 98 (值=5) */
    program[2] = 4300;   /* HALT */
    program[99] = 17;
    program[98] = 5;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 2);  /* 17 % 5 = 2 */
}

/* ============================================================================
 *                              控制流指令测试
 * ============================================================================ */

/**
 * @brief 测试 BRANCH (无条件跳转) 指令
 */
void test_vm_branch(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 4005;   /* BRANCH 5 */
    program[1] = 2099;   /* LOAD 99 (应该被跳过) */
    program[2] = 2198;   /* STORE 98 (应该被跳过) */
    program[3] = 4300;   /* HALT (应该被跳过) */
    program[4] = 4300;   /* HALT (应该被跳过) */
    program[5] = 2097;   /* LOAD 97 (跳转目标) */
    program[6] = 4300;   /* HALT */
    program[97] = 999;
    program[99] = 1;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    /* 应该加载了地址 97 的值，而不是 99 */
    ASSERT_EQ(vm.accumulator, 999);
}

/**
 * @brief 测试 BRANCHNEG (负数跳转) 指令
 */
void test_vm_branchneg(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 测试累加器为负时跳转 */
    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=-5) */
    program[1] = 4105;   /* BRANCHNEG 5 */
    program[2] = 2098;   /* LOAD 98 (不应执行) */
    program[3] = 4300;   /* HALT */
    program[4] = 4300;   /* HALT */
    program[5] = 2097;   /* LOAD 97 (跳转目标) */
    program[6] = 4300;   /* HALT */
    program[99] = -5;
    program[98] = 111;
    program[97] = 222;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 222);
}

/**
 * @brief 测试 BRANCHNEG 不跳转的情况
 */
void test_vm_branchneg_no_jump(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 测试累加器为正时不跳转 */
    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=5, 正数) */
    program[1] = 4105;   /* BRANCHNEG 5 (不应跳转) */
    program[2] = 2098;   /* LOAD 98 (应该执行) */
    program[3] = 4300;   /* HALT */
    program[99] = 5;
    program[98] = 333;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 333);
}

/**
 * @brief 测试 BRANCHZERO (零跳转) 指令
 */
void test_vm_branchzero(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 测试累加器为零时跳转 */
    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=0) */
    program[1] = 4205;   /* BRANCHZERO 5 */
    program[2] = 2098;   /* LOAD 98 (不应执行) */
    program[3] = 4300;   /* HALT */
    program[4] = 4300;   /* HALT */
    program[5] = 2097;   /* LOAD 97 (跳转目标) */
    program[6] = 4300;   /* HALT */
    program[99] = 0;
    program[98] = 111;
    program[97] = 444;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    ASSERT_EQ(vm.accumulator, 444);
}

/**
 * @brief 测试 HALT 指令
 */
void test_vm_halt(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 */
    program[1] = 4300;   /* HALT */
    program[2] = 2098;   /* LOAD 98 (不应执行) */
    program[99] = 100;
    program[98] = 200;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    /* 应该停在 HALT，累加器保持 LOAD 99 的值 */
    ASSERT_EQ(vm.accumulator, 100);
    ASSERT_EQ(vm.running, 0);
}

/* ============================================================================
 *                              错误处理测试
 * ============================================================================ */

/**
 * @brief 测试除零错误
 */
void test_vm_divide_by_zero(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=10) */
    program[1] = 3298;   /* DIV 98 (值=0, 除零!) */
    program[2] = 4300;   /* HALT */
    program[99] = 10;
    program[98] = 0;     /* 除数为零 */

    sml_vm_load(&vm, program);
    int result = sml_vm_run(&vm);

    /* 应该返回错误 */
    ASSERT_FALSE(result);
    /* 应该有错误信息 */
    const char *error = sml_vm_get_error(&vm);
    ASSERT_NOT_NULL(error);
    ASSERT_TRUE(strlen(error) > 0);
}

/**
 * @brief 测试周期计数
 */
void test_vm_cycle_count(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 */
    program[1] = 3098;   /* ADD 98 */
    program[2] = 2197;   /* STORE 97 */
    program[3] = 4300;   /* HALT */
    program[99] = 10;
    program[98] = 20;

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    /* 应该执行了 3-4 条指令 (HALT 是否计数取决于实现) */
    ASSERT_TRUE(vm.cycle_count >= 3 && vm.cycle_count <= 4);
}

/* ============================================================================
 *                              单步执行测试
 * ============================================================================ */

/**
 * @brief 测试单步执行
 */
void test_vm_step(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    int program[MEMORY_SIZE] = {0};
    program[0] = 2099;   /* LOAD 99 (值=10) */
    program[1] = 3098;   /* ADD 98 (值=5) */
    program[2] = 4300;   /* HALT */
    program[99] = 10;
    program[98] = 5;

    sml_vm_load(&vm, program);

    /* 第一步: LOAD 99 */
    int result = sml_vm_step(&vm);
    ASSERT_TRUE(result);
    ASSERT_EQ(vm.accumulator, 10);
    ASSERT_EQ(vm.instruction_counter, 1);

    /* 第二步: ADD 98 */
    result = sml_vm_step(&vm);
    ASSERT_TRUE(result);
    ASSERT_EQ(vm.accumulator, 15);
    ASSERT_EQ(vm.instruction_counter, 2);

    /* 第三步: HALT */
    result = sml_vm_step(&vm);
    ASSERT_FALSE(result);  /* HALT 返回 false */
    ASSERT_EQ(vm.running, 0);
}

/* ============================================================================
 *                              复杂程序测试
 * ============================================================================ */

/**
 * @brief 测试简单循环程序
 * 计算 1+2+3 = 6
 */
void test_vm_loop_program(void) {
    SML_VM vm;
    sml_vm_init(&vm);

    /* 手工编写 SML 程序: 计算 1+2+3
     * 内存布局:
     *   97: sum (结果)
     *   98: i (计数器)
     *   99: 常量 1
     *   96: 常量 3 (结束值)
     *   95: 临时变量
     */
    int program[MEMORY_SIZE] = {0};

    /* 初始化 sum = 0 (已经是0) */
    /* 初始化 i = 1 */
    program[0] = 2099;   /* LOAD 99 (常量1) */
    program[1] = 2198;   /* STORE 98 (i=1) */

    /* 循环开始 (地址 2) */
    /* sum = sum + i */
    program[2] = 2097;   /* LOAD 97 (sum) */
    program[3] = 3098;   /* ADD 98 (i) */
    program[4] = 2197;   /* STORE 97 (sum) */

    /* i = i + 1 */
    program[5] = 2098;   /* LOAD 98 (i) */
    program[6] = 3099;   /* ADD 99 (常量1) */
    program[7] = 2198;   /* STORE 98 (i) */

    /* 检查 i <= 3 */
    program[8] = 2096;   /* LOAD 96 (常量3) */
    program[9] = 3198;   /* SUB 98 (i) */
    program[10] = 4112;  /* BRANCHNEG 12 (如果 3-i<0, 退出) */
    program[11] = 4002;  /* BRANCH 2 (循环) */

    /* 退出 */
    program[12] = 4300;  /* HALT */

    /* 数据 */
    program[99] = 1;     /* 常量 1 */
    program[96] = 3;     /* 常量 3 */
    program[97] = 0;     /* sum */
    program[98] = 0;     /* i */

    sml_vm_load(&vm, program);
    sml_vm_run(&vm);

    /* sum 应该是 1+2+3 = 6 */
    ASSERT_EQ(vm.memory[97], 6);
}

/* ============================================================================
 *                              主函数
 * ============================================================================ */

int main(void) {
    TEST_BEGIN();

    /* 初始化测试 */
    RUN_TEST(test_vm_init);
    RUN_TEST(test_vm_load);

    /* 数据传输指令测试 */
    RUN_TEST(test_vm_load_instruction);
    RUN_TEST(test_vm_store_instruction);

    /* 算术指令测试 */
    RUN_TEST(test_vm_add);
    RUN_TEST(test_vm_subtract);
    RUN_TEST(test_vm_multiply);
    RUN_TEST(test_vm_divide);
    RUN_TEST(test_vm_mod);

    /* 控制流指令测试 */
    RUN_TEST(test_vm_branch);
    RUN_TEST(test_vm_branchneg);
    RUN_TEST(test_vm_branchneg_no_jump);
    RUN_TEST(test_vm_branchzero);
    RUN_TEST(test_vm_halt);

    /* 错误处理测试 */
    RUN_TEST(test_vm_divide_by_zero);
    RUN_TEST(test_vm_cycle_count);

    /* 单步执行测试 */
    RUN_TEST(test_vm_step);

    /* 复杂程序测试 */
    RUN_TEST(test_vm_loop_program);

    TEST_END();
    return test_failed;
}
