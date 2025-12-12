/**
 * @file main.c
 * @brief Simple 语言编译器/解释器主程序
 *
 * 提供四种运行模式:
 * 1. 解释模式 (-i): 直接解释执行 Simple 源代码
 * 2. 编译模式 (-c): 编译为 SML 机器码，输出 .sml 文件
 * 3. 编译运行 (-r): 编译后立即在内置 SML VM 上执行
 * 4. 执行模式 (-x): 直接执行 .sml 文件
 * 5. 交互模式:      无参数启动，提供 REPL 环境
 *
 * 使用示例:
 * @code
 * ./simple                        # 交互模式
 * ./simple program.simple         # 解释执行
 * ./simple -c program.simple      # 编译生成 .sml
 * ./simple -r program.simple      # 编译并运行
 * ./simple -x program.sml         # 执行 SML 文件
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "compiler.h"
#include "sml_vm.h"

/* ==================== 前向声明 ==================== */

void run_compiler(const char *filename);
void run_compiled(const char *filename);

/* ==================== 辅助函数 ==================== */

/** 打印使用帮助 */
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

/* ==================== 运行模式 ==================== */

/**
 * @brief 解释模式: 直接执行 Simple 源代码
 * @param filename 源文件路径
 */
void run_interpreter(const char *filename) {
    Interpreter interp;
    interpreter_init(&interp);

    if (!interpreter_load_file(&interp, filename)) {
        fprintf(stderr, "Error: %s\n", interpreter_get_error(&interp));
        interpreter_free(&interp);
        return;
    }

    printf("=== Running %s ===\n", filename);

    if (!interpreter_run(&interp)) {
        fprintf(stderr, "Runtime Error: %s\n", interpreter_get_error(&interp));
    }

    printf("=== Program finished ===\n");
    interpreter_free(&interp);
}

/**
 * @brief 交互模式: REPL 环境
 *
 * 允许用户逐行输入程序，支持命令:
 * - run:   执行当前程序
 * - list:  显示当前程序
 * - clear: 清空程序
 * - help:  显示帮助
 * - quit:  退出
 */
void run_interactive(void) {
    printf("Simple Language Interpreter v1.0\n");
    printf("Enter 'run' to execute, 'list' to show code, 'clear' to reset, 'quit' to exit\n\n");

    char buffer[4096] = {0};
    char line[256];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // 去掉换行符
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }

        if (strcmp(line, "run") == 0) {
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
            if (strlen(buffer) == 0) {
                printf("(empty)\n");
            } else {
                printf("%s", buffer);
            }
            continue;
        }

        if (strcmp(line, "clear") == 0) {
            buffer[0] = 0;
            printf("Program cleared.\n");
            continue;
        }

        if (strcmp(line, "help") == 0) {
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

        // 检查是否是程序行 (以数字开头)
        if (line[0] >= '0' && line[0] <= '9') {
            strcat(buffer, line);
            strcat(buffer, "\n");
        } else if (strlen(line) > 0) {
            printf("Lines must start with a line number (e.g., '10 print x')\n");
        }
    }

    printf("Goodbye!\n");
}

/* ==================== 主程序入口 ==================== */

/**
 * @brief 主程序入口
 *
 * 解析命令行参数，选择运行模式。
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        // 无参数，进入交互模式
        run_interactive();
        return 0;
    }

    // 解析命令行参数
    // 0 = interpret, 1 = compile only, 2 = compile and run, 3 = execute SML
    int mode = 0;
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

    if (!filename) {
        fprintf(stderr, "Error: No input file specified.\n");
        print_usage(argv[0]);
        return 1;
    }

    switch (mode) {
        case 0:
            run_interpreter(filename);
            break;
        case 1:
            run_compiler(filename);
            break;
        case 2:
            run_compiled(filename);
            break;
        case 3:
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

/**
 * @brief 编译模式: 编译并显示生成的 SML 代码
 * @param filename 源文件路径
 *
 * 输出:
 * 1. 符号表
 * 2. SML 指令列表
 * 3. .sml 文件
 */
void run_compiler(const char *filename) {
    Compiler comp;
    compiler_init(&comp);

    printf("=== Compiling %s ===\n", filename);

    if (!compiler_compile_file(&comp, filename)) {
        fprintf(stderr, "Compile Error: %s\n", compiler_get_error(&comp));
        compiler_free(&comp);
        return;
    }

    printf("Compilation successful!\n\n");

    compiler_dump_symbols(&comp);  // 打印符号表
    printf("\n");
    compiler_dump(&comp);          // 打印 SML 代码

    // 输出到 .sml 文件
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.sml", filename);
    if (compiler_output(&comp, output_file)) {
        printf("\nSML program written to: %s\n", output_file);
    }

    compiler_free(&comp);
}

/**
 * @brief 编译运行模式: 编译后立即在 SML VM 上执行
 * @param filename 源文件路径
 *
 * 流程: Simple源码 → 编译 → SML码 → 虚拟机执行
 */
void run_compiled(const char *filename) {
    Compiler comp;
    compiler_init(&comp);

    printf("=== Compiling %s ===\n", filename);

    if (!compiler_compile_file(&comp, filename)) {
        fprintf(stderr, "Compile Error: %s\n", compiler_get_error(&comp));
        compiler_free(&comp);
        return;
    }

    printf("Compilation successful! Running on SML VM...\n\n");

    // 加载编译结果到虚拟机
    SML_VM vm;
    sml_vm_init(&vm);
    sml_vm_load(&vm, compiler_get_memory(&comp));

    // 执行
    if (!sml_vm_run(&vm)) {
        fprintf(stderr, "Runtime Error: %s\n", sml_vm_get_error(&vm));
    }

    printf("\n=== Program finished (cycles: %d) ===\n", vm.cycle_count);

    compiler_free(&comp);
}
