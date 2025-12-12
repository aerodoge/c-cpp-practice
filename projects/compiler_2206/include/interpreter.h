#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "lexer.h"

/**
 * @file interpreter.h
 * @brief Simple 语言解释器
 *
 * 解释器采用"边解析边执行"策略，不生成中间代码：
 * 1. 预处理: 扫描源码建立行号索引
 * 2. 执行: 按行号顺序执行，遇到 goto/if 跳转
 *
 * 与编译器的区别:
 * - 解释器: 直接执行，支持动态数组索引、浮点运算
 * - 编译器: 生成 SML 码，受 SML 指令集限制
 *
 * 支持的完整功能:
 * - 浮点数运算
 * - 动态数组索引 (如 a(i) 其中 i 是变量)
 * - 任意步长的 for 循环
 * - 字符串输出
 *
 * @see compiler.h 编译器 (生成 SML)
 */

#define MAX_VARIABLES 26     /**< 变量数量 (a-z) */
#define MAX_ARRAY_SIZE 100   /**< 单个数组最大元素数 */
#define MAX_LINES 1000       /**< 最大程序行数 */
#define MAX_FOR_DEPTH 10     /**< for 循环最大嵌套深度 */

/**
 * @struct Variable
 * @brief 变量存储 (支持浮点数)
 */
typedef struct {
    double value;          /**< 变量值 */
    int initialized;       /**< 是否已初始化 */
} Variable;

/**
 * @struct Array
 * @brief 数组存储
 */
typedef struct {
    double values[MAX_ARRAY_SIZE]; /**< 元素值数组 */
    int size;              /**< 已使用大小 */
    int initialized;       /**< 是否已初始化 */
} Array;

/**
 * @struct ForState
 * @brief for 循环运行时状态
 */
typedef struct {
    char var;              /**< 循环变量 (a-z) */
    double end_value;      /**< 结束值 */
    double step;           /**< 步长 (可正可负) */
    int next_line_index;   /**< next 语句后的行索引 */
    const char *loop_start;/**< 循环体起始位置 (用于重新解析) */
} ForState;

/**
 * @struct LineInfo
 * @brief 源代码行索引
 */
typedef struct {
    int line_number;       /**< 行号 (10, 20, 30...) */
    const char *start;     /**< 行起始位置指针 */
} LineInfo;

/**
 * @struct Interpreter
 * @brief 解释器主结构
 */
typedef struct {
    /* ===== 源代码 ===== */
    char *source;                       /**< 源代码副本 */
    LineInfo lines[MAX_LINES];          /**< 行号索引表 */
    int line_count;                     /**< 总行数 */

    /* ===== 变量存储 ===== */
    Variable variables[MAX_VARIABLES];  /**< 标量变量 a-z */
    Array arrays[MAX_VARIABLES];        /**< 数组 a()-z() */

    /* ===== for 循环栈 ===== */
    ForState for_stack[MAX_FOR_DEPTH];  /**< 循环状态栈 */
    int for_depth;                      /**< 当前嵌套深度 */

    /* ===== 执行状态 ===== */
    int current_line_index;             /**< 当前执行的行索引 */
    int running;                        /**< 运行标志 */

    /* ===== 词法分析 ===== */
    Lexer lexer;                        /**< 词法分析器 */
    Token current_token;                /**< 当前 Token */

    /* ===== 错误处理 ===== */
    char error_message[256];            /**< 错误信息 */
    int has_error;                      /**< 错误标志 */
} Interpreter;

/* ==================== 公共 API ==================== */

/**
 * @brief 初始化解释器
 * @param interp 解释器指针
 */
void interpreter_init(Interpreter *interp);

/**
 * @brief 加载源代码字符串
 * @param interp 解释器指针
 * @param source 源代码字符串
 * @return 成功返回 1，失败返回 0
 */
int interpreter_load(Interpreter *interp, const char *source);

/**
 * @brief 从文件加载源代码
 * @param interp 解释器指针
 * @param filename 文件路径
 * @return 成功返回 1，失败返回 0
 */
int interpreter_load_file(Interpreter *interp, const char *filename);

/**
 * @brief 执行程序
 * @param interp 解释器指针
 * @return 成功返回 1，出错返回 0
 */
int interpreter_run(Interpreter *interp);

/**
 * @brief 释放解释器资源
 * @param interp 解释器指针
 */
void interpreter_free(Interpreter *interp);

/**
 * @brief 获取错误信息
 * @param interp 解释器指针
 * @return 错误信息字符串
 */
const char* interpreter_get_error(Interpreter *interp);

#endif /* INTERPRETER_H */
