/**
 * @file lexer.c
 * @brief Simple 语言词法分析器实现
 *
 * ============================================================================
 *                              词法分析器原理
 * ============================================================================
 *
 * 词法分析器 (Lexer/Scanner/Tokenizer) 是编译器的第一个阶段，负责将源代码
 * 字符流转换为 Token 序列。
 *
 * 工作原理:
 * ┌─────────────────────────────────────────────────────────────────────────┐
 * │  "let x = 10 + 20"                                                      │
 * │       ↓ 词法分析                                                         │
 * │  [LET] [IDENT:x] [ASSIGN] [NUMBER:10] [PLUS] [NUMBER:20] [EOF]          │
 * └─────────────────────────────────────────────────────────────────────────┘
 *
 * 本实现采用手写的递归下降词法分析器，主要功能：
 *
 * 1. 单字符运算符识别:
 *    +, -, *, /, %, ^, (, ), ,
 *
 * 2. 双字符运算符识别 (需要前瞻):
 *    ==, !=, <=, >=
 *
 * 3. 数字字面量识别:
 *    - 整数: 123, 0, 999
 *    - 浮点数: 3.14, 0.5 (需要前瞻确认小数点后有数字)
 *
 * 4. 字符串字面量识别:
 *    双引号包围的文本: "hello world"
 *
 * 5. 标识符和关键字识别:
 *    - 标识符: 字母开头的字母数字序列
 *    - 关键字: 与预定义关键字表匹配 (大小写不敏感)
 *
 * ============================================================================
 *                              核心数据结构
 * ============================================================================
 *
 * Lexer 结构维护扫描状态:
 *   - source:  指向完整源代码的指针
 *   - start:   当前 Token 起始位置
 *   - current: 当前扫描位置
 *   - line:    当前行号
 *   - column:  当前列号
 *
 * 扫描指针移动示意:
 *
 *   "let x = 123\n..."
 *    ^   ^
 *    |   └── current (当前扫描位置)
 *    └────── start (当前Token起始)
 *
 * 当识别完一个 Token 后:
 *   1. 创建 Token，text = source[start..current)
 *   2. start 移动到 current
 *   3. 继续扫描下一个 Token
 */

#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 *                              Token 工具函数
 * ============================================================================ */

/**
 * @brief Token 类型名称映射表 (用于调试输出)
 *
 * 使用 C99 的指定初始化器语法，将枚举值直接映射到字符串。
 * 这样可以保证枚举值变化时不会出现映射错误。
 *
 * @param type Token 类型
 * @return 对应的字符串名称
 */
const char* token_type_name(TokenType type) {
    /* 使用指定初始化器 [枚举值] = "字符串" 语法
     * 优点：即使枚举值不连续或顺序变化，映射仍然正确 */
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

/* ============================================================================
 *                              关键字表
 * ============================================================================
 *
 * Simple 语言支持 11 个关键字，使用线性搜索查找。
 * 对于小型语言，线性搜索足够高效；大型语言可使用哈希表或完美哈希。
 *
 * 关键字识别流程:
 *   1. 先按标识符规则扫描
 *   2. 然后在关键字表中查找
 *   3. 匹配则返回关键字类型，否则返回 TOKEN_IDENT
 */

/**
 * @brief 关键字-类型映射结构
 */
typedef struct {
    const char *name;      /**< 关键字字符串 */
    TokenType type;        /**< 对应的 Token 类型 */
} Keyword;

/**
 * @brief 关键字表 (以 NULL 结尾)
 *
 * 包含 Simple 语言的所有 11 个关键字。
 * 匹配时使用 strcasecmp 进行大小写不敏感比较。
 */
static const Keyword keywords[] = {
    {"rem",   TOKEN_REM},      /* 注释 */
    {"input", TOKEN_INPUT},    /* 输入 */
    {"print", TOKEN_PRINT},    /* 输出 */
    {"let",   TOKEN_LET},      /* 赋值 */
    {"goto",  TOKEN_GOTO},     /* 无条件跳转 */
    {"if",    TOKEN_IF},       /* 条件跳转 */
    {"for",   TOKEN_FOR},      /* 循环开始 */
    {"to",    TOKEN_TO},       /* 循环结束值 */
    {"step",  TOKEN_STEP},     /* 循环步长 */
    {"next",  TOKEN_NEXT},     /* 循环结束 */
    {"end",   TOKEN_END},      /* 程序结束 */
    {NULL,    TOKEN_EOF}       /* 哨兵，标记表结束 */
};

/* ============================================================================
 *                              词法分析器初始化
 * ============================================================================ */

/**
 * @brief 初始化词法分析器
 *
 * 设置词法分析器的初始状态，准备开始扫描。
 *
 * @param lexer  词法分析器指针
 * @param source 源代码字符串 (必须以 '\0' 结尾)
 *
 * 初始状态:
 *   - source, start, current 都指向源代码开头
 *   - 行号从 1 开始
 *   - 列号从 1 开始
 */
void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;     /* 保存源代码指针 */
    lexer->start = source;      /* Token 起始位置 */
    lexer->current = source;    /* 当前扫描位置 */
    lexer->line = 1;            /* 行号从 1 开始 */
    lexer->column = 1;          /* 列号从 1 开始 */
}

/* ============================================================================
 *                              字符级辅助函数
 * ============================================================================
 *
 * 这些函数操作词法分析器的扫描指针，实现字符级的输入处理。
 * 它们是构建更高级扫描函数的基础。
 */

/**
 * @brief 检查是否到达源代码末尾
 *
 * @param lexer 词法分析器指针
 * @return 如果当前位置是 '\0'，返回 1；否则返回 0
 *
 * 用于循环条件判断，防止越界访问。
 */
static int is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

/**
 * @brief 消费当前字符并前进
 *
 * 读取当前字符，然后将扫描指针向前移动一位。
 * 这是最基本的"消费型"读取操作。
 *
 * @param lexer 词法分析器指针
 * @return 移动前的字符
 *
 * 示例:
 *   current 指向 'a' → 返回 'a'，current 指向下一个字符
 */
static char advance(Lexer *lexer) {
    lexer->column++;            /* 更新列号 */
    return *lexer->current++;   /* 返回当前字符，然后指针后移 */
}

/**
 * @brief 查看当前字符 (不消费)
 *
 * 返回当前字符但不移动扫描指针。
 * 这是"前瞻"操作的基础。
 *
 * @param lexer 词法分析器指针
 * @return 当前字符
 */
static char peek(Lexer *lexer) {
    return *lexer->current;
}

/**
 * @brief 查看下一个字符 (不消费)
 *
 * 返回当前位置之后的字符，用于双字符前瞻。
 * 例如：区分 '=' 和 '==' 时，需要看第二个字符。
 *
 * @param lexer 词法分析器指针
 * @return 下一个字符；如果已到末尾，返回 '\0'
 *
 * 注意: 必须先检查 is_at_end()，避免越界
 */
static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];   /* 访问下一个位置 */
}

/**
 * @brief 条件匹配并消费字符
 *
 * 如果当前字符等于 expected，则消费它并返回 1；
 * 否则不消费，返回 0。
 *
 * @param lexer    词法分析器指针
 * @param expected 期望的字符
 * @return 匹配成功返回 1，失败返回 0
 *
 * 用途: 识别双字符运算符
 * 示例:
 *   - 当前是 '='，调用 match(lexer, '=')
 *   - 如果下一个也是 '='，消费它，返回 1 → TOKEN_EQ
 *   - 否则返回 0 → TOKEN_ASSIGN
 */
static int match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return 0;          /* 到达末尾，不匹配 */
    if (*lexer->current != expected) return 0; /* 字符不匹配 */

    /* 匹配成功，消费该字符 */
    lexer->current++;
    lexer->column++;
    return 1;
}

/* ============================================================================
 *                              空白字符处理
 * ============================================================================ */

/**
 * @brief 跳过空白字符 (不包括换行符)
 *
 * 空白字符包括空格 ' '、制表符 '\t'、回车符 '\r'。
 * 换行符 '\n' 不跳过，因为它在 Simple 语言中有语法意义。
 *
 * @param lexer 词法分析器指针
 *
 * 循环消费空白字符，直到遇到非空白字符或换行符。
 */
static void skip_whitespace(Lexer *lexer) {
    while (1) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(lexer);     /* 消费空白字符 */
        } else {
            return;             /* 遇到非空白字符，停止 */
        }
    }
}

/* ============================================================================
 *                              Token 构造函数
 * ============================================================================ */

/**
 * @brief 创建一个 Token
 *
 * 从当前扫描状态构造一个完整的 Token 结构。
 * Token 的文本内容为 source[start..current) 区间的字符串。
 *
 * @param lexer 词法分析器指针
 * @param type  Token 类型
 * @return 构造好的 Token 结构
 *
 * 工作流程:
 *   1. 设置 Token 类型
 *   2. 计算 Token 在源代码中的位置 (行号、列号)
 *   3. 复制 Token 文本到 text 字段
 *   4. 初始化 num_value 为 0
 */
static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.line = lexer->line;

    /* 计算 Token 起始列号
     * 当前列号 - Token 长度 = 起始列号 */
    token.column = lexer->column - (int)(lexer->current - lexer->start);
    token.num_value = 0;

    /* 复制 Token 文本
     * 注意: 限制最大长度为 255，防止缓冲区溢出 */
    int length = (int)(lexer->current - lexer->start);
    if (length > 255) length = 255;
    strncpy(token.text, lexer->start, length);
    token.text[length] = '\0';  /* 确保字符串以 null 结尾 */

    return token;
}

/**
 * @brief 创建一个错误 Token
 *
 * 当遇到词法错误时，创建包含错误信息的 Token。
 *
 * @param lexer   词法分析器指针
 * @param message 错误描述信息
 * @return 错误 Token
 *
 * 错误 Token 的特点:
 *   - type = TOKEN_ERROR
 *   - text = 错误描述信息 (不是源代码文本)
 */
static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.line = lexer->line;
    token.column = lexer->column;
    token.num_value = 0;

    /* 复制错误信息到 text 字段 */
    strncpy(token.text, message, 255);
    token.text[255] = '\0';

    return token;
}

/* ============================================================================
 *                              数字字面量扫描
 * ============================================================================
 *
 * 数字识别规则:
 *   整数:   [0-9]+
 *   浮点数: [0-9]+ '.' [0-9]+
 *
 * 注意: 必须确保小数点后有数字，否则可能误识别。
 * 例如: "10." 后面如果是运算符，则 "10" 是整数，"." 是错误字符。
 */

/**
 * @brief 扫描数字字面量 (整数或浮点数)
 *
 * 调用时，lexer->current 已经指向第一个数字字符。
 *
 * @param lexer 词法分析器指针
 * @return 数字 Token (TOKEN_NUMBER 或 TOKEN_FLOAT)
 *
 * 算法:
 *   1. 消费所有连续数字 → 整数部分
 *   2. 检查是否有小数点且小数点后有数字
 *   3. 如果有，消费小数点和小数部分 → 浮点数
 *   4. 将文本转换为数值存入 num_value
 */
static Token scan_number(Lexer *lexer) {
    /* 第一步: 扫描整数部分
     * 消费所有连续的数字字符 */
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    TokenType type = TOKEN_NUMBER;  /* 默认是整数 */

    /* 第二步: 检查小数部分
     * 条件: 当前是 '.' 且下一个字符是数字
     * 这样可以避免 "10." 被误识别为浮点数 */
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        type = TOKEN_FLOAT;         /* 改为浮点数类型 */
        advance(lexer);             /* 消费小数点 '.' */

        /* 扫描小数部分 */
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    /* 第三步: 创建 Token 并解析数值 */
    Token token = make_token(lexer, type);
    token.num_value = strtod(token.text, NULL);  /* 字符串转 double */

    return token;
}

/* ============================================================================
 *                              字符串字面量扫描
 * ============================================================================
 *
 * 字符串识别规则:
 *   - 以双引号 " 开始和结束
 *   - 不支持转义字符 (如 \n, \t)
 *   - 不能跨行
 */

/**
 * @brief 扫描字符串字面量
 *
 * 调用时，开始的双引号已经被消费，lexer->current 指向字符串内容。
 *
 * @param lexer 词法分析器指针
 * @return 字符串 Token 或错误 Token
 *
 * 算法:
 *   1. 循环消费字符，直到遇到闭合双引号或换行符或文件结尾
 *   2. 如果遇到换行符或文件结尾，返回"未终止字符串"错误
 *   3. 消费闭合双引号，返回字符串 Token
 */
static Token scan_string(Lexer *lexer) {
    /* 扫描字符串内容，直到遇到结束引号 */
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            /* 字符串不能跨行 */
            return error_token(lexer, "Unterminated string");
        }
        advance(lexer);
    }

    /* 检查是否到达文件末尾而没有闭合引号 */
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }

    /* 消费结束的双引号 */
    advance(lexer);

    /* 返回字符串 Token (包含双引号) */
    return make_token(lexer, TOKEN_STRING);
}

/* ============================================================================
 *                              标识符和关键字扫描
 * ============================================================================
 *
 * 标识符识别规则:
 *   - 以字母或下划线开头
 *   - 后续可以是字母、数字或下划线
 *
 * 关键字识别:
 *   - 先按标识符规则扫描
 *   - 然后在关键字表中查找 (大小写不敏感)
 *   - 匹配则返回关键字类型，否则返回 TOKEN_IDENT
 */

/**
 * @brief 扫描标识符或关键字
 *
 * 调用时，第一个字符已确认是字母或下划线。
 *
 * @param lexer 词法分析器指针
 * @return 标识符 Token 或关键字 Token
 *
 * 算法:
 *   1. 消费所有字母、数字、下划线字符
 *   2. 创建 Token
 *   3. 在关键字表中查找 (大小写不敏感)
 *   4. 如果匹配关键字，修改 Token 类型
 */
static Token scan_identifier(Lexer *lexer) {
    /* 消费所有合法的标识符字符 */
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    /* 先创建为标识符类型 */
    Token token = make_token(lexer, TOKEN_IDENT);

    /* 在关键字表中查找
     * 使用 strcasecmp 进行大小写不敏感比较 */
    for (int i = 0; keywords[i].name != NULL; i++) {
        if (strcasecmp(token.text, keywords[i].name) == 0) {
            /* 匹配到关键字，修改 Token 类型 */
            token.type = keywords[i].type;
            break;
        }
    }

    return token;
}

/* ============================================================================
 *                              主扫描函数
 * ============================================================================
 *
 * lexer_next_token 是词法分析器的核心函数，每次调用返回一个 Token。
 * 它是一个有限状态机的实现:
 *
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │                        开始                                 │
 *   │                          ↓                                  │
 *   │                   跳过空白字符                               │
 *   │                          ↓                                  │
 *   │                   记录 Token 起始                           │
 *   │                          ↓                                  │
 *   │                   读取下一个字符                             │
 *   │                          ↓                                  │
 *   │   ┌──────────────────────┼──────────────────────┐          │
 *   │   ↓           ↓          ↓          ↓           ↓          │
 *   │  EOF       换行符      数字      字母/下划线   其他字符      │
 *   │   ↓           ↓          ↓          ↓           ↓          │
 *   │ TOKEN_EOF  NEWLINE   scan_number  scan_id   运算符/错误    │
 *   └─────────────────────────────────────────────────────────────┘
 */

/**
 * @brief 获取下一个 Token (消费型)
 *
 * 这是词法分析器的主入口函数。每次调用消费源代码并返回一个 Token。
 * 调用者通过循环调用此函数获取所有 Token，直到遇到 TOKEN_EOF。
 *
 * @param lexer 词法分析器指针
 * @return 下一个 Token
 *
 * 使用示例:
 * @code
 *   Lexer lexer;
 *   lexer_init(&lexer, "let x = 10");
 *   Token tok;
 *   while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
 *       printf("%s: %s\n", token_type_name(tok.type), tok.text);
 *   }
 * @endcode
 */
Token lexer_next_token(Lexer *lexer) {
    /* 第一步: 跳过空白字符 (不包括换行) */
    skip_whitespace(lexer);

    /* 第二步: 记录新 Token 的起始位置 */
    lexer->start = lexer->current;

    /* 第三步: 检查是否到达源代码末尾 */
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    /* 第四步: 读取下一个字符 */
    char c = advance(lexer);

    /* ========== 换行符处理 ========== */
    if (c == '\n') {
        Token token = make_token(lexer, TOKEN_NEWLINE);
        lexer->line++;          /* 行号递增 */
        lexer->column = 1;      /* 列号重置 */
        return token;
    }

    /* ========== 数字字面量 ========== */
    if (isdigit(c)) {
        return scan_number(lexer);
    }

    /* ========== 标识符或关键字 ========== */
    if (isalpha(c) || c == '_') {
        return scan_identifier(lexer);
    }

    /* ========== 字符串字面量 ========== */
    if (c == '"') {
        return scan_string(lexer);
    }

    /* ========== 运算符和分隔符 ========== */
    switch (c) {
        /* 单字符运算符: 直接返回对应 Token */
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '-': return make_token(lexer, TOKEN_MINUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '^': return make_token(lexer, TOKEN_CARET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);

        /* 双字符运算符: 需要前瞻判断 */

        case '=':
            /* '=' 后面如果是 '='，则是 '==' (等于)
             * 否则是 '=' (赋值) */
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQ : TOKEN_ASSIGN);

        case '!':
            /* '!' 后面必须是 '='，组成 '!=' (不等于)
             * 单独的 '!' 是错误 */
            return match(lexer, '=') ? make_token(lexer, TOKEN_NE)
                                     : error_token(lexer, "Expected '=' after '!'");

        case '<':
            /* '<' 后面如果是 '='，则是 '<=' (小于等于)
             * 否则是 '<' (小于) */
            return make_token(lexer, match(lexer, '=') ? TOKEN_LE : TOKEN_LT);

        case '>':
            /* '>' 后面如果是 '='，则是 '>=' (大于等于)
             * 否则是 '>' (大于) */
            return make_token(lexer, match(lexer, '=') ? TOKEN_GE : TOKEN_GT);
    }

    /* ========== 无法识别的字符 ========== */
    return error_token(lexer, "Unexpected character");
}

/* ============================================================================
 *                              预览和重置函数
 * ============================================================================
 *
 * 这些函数用于特殊场景:
 *   - peek_token: 预览下一个 Token 但不消费
 *   - reset_line: 重置扫描位置到指定行
 */

/**
 * @brief 预览下一个 Token (非消费型)
 *
 * 获取下一个 Token 但不改变词法分析器状态。
 * 实现方式: 保存状态 → 扫描 → 恢复状态。
 *
 * @param lexer 词法分析器指针
 * @return 下一个 Token
 *
 * 用途:
 *   - 解析器中的前瞻判断
 *   - 决定是否继续解析某个语法结构
 */
Token lexer_peek_token(Lexer *lexer) {
    /* 保存当前状态 */
    const char *saved_start = lexer->start;
    const char *saved_current = lexer->current;
    int saved_line = lexer->line;
    int saved_column = lexer->column;

    /* 执行扫描 */
    Token token = lexer_next_token(lexer);

    /* 恢复状态 */
    lexer->start = saved_start;
    lexer->current = saved_current;
    lexer->line = saved_line;
    lexer->column = saved_column;

    return token;
}

/**
 * @brief 重置词法分析器到指定位置
 *
 * 将扫描位置重置到源代码的某个位置。
 * 主要用于解释器逐行执行时，重新定位到某行开始。
 *
 * @param lexer      词法分析器指针
 * @param line_start 新的起始位置指针
 *
 * 注意: 不会重置行号，调用者需要自行管理。
 */
void lexer_reset_line(Lexer *lexer, const char *line_start) {
    lexer->start = line_start;
    lexer->current = line_start;
    lexer->column = 1;  /* 列号重置为 1 */
}
