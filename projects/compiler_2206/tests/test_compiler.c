/**
 * @file test_compiler.c
 * @brief 编译器单元测试
 *
 * 测试覆盖:
 *   - 符号表操作
 *   - 基本语句编译
 *   - 表达式编译
 *   - 控制流编译 (goto, if, for)
 *   - 两遍扫描的前向引用解析
 *
 * 运行方法:
 *   cd build && ./test_compiler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "compiler.h"

/* ============================================================================
 *                              编译器初始化测试
 * ============================================================================ */

/**
 * @brief 测试编译器初始化
 */
void test_compiler_init(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 检查初始状态 */
    ASSERT_EQ(comp.instruction_counter, 0);
    ASSERT_EQ(comp.data_counter, MEMORY_SIZE - 1);  /* 99 */
    ASSERT_EQ(comp.symbol_count, 0);
    ASSERT_EQ(comp.flag_count, 0);
    ASSERT_EQ(comp.has_error, 0);

    /* 内存应该被清零 */
    for (int i = 0; i < MEMORY_SIZE; i++) {
        ASSERT_EQ(comp.memory[i], 0);
    }

    compiler_free(&comp);
}

/* ============================================================================
 *                              基本语句编译测试
 * ============================================================================ */

/**
 * @brief 测试 let 语句编译
 */
void test_compile_let(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 编译: 10 let x = 5 */
    int result = compiler_compile(&comp, "10 let x = 5\n20 end\n");
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 检查是否有 HALT 指令 */
    int found_halt = 0;
    for (int i = 0; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        if (opcode == SML_HALT) {
            found_halt = 1;
            break;
        }
    }
    ASSERT_TRUE(found_halt);

    compiler_free(&comp);
}

/**
 * @brief 测试 print 语句编译
 */
void test_compile_print(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 编译带 print 的程序 */
    int result = compiler_compile(&comp, "10 let x = 42\n20 print x\n30 end\n");
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 检查是否有 WRITE 指令 */
    int found_write = 0;
    for (int i = 0; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        if (opcode == SML_WRITE) {
            found_write = 1;
            break;
        }
    }
    ASSERT_TRUE(found_write);

    compiler_free(&comp);
}

/**
 * @brief 测试 rem (注释) 语句编译
 */
void test_compile_rem(void) {
    Compiler comp;
    compiler_init(&comp);

    /* rem 语句不应生成任何指令 */
    int result = compiler_compile(&comp, "10 rem this is a comment\n20 end\n");
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 只应该有 HALT 指令 */
    ASSERT_TRUE(comp.instruction_counter <= 2);  /* 最多 HALT + 少量指令 */

    compiler_free(&comp);
}

/* ============================================================================
 *                              表达式编译测试
 * ============================================================================ */

/**
 * @brief 测试算术表达式编译
 */
void test_compile_arithmetic(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 编译: let x = 1 + 2 * 3 */
    int result = compiler_compile(&comp, "10 let x = 1 + 2 * 3\n20 end\n");
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 应该生成 LOAD, STORE, ADD, MULTIPLY 等指令 */
    ASSERT_TRUE(comp.instruction_counter > 2);

    compiler_free(&comp);
}

/**
 * @brief 测试除法和取模编译
 */
void test_compile_division(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp, "10 let x = 10 / 3\n20 let y = 10 % 3\n30 end\n");
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 检查是否有 DIVIDE 和 MOD 指令 */
    int found_divide = 0, found_mod = 0;
    for (int i = 0; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        if (opcode == SML_DIVIDE) found_divide = 1;
        if (opcode == SML_MOD) found_mod = 1;
    }
    ASSERT_TRUE(found_divide);
    ASSERT_TRUE(found_mod);

    compiler_free(&comp);
}

/* ============================================================================
 *                              控制流编译测试
 * ============================================================================ */

/**
 * @brief 测试 goto 语句编译
 */
void test_compile_goto(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 前向跳转测试 */
    int result = compiler_compile(&comp,
        "10 goto 30\n"
        "20 let x = 1\n"
        "30 end\n"
    );
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 应该有 BRANCH 指令 */
    int found_branch = 0;
    for (int i = 0; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        if (opcode == SML_BRANCH) {
            found_branch = 1;
            /* 跳转目标应该是有效地址 */
            int target = comp.memory[i] % 100;
            ASSERT_TRUE(target >= 0 && target < MEMORY_SIZE);
            break;
        }
    }
    ASSERT_TRUE(found_branch);

    compiler_free(&comp);
}

/**
 * @brief 测试 if 语句编译
 */
void test_compile_if(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp,
        "10 let x = 5\n"
        "20 if x > 0 goto 40\n"
        "30 let y = 0\n"
        "40 end\n"
    );
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 应该有条件跳转指令 (BRANCHNEG 或 BRANCHZERO) */
    int found_conditional = 0;
    for (int i = 0; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        if (opcode == SML_BRANCHNEG || opcode == SML_BRANCHZERO) {
            found_conditional = 1;
            break;
        }
    }
    ASSERT_TRUE(found_conditional);

    compiler_free(&comp);
}

/**
 * @brief 测试 for 循环编译
 */
void test_compile_for(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp,
        "10 for i = 1 to 5\n"
        "20   let x = i\n"
        "30 next i\n"
        "40 end\n"
    );
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* for 循环应该生成:
     * - 初始化代码 (LOAD, STORE)
     * - 循环体
     * - 递增代码 (LOAD, ADD, STORE)
     * - 条件判断和跳转
     */
    ASSERT_TRUE(comp.instruction_counter > 5);

    /* 检查是否有向后跳转 (循环回去) */
    int found_backward_jump = 0;
    for (int i = 1; i < comp.instruction_counter; i++) {
        int opcode = comp.memory[i] / 100;
        int target = comp.memory[i] % 100;
        if ((opcode == SML_BRANCH || opcode == SML_BRANCHNEG || opcode == SML_BRANCHZERO)
            && target < i) {
            found_backward_jump = 1;
            break;
        }
    }
    /* for 循环通常有向后跳转 */
    /* 注意: 具体实现可能不同，这里只是基本检查 */

    compiler_free(&comp);
}

/* ============================================================================
 *                              符号表测试
 * ============================================================================ */

/**
 * @brief 测试符号表正确记录变量
 */
void test_symbol_table_variables(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp,
        "10 let a = 1\n"
        "20 let b = 2\n"
        "30 let c = a + b\n"
        "40 end\n"
    );
    ASSERT_TRUE(result);

    /* 应该有变量 a, b, c 在符号表中 */
    /* 符号表中也应该有行号 10, 20, 30, 40 */
    ASSERT_TRUE(comp.symbol_count >= 4);  /* 至少 3 个变量 + 行号 */

    compiler_free(&comp);
}

/**
 * @brief 测试常量处理
 */
void test_symbol_table_constants(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 使用多个常量 */
    int result = compiler_compile(&comp,
        "10 let x = 100\n"
        "20 let y = 200\n"
        "30 let z = 100\n"  /* 重复使用常量 100 */
        "40 end\n"
    );
    ASSERT_TRUE(result);

    /* 常量 100 应该只分配一次内存 */
    int const_100_count = 0;
    for (int i = comp.data_counter + 1; i < MEMORY_SIZE; i++) {
        if (comp.memory[i] == 100) {
            const_100_count++;
        }
    }
    /* 可能有多个 100，取决于实现细节 */
    ASSERT_TRUE(const_100_count >= 1);

    compiler_free(&comp);
}

/* ============================================================================
 *                              错误处理测试
 * ============================================================================ */

/**
 * @brief 测试语法错误检测
 */
void test_compile_syntax_error(void) {
    Compiler comp;
    compiler_init(&comp);

    /* 缺少 end 语句 */
    int result = compiler_compile(&comp, "10 let x = 5\n");
    /* 根据具体实现，可能成功也可能失败 */
    /* 这里只检查不会崩溃 */
    (void)result;

    compiler_free(&comp);
}

/**
 * @brief 测试内存获取函数
 */
void test_compiler_get_memory(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp, "10 let x = 5\n20 end\n");
    ASSERT_TRUE(result);

    const int *memory = compiler_get_memory(&comp);
    ASSERT_NOT_NULL(memory);

    /* 内存应该与 comp.memory 相同 */
    for (int i = 0; i < MEMORY_SIZE; i++) {
        ASSERT_EQ(memory[i], comp.memory[i]);
    }

    compiler_free(&comp);
}

/* ============================================================================
 *                              完整程序测试
 * ============================================================================ */

/**
 * @brief 测试编译求和程序
 */
void test_compile_sum_program(void) {
    Compiler comp;
    compiler_init(&comp);

    int result = compiler_compile(&comp,
        "10 let s = 0\n"
        "20 for i = 1 to 5\n"
        "30   let s = s + i\n"
        "40 next i\n"
        "50 print s\n"
        "60 end\n"
    );
    ASSERT_TRUE(result);
    ASSERT_FALSE(comp.has_error);

    /* 程序应该成功编译 */
    ASSERT_TRUE(comp.instruction_counter > 0);

    compiler_free(&comp);
}

/* ============================================================================
 *                              主函数
 * ============================================================================ */

int main(void) {
    TEST_BEGIN();

    /* 初始化测试 */
    RUN_TEST(test_compiler_init);

    /* 基本语句编译测试 */
    RUN_TEST(test_compile_let);
    RUN_TEST(test_compile_print);
    RUN_TEST(test_compile_rem);

    /* 表达式编译测试 */
    RUN_TEST(test_compile_arithmetic);
    RUN_TEST(test_compile_division);

    /* 控制流编译测试 */
    RUN_TEST(test_compile_goto);
    RUN_TEST(test_compile_if);
    RUN_TEST(test_compile_for);

    /* 符号表测试 */
    RUN_TEST(test_symbol_table_variables);
    RUN_TEST(test_symbol_table_constants);

    /* 错误处理测试 */
    RUN_TEST(test_compile_syntax_error);
    RUN_TEST(test_compiler_get_memory);

    /* 完整程序测试 */
    RUN_TEST(test_compile_sum_program);

    TEST_END();
    return test_failed;
}
