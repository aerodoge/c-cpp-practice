#ifndef LEXER_H
#define LEXER_H

#include "token.h"

/**
 * @file lexer.h
 * @brief Simple 语言词法分析器 (Lexer)
 *
 * 词法分析器负责将源代码字符流转换为 Token 序列。
 * 这是编译/解释过程的第一阶段。
 *
 * 工作原理:
 * 1. 跳过空白字符 (空格、制表符)
 * 2. 识别数字 -> TOKEN_NUMBER / TOKEN_FLOAT
 * 3. 识别标识符 -> TOKEN_IDENT 或关键字
 * 4. 识别字符串 -> TOKEN_STRING
 * 5. 识别运算符和分隔符
 * 6. 保留换行符 -> TOKEN_NEWLINE (用于行号定位)
 *
 * @see token.h Token 类型定义
 */

/**
 * @struct Lexer
 * @brief 词法分析器状态
 */
typedef struct {
    const char *source;    /**< 完整源代码字符串 */
    const char *start;     /**< 当前 Token 起始位置 */
    const char *current;   /**< 当前扫描位置 (指针) */
    int line;              /**< 当前行号 (从 1 开始) */
    int column;            /**< 当前列号 (从 1 开始) */
} Lexer;

/**
 * @brief 初始化词法分析器
 * @param lexer 词法分析器指针
 * @param source 源代码字符串 (必须以 '\0' 结尾)
 */
void lexer_init(Lexer *lexer, const char *source);

/**
 * @brief 获取下一个 Token (消费型)
 * @param lexer 词法分析器指针
 * @return 下一个 Token，到达末尾返回 TOKEN_EOF
 */
Token lexer_next_token(Lexer *lexer);

/**
 * @brief 预览下一个 Token (非消费型)
 * @param lexer 词法分析器指针
 * @return 下一个 Token，不改变词法分析器状态
 */
Token lexer_peek_token(Lexer *lexer);

/**
 * @brief 重置词法分析器到指定位置
 * @param lexer 词法分析器指针
 * @param line_start 新的起始位置
 */
void lexer_reset_line(Lexer *lexer, const char *line_start);

#endif /* LEXER_H */
