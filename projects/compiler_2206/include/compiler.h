#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"

/**
 * @file compiler.h
 * @brief Simple 语言编译器 - 将高级语言编译为 SML 机器码
 *
 * 编译器采用经典的两遍扫描 (Two-Pass) 算法:
 *
 * 第一遍 (Pass 1):
 * - 逐行解析 Simple 源代码
 * - 建立符号表 (行号、变量、常量、数组、字符串)
 * - 生成 SML 指令，前向引用处 (如 goto) 留空
 *
 * 第二遍 (Pass 2):
 * - 解析符号表中的所有行号引用
 * - 填充第一遍中未解决的跳转地址
 *
 * 内存布局 (冯诺依曼架构):
 * ┌─────────────────────────────────────┐
 * │  0: 指令区 (向高地址增长 →)         │
 * │  ...                                │
 * │  instruction_counter                │
 * │  ─────── 空闲区 ───────             │
 * │  data_counter                       │
 * │  ...                                │
 * │ 99: 数据区 (向低地址增长 ←)         │
 * └─────────────────────────────────────┘
 *
 * 指令格式: ±XXYY
 * - XX: 操作码 (10-43)
 * - YY: 操作数 (内存地址 00-99)
 * - 符号: + 表示正常指令，- 用于负常量存储
 *
 * @see sml_vm.h SML 虚拟机
 * @see docs/SIMPLE_LANGUAGE.md 语言规范
 */

#define MEMORY_SIZE 100    /**< SML 内存大小 (指令+数据共享) */
#define MAX_SYMBOLS 100    /**< 符号表最大条目数 */
#define MAX_FLAGS 100      /**< 未解决引用最大数量 */

/**
 * @enum SMLOpCode
 * @brief SML 机器指令操作码
 *
 * 与 vm_2206 兼容的指令集:
 * - I/O 指令: 10-13
 * - 数据传输: 20-21
 * - 算术运算: 30-34
 * - 控制流:   40-43
 */
typedef enum {
    SML_READ      = 10,    // 从键盘读取到内存
    SML_WRITE     = 11,    // 从内存输出
    SML_NEWLINE   = 12,    // 输出换行符 (扩展)
    SML_WRITES    = 13,    // 输出字符串 (扩展)
    SML_LOAD      = 20,    // 内存 -> 累加器
    SML_STORE     = 21,    // 累加器 -> 内存
    SML_ADD       = 30,    // 累加器 += 内存
    SML_SUBTRACT  = 31,    // 累加器 -= 内存
    SML_DIVIDE    = 32,    // 累加器 /= 内存 (注意：与 vm_2206 一致)
    SML_MULTIPLY  = 33,    // 累加器 *= 内存 (注意：与 vm_2206 一致)
    SML_MOD       = 34,    // 累加器 %= 内存 (扩展)
    SML_BRANCH    = 40,    // 无条件跳转
    SML_BRANCHNEG = 41,    // 负数跳转
    SML_BRANCHZERO= 42,    // 零跳转
    SML_HALT      = 43,    // 停机
} SMLOpCode;

/**
 * @enum SymbolType
 * @brief 符号表条目类型
 */
typedef enum {
    SYMBOL_LINE,           /**< 行号标签 (goto/if 跳转目标) */
    SYMBOL_VARIABLE,       /**< 变量 (a-z, 单字符) */
    SYMBOL_CONSTANT,       /**< 整数常量 */
    SYMBOL_ARRAY,          /**< 数组 (如 a(0), a(1)) */
    SYMBOL_STRING,         /**< 字符串常量 */
} SymbolType;

/**
 * @struct Symbol
 * @brief 符号表条目
 *
 * 符号表记录源程序中所有符号及其内存映射:
 * - 行号: symbol=行号值, location=指令地址
 * - 变量: symbol=变量索引(a=0), location=数据地址
 * - 常量: symbol=常量值, location=数据地址
 * - 数组: symbol=数组基地址偏移, location=基地址, size=元素数
 */
typedef struct {
    SymbolType type;       /**< 符号类型 */
    int symbol;            /**< 符号值 (行号/变量索引/常量值) */
    int location;          /**< 内存位置 */
    int size;              /**< 数组大小 (仅 SYMBOL_ARRAY) */
} Symbol;

/**
 * @struct Flag
 * @brief 未解决的前向引用
 *
 * 第一遍编译时，前向跳转 (goto 到后面的行) 的目标地址未知，
 * 记录下来在第二遍时填充。
 */
typedef struct {
    int instruction_location;  /**< 需要修补的指令地址 */
    int target_line_number;    /**< 目标行号 */
} Flag;

/**
 * @struct ForCompileState
 * @brief for 循环编译状态
 *
 * 用于匹配 for/next 对，记录循环所需的编译信息。
 * 支持嵌套循环，使用栈结构管理。
 *
 * for 循环编译策略:
 * ```
 * for i = start to end step s
 *   [循环体]
 * next i
 * ```
 * 编译为:
 * ```
 * LOAD start; STORE i         // 初始化
 * LOAD step;  STORE step_loc  // 保存步长
 * loop_start:
 *   [循环体代码]
 * LOAD i; ADD step; STORE i   // i = i + step
 * LOAD end; SUB i             // 比较 (正步长)
 * JMPNEG exit / JMPZERO loop  // 条件跳转
 * ```
 */
#define MAX_FOR_DEPTH 10       /**< 最大循环嵌套深度 */
typedef struct {
    char var;                  /**< 循环变量名 (a-z) */
    int var_location;          /**< 变量 i 的内存位置 */
    int end_location;          /**< 结束值的内存位置 */
    int step_location;         /**< 步长的内存位置 */
    int loop_start;            /**< 循环体起始指令地址 */
    int step_is_negative;      /**< 步长是否为负 (影响比较方向) */
} ForCompileState;

/**
 * @struct StringEntry
 * @brief 字符串常量表条目
 *
 * 字符串在内存中以长度前缀格式存储:
 * [length][char1][char2]...[charN]
 */
#define MAX_STRINGS 50         /**< 最大字符串常量数 */
#define MAX_STRING_LEN 64      /**< 单个字符串最大长度 */
typedef struct {
    char text[MAX_STRING_LEN]; /**< 字符串内容 */
    int location;              /**< 数据区起始位置 */
} StringEntry;

/**
 * @struct Compiler
 * @brief 编译器主结构
 *
 * 包含编译过程所需的全部状态信息。
 */
typedef struct {
    /* ===== 源代码 ===== */
    char *source;              /**< 源代码副本 (动态分配) */

    /* ===== 词法分析 ===== */
    Lexer lexer;               /**< 词法分析器实例 */
    Token current_token;       /**< 当前 Token (向前看一个) */

    /* ===== 符号表 ===== */
    Symbol symbols[MAX_SYMBOLS]; /**< 符号表数组 */
    int symbol_count;            /**< 当前符号数量 */

    /* ===== 前向引用 ===== */
    Flag flags[MAX_FLAGS];     /**< 未解决引用数组 */
    int flag_count;            /**< 未解决引用数量 */

    /* ===== for 循环栈 ===== */
    ForCompileState for_stack[MAX_FOR_DEPTH]; /**< 循环状态栈 */
    int for_depth;             /**< 当前嵌套深度 */

    /* ===== 字符串表 ===== */
    StringEntry strings[MAX_STRINGS]; /**< 字符串常量表 */
    int string_count;          /**< 字符串数量 */

    /* ===== SML 内存 ===== */
    int memory[MEMORY_SIZE];   /**< SML 程序内存 (指令+数据) */
    int instruction_counter;   /**< 指令指针 (从 0 递增) */
    int data_counter;          /**< 数据指针 (从 99 递减) */

    /* ===== 编译状态 ===== */
    int current_line_number;   /**< 当前处理的 Simple 行号 */

    /* ===== 错误处理 ===== */
    char error_message[256];   /**< 错误信息 */
    int has_error;             /**< 错误标志 */
} Compiler;

/* ==================== 公共 API ==================== */

/**
 * @brief 初始化编译器
 * @param comp 编译器指针
 */
void compiler_init(Compiler *comp);

/**
 * @brief 编译源代码字符串
 * @param comp 编译器指针
 * @param source 源代码字符串
 * @return 成功返回 1，失败返回 0
 */
int compiler_compile(Compiler *comp, const char *source);

/**
 * @brief 从文件编译
 * @param comp 编译器指针
 * @param filename 源文件路径
 * @return 成功返回 1，失败返回 0
 */
int compiler_compile_file(Compiler *comp, const char *filename);

/**
 * @brief 输出 SML 程序到文件
 * @param comp 编译器指针
 * @param filename 输出文件路径 (通常 .sml 后缀)
 * @return 成功返回 1，失败返回 0
 */
int compiler_output(Compiler *comp, const char *filename);

/**
 * @brief 打印 SML 程序 (调试用)
 * @param comp 编译器指针
 *
 * 输出格式:
 * - 指令区: 地址 指令 操作码名 操作数
 * - 数据区: 地址 值
 */
void compiler_dump(Compiler *comp);

/**
 * @brief 打印符号表 (调试用)
 * @param comp 编译器指针
 */
void compiler_dump_symbols(Compiler *comp);

/**
 * @brief 释放编译器资源
 * @param comp 编译器指针
 */
void compiler_free(Compiler *comp);

/**
 * @brief 获取错误信息
 * @param comp 编译器指针
 * @return 错误信息字符串
 */
const char* compiler_get_error(Compiler *comp);

/**
 * @brief 获取 SML 内存 (用于加载到虚拟机)
 * @param comp 编译器指针
 * @return 100 个整数的数组指针
 */
const int* compiler_get_memory(Compiler *comp);

#endif /* COMPILER_H */
