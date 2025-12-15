/**
 * @file test_lexer.c
 * @brief 词法分析器单元测试
 *
 * 测试覆盖:
 *   - 数字识别 (整数、浮点数、负数)
 *   - 字符串识别
 *   - 标识符和关键字识别
 *   - 运算符识别
 *   - 边界情况和错误处理
 *
 * 运行方法:
 *   cd build && ./test_lexer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "lexer.h"
#include "token.h"

/* ============================================================================
 *                              数字识别测试
 * ============================================================================ */

/**
 * @brief 测试整数识别
 */
void test_lexer_integer(void) {
    Lexer lexer;
    Token token;

    /* 测试简单整数 */
    lexer_init(&lexer, "123");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 123);

    /* 测试零 */
    lexer_init(&lexer, "0");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 0);

    /* 测试大整数 */
    lexer_init(&lexer, "9999");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 9999);
}

/**
 * @brief 测试浮点数识别
 */
void test_lexer_float(void) {
    Lexer lexer;
    Token token;

    /* 测试简单浮点数 */
    lexer_init(&lexer, "3.14");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_FLOAT);
    ASSERT_FLOAT_EQ(token.num_value, 3.14, 0.001);

    /* 测试以零开头的浮点数 */
    lexer_init(&lexer, "0.5");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_FLOAT);
    ASSERT_FLOAT_EQ(token.num_value, 0.5, 0.001);
}

/* ============================================================================
 *                              字符串识别测试
 * ============================================================================ */

/**
 * @brief 测试字符串识别
 * 注意: token.text 保留原始字符串，包含双引号
 */
void test_lexer_string(void) {
    Lexer lexer;
    Token token;

    /* 测试简单字符串 (text 包含引号) */
    lexer_init(&lexer, "\"hello\"");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STRING);
    ASSERT_STR_EQ(token.text, "\"hello\"");

    /* 测试空字符串 */
    lexer_init(&lexer, "\"\"");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STRING);
    ASSERT_STR_EQ(token.text, "\"\"");

    /* 测试带空格的字符串 */
    lexer_init(&lexer, "\"hello world\"");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STRING);
    ASSERT_STR_EQ(token.text, "\"hello world\"");
}

/* ============================================================================
 *                              标识符和关键字测试
 * ============================================================================ */

/**
 * @brief 测试标识符识别
 */
void test_lexer_identifier(void) {
    Lexer lexer;
    Token token;

    /* 测试单字符标识符 */
    lexer_init(&lexer, "x");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENT);
    ASSERT_STR_EQ(token.text, "x");

    /* 测试多字符标识符 */
    lexer_init(&lexer, "abc");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENT);
    ASSERT_STR_EQ(token.text, "abc");
}

/**
 * @brief 测试关键字识别
 */
void test_lexer_keywords(void) {
    Lexer lexer;
    Token token;

    /* 测试所有关键字 */
    struct {
        const char *text;
        TokenType expected;
    } keywords[] = {
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
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        lexer_init(&lexer, keywords[i].text);
        token = lexer_next_token(&lexer);
        ASSERT_EQ(token.type, keywords[i].expected);
    }

    /* 测试大小写不敏感 */
    lexer_init(&lexer, "REM");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_REM);

    lexer_init(&lexer, "Print");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_PRINT);
}

/* ============================================================================
 *                              运算符测试
 * ============================================================================ */

/**
 * @brief 测试算术运算符
 */
void test_lexer_arithmetic_operators(void) {
    Lexer lexer;
    Token token;

    struct {
        const char *text;
        TokenType expected;
    } operators[] = {
        {"+", TOKEN_PLUS},
        {"-", TOKEN_MINUS},
        {"*", TOKEN_STAR},
        {"/", TOKEN_SLASH},
        {"%", TOKEN_PERCENT},
        {"^", TOKEN_CARET},
    };

    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
        lexer_init(&lexer, operators[i].text);
        token = lexer_next_token(&lexer);
        ASSERT_EQ(token.type, operators[i].expected);
    }
}

/**
 * @brief 测试关系运算符
 */
void test_lexer_relational_operators(void) {
    Lexer lexer;
    Token token;

    /* 测试双字符运算符 */
    lexer_init(&lexer, "==");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EQ);

    lexer_init(&lexer, "!=");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NE);

    lexer_init(&lexer, "<=");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LE);

    lexer_init(&lexer, ">=");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_GE);

    /* 测试单字符运算符 */
    lexer_init(&lexer, "<");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LT);

    lexer_init(&lexer, ">");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_GT);
}

/* ============================================================================
 *                              分隔符测试
 * ============================================================================ */

/**
 * @brief 测试分隔符识别
 */
void test_lexer_delimiters(void) {
    Lexer lexer;
    Token token;

    lexer_init(&lexer, "(");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LPAREN);

    lexer_init(&lexer, ")");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_RPAREN);

    lexer_init(&lexer, ",");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_COMMA);

    lexer_init(&lexer, "=");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_ASSIGN);
}

/* ============================================================================
 *                              复杂表达式测试
 * ============================================================================ */

/**
 * @brief 测试复杂表达式的 Token 流
 */
void test_lexer_expression(void) {
    Lexer lexer;
    Token token;

    /* 表达式: let x = 10 + y * 2 */
    lexer_init(&lexer, "let x = 10 + y * 2");

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LET);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENT);
    ASSERT_STR_EQ(token.text, "x");

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_ASSIGN);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 10);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_PLUS);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENT);
    ASSERT_STR_EQ(token.text, "y");

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_STAR);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 2);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
}

/**
 * @brief 测试完整的 Simple 程序
 */
void test_lexer_program(void) {
    Lexer lexer;
    Token token;

    /* 简单程序 */
    lexer_init(&lexer, "10 let x = 5\n20 print x\n30 end");

    /* 第 10 行 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 10);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_LET);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_IDENT);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_ASSIGN);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 5);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NEWLINE);

    /* 第 20 行 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 20);

    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_PRINT);
}

/* ============================================================================
 *                              边界情况测试
 * ============================================================================ */

/**
 * @brief 测试空输入
 */
void test_lexer_empty(void) {
    Lexer lexer;
    Token token;

    lexer_init(&lexer, "");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_EOF);
}

/**
 * @brief 测试空白字符处理
 */
void test_lexer_whitespace(void) {
    Lexer lexer;
    Token token;

    /* 多个空格 */
    lexer_init(&lexer, "   123   ");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 123);

    /* 制表符 */
    lexer_init(&lexer, "\t\t456\t");
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOKEN_NUMBER);
    ASSERT_EQ((int)token.num_value, 456);
}

/**
 * @brief 测试 peek 功能
 */
void test_lexer_peek(void) {
    Lexer lexer;
    Token token1, token2;

    lexer_init(&lexer, "10 20 30");

    /* peek 不应该消费 token */
    token1 = lexer_peek_token(&lexer);
    token2 = lexer_peek_token(&lexer);
    ASSERT_EQ(token1.type, token2.type);
    ASSERT_EQ((int)token1.num_value, (int)token2.num_value);

    /* next 应该消费 token */
    token1 = lexer_next_token(&lexer);
    ASSERT_EQ((int)token1.num_value, 10);

    token2 = lexer_next_token(&lexer);
    ASSERT_EQ((int)token2.num_value, 20);
}

/* ============================================================================
 *                              主函数
 * ============================================================================ */

int main(void) {
    TEST_BEGIN();

    /* 数字识别测试 */
    RUN_TEST(test_lexer_integer);
    RUN_TEST(test_lexer_float);

    /* 字符串测试 */
    RUN_TEST(test_lexer_string);

    /* 标识符和关键字测试 */
    RUN_TEST(test_lexer_identifier);
    RUN_TEST(test_lexer_keywords);

    /* 运算符测试 */
    RUN_TEST(test_lexer_arithmetic_operators);
    RUN_TEST(test_lexer_relational_operators);
    RUN_TEST(test_lexer_delimiters);

    /* 复杂表达式测试 */
    RUN_TEST(test_lexer_expression);
    RUN_TEST(test_lexer_program);

    /* 边界情况测试 */
    RUN_TEST(test_lexer_empty);
    RUN_TEST(test_lexer_whitespace);
    RUN_TEST(test_lexer_peek);

    TEST_END();
    return test_failed;
}
