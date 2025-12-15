/**
 * @file main.c
 * @brief Simple 语言编译器/解释器主程序
 *
 * ============================================================================
 *                              程序概述
 * ============================================================================
 *
 * 本程序是 Simple 语言的完整工具链，包含:
 *   - 词法分析器 (Lexer): 将源代码转换为 Token 流
 *   - 解释器 (Interpreter): 直接执行 Simple 源代码
 *   - 编译器 (Compiler): 将 Simple 编译为 SML 机器码
 *   - 虚拟机 (SML VM): 执行 SML 机器码
 *
 * ============================================================================
 *                              运行模式
 * ============================================================================
 *
 * 程序支持以下运行模式:
 *
 * 1. 解释模式 (Interpret Mode):
 *    - 命令: ./simple program.simple 或 ./simple -i program.simple
 *    - 特点: 直接执行，支持浮点数，支持动态数组索引
 *    - 用途: 开发调试，快速运行
 *
 * 2. 编译模式 (Compile Mode):
 *    - 命令: ./simple -c program.simple
 *    - 特点: 生成 .sml 文件，显示符号表和 SML 代码
 *    - 用途: 查看编译结果，学习编译原理
 *
 * 3. 编译运行模式 (Compile & Run Mode):
 *    - 命令: ./simple -r program.simple
 *    - 特点: 编译后立即在内置 SML VM 上运行
 *    - 用途: 测试编译器生成的代码
 *
 * 4. 执行模式 (Execute Mode):
 *    - 命令: ./simple -x program.sml
 *    - 特点: 直接执行 .sml 机器码文件
 *    - 用途: 运行预编译的程序
 *
 * 5. 交互模式 (Interactive/REPL Mode):
 *    - 命令: ./simple (无参数)
 *    - 特点: 逐行输入程序，支持 run/list/clear 命令
 *    - 用途: 学习实验，快速测试
 *
 * ============================================================================
 *                              使用示例
 * ============================================================================
 *
 * 示例程序 (sum.simple):
 *   10 rem 计算 1 到 n 的和
 *   20 input n
 *   30 let s = 0
 *   40 for i = 1 to n
 *   50   let s = s + i
 *   60 next i
 *   70 print "Sum = ", s
 *   80 end
 *
 * 运行方式:
 *   $ ./simple sum.simple        # 解释执行
 *   $ ./simple -c sum.simple     # 编译为 SML
 *   $ ./simple -r sum.simple     # 编译并运行
 *   $ ./simple -x sum.simple.sml # 执行 SML 文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "compiler.h"
#include "sml_vm.h"

/* ============================================================================
 *                              前向声明
 * ============================================================================ */

void run_compiler(const char *filename);
void run_compiled(const char *filename);

/* ============================================================================
 *                              辅助函数
 * ============================================================================ */

/**
 * @brief 打印使用帮助
 *
 * 显示程序的命令行用法和可用选项。
 *
 * @param program 程序名 (argv[0])
 */
void print_usage(const char *program) {
    printf("Simple Language Interpreter/Compiler v2.0\n");
    printf("Usage: %s [options] <file.simple>\n", program);
    printf("Options:\n");
    printf("  -i, --interpret    Run in interpreter mode (default)\n");
    printf("  -c, --compile      Compile to SML and show generated code\n");
    printf("  -r, --run          Compile and run on SML VM\n");
    printf("  -x, --execute      Execute a .sml file directly\n");
    printf("  -h, --help         Show this help\n");
    printf("\nExamples:\n");
    printf("  %s examples/sum.simple           # interpret\n", program);
    printf("  %s -c examples/sum.simple        # compile only\n", program);
    printf("  %s -r examples/sum.simple        # compile and run\n", program);
    printf("  %s -x program.sml                # run SML file\n", program);
}

/* ============================================================================
 *                              运行模式实现
 * ============================================================================ */

/**
 * @brief 解释模式: 直接执行 Simple 源代码
 *
 * 使用解释器边解析边执行源代码。
 *
 * @param filename 源文件路径
 *
 * 流程:
 *   1. 初始化解释器
 *   2. 加载源文件
 *   3. 执行程序
 *   4. 报告错误 (如果有)
 *   5. 释放资源
 */
void run_interpreter(const char *filename) {
    Interpreter interp;
    interpreter_init(&interp);

    /* 加载源文件 */
    if (!interpreter_load_file(&interp, filename)) {
        fprintf(stderr, "Error: %s\n", interpreter_get_error(&interp));
        interpreter_free(&interp);
        return;
    }

    printf("=== Running %s ===\n", filename);

    /* 执行程序 */
    if (!interpreter_run(&interp)) {
        fprintf(stderr, "Runtime Error: %s\n", interpreter_get_error(&interp));
    }

    printf("=== Program finished ===\n");
    interpreter_free(&interp);
}

/**
 * @brief 交互模式: REPL 环境
 *
 * REPL = Read-Eval-Print-Loop
 * 允许用户逐行输入程序并执行。
 *
 * 支持的命令:
 *   - run:   执行当前程序
 *   - list:  显示当前程序
 *   - clear: 清空程序
 *   - help:  显示帮助
 *   - quit:  退出
 *
 * 程序输入格式:
 *   - 每行以行号开始 (如 10, 20, 30)
 *   - 格式: 行号 语句
 *   - 示例: 10 print "hello"
 */
void run_interactive(void) {
    printf("Simple Language Interpreter v1.0\n");
    printf("Enter 'run' to execute, 'list' to show code, 'clear' to reset, 'quit' to exit\n\n");

    char buffer[4096] = {0};  /* 存储用户输入的程序 */
    char line[256];           /* 单行输入缓冲区 */

    /* REPL 主循环 */
    while (1) {
        printf("> ");
        fflush(stdout);

        /* 读取一行输入 */
        if (!fgets(line, sizeof(line), stdin)) {
            break;  /* EOF 或错误 */
        }

        /* 去掉换行符 */
        line[strcspn(line, "\n")] = 0;

        /* ========== 处理命令 ========== */

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }

        if (strcmp(line, "run") == 0) {
            /* 执行当前程序 */
            if (strlen(buffer) == 0) {
                printf("No program to run.\n");
                continue;
            }
            Interpreter interp;
            interpreter_init(&interp);
            if (interpreter_load(&interp, buffer)) {
                printf("--- Output ---\n");
                if (!interpreter_run(&interp)) {
                    fprintf(stderr, "Error: %s\n", interpreter_get_error(&interp));
                }
                printf("--------------\n");
            } else {
                fprintf(stderr, "Error: %s\n", interpreter_get_error(&interp));
            }
            interpreter_free(&interp);
            continue;
        }

        if (strcmp(line, "list") == 0) {
            /* 显示当前程序 */
            if (strlen(buffer) == 0) {
                printf("(empty)\n");
            } else {
                printf("%s", buffer);
            }
            continue;
        }

        if (strcmp(line, "clear") == 0) {
            /* 清空程序 */
            buffer[0] = 0;
            printf("Program cleared.\n");
            continue;
        }

        if (strcmp(line, "help") == 0) {
            /* 显示帮助 */
            printf("Commands:\n");
            printf("  run   - Execute the program\n");
            printf("  list  - Show current program\n");
            printf("  clear - Clear the program\n");
            printf("  quit  - Exit interpreter\n");
            printf("\nEnter lines like:\n");
            printf("  10 input x\n");
            printf("  20 let y = x * 2\n");
            printf("  30 print y\n");
            printf("  40 end\n");
            continue;
        }

        /* ========== 处理程序行 ========== */

        /* 检查是否是程序行 (以数字开头) */
        if (line[0] >= '0' && line[0] <= '9') {
            strcat(buffer, line);
            strcat(buffer, "\n");
        } else if (strlen(line) > 0) {
            printf("Lines must start with a line number (e.g., '10 print x')\n");
        }
    }

    printf("Goodbye!\n");
}

/* ============================================================================
 *                              主程序入口
 * ============================================================================ */

/**
 * @brief 主程序入口
 *
 * 解析命令行参数，选择并执行相应的运行模式。
 *
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 成功返回 0，失败返回 1
 *
 * 参数解析:
 *   - 无参数: 进入交互模式
 *   - -i: 解释模式 (默认)
 *   - -c: 编译模式
 *   - -r: 编译运行模式
 *   - -x: 执行模式
 *   - -h: 显示帮助
 */
int main(int argc, char *argv[]) {
    /* 无参数，进入交互模式 */
    if (argc < 2) {
        run_interactive();
        return 0;
    }

    /* 解析命令行参数 */
    int mode = 0;  /* 0=解释, 1=编译, 2=编译运行, 3=执行SML */
    const char *filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interpret") == 0) {
            mode = 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
            mode = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--run") == 0) {
            mode = 2;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--execute") == 0) {
            mode = 3;
        } else {
            filename = argv[i];
        }
    }

    /* 检查是否提供了文件名 */
    if (!filename) {
        fprintf(stderr, "Error: No input file specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    /* 执行对应的模式 */
    switch (mode) {
        case 0:  /* 解释模式 */
            run_interpreter(filename);
            break;

        case 1:  /* 编译模式 */
            run_compiler(filename);
            break;

        case 2:  /* 编译运行模式 */
            run_compiled(filename);
            break;

        case 3:  /* 执行 SML 模式 */
            {
                SML_VM vm;
                if (!sml_vm_load_file(&vm, filename)) {
                    fprintf(stderr, "Error: %s\n", sml_vm_get_error(&vm));
                    return 1;
                }
                printf("=== Executing %s ===\n", filename);
                if (!sml_vm_run(&vm)) {
                    fprintf(stderr, "Runtime Error: %s\n", sml_vm_get_error(&vm));
                }
                printf("=== Program finished ===\n");
            }
            break;
    }

    return 0;
}

/* ============================================================================
 *                              编译模式实现
 * ============================================================================ */

/**
 * @brief 编译模式: 编译并显示生成的 SML 代码
 *
 * 将 Simple 源代码编译为 SML 机器码，显示:
 *   1. 符号表 (变量、常量、行号的内存映射)
 *   2. SML 指令列表 (操作码和操作数)
 *   3. 生成 .sml 输出文件
 *
 * @param filename 源文件路径
 *
 * 输出示例:
 *   === Compiling sum.simple ===
 *   Compilation successful!
 *
 *   === Symbol Table ===
 *     LINE    10 -> loc 00
 *     VAR     'n' -> loc 99
 *     CONST   1 -> loc 98
 *
 *   === SML Program ===
 *   Instructions (0-12):
 *     00: +1099  READ     99
 *     01: +2098  LOAD     98
 *     ...
 *
 *   SML program written to: sum.simple.sml
 */
void run_compiler(const char *filename) {
    Compiler comp;
    compiler_init(&comp);

    printf("=== Compiling %s ===\n", filename);

    /* 编译源文件 */
    if (!compiler_compile_file(&comp, filename)) {
        fprintf(stderr, "Compile Error: %s\n", compiler_get_error(&comp));
        compiler_free(&comp);
        return;
    }

    printf("Compilation successful!\n\n");

    /* 打印符号表 (调试信息) */
    compiler_dump_symbols(&comp);
    printf("\n");

    /* 打印 SML 代码 (调试信息) */
    compiler_dump(&comp);

    /* 输出到 .sml 文件 */
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.sml", filename);
    if (compiler_output(&comp, output_file)) {
        printf("\nSML program written to: %s\n", output_file);
    }

    compiler_free(&comp);
}

/**
 * @brief 编译运行模式: 编译后立即在 SML VM 上执行
 *
 * 完整的编译-执行流程:
 *   1. 编译 Simple 源代码为 SML 机器码
 *   2. 加载 SML 代码到虚拟机
 *   3. 执行程序
 *   4. 显示执行统计 (周期数)
 *
 * @param filename 源文件路径
 *
 * 这是学习编译原理的最佳方式:
 *   - 可以看到高级语言如何转换为机器码
 *   - 可以观察虚拟机如何执行这些指令
 */
void run_compiled(const char *filename) {
    Compiler comp;
    compiler_init(&comp);

    printf("=== Compiling %s ===\n", filename);

    /* 编译源文件 */
    if (!compiler_compile_file(&comp, filename)) {
        fprintf(stderr, "Compile Error: %s\n", compiler_get_error(&comp));
        compiler_free(&comp);
        return;
    }

    printf("Compilation successful! Running on SML VM...\n\n");

    /* 将编译结果加载到虚拟机 */
    SML_VM vm;
    sml_vm_init(&vm);
    sml_vm_load(&vm, compiler_get_memory(&comp));

    /* 执行程序 */
    if (!sml_vm_run(&vm)) {
        fprintf(stderr, "Runtime Error: %s\n", sml_vm_get_error(&vm));
    }

    /* 显示执行统计 */
    printf("\n=== Program finished (cycles: %d) ===\n", vm.cycle_count);

    compiler_free(&comp);
}
