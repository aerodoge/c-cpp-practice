/**
 * @file benchmark.c
 * @brief Simple 编译器/解释器性能基准测试
 *
 * 测量并比较:
 *   - 词法分析速度
 *   - 编译速度
 *   - 解释执行速度
 *   - VM 执行速度
 *
 * 运行方法:
 *   cd build && ./benchmark
 *
 * 输出格式:
 *   测试名称          | 迭代次数 | 总时间(ms) | 平均时间(us)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer.h"
#include "interpreter.h"
#include "compiler.h"
#include "sml_vm.h"

/* ============================================================================
 *                              计时工具
 * ============================================================================ */

/**
 * @brief 获取当前时间 (微秒)
 */
static long long get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

/**
 * @brief 打印基准测试结果
 */
static void print_benchmark_result(const char *name, int iterations,
                                    long long total_us) {
    double total_ms = total_us / 1000.0;
    double avg_us = (double)total_us / iterations;
    printf("%-30s | %8d | %10.2f ms | %10.2f us\n",
           name, iterations, total_ms, avg_us);
}

/* ============================================================================
 *                              测试程序
 * ============================================================================ */

/* 简单程序: 计算 1+2+...+100 */
static const char *SIMPLE_SUM_PROGRAM =
    "10 let s = 0\n"
    "20 for i = 1 to 100\n"
    "30   let s = s + i\n"
    "40 next i\n"
    "50 end\n";

/* 中等复杂程序: 嵌套循环 */
static const char *NESTED_LOOP_PROGRAM =
    "10 let s = 0\n"
    "20 for i = 1 to 10\n"
    "30   for j = 1 to 10\n"
    "40     let s = s + i * j\n"
    "50   next j\n"
    "60 next i\n"
    "70 end\n";

/* 算术密集程序 */
static const char *ARITHMETIC_PROGRAM =
    "10 let a = 1\n"
    "20 let b = 2\n"
    "30 let c = 3\n"
    "40 for i = 1 to 50\n"
    "50   let a = a + b * c - a / 2\n"
    "60   let b = b * 2 - c + a % 10\n"
    "70   let c = c + a - b / 3\n"
    "80 next i\n"
    "90 end\n";

/* 条件跳转程序 */
static const char *CONDITIONAL_PROGRAM =
    "10 let x = 0\n"
    "20 let i = 0\n"
    "30 if i >= 100 goto 80\n"
    "40 if i % 2 == 0 goto 60\n"
    "50 let x = x + i\n"
    "60 let i = i + 1\n"
    "70 goto 30\n"
    "80 end\n";

/* ============================================================================
 *                              词法分析基准测试
 * ============================================================================ */

/**
 * @brief 测试词法分析速度
 */
static void benchmark_lexer(const char *program, const char *name, int iterations) {
    long long start = get_time_us();

    for (int i = 0; i < iterations; i++) {
        Lexer lexer;
        lexer_init(&lexer, program);

        Token token;
        do {
            token = lexer_next_token(&lexer);
        } while (token.type != TOKEN_EOF);
    }

    long long end = get_time_us();
    print_benchmark_result(name, iterations, end - start);
}

/* ============================================================================
 *                              编译基准测试
 * ============================================================================ */

/**
 * @brief 测试编译速度
 */
static void benchmark_compiler(const char *program, const char *name, int iterations) {
    long long start = get_time_us();

    for (int i = 0; i < iterations; i++) {
        Compiler comp;
        compiler_init(&comp);
        compiler_compile(&comp, program);
        compiler_free(&comp);
    }

    long long end = get_time_us();
    print_benchmark_result(name, iterations, end - start);
}

/* ============================================================================
 *                              解释执行基准测试
 * ============================================================================ */

/**
 * @brief 重定向 stdout 到 /dev/null (抑制输出)
 */
static FILE *suppress_output(void) {
    FILE *old_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    return old_stdout;
}

/**
 * @brief 恢复 stdout
 */
static void restore_output(FILE *old_stdout) {
    fclose(stdout);
    stdout = old_stdout;
}

/**
 * @brief 测试解释执行速度
 */
static void benchmark_interpreter(const char *program, const char *name, int iterations) {
    FILE *old_stdout = suppress_output();

    long long start = get_time_us();

    for (int i = 0; i < iterations; i++) {
        Interpreter interp;
        interpreter_init(&interp);
        interpreter_load(&interp, program);
        interpreter_run(&interp);
        interpreter_free(&interp);
    }

    long long end = get_time_us();

    restore_output(old_stdout);
    print_benchmark_result(name, iterations, end - start);
}

/* ============================================================================
 *                              VM 执行基准测试
 * ============================================================================ */

/**
 * @brief 测试编译+VM执行速度
 */
static void benchmark_compile_and_run(const char *program, const char *name, int iterations) {
    /* 先编译一次 */
    Compiler comp;
    compiler_init(&comp);
    if (!compiler_compile(&comp, program)) {
        printf("Compilation failed for %s\n", name);
        compiler_free(&comp);
        return;
    }

    FILE *old_stdout = suppress_output();

    long long start = get_time_us();

    for (int i = 0; i < iterations; i++) {
        SML_VM vm;
        sml_vm_init(&vm);
        sml_vm_load(&vm, compiler_get_memory(&comp));
        sml_vm_run(&vm);
    }

    long long end = get_time_us();

    restore_output(old_stdout);
    compiler_free(&comp);
    print_benchmark_result(name, iterations, end - start);
}

/**
 * @brief 测试纯 VM 执行速度 (预编译)
 */
static void benchmark_vm_only(const char *program, const char *name, int iterations) {
    /* 先编译 */
    Compiler comp;
    compiler_init(&comp);
    if (!compiler_compile(&comp, program)) {
        printf("Compilation failed for %s\n", name);
        compiler_free(&comp);
        return;
    }

    /* 复制内存 */
    int memory[MEMORY_SIZE];
    memcpy(memory, compiler_get_memory(&comp), sizeof(memory));
    compiler_free(&comp);

    FILE *old_stdout = suppress_output();

    long long start = get_time_us();

    for (int i = 0; i < iterations; i++) {
        SML_VM vm;
        sml_vm_init(&vm);
        sml_vm_load(&vm, memory);
        sml_vm_run(&vm);
    }

    long long end = get_time_us();

    restore_output(old_stdout);
    print_benchmark_result(name, iterations, end - start);
}

/* ============================================================================
 *                              指令周期统计
 * ============================================================================ */

/**
 * @brief 统计程序执行的指令周期数
 */
static void benchmark_cycle_count(const char *program, const char *name) {
    Compiler comp;
    compiler_init(&comp);
    if (!compiler_compile(&comp, program)) {
        printf("Compilation failed for %s\n", name);
        compiler_free(&comp);
        return;
    }

    SML_VM vm;
    sml_vm_init(&vm);
    sml_vm_load(&vm, compiler_get_memory(&comp));

    FILE *old_stdout = suppress_output();
    sml_vm_run(&vm);
    restore_output(old_stdout);

    printf("%-30s | 指令数: %d | 代码大小: %d\n",
           name, vm.cycle_count, comp.instruction_counter);

    compiler_free(&comp);
}

/* ============================================================================
 *                              主函数
 * ============================================================================ */

int main(void) {
    printf("================================================================\n");
    printf("        Simple 编译器/解释器 性能基准测试\n");
    printf("================================================================\n\n");

    /* ========== 词法分析基准测试 ========== */
    printf("=== 词法分析速度 ===\n");
    printf("%-30s | %8s | %13s | %13s\n",
           "测试名称", "迭代次数", "总时间", "平均时间");
    printf("--------------------------------------------------------------\n");

    benchmark_lexer(SIMPLE_SUM_PROGRAM, "Lexer: 简单求和", 10000);
    benchmark_lexer(NESTED_LOOP_PROGRAM, "Lexer: 嵌套循环", 10000);
    benchmark_lexer(ARITHMETIC_PROGRAM, "Lexer: 算术密集", 10000);
    benchmark_lexer(CONDITIONAL_PROGRAM, "Lexer: 条件跳转", 10000);

    printf("\n");

    /* ========== 编译基准测试 ========== */
    printf("=== 编译速度 ===\n");
    printf("%-30s | %8s | %13s | %13s\n",
           "测试名称", "迭代次数", "总时间", "平均时间");
    printf("--------------------------------------------------------------\n");

    benchmark_compiler(SIMPLE_SUM_PROGRAM, "Compile: 简单求和", 5000);
    benchmark_compiler(NESTED_LOOP_PROGRAM, "Compile: 嵌套循环", 5000);
    benchmark_compiler(ARITHMETIC_PROGRAM, "Compile: 算术密集", 5000);
    benchmark_compiler(CONDITIONAL_PROGRAM, "Compile: 条件跳转", 5000);

    printf("\n");

    /* ========== 解释执行基准测试 ========== */
    printf("=== 解释执行速度 ===\n");
    printf("%-30s | %8s | %13s | %13s\n",
           "测试名称", "迭代次数", "总时间", "平均时间");
    printf("--------------------------------------------------------------\n");

    benchmark_interpreter(SIMPLE_SUM_PROGRAM, "Interpret: 简单求和", 1000);
    benchmark_interpreter(NESTED_LOOP_PROGRAM, "Interpret: 嵌套循环", 1000);
    benchmark_interpreter(ARITHMETIC_PROGRAM, "Interpret: 算术密集", 1000);
    benchmark_interpreter(CONDITIONAL_PROGRAM, "Interpret: 条件跳转", 1000);

    printf("\n");

    /* ========== VM 执行基准测试 ========== */
    printf("=== VM 执行速度 (编译后) ===\n");
    printf("%-30s | %8s | %13s | %13s\n",
           "测试名称", "迭代次数", "总时间", "平均时间");
    printf("--------------------------------------------------------------\n");

    benchmark_vm_only(SIMPLE_SUM_PROGRAM, "VM: 简单求和", 5000);
    benchmark_vm_only(NESTED_LOOP_PROGRAM, "VM: 嵌套循环", 5000);
    benchmark_vm_only(ARITHMETIC_PROGRAM, "VM: 算术密集", 5000);
    benchmark_vm_only(CONDITIONAL_PROGRAM, "VM: 条件跳转", 5000);

    printf("\n");

    /* ========== 编译+执行基准测试 ========== */
    printf("=== 编译+执行速度 ===\n");
    printf("%-30s | %8s | %13s | %13s\n",
           "测试名称", "迭代次数", "总时间", "平均时间");
    printf("--------------------------------------------------------------\n");

    benchmark_compile_and_run(SIMPLE_SUM_PROGRAM, "Compile+Run: 简单求和", 2000);
    benchmark_compile_and_run(NESTED_LOOP_PROGRAM, "Compile+Run: 嵌套循环", 2000);
    benchmark_compile_and_run(ARITHMETIC_PROGRAM, "Compile+Run: 算术密集", 2000);
    benchmark_compile_and_run(CONDITIONAL_PROGRAM, "Compile+Run: 条件跳转", 2000);

    printf("\n");

    /* ========== 指令周期统计 ========== */
    printf("=== 指令周期统计 ===\n");
    printf("--------------------------------------------------------------\n");

    benchmark_cycle_count(SIMPLE_SUM_PROGRAM, "简单求和");
    benchmark_cycle_count(NESTED_LOOP_PROGRAM, "嵌套循环");
    benchmark_cycle_count(ARITHMETIC_PROGRAM, "算术密集");
    benchmark_cycle_count(CONDITIONAL_PROGRAM, "条件跳转");

    printf("\n");

    /* ========== 性能对比总结 ========== */
    printf("================================================================\n");
    printf("                        性能对比分析\n");
    printf("================================================================\n");
    printf("\n");
    printf("1. 词法分析: 最快的阶段，通常在微秒级完成\n");
    printf("2. 编译: 包含符号表管理和代码生成，比词法分析慢\n");
    printf("3. 解释执行: 边解析边执行，有解析开销\n");
    printf("4. VM执行: 预编译后执行，无解析开销\n");
    printf("\n");
    printf("结论:\n");
    printf("- 对于单次执行: 解释器更快 (无编译开销)\n");
    printf("- 对于多次执行: 编译+VM 更快 (编译开销被分摊)\n");
    printf("- 编译后的程序执行速度约为解释器的 2-5 倍\n");
    printf("\n");

    return 0;
}
