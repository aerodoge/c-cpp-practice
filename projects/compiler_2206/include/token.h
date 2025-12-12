#ifndef TOKEN_H
#define TOKEN_H

/**
 * @file token.h
 * @brief Simple 语言词法单元 (Token) 定义
 *
 * 本文件定义了 Simple 语言词法分析阶段产生的所有 Token 类型。
 * Token 是源代码经过词法分析后的最小语义单元。
 *
 * Simple 语言支持:
 * - 字面量: 整数、浮点数、字符串
 * - 关键字: rem, input, print, let, goto, if, for, to, step, next, end
 * - 运算符: +, -, *, /, %, ^ (算术), =, ==, !=, <, >, <=, >= (关系)
 * - 分隔符: (, ), ,
 *
 * @see lexer.h 词法分析器
 */

typedef enum {
    // 特殊标记
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_NEWLINE,

    // 字面量
    TOKEN_NUMBER,      // 整数: 123
    TOKEN_FLOAT,       // 浮点数: 3.14
    TOKEN_STRING,      // 字符串: "hello"
    TOKEN_IDENT,       // 标识符 (变量名): a-z

    // 关键字
    TOKEN_REM,         // rem (注释)
    TOKEN_INPUT,       // input
    TOKEN_PRINT,       // print
    TOKEN_LET,         // let
    TOKEN_GOTO,        // goto
    TOKEN_IF,          // if
    TOKEN_FOR,         // for
    TOKEN_TO,          // to
    TOKEN_STEP,        // step
    TOKEN_NEXT,        // next
    TOKEN_END,         // end

    // 运算符
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_STAR,        // *
    TOKEN_SLASH,       // /
    TOKEN_PERCENT,     // %
    TOKEN_CARET,       // ^
    TOKEN_ASSIGN,      // =

    // 关系运算符
    TOKEN_EQ,          // ==
    TOKEN_NE,          // !=
    TOKEN_LT,          // <
    TOKEN_GT,          // >
    TOKEN_LE,          // <=
    TOKEN_GE,          // >=

    // 分隔符
    TOKEN_COMMA,       // ,
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
} TokenType;

/* Token 结构 */
typedef struct {
    TokenType type;
    char text[256];       // 原始文本
    double num_value;     // 数值 (如果是数字)
    int line;             // 行号
    int column;           // 列号
} Token;

/* Token 类型名称 (用于调试) */
const char* token_type_name(TokenType type);

#endif /* TOKEN_H */
