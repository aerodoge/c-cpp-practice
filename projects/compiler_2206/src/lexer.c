/**
 * @file lexer.c
 * @brief Simple 语言词法分析器实现
 *
 * 实现了一个手写的递归下降词法分析器，主要功能：
 * - 单字符运算符: +, -, *, /, %, ^, (, ), ,
 * - 双字符运算符: ==, !=, <=, >=
 * - 数字字面量: 整数和浮点数
 * - 字符串字面量: 双引号包围
 * - 标识符和关键字: 大小写不敏感
 */

#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* ==================== Token 工具函数 ==================== */

/** Token 类型名称表 (用于调试输出) */
const char* token_type_name(TokenType type) {
    static const char *names[] = {
        [TOKEN_EOF] = "EOF",
        [TOKEN_ERROR] = "ERROR",
        [TOKEN_NEWLINE] = "NEWLINE",
        [TOKEN_NUMBER] = "NUMBER",
        [TOKEN_FLOAT] = "FLOAT",
        [TOKEN_STRING] = "STRING",
        [TOKEN_IDENT] = "IDENT",
        [TOKEN_REM] = "REM",
        [TOKEN_INPUT] = "INPUT",
        [TOKEN_PRINT] = "PRINT",
        [TOKEN_LET] = "LET",
        [TOKEN_GOTO] = "GOTO",
        [TOKEN_IF] = "IF",
        [TOKEN_FOR] = "FOR",
        [TOKEN_TO] = "TO",
        [TOKEN_STEP] = "STEP",
        [TOKEN_NEXT] = "NEXT",
        [TOKEN_END] = "END",
        [TOKEN_PLUS] = "PLUS",
        [TOKEN_MINUS] = "MINUS",
        [TOKEN_STAR] = "STAR",
        [TOKEN_SLASH] = "SLASH",
        [TOKEN_PERCENT] = "PERCENT",
        [TOKEN_CARET] = "CARET",
        [TOKEN_ASSIGN] = "ASSIGN",
        [TOKEN_EQ] = "EQ",
        [TOKEN_NE] = "NE",
        [TOKEN_LT] = "LT",
        [TOKEN_GT] = "GT",
        [TOKEN_LE] = "LE",
        [TOKEN_GE] = "GE",
        [TOKEN_COMMA] = "COMMA",
        [TOKEN_LPAREN] = "LPAREN",
        [TOKEN_RPAREN] = "RPAREN",
    };
    return names[type];
}

/* 关键字表 */
typedef struct {
    const char *name;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"rem",   TOKEN_REM},
    {"input", TOKEN_INPUT},
    {"print", TOKEN_PRINT},
    {"let",   TOKEN_LET},
    {"goto",  TOKEN_GOTO},
    {"if",    TOKEN_IF},
    {"for",   TOKEN_FOR},
    {"to",    TOKEN_TO},
    {"step",  TOKEN_STEP},
    {"next",  TOKEN_NEXT},
    {"end",   TOKEN_END},
    {NULL,    TOKEN_EOF}
};

/* 初始化词法分析器 */
void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

/* 辅助函数: 是否到达结尾 */
static int is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

/* 辅助函数: 前进一个字符 */
static char advance(Lexer *lexer) {
    lexer->column++;
    return *lexer->current++;
}

/* 辅助函数: 查看当前字符 */
static char peek(Lexer *lexer) {
    return *lexer->current;
}

/* 辅助函数: 查看下一个字符 */
static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

/* 辅助函数: 匹配字符 */
static int match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (*lexer->current != expected) return 0;
    lexer->current++;
    lexer->column++;
    return 1;
}

/* 跳过空白字符 (不包括换行) */
static void skip_whitespace(Lexer *lexer) {
    while (1) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(lexer);
        } else {
            return;
        }
    }
}

/* 创建 Token */
static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;
    token.column = lexer->column - (int)(lexer->current - lexer->start);
    token.num_value = 0;

    int length = (int)(lexer->current - lexer->start);
    if (length > 255) length = 255;
    strncpy(token.text, lexer->start, length);
    token.text[length] = '\0';

    return token;
}

/* 创建错误 Token */
static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.column = lexer->column;
    token.num_value = 0;
    strncpy(token.text, message, 255);
    token.text[255] = '\0';
    return token;
}

/* 扫描数字 */
static Token scan_number(Lexer *lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    TokenType type = TOKEN_NUMBER;

    // 检查小数部分
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        type = TOKEN_FLOAT;
        advance(lexer);  // 消费 '.'
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    Token token = make_token(lexer, type);
    token.num_value = strtod(token.text, NULL);
    return token;
}

/* 扫描字符串 */
static Token scan_string(Lexer *lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            return error_token(lexer, "Unterminated string");
        }
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }

    advance(lexer);  // 消费结束的 '"'
    return make_token(lexer, TOKEN_STRING);
}

/* 扫描标识符或关键字 */
static Token scan_identifier(Lexer *lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    Token token = make_token(lexer, TOKEN_IDENT);

    // 检查是否为关键字
    for (int i = 0; keywords[i].name != NULL; i++) {
        if (strcasecmp(token.text, keywords[i].name) == 0) {
            token.type = keywords[i].type;
            break;
        }
    }

    return token;
}

/* 获取下一个 Token */
Token lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance(lexer);

    // 换行符
    if (c == '\n') {
        Token token = make_token(lexer, TOKEN_NEWLINE);
        lexer->line++;
        lexer->column = 1;
        return token;
    }

    // 数字
    if (isdigit(c)) {
        return scan_number(lexer);
    }

    // 标识符或关键字
    if (isalpha(c) || c == '_') {
        return scan_identifier(lexer);
    }

    // 字符串
    if (c == '"') {
        return scan_string(lexer);
    }

    // 运算符和分隔符
    switch (c) {
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '-': return make_token(lexer, TOKEN_MINUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '^': return make_token(lexer, TOKEN_CARET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);

        case '=':
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQ : TOKEN_ASSIGN);
        case '!':
            return match(lexer, '=') ? make_token(lexer, TOKEN_NE)
                                     : error_token(lexer, "Expected '=' after '!'");
        case '<':
            return make_token(lexer, match(lexer, '=') ? TOKEN_LE : TOKEN_LT);
        case '>':
            return make_token(lexer, match(lexer, '=') ? TOKEN_GE : TOKEN_GT);
    }

    return error_token(lexer, "Unexpected character");
}

/* 查看下一个 Token (保存状态) */
Token lexer_peek_token(Lexer *lexer) {
    // 保存当前状态
    const char *saved_start = lexer->start;
    const char *saved_current = lexer->current;
    int saved_line = lexer->line;
    int saved_column = lexer->column;

    Token token = lexer_next_token(lexer);

    // 恢复状态
    lexer->start = saved_start;
    lexer->current = saved_current;
    lexer->line = saved_line;
    lexer->column = saved_column;

    return token;
}

/* 重置到指定位置 */
void lexer_reset_line(Lexer *lexer, const char *line_start) {
    lexer->start = line_start;
    lexer->current = line_start;
    lexer->column = 1;
}
