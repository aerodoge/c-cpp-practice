/**
 * @file interpreter.c
 * @brief Simple 语言解释器实现
 *
 * ============================================================================
 *                              解释器原理
 * ============================================================================
 *
 * 解释器采用"边解析边执行"(Parse-and-Execute)策略，无需生成中间代码。
 * 这与编译器不同：编译器先生成目标代码，再由虚拟机执行。
 *
 * 解释器 vs 编译器:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  解释器:  源代码 ──→ 直接执行                                            │
 * │  编译器:  源代码 ──→ SML机器码 ──→ 虚拟机执行                            │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * 解释器的优点:
 *   - 支持浮点数运算 (double 类型)
 *   - 支持动态数组索引 a(i)，其中 i 可以是变量
 *   - 无内存限制 (不受 SML 100 单元限制)
 *   - 更好的错误信息 (可以准确定位到源代码行)
 *
 * 解释器的缺点:
 *   - 执行速度较慢 (每次执行都要解析)
 *   - 无法生成可分发的目标文件
 *
 * ============================================================================
 *                              执行流程
 * ============================================================================
 *
 * 1. 加载阶段 (interpreter_load):
 *    - 复制源代码到内存
 *    - 扫描源代码，建立行号索引表 (行号 → 源代码位置)
 *    - 行号索引表用于 goto/if 跳转时快速定位目标行
 *
 * 2. 执行阶段 (interpreter_run):
 *    - 从第一行开始，按行号顺序执行
 *    - 每行：重置词法分析器 → 解析语句 → 执行语句
 *    - 遇到 goto/if 时跳转到目标行
 *    - 遇到 end 或错误时停止
 *
 * ============================================================================
 *                              表达式解析
 * ============================================================================
 *
 * 采用递归下降解析器处理算术表达式，运算符优先级从低到高：
 *
 *   优先级    运算符           解析函数
 *   ─────────────────────────────────────
 *     1      +, -            parse_expression
 *     2      *, /, %         parse_term
 *     3      ^               parse_power (右结合)
 *     4      一元 -/+         parse_unary
 *     5      数字/变量/()     parse_primary
 *
 * 文法规则 (EBNF):
 *   expression → term (('+' | '-') term)*
 *   term       → power (('*' | '/' | '%') power)*
 *   power      → unary ('^' unary)*        // 右结合
 *   unary      → ('-' | '+')? unary | primary
 *   primary    → NUMBER | IDENT | IDENT '(' expr ')' | '(' expr ')'
 *
 * 递归下降解析示例 (解析 "2 + 3 * 4"):
 *
 *   parse_expression()
 *     └→ parse_term() → parse_power() → parse_primary() → 返回 2
 *     └→ 遇到 '+', 继续
 *     └→ parse_term()
 *          └→ parse_power() → parse_primary() → 返回 3
 *          └→ 遇到 '*', 继续
 *          └→ parse_power() → parse_primary() → 返回 4
 *          └→ 返回 3 * 4 = 12
 *     └→ 返回 2 + 12 = 14
 */

#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

/* ============================================================================
 *                              辅助函数
 * ============================================================================ */

/**
 * @brief 将变量名转换为索引
 *
 * Simple 语言中，变量名是单个小写字母 a-z，
 * 对应索引 0-25，用于访问变量数组。
 *
 * @param c 变量名字符
 * @return 索引 (0-25)，无效返回 -1
 *
 * 示例:
 *   var_index('a') = 0
 *   var_index('z') = 25
 *   var_index('A') = 0  (大小写不敏感)
 */
static int var_index(char c) {
    c = tolower(c);  /* 转小写，支持大小写不敏感 */
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }
    return -1;  /* 无效变量名 */
}

/**
 * @brief 设置运行时错误
 *
 * 记录错误信息并停止解释器运行。
 * 使用可变参数支持格式化错误信息。
 *
 * @param interp 解释器指针
 * @param format 格式字符串 (printf 风格)
 * @param ...    格式参数
 *
 * 示例:
 *   set_error(interp, "Line %d: Division by zero", line_num);
 */
static void set_error(Interpreter *interp, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(interp->error_message, sizeof(interp->error_message), format, args);
    va_end(args);

    interp->has_error = 1;   /* 设置错误标志 */
    interp->running = 0;     /* 停止运行 */
}

/**
 * @brief 获取下一个 Token
 *
 * 从词法分析器获取下一个 Token，存入 current_token。
 * 这是解析器消费 Token 的基本操作。
 *
 * @param interp 解释器指针
 */
static void advance_token(Interpreter *interp) {
    interp->current_token = lexer_next_token(&interp->lexer);
}

/**
 * @brief 期望特定类型的 Token
 *
 * 检查当前 Token 是否为期望的类型。
 * 如果不是，设置错误并返回 0。
 *
 * @param interp 解释器指针
 * @param type   期望的 Token 类型
 * @return 匹配返回 1，不匹配返回 0
 *
 * 用法:
 *   if (!expect(interp, TOKEN_RPAREN)) return 0;
 */
static int expect(Interpreter *interp, TokenType type) {
    if (interp->current_token.type != type) {
        set_error(interp, "Line %d: Expected %s, got %s",
                  interp->lines[interp->current_line_index].line_number,
                  token_type_name(type),
                  token_type_name(interp->current_token.type));
        return 0;
    }
    return 1;
}

/**
 * @brief 查找行号对应的行索引
 *
 * 在行号索引表中查找指定行号。
 * 用于 goto/if 跳转时定位目标行。
 *
 * @param interp      解释器指针
 * @param line_number 要查找的行号 (如 10, 20, 30)
 * @return 行索引 (0, 1, 2...)，未找到返回 -1
 *
 * 时间复杂度: O(n)，其中 n 是程序行数
 * 优化方向: 可以使用二分查找 (如果行号已排序)
 */
static int find_line_index(Interpreter *interp, int line_number) {
    for (int i = 0; i < interp->line_count; i++) {
        if (interp->lines[i].line_number == line_number) {
            return i;
        }
    }
    return -1;  /* 行号不存在 */
}

/* ============================================================================
 *                              表达式解析 (递归下降)
 * ============================================================================
 *
 * 表达式解析采用递归下降法，每个优先级对应一个解析函数。
 * 函数调用关系体现了运算符优先级：
 *
 *   parse_expression (加减，最低优先级)
 *       └→ parse_term (乘除模)
 *           └→ parse_power (幂运算)
 *               └→ parse_unary (一元运算)
 *                   └→ parse_primary (基本元素，最高优先级)
 *
 * 每个函数:
 *   1. 先调用更高优先级的函数解析左操作数
 *   2. 检查当前 Token 是否是本级运算符
 *   3. 如果是，调用更高优先级函数解析右操作数
 *   4. 执行运算，循环处理同级运算符
 */

/* 前向声明：函数相互递归调用 */
static double parse_expression(Interpreter *interp);
static double parse_term(Interpreter *interp);
static double parse_power(Interpreter *interp);
static double parse_unary(Interpreter *interp);
static double parse_primary(Interpreter *interp);

/**
 * @brief 解析加减表达式 (最低优先级)
 *
 * 文法: expression → term (('+' | '-') term)*
 *
 * @param interp 解释器指针
 * @return 表达式的值
 *
 * 示例: "10 + 20 - 5" → 25
 *
 * 算法:
 *   1. 解析第一个 term
 *   2. 循环: 遇到 +/- 则解析下一个 term 并计算
 */
static double parse_expression(Interpreter *interp) {
    /* 解析第一个操作数 (更高优先级) */
    double result = parse_term(interp);

    /* 循环处理同级运算符 +, - */
    while (!interp->has_error &&
           (interp->current_token.type == TOKEN_PLUS ||
            interp->current_token.type == TOKEN_MINUS)) {

        TokenType op = interp->current_token.type;  /* 保存运算符 */
        advance_token(interp);                       /* 消费运算符 */

        double right = parse_term(interp);           /* 解析右操作数 */

        /* 执行运算 */
        if (op == TOKEN_PLUS) {
            result += right;
        } else {
            result -= right;
        }
    }

    return result;
}

/**
 * @brief 解析乘除模表达式
 *
 * 文法: term → power (('*' | '/' | '%') power)*
 *
 * @param interp 解释器指针
 * @return 表达式的值
 *
 * 示例: "6 * 7 / 2" → 21
 *
 * 注意:
 *   - 除零检查: 除数为 0 时报错
 *   - 浮点除法: 与编译器的整数除法不同
 */
static double parse_term(Interpreter *interp) {
    /* 解析第一个操作数 */
    double result = parse_power(interp);

    /* 循环处理 *, /, % */
    while (!interp->has_error &&
           (interp->current_token.type == TOKEN_STAR ||
            interp->current_token.type == TOKEN_SLASH ||
            interp->current_token.type == TOKEN_PERCENT)) {

        TokenType op = interp->current_token.type;
        advance_token(interp);

        double right = parse_power(interp);

        /* 执行运算 */
        if (op == TOKEN_STAR) {
            result *= right;
        } else if (op == TOKEN_SLASH) {
            /* 除零检查 */
            if (right == 0) {
                set_error(interp, "Division by zero");
                return 0;
            }
            result /= right;
        } else {  /* TOKEN_PERCENT */
            /* 取模也需要除零检查 */
            if (right == 0) {
                set_error(interp, "Modulo by zero");
                return 0;
            }
            result = fmod(result, right);  /* 浮点取模 */
        }
    }

    return result;
}

/**
 * @brief 解析幂运算表达式 (右结合)
 *
 * 文法: power → unary ('^' power)*  // 注意: 右结合用递归实现
 *
 * @param interp 解释器指针
 * @return 表达式的值
 *
 * 示例:
 *   "2 ^ 3"       → 8
 *   "2 ^ 3 ^ 2"   → 2^(3^2) = 2^9 = 512 (右结合!)
 *
 * 右结合实现:
 *   普通左结合: while 循环
 *   右结合:     递归调用 (先计算右边的 ^)
 */
static double parse_power(Interpreter *interp) {
    double result = parse_unary(interp);

    /* 幂运算是右结合的，用递归而非循环实现 */
    if (!interp->has_error && interp->current_token.type == TOKEN_CARET) {
        advance_token(interp);

        /* 递归调用自己，实现右结合
         * 2^3^2 → 2^(parse_power()) → 2^(3^2) → 2^9 = 512 */
        double right = parse_power(interp);

        result = pow(result, right);  /* 使用数学库的 pow 函数 */
    }

    return result;
}

/**
 * @brief 解析一元运算符
 *
 * 文法: unary → ('-' | '+')? unary | primary
 *
 * @param interp 解释器指针
 * @return 表达式的值
 *
 * 示例:
 *   "-5"   → -5
 *   "--5"  → 5 (双重否定)
 *   "+5"   → 5
 */
static double parse_unary(Interpreter *interp) {
    /* 一元负号 */
    if (interp->current_token.type == TOKEN_MINUS) {
        advance_token(interp);
        return -parse_unary(interp);  /* 递归处理连续的一元运算符 */
    }

    /* 一元正号 (可选，实际上不改变值) */
    if (interp->current_token.type == TOKEN_PLUS) {
        advance_token(interp);
        return parse_unary(interp);
    }

    /* 不是一元运算符，解析基本元素 */
    return parse_primary(interp);
}

/**
 * @brief 解析基本元素 (最高优先级)
 *
 * 文法: primary → NUMBER | FLOAT | IDENT | IDENT '(' expr ')' | '(' expr ')'
 *
 * @param interp 解释器指针
 * @return 表达式的值
 *
 * 支持的基本元素:
 *   1. 数字字面量: 123, 3.14
 *   2. 变量: a, b, x
 *   3. 数组元素: a(0), a(i)  -- 支持动态索引!
 *   4. 括号表达式: (expr)
 */
static double parse_primary(Interpreter *interp) {
    Token token = interp->current_token;

    /* ========== 数字字面量 ========== */
    if (token.type == TOKEN_NUMBER || token.type == TOKEN_FLOAT) {
        advance_token(interp);
        return token.num_value;  /* 直接返回词法分析时解析的数值 */
    }

    /* ========== 变量或数组元素 ========== */
    if (token.type == TOKEN_IDENT) {
        int idx = var_index(token.text[0]);
        if (idx < 0) {
            set_error(interp, "Invalid variable: %s", token.text);
            return 0;
        }
        advance_token(interp);

        /* 检查是否是数组访问 a(index) */
        if (interp->current_token.type == TOKEN_LPAREN) {
            advance_token(interp);  /* 消费 '(' */

            /* 解析数组索引表达式 (可以是变量!)
             * 这是解释器比编译器强大的地方 */
            int array_idx = (int)parse_expression(interp);

            if (!expect(interp, TOKEN_RPAREN)) return 0;
            advance_token(interp);  /* 消费 ')' */

            /* 边界检查 */
            if (array_idx < 0 || array_idx >= MAX_ARRAY_SIZE) {
                set_error(interp, "Array index out of bounds: %d", array_idx);
                return 0;
            }

            /* 返回数组元素的值 */
            return interp->arrays[idx].values[array_idx];
        }

        /* 普通变量 */
        if (!interp->variables[idx].initialized) {
            set_error(interp, "Uninitialized variable: %c", 'a' + idx);
            return 0;
        }
        return interp->variables[idx].value;
    }

    /* ========== 括号表达式 ========== */
    if (token.type == TOKEN_LPAREN) {
        advance_token(interp);  /* 消费 '(' */

        double result = parse_expression(interp);  /* 递归解析括号内的表达式 */

        if (!expect(interp, TOKEN_RPAREN)) return 0;
        advance_token(interp);  /* 消费 ')' */

        return result;
    }

    /* ========== 未知 Token ========== */
    set_error(interp, "Unexpected token in expression: %s", token.text);
    return 0;
}

/* ============================================================================
 *                              条件表达式解析
 * ============================================================================
 *
 * 条件表达式用于 if 语句: if expr1 <op> expr2 goto line
 * 其中 <op> 是关系运算符: ==, !=, <, >, <=, >=
 */

/**
 * @brief 解析条件表达式
 *
 * @param interp 解释器指针
 * @return 条件为真返回 1，为假返回 0
 *
 * 语法: expr1 <relop> expr2
 * 示例: x > 0, a == b, i <= 10
 */
static int parse_condition(Interpreter *interp) {
    /* 解析左操作数 */
    double left = parse_expression(interp);
    if (interp->has_error) return 0;

    /* 获取关系运算符 */
    TokenType op = interp->current_token.type;
    if (op != TOKEN_EQ && op != TOKEN_NE && op != TOKEN_LT &&
        op != TOKEN_GT && op != TOKEN_LE && op != TOKEN_GE) {
        set_error(interp, "Expected comparison operator");
        return 0;
    }
    advance_token(interp);

    /* 解析右操作数 */
    double right = parse_expression(interp);
    if (interp->has_error) return 0;

    /* 执行比较 */
    switch (op) {
        case TOKEN_EQ: return left == right;   /* == */
        case TOKEN_NE: return left != right;   /* != */
        case TOKEN_LT: return left < right;    /* <  */
        case TOKEN_GT: return left > right;    /* >  */
        case TOKEN_LE: return left <= right;   /* <= */
        case TOKEN_GE: return left >= right;   /* >= */
        default: return 0;
    }
}

/* ============================================================================
 *                              语句执行
 * ============================================================================
 *
 * 每种语句类型对应一个 exec_xxx 函数。
 * 函数负责:
 *   1. 解析语句的参数
 *   2. 执行语句的操作
 *   3. 更新解释器状态
 */

/**
 * @brief 执行 rem 语句 (注释)
 *
 * rem 语句用于添加注释，执行时直接忽略整行。
 * 语法: 行号 rem 任意文本
 *
 * @param interp 解释器指针
 */
static void exec_rem(Interpreter *interp) {
    (void)interp;  /* 显式忽略参数，避免编译器警告 */
    /* 注释什么都不做，跳过整行 */
}

/**
 * @brief 执行 input 语句
 *
 * 从标准输入读取值到变量或数组元素。
 * 语法: 行号 input var [, var ...]
 *
 * @param interp 解释器指针
 *
 * 示例:
 *   10 input x       -- 读取一个值到 x
 *   20 input a, b, c -- 读取三个值
 *   30 input a(0)    -- 读取到数组元素
 */
static void exec_input(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'input' 关键字 */

    /* 循环处理多个变量 (逗号分隔) */
    do {
        /* 跳过逗号 */
        if (interp->current_token.type == TOKEN_COMMA) {
            advance_token(interp);
        }

        /* 期望变量名 */
        if (interp->current_token.type != TOKEN_IDENT) {
            set_error(interp, "Expected variable name after 'input'");
            return;
        }

        int idx = var_index(interp->current_token.text[0]);
        if (idx < 0) {
            set_error(interp, "Invalid variable: %s", interp->current_token.text);
            return;
        }

        advance_token(interp);

        /* 检查是否是数组元素 */
        int array_idx = -1;
        if (interp->current_token.type == TOKEN_LPAREN) {
            advance_token(interp);
            array_idx = (int)parse_expression(interp);  /* 动态索引 */
            if (!expect(interp, TOKEN_RPAREN)) return;
            advance_token(interp);
        }

        /* 显示提示符，读取输入 */
        double value;
        printf("? ");       /* 传统 BASIC 风格的输入提示 */
        fflush(stdout);     /* 确保提示符立即显示 */

        if (scanf("%lf", &value) != 1) {
            set_error(interp, "Invalid input");
            return;
        }

        /* 存储值 */
        if (array_idx >= 0) {
            /* 数组元素 */
            if (array_idx >= MAX_ARRAY_SIZE) {
                set_error(interp, "Array index out of bounds");
                return;
            }
            interp->arrays[idx].values[array_idx] = value;
            interp->arrays[idx].initialized = 1;
        } else {
            /* 普通变量 */
            interp->variables[idx].value = value;
            interp->variables[idx].initialized = 1;
        }

    } while (interp->current_token.type == TOKEN_COMMA);
}

/**
 * @brief 执行 print 语句
 *
 * 输出表达式值或字符串到标准输出。
 * 语法: 行号 print expr|"string" [, expr|"string" ...]
 *
 * @param interp 解释器指针
 *
 * 示例:
 *   10 print x           -- 输出变量值
 *   20 print "Hello"     -- 输出字符串
 *   30 print "x=", x     -- 混合输出
 *   40 print             -- 只输出换行
 */
static void exec_print(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'print' */

    int first = 1;  /* 标记是否是第一个输出项 */

    do {
        /* 处理逗号分隔符 */
        if (interp->current_token.type == TOKEN_COMMA) {
            advance_token(interp);
            first = 0;
        }

        /* 多个输出项之间用空格分隔 */
        if (!first) {
            printf(" ");
        }
        first = 0;

        /* 根据 Token 类型决定输出方式 */
        if (interp->current_token.type == TOKEN_STRING) {
            /* 输出字符串 (去掉首尾引号) */
            char *str = interp->current_token.text;
            int len = strlen(str);
            if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
                printf("%.*s", len - 2, str + 1);  /* 跳过引号 */
            } else {
                printf("%s", str);
            }
            advance_token(interp);

        } else if (interp->current_token.type == TOKEN_NEWLINE ||
                   interp->current_token.type == TOKEN_EOF) {
            /* 空 print 或行末 */
            break;

        } else {
            /* 输出表达式的值 */
            double value = parse_expression(interp);
            if (interp->has_error) return;

            /* 智能格式化: 整数不显示小数点 */
            if (value == (int)value) {
                printf("%d", (int)value);
            } else {
                printf("%g", value);  /* %g 自动选择最短格式 */
            }
        }

    } while (interp->current_token.type == TOKEN_COMMA);

    printf("\n");  /* 每个 print 语句后输出换行 */
}

/**
 * @brief 执行 let 语句 (赋值)
 *
 * 将表达式的值赋给变量或数组元素。
 * 语法: 行号 let var = expr
 *       行号 let var(index) = expr
 *
 * @param interp 解释器指针
 *
 * 示例:
 *   10 let x = 10
 *   20 let y = x * 2 + 1
 *   30 let a(0) = 100
 *   40 let a(i) = a(i-1) + 1   -- 支持动态索引!
 */
static void exec_let(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'let' */

    /* 获取目标变量 */
    if (interp->current_token.type != TOKEN_IDENT) {
        set_error(interp, "Expected variable name after 'let'");
        return;
    }

    int idx = var_index(interp->current_token.text[0]);
    if (idx < 0) {
        set_error(interp, "Invalid variable: %s", interp->current_token.text);
        return;
    }
    advance_token(interp);

    /* 检查是否是数组赋值 */
    int array_idx = -1;
    if (interp->current_token.type == TOKEN_LPAREN) {
        advance_token(interp);
        array_idx = (int)parse_expression(interp);  /* 动态索引 */
        if (!expect(interp, TOKEN_RPAREN)) return;
        advance_token(interp);
    }

    /* 期望 '=' */
    if (!expect(interp, TOKEN_ASSIGN)) return;
    advance_token(interp);

    /* 解析右侧表达式 */
    double value = parse_expression(interp);
    if (interp->has_error) return;

    /* 存储结果 */
    if (array_idx >= 0) {
        /* 数组元素 */
        if (array_idx < 0 || array_idx >= MAX_ARRAY_SIZE) {
            set_error(interp, "Array index out of bounds: %d", array_idx);
            return;
        }
        interp->arrays[idx].values[array_idx] = value;
        interp->arrays[idx].initialized = 1;
    } else {
        /* 普通变量 */
        interp->variables[idx].value = value;
        interp->variables[idx].initialized = 1;
    }
}

/**
 * @brief 执行 goto 语句 (无条件跳转)
 *
 * 跳转到指定行号继续执行。
 * 语法: 行号 goto 目标行号
 *
 * @param interp 解释器指针
 *
 * 实现细节:
 *   - 查找目标行号在行索引表中的位置
 *   - 设置 current_line_index 为目标位置 - 1
 *   - -1 是因为主循环会自动 +1
 */
static void exec_goto(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'goto' */

    /* 获取目标行号 */
    if (interp->current_token.type != TOKEN_NUMBER) {
        set_error(interp, "Expected line number after 'goto'");
        return;
    }

    int target_line = (int)interp->current_token.num_value;
    int target_index = find_line_index(interp, target_line);

    if (target_index < 0) {
        set_error(interp, "Line %d not found", target_line);
        return;
    }

    /* 设置跳转目标 (-1 因为主循环会 +1) */
    interp->current_line_index = target_index - 1;
}

/**
 * @brief 执行 if 语句 (条件跳转)
 *
 * 如果条件为真，跳转到指定行号。
 * 语法: 行号 if 条件 goto 目标行号
 *
 * @param interp 解释器指针
 *
 * 示例:
 *   10 if x > 0 goto 100      -- 如果 x > 0，跳转到行 100
 *   20 if a == b goto 50      -- 如果 a == b，跳转到行 50
 */
static void exec_if(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'if' */

    /* 解析条件表达式 */
    int condition = parse_condition(interp);
    if (interp->has_error) return;

    /* 期望 'goto' */
    if (interp->current_token.type != TOKEN_GOTO) {
        set_error(interp, "Expected 'goto' in if statement");
        return;
    }
    advance_token(interp);

    /* 获取目标行号 */
    if (interp->current_token.type != TOKEN_NUMBER) {
        set_error(interp, "Expected line number after 'goto'");
        return;
    }

    /* 只有条件为真时才跳转 */
    if (condition) {
        int target_line = (int)interp->current_token.num_value;
        int target_index = find_line_index(interp, target_line);

        if (target_index < 0) {
            set_error(interp, "Line %d not found", target_line);
            return;
        }

        interp->current_line_index = target_index - 1;
    }
}

/**
 * @brief 执行 for 语句 (循环开始)
 *
 * 开始一个计数循环。
 * 语法: 行号 for var = start to end [step value]
 *
 * @param interp 解释器指针
 *
 * 示例:
 *   10 for i = 1 to 10          -- 默认步长 1
 *   20 for j = 10 to 1 step -1  -- 负步长倒计数
 *   30 for k = 0 to 100 step 5  -- 步长 5
 *
 * 实现:
 *   1. 初始化循环变量
 *   2. 检查是否需要执行循环 (起始值与结束值的关系)
 *   3. 如果需要执行，将循环状态压入 for 栈
 *   4. 如果不需要执行，跳过循环体 (找到对应的 next)
 */
static void exec_for(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'for' */

    /* 获取循环变量 */
    if (interp->current_token.type != TOKEN_IDENT) {
        set_error(interp, "Expected variable after 'for'");
        return;
    }
    char loop_var = interp->current_token.text[0];
    int idx = var_index(loop_var);
    if (idx < 0) {
        set_error(interp, "Invalid loop variable");
        return;
    }
    advance_token(interp);

    /* 期望 '=' */
    if (!expect(interp, TOKEN_ASSIGN)) return;
    advance_token(interp);

    /* 解析起始值 */
    double start_value = parse_expression(interp);
    if (interp->has_error) return;

    /* 期望 'to' */
    if (interp->current_token.type != TOKEN_TO) {
        set_error(interp, "Expected 'to' in for statement");
        return;
    }
    advance_token(interp);

    /* 解析结束值 */
    double end_value = parse_expression(interp);
    if (interp->has_error) return;

    /* 检查可选的 'step'，默认步长为 1 */
    double step = 1.0;
    if (interp->current_token.type == TOKEN_STEP) {
        advance_token(interp);
        step = parse_expression(interp);
        if (interp->has_error) return;
    }

    /* 初始化循环变量 */
    interp->variables[idx].value = start_value;
    interp->variables[idx].initialized = 1;

    /* 检查是否需要执行循环
     * 正步长: start <= end
     * 负步长: start >= end */
    int should_loop = (step > 0) ? (start_value <= end_value)
                                 : (start_value >= end_value);

    if (should_loop) {
        /* 循环需要执行，压入状态栈 */
        if (interp->for_depth >= MAX_FOR_DEPTH) {
            set_error(interp, "For loop nested too deep");
            return;
        }

        ForState *state = &interp->for_stack[interp->for_depth++];
        state->var = loop_var;
        state->end_value = end_value;
        state->step = step;
        state->loop_start = interp->lines[interp->current_line_index + 1].start;
        state->next_line_index = -1;  /* 稍后在 next 中设置 */

    } else {
        /* 循环条件不满足，跳过循环体
         * 需要找到对应的 next 语句 */
        int depth = 1;  /* 处理嵌套循环 */
        for (int i = interp->current_line_index + 1; i < interp->line_count && depth > 0; i++) {
            /* 临时解析每行，查找 for/next */
            lexer_reset_line(&interp->lexer, interp->lines[i].start);
            advance_token(interp);  /* 跳过行号 */
            if (interp->current_token.type == TOKEN_NUMBER) {
                advance_token(interp);
            }

            if (interp->current_token.type == TOKEN_FOR) {
                depth++;  /* 进入嵌套循环 */
            } else if (interp->current_token.type == TOKEN_NEXT) {
                depth--;  /* 退出循环 */
                if (depth == 0) {
                    interp->current_line_index = i;  /* 跳转到 next 行 */
                    break;
                }
            }
        }
    }
}

/**
 * @brief 执行 next 语句 (循环结束)
 *
 * 结束当前循环迭代，检查是否继续循环。
 * 语法: 行号 next var
 *
 * @param interp 解释器指针
 *
 * 实现:
 *   1. 验证循环变量匹配
 *   2. 循环变量 += 步长
 *   3. 检查是否继续循环
 *   4. 如果继续，跳回循环体开始
 *   5. 如果结束，弹出 for 栈
 */
static void exec_next(Interpreter *interp) {
    advance_token(interp);  /* 跳过 'next' */

    /* 获取循环变量 */
    if (interp->current_token.type != TOKEN_IDENT) {
        set_error(interp, "Expected variable after 'next'");
        return;
    }
    char loop_var = interp->current_token.text[0];
    int idx = var_index(loop_var);

    /* 检查 for 栈 */
    if (interp->for_depth == 0) {
        set_error(interp, "next without for");
        return;
    }

    ForState *state = &interp->for_stack[interp->for_depth - 1];

    /* 验证变量匹配 */
    if (state->var != loop_var) {
        set_error(interp, "next variable mismatch");
        return;
    }

    /* 更新循环变量 */
    interp->variables[idx].value += state->step;
    double current = interp->variables[idx].value;

    /* 检查是否继续循环
     * 正步长: current <= end
     * 负步长: current >= end */
    int should_continue = (state->step > 0) ? (current <= state->end_value)
                                            : (current >= state->end_value);

    if (should_continue) {
        /* 继续循环，跳回循环体开始 */
        for (int i = 0; i < interp->line_count; i++) {
            if (interp->lines[i].start == state->loop_start) {
                interp->current_line_index = i - 1;  /* -1 因为主循环会 +1 */
                break;
            }
        }
    } else {
        /* 循环结束，弹出状态 */
        interp->for_depth--;
    }
}

/**
 * @brief 执行 end 语句 (程序结束)
 *
 * 停止程序执行。
 * 语法: 行号 end
 *
 * @param interp 解释器指针
 */
static void exec_end(Interpreter *interp) {
    interp->running = 0;  /* 设置停止标志 */
}

/* ============================================================================
 *                              主执行逻辑
 * ============================================================================ */

/**
 * @brief 执行单行语句
 *
 * 解析并执行一行 Simple 代码。
 *
 * @param interp 解释器指针
 *
 * 流程:
 *   1. 重置词法分析器到当前行
 *   2. 跳过行号
 *   3. 根据关键字分发到对应的 exec_xxx 函数
 */
static void execute_line(Interpreter *interp) {
    LineInfo *line = &interp->lines[interp->current_line_index];

    /* 重置词法分析器到当前行 */
    lexer_reset_line(&interp->lexer, line->start);
    advance_token(interp);

    /* 跳过行号 */
    if (interp->current_token.type == TOKEN_NUMBER) {
        advance_token(interp);
    }

    /* 根据关键字执行相应语句 */
    switch (interp->current_token.type) {
        case TOKEN_REM:   exec_rem(interp);   break;
        case TOKEN_INPUT: exec_input(interp); break;
        case TOKEN_PRINT: exec_print(interp); break;
        case TOKEN_LET:   exec_let(interp);   break;
        case TOKEN_GOTO:  exec_goto(interp);  break;
        case TOKEN_IF:    exec_if(interp);    break;
        case TOKEN_FOR:   exec_for(interp);   break;
        case TOKEN_NEXT:  exec_next(interp);  break;
        case TOKEN_END:   exec_end(interp);   break;
        case TOKEN_NEWLINE:
        case TOKEN_EOF:
            /* 空行，什么都不做 */
            break;
        default:
            set_error(interp, "Unknown statement: %s", interp->current_token.text);
            break;
    }
}

/* ============================================================================
 *                              公开 API
 * ============================================================================ */

/**
 * @brief 初始化解释器
 *
 * 将解释器所有字段清零。
 *
 * @param interp 解释器指针
 */
void interpreter_init(Interpreter *interp) {
    memset(interp, 0, sizeof(Interpreter));
}

/**
 * @brief 从字符串加载源代码
 *
 * 解析源代码，建立行号索引表。
 *
 * @param interp 解释器指针
 * @param source 源代码字符串
 * @return 成功返回 1，失败返回 0
 *
 * 行号索引表的作用:
 *   - 快速定位 goto/if 的目标行
 *   - 支持乱序执行 (按行号顺序，而非物理顺序)
 */
int interpreter_load(Interpreter *interp, const char *source) {
    /* 复制源代码 (需要在整个执行期间保持有效) */
    interp->source = strdup(source);
    if (!interp->source) {
        set_error(interp, "Memory allocation failed");
        return 0;
    }

    /* 初始化词法分析器 */
    lexer_init(&interp->lexer, interp->source);
    interp->line_count = 0;

    /* 扫描源代码，建立行号索引表 */
    const char *line_start = interp->source;
    while (*line_start) {
        /* 跳过行首空白 */
        while (*line_start == ' ' || *line_start == '\t') {
            line_start++;
        }

        /* 跳过空行 */
        if (*line_start == '\0' || *line_start == '\n') {
            if (*line_start == '\n') line_start++;
            continue;
        }

        /* 解析行号 */
        lexer_reset_line(&interp->lexer, line_start);
        Token token = lexer_next_token(&interp->lexer);

        if (token.type == TOKEN_NUMBER) {
            /* 检查行数限制 */
            if (interp->line_count >= MAX_LINES) {
                set_error(interp, "Too many lines");
                return 0;
            }

            /* 记录行号和位置 */
            interp->lines[interp->line_count].line_number = (int)token.num_value;
            interp->lines[interp->line_count].start = line_start;
            interp->line_count++;
        }

        /* 找到下一行 */
        while (*line_start && *line_start != '\n') {
            line_start++;
        }
        if (*line_start == '\n') {
            line_start++;
        }
    }

    return 1;
}

/**
 * @brief 从文件加载源代码
 *
 * @param interp   解释器指针
 * @param filename 文件路径
 * @return 成功返回 1，失败返回 0
 */
int interpreter_load_file(Interpreter *interp, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        set_error(interp, "Cannot open file: %s", filename);
        return 0;
    }

    /* 获取文件大小 */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* 读取文件内容 */
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        set_error(interp, "Memory allocation failed");
        return 0;
    }

    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);

    /* 加载到解释器 */
    int result = interpreter_load(interp, content);
    free(content);  /* 内容已被复制，可以释放 */
    return result;
}

/**
 * @brief 执行程序
 *
 * 从第一行开始，按行号顺序执行程序。
 *
 * @param interp 解释器指针
 * @return 成功返回 1，出错返回 0
 */
int interpreter_run(Interpreter *interp) {
    interp->running = 1;
    interp->current_line_index = 0;
    interp->has_error = 0;

    /* 主执行循环 */
    while (interp->running && interp->current_line_index < interp->line_count) {
        execute_line(interp);

        if (interp->has_error) {
            return 0;  /* 执行出错 */
        }

        interp->current_line_index++;  /* 前进到下一行 */
    }

    return 1;
}

/**
 * @brief 释放解释器资源
 *
 * @param interp 解释器指针
 */
void interpreter_free(Interpreter *interp) {
    if (interp->source) {
        free(interp->source);
        interp->source = NULL;
    }
}

/**
 * @brief 获取错误信息
 *
 * @param interp 解释器指针
 * @return 错误信息字符串
 */
const char* interpreter_get_error(Interpreter *interp) {
    return interp->error_message;
}
