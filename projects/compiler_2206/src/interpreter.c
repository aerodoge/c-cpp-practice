/**
 * @file interpreter.c
 * @brief Simple 语言解释器实现
 *
 * 采用"边解析边执行"策略，无需生成中间代码。
 *
 * 执行流程：
 *   1. 加载源码，建立行号索引表
 *   2. 从第一行开始，按行号顺序执行
 *   3. 遇到 goto/if 时跳转到目标行
 *   4. 遇到 end 或错误时停止
 *
 * 与编译器的区别：
 *   - 支持浮点数运算 (double)
 *   - 支持动态数组索引 a(i)
 *   - 无内存限制
 *   - 执行速度较慢
 *
 * 表达式解析采用递归下降：
 *   expression → term (('+' | '-') term)*
 *   term       → power (('*' | '/' | '%') power)*
 *   power      → unary ('^' unary)*
 *   unary      → '-' unary | primary
 *   primary    → NUMBER | IDENT | IDENT '(' expr ')' | '(' expr ')'
 */

#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

/* ======================== 辅助函数 ======================== */

/* 变量名转索引 (a=0, b=1, ..., z=25) */
static int var_index(char c) {
    c = tolower(c);
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }
    return -1;
}

/* 设置错误 */
static void set_error(Interpreter *interp, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(interp->error_message, sizeof(interp->error_message), format, args);
    va_end(args);
    interp->has_error = 1;
    interp->running = 0;
}

/* 获取下一个 token */
static void advance_token(Interpreter *interp) {
    interp->current_token = lexer_next_token(&interp->lexer);
}

/* 期望某个 token */
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

/* 查找行号对应的索引 */
static int find_line_index(Interpreter *interp, int line_number) {
    for (int i = 0; i < interp->line_count; i++) {
        if (interp->lines[i].line_number == line_number) {
            return i;
        }
    }
    return -1;
}

/* ======================== 表达式解析 ======================== */

static double parse_expression(Interpreter *interp);
static double parse_term(Interpreter *interp);
static double parse_power(Interpreter *interp);
static double parse_unary(Interpreter *interp);
static double parse_primary(Interpreter *interp);

/* 表达式: term ((+|-) term)* */
static double parse_expression(Interpreter *interp) {
    double result = parse_term(interp);

    while (!interp->has_error &&
           (interp->current_token.type == TOKEN_PLUS ||
            interp->current_token.type == TOKEN_MINUS)) {
        TokenType op = interp->current_token.type;
        advance_token(interp);
        double right = parse_term(interp);
        if (op == TOKEN_PLUS) {
            result += right;
        } else {
            result -= right;
        }
    }

    return result;
}

/* term: factor ((*|/|%) factor)* */
static double parse_term(Interpreter *interp) {
    double result = parse_power(interp);

    while (!interp->has_error &&
           (interp->current_token.type == TOKEN_STAR ||
            interp->current_token.type == TOKEN_SLASH ||
            interp->current_token.type == TOKEN_PERCENT)) {
        TokenType op = interp->current_token.type;
        advance_token(interp);
        double right = parse_power(interp);
        if (op == TOKEN_STAR) {
            result *= right;
        } else if (op == TOKEN_SLASH) {
            if (right == 0) {
                set_error(interp, "Division by zero");
                return 0;
            }
            result /= right;
        } else {
            if (right == 0) {
                set_error(interp, "Modulo by zero");
                return 0;
            }
            result = fmod(result, right);
        }
    }

    return result;
}

/* power: unary (^ unary)* (右结合) */
static double parse_power(Interpreter *interp) {
    double result = parse_unary(interp);

    if (!interp->has_error && interp->current_token.type == TOKEN_CARET) {
        advance_token(interp);
        double right = parse_power(interp);  // 右结合
        result = pow(result, right);
    }

    return result;
}

/* unary: (-|+)? primary */
static double parse_unary(Interpreter *interp) {
    if (interp->current_token.type == TOKEN_MINUS) {
        advance_token(interp);
        return -parse_unary(interp);
    }
    if (interp->current_token.type == TOKEN_PLUS) {
        advance_token(interp);
        return parse_unary(interp);
    }
    return parse_primary(interp);
}

/* primary: NUMBER | IDENT | IDENT(expr) | (expr) */
static double parse_primary(Interpreter *interp) {
    Token token = interp->current_token;

    if (token.type == TOKEN_NUMBER || token.type == TOKEN_FLOAT) {
        advance_token(interp);
        return token.num_value;
    }

    if (token.type == TOKEN_IDENT) {
        int idx = var_index(token.text[0]);
        if (idx < 0) {
            set_error(interp, "Invalid variable: %s", token.text);
            return 0;
        }
        advance_token(interp);

        // 检查是否是数组访问
        if (interp->current_token.type == TOKEN_LPAREN) {
            advance_token(interp);  // 消费 '('
            int array_idx = (int)parse_expression(interp);
            if (!expect(interp, TOKEN_RPAREN)) return 0;
            advance_token(interp);  // 消费 ')'

            if (array_idx < 0 || array_idx >= MAX_ARRAY_SIZE) {
                set_error(interp, "Array index out of bounds: %d", array_idx);
                return 0;
            }
            return interp->arrays[idx].values[array_idx];
        }

        // 普通变量
        if (!interp->variables[idx].initialized) {
            set_error(interp, "Uninitialized variable: %c", 'a' + idx);
            return 0;
        }
        return interp->variables[idx].value;
    }

    if (token.type == TOKEN_LPAREN) {
        advance_token(interp);  // 消费 '('
        double result = parse_expression(interp);
        if (!expect(interp, TOKEN_RPAREN)) return 0;
        advance_token(interp);  // 消费 ')'
        return result;
    }

    set_error(interp, "Unexpected token in expression: %s", token.text);
    return 0;
}

/* ======================== 条件表达式 ======================== */

/* 解析关系运算符 */
static int parse_condition(Interpreter *interp) {
    double left = parse_expression(interp);
    if (interp->has_error) return 0;

    TokenType op = interp->current_token.type;
    if (op != TOKEN_EQ && op != TOKEN_NE && op != TOKEN_LT &&
        op != TOKEN_GT && op != TOKEN_LE && op != TOKEN_GE) {
        set_error(interp, "Expected comparison operator");
        return 0;
    }
    advance_token(interp);

    double right = parse_expression(interp);
    if (interp->has_error) return 0;

    switch (op) {
        case TOKEN_EQ: return left == right;
        case TOKEN_NE: return left != right;
        case TOKEN_LT: return left < right;
        case TOKEN_GT: return left > right;
        case TOKEN_LE: return left <= right;
        case TOKEN_GE: return left >= right;
        default: return 0;
    }
}

/* ======================== 语句执行 ======================== */

/* rem: 注释，跳过整行 */
static void exec_rem(Interpreter *interp) {
    (void)interp;  // 什么都不做，跳过整行
}

/* input: 读取变量 */
static void exec_input(Interpreter *interp) {
    advance_token(interp);  // 跳过 'input'

    do {
        if (interp->current_token.type == TOKEN_COMMA) {
            advance_token(interp);
        }

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

        // 检查是否是数组
        int array_idx = -1;
        if (interp->current_token.type == TOKEN_LPAREN) {
            advance_token(interp);
            array_idx = (int)parse_expression(interp);
            if (!expect(interp, TOKEN_RPAREN)) return;
            advance_token(interp);
        }

        // 读取输入
        double value;
        printf("? ");
        fflush(stdout);
        if (scanf("%lf", &value) != 1) {
            set_error(interp, "Invalid input");
            return;
        }

        // 存储值
        if (array_idx >= 0) {
            if (array_idx >= MAX_ARRAY_SIZE) {
                set_error(interp, "Array index out of bounds");
                return;
            }
            interp->arrays[idx].values[array_idx] = value;
            interp->arrays[idx].initialized = 1;
        } else {
            interp->variables[idx].value = value;
            interp->variables[idx].initialized = 1;
        }

    } while (interp->current_token.type == TOKEN_COMMA);
}

/* print: 输出 */
static void exec_print(Interpreter *interp) {
    advance_token(interp);  // 跳过 'print'

    int first = 1;
    do {
        if (interp->current_token.type == TOKEN_COMMA) {
            advance_token(interp);
            first = 0;
        }

        if (!first) {
            printf(" ");  // 多个值之间用空格分隔
        }
        first = 0;

        if (interp->current_token.type == TOKEN_STRING) {
            // 输出字符串 (去掉引号)
            char *str = interp->current_token.text;
            int len = strlen(str);
            if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
                printf("%.*s", len - 2, str + 1);
            } else {
                printf("%s", str);
            }
            advance_token(interp);
        } else if (interp->current_token.type == TOKEN_NEWLINE ||
                   interp->current_token.type == TOKEN_EOF) {
            // 没有参数，只打印换行
            break;
        } else {
            // 输出表达式的值
            double value = parse_expression(interp);
            if (interp->has_error) return;

            // 如果是整数，不显示小数点
            if (value == (int)value) {
                printf("%d", (int)value);
            } else {
                printf("%g", value);
            }
        }
    } while (interp->current_token.type == TOKEN_COMMA);

    printf("\n");
}

/* let: 赋值 */
static void exec_let(Interpreter *interp) {
    advance_token(interp);  // 跳过 'let'

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

    // 检查是否是数组赋值
    int array_idx = -1;
    if (interp->current_token.type == TOKEN_LPAREN) {
        advance_token(interp);
        array_idx = (int)parse_expression(interp);
        if (!expect(interp, TOKEN_RPAREN)) return;
        advance_token(interp);
    }

    // 期望 '='
    if (!expect(interp, TOKEN_ASSIGN)) return;
    advance_token(interp);

    // 解析表达式
    double value = parse_expression(interp);
    if (interp->has_error) return;

    // 存储值
    if (array_idx >= 0) {
        if (array_idx < 0 || array_idx >= MAX_ARRAY_SIZE) {
            set_error(interp, "Array index out of bounds: %d", array_idx);
            return;
        }
        interp->arrays[idx].values[array_idx] = value;
        interp->arrays[idx].initialized = 1;
    } else {
        interp->variables[idx].value = value;
        interp->variables[idx].initialized = 1;
    }
}

/* goto: 无条件跳转 */
static void exec_goto(Interpreter *interp) {
    advance_token(interp);  // 跳过 'goto'

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

    interp->current_line_index = target_index - 1;  // -1 因为主循环会 +1
}

/* if: 条件跳转 */
static void exec_if(Interpreter *interp) {
    advance_token(interp);  // 跳过 'if'

    int condition = parse_condition(interp);
    if (interp->has_error) return;

    // 期望 'goto'
    if (interp->current_token.type != TOKEN_GOTO) {
        set_error(interp, "Expected 'goto' in if statement");
        return;
    }
    advance_token(interp);

    // 获取目标行号
    if (interp->current_token.type != TOKEN_NUMBER) {
        set_error(interp, "Expected line number after 'goto'");
        return;
    }

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

/* for: 循环开始 */
static void exec_for(Interpreter *interp) {
    advance_token(interp);  // 跳过 'for'

    // 获取循环变量
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

    // 期望 '='
    if (!expect(interp, TOKEN_ASSIGN)) return;
    advance_token(interp);

    // 获取起始值
    double start_value = parse_expression(interp);
    if (interp->has_error) return;

    // 期望 'to'
    if (interp->current_token.type != TOKEN_TO) {
        set_error(interp, "Expected 'to' in for statement");
        return;
    }
    advance_token(interp);

    // 获取结束值
    double end_value = parse_expression(interp);
    if (interp->has_error) return;

    // 检查可选的 'step'
    double step = 1.0;
    if (interp->current_token.type == TOKEN_STEP) {
        advance_token(interp);
        step = parse_expression(interp);
        if (interp->has_error) return;
    }

    // 设置循环变量初始值
    interp->variables[idx].value = start_value;
    interp->variables[idx].initialized = 1;

    // 检查是否需要执行循环
    int should_loop = (step > 0) ? (start_value <= end_value)
                                 : (start_value >= end_value);

    if (should_loop) {
        // 压入 for 栈
        if (interp->for_depth >= MAX_FOR_DEPTH) {
            set_error(interp, "For loop nested too deep");
            return;
        }
        ForState *state = &interp->for_stack[interp->for_depth++];
        state->var = loop_var;
        state->end_value = end_value;
        state->step = step;
        state->loop_start = interp->lines[interp->current_line_index + 1].start;
        state->next_line_index = -1;  // 稍后设置
    } else {
        // 跳过循环体，找到对应的 next
        int depth = 1;
        for (int i = interp->current_line_index + 1; i < interp->line_count && depth > 0; i++) {
            lexer_reset_line(&interp->lexer, interp->lines[i].start);
            advance_token(interp);  // 跳过行号
            if (interp->current_token.type == TOKEN_NUMBER) {
                advance_token(interp);
            }
            if (interp->current_token.type == TOKEN_FOR) {
                depth++;
            } else if (interp->current_token.type == TOKEN_NEXT) {
                depth--;
                if (depth == 0) {
                    interp->current_line_index = i;
                    break;
                }
            }
        }
    }
}

/* next: 循环结束 */
static void exec_next(Interpreter *interp) {
    advance_token(interp);  // 跳过 'next'

    // 获取循环变量
    if (interp->current_token.type != TOKEN_IDENT) {
        set_error(interp, "Expected variable after 'next'");
        return;
    }
    char loop_var = interp->current_token.text[0];
    int idx = var_index(loop_var);

    // 查找对应的 for
    if (interp->for_depth == 0) {
        set_error(interp, "next without for");
        return;
    }

    ForState *state = &interp->for_stack[interp->for_depth - 1];
    if (state->var != loop_var) {
        set_error(interp, "next variable mismatch");
        return;
    }

    // 增加循环变量
    interp->variables[idx].value += state->step;
    double current = interp->variables[idx].value;

    // 检查是否继续循环
    int should_continue = (state->step > 0) ? (current <= state->end_value)
                                            : (current >= state->end_value);

    if (should_continue) {
        // 回到循环体开始
        for (int i = 0; i < interp->line_count; i++) {
            if (interp->lines[i].start == state->loop_start) {
                interp->current_line_index = i - 1;
                break;
            }
        }
    } else {
        // 结束循环
        interp->for_depth--;
    }
}

/* end: 结束程序 */
static void exec_end(Interpreter *interp) {
    interp->running = 0;
}

/* ======================== 主执行逻辑 ======================== */

/* 执行单行 */
static void execute_line(Interpreter *interp) {
    LineInfo *line = &interp->lines[interp->current_line_index];
    lexer_reset_line(&interp->lexer, line->start);
    advance_token(interp);

    // 跳过行号
    if (interp->current_token.type == TOKEN_NUMBER) {
        advance_token(interp);
    }

    // 根据关键字执行
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
            // 空行
            break;
        default:
            set_error(interp, "Unknown statement: %s", interp->current_token.text);
            break;
    }
}

/* ======================== 公开 API ======================== */

void interpreter_init(Interpreter *interp) {
    memset(interp, 0, sizeof(Interpreter));
}

int interpreter_load(Interpreter *interp, const char *source) {
    // 复制源代码
    interp->source = strdup(source);
    if (!interp->source) {
        set_error(interp, "Memory allocation failed");
        return 0;
    }

    // 解析行信息
    lexer_init(&interp->lexer, interp->source);
    interp->line_count = 0;

    const char *line_start = interp->source;
    while (*line_start) {
        // 跳过空白
        while (*line_start == ' ' || *line_start == '\t') {
            line_start++;
        }

        if (*line_start == '\0' || *line_start == '\n') {
            if (*line_start == '\n') line_start++;
            continue;
        }

        // 解析行号
        lexer_reset_line(&interp->lexer, line_start);
        Token token = lexer_next_token(&interp->lexer);

        if (token.type == TOKEN_NUMBER) {
            if (interp->line_count >= MAX_LINES) {
                set_error(interp, "Too many lines");
                return 0;
            }
            interp->lines[interp->line_count].line_number = (int)token.num_value;
            interp->lines[interp->line_count].start = line_start;
            interp->line_count++;
        }

        // 找到下一行
        while (*line_start && *line_start != '\n') {
            line_start++;
        }
        if (*line_start == '\n') {
            line_start++;
        }
    }

    return 1;
}

int interpreter_load_file(Interpreter *interp, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        set_error(interp, "Cannot open file: %s", filename);
        return 0;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 读取内容
    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        set_error(interp, "Memory allocation failed");
        return 0;
    }

    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);

    int result = interpreter_load(interp, content);
    free(content);
    return result;
}

int interpreter_run(Interpreter *interp) {
    interp->running = 1;
    interp->current_line_index = 0;
    interp->has_error = 0;

    while (interp->running && interp->current_line_index < interp->line_count) {
        execute_line(interp);
        if (interp->has_error) {
            return 0;
        }
        interp->current_line_index++;
    }

    return 1;
}

void interpreter_free(Interpreter *interp) {
    if (interp->source) {
        free(interp->source);
        interp->source = NULL;
    }
}

const char* interpreter_get_error(Interpreter *interp) {
    return interp->error_message;
}
