/**
 * @file compiler.c
 * @brief Simple 语言编译器实现
 *
 * ============================================================================
 *                              编译器原理
 * ============================================================================
 *
 * 编译器将 Simple 高级语言编译成 SML (Simpletron Machine Language) 机器指令。
 * 编译后的程序可以由 SML 虚拟机执行。
 *
 * 编译 vs 解释:
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │  编译器:  源代码 ──→ SML机器码 ──→ .sml文件 ──→ 虚拟机执行                    │
 * │  解释器:  源代码 ──→ 直接执行                                               │
 * └──────────────────────────────────────────────────────────────────────────┘
 *
 * 编译器的优点:
 *   - 执行速度快 (预编译，无需每次解析)
 *   - 可生成可分发的目标文件
 *   - 可以进行更多优化
 *
 * 编译器的限制 (受 SML 指令集限制):
 *   - 仅支持整数运算 (浮点数会被截断)
 *   - 数组索引必须是常量 (SML 无间接寻址)
 *   - 内存限制: 100 单元 (指令 + 数据共享)
 *
 * ============================================================================
 *                              两遍扫描算法 (Two-Pass)
 * ============================================================================
 *
 * 编译器采用经典的两遍扫描 (Two-Pass) 算法解决前向引用问题：
 *
 * 问题: goto 可能跳转到后面还未定义的行号
 *   10 goto 100    ← 此时不知道行号 100 对应什么指令地址
 *   ...
 *   100 print x    ← 行号 100 在这里定义
 *
 * 解决方案:
 *
 * 第一遍 (Pass 1):
 *   1. 逐行解析 Simple 源代码
 *   2. 为每个行号、变量、常量分配内存位置
 *   3. 生成 SML 指令
 *   4. 前向引用 (如 goto 到未知行号) 处留空，记录在 flags 表中
 *
 * 第二遍 (Pass 2):
 *   1. 查找 flags 表中所有未解决的引用
 *   2. 从符号表中查找目标行号的指令地址
 *   3. 填充第一遍中留空的跳转地址
 *
 * ============================================================================
 *                              内存布局 (冯诺依曼架构)
 * ============================================================================
 *
 * SML 采用冯诺依曼架构，指令和数据共享同一块内存:
 *
 *   地址    内容
 *   ────────────────────────────────────
 *    0     指令区起始 (从低地址向高增长)
 *    1     ↓
 *    ...   instruction_counter
 *          ─────── 空闲区 ───────
 *          data_counter
 *    ...   ↑
 *    98    数据区 (从高地址向低增长)
 *    99    数据区起始
 *   ────────────────────────────────────
 *
 * 如果 instruction_counter 和 data_counter 相遇，则内存溢出。
 *
 * ============================================================================
 *                              SML 指令格式
 * ============================================================================
 *
 * 指令格式: ±XXYY (4位数字)
 *   - XX: 操作码 (10-43)
 *   - YY: 操作数 (内存地址 00-99)
 *   - 符号: 正数为指令，负数用于存储负常量
 *
 * 示例:
 *   +1099 → 操作码 10 (READ)，操作数 99 (内存地址 99)
 *   +2099 → 操作码 20 (LOAD)，操作数 99
 *   +4300 → 操作码 43 (HALT)，操作数 00 (无意义)
 */

#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/* ============================================================================
 *                              辅助函数
 * ============================================================================ */

/**
 * @brief 将变量名转换为索引 (a=0, b=1, ..., z=25)
 *
 * @param c 变量名字符
 * @return 索引 (0-25)，无效返回 -1
 */
static int var_index(char c) {
    c = tolower(c);
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }
    return -1;
}

/**
 * @brief 设置编译错误信息
 *
 * @param comp   编译器指针
 * @param format 格式字符串
 * @param ...    格式参数
 */
static void set_error(Compiler *comp, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(comp->error_message, sizeof(comp->error_message), format, args);
    va_end(args);
    comp->has_error = 1;
}

/**
 * @brief 获取下一个 Token
 *
 * @param comp 编译器指针
 */
static void advance_token(Compiler *comp) {
    comp->current_token = lexer_next_token(&comp->lexer);
}

/**
 * @brief 生成一条 SML 指令
 *
 * 将指令写入内存的指令区，指令计数器递增。
 *
 * @param comp        编译器指针
 * @param instruction 完整指令 (操作码 * 100 + 操作数)
 *
 * 示例:
 *   emit(comp, SML_LOAD * 100 + 99);  // 生成 LOAD 99
 *   emit(comp, SML_HALT * 100 + 0);   // 生成 HALT 00
 */
static void emit(Compiler *comp, int instruction) {
    /* 检查代码区和数据区是否冲突 */
    if (comp->instruction_counter >= comp->data_counter) {
        set_error(comp, "Memory overflow: code and data collision");
        return;
    }
    comp->memory[comp->instruction_counter++] = instruction;
}

/**
 * @brief 分配数据空间 (从内存末尾向前分配)
 *
 * 数据区从地址 99 开始，向低地址方向增长。
 *
 * @param comp 编译器指针
 * @return 分配的内存地址，失败返回 -1
 */
static int alloc_data(Compiler *comp) {
    /* 检查是否与代码区冲突 */
    if (comp->data_counter <= comp->instruction_counter) {
        set_error(comp, "Memory overflow: code and data collision");
        return -1;
    }
    return comp->data_counter--;  /* 返回当前地址，然后递减 */
}

/* ============================================================================
 *                              符号表操作
 * ============================================================================
 *
 * 符号表记录源程序中所有符号及其内存映射:
 *   - 行号 (SYMBOL_LINE):    行号值 → 指令地址
 *   - 变量 (SYMBOL_VARIABLE): 变量索引 → 数据地址
 *   - 常量 (SYMBOL_CONSTANT): 常量值 → 数据地址 (值已存储)
 *   - 数组 (SYMBOL_ARRAY):   变量索引 → 基地址 + 大小
 *   - 字符串 (SYMBOL_STRING): 字符串内容 → 数据地址
 */

/**
 * @brief 在符号表中查找符号
 *
 * @param comp   编译器指针
 * @param type   符号类型
 * @param symbol 符号值 (行号/变量索引/常量值)
 * @return 找到返回符号指针，未找到返回 NULL
 */
static Symbol* find_symbol(Compiler *comp, SymbolType type, int symbol) {
    for (int i = 0; i < comp->symbol_count; i++) {
        if (comp->symbols[i].type == type && comp->symbols[i].symbol == symbol) {
            return &comp->symbols[i];
        }
    }
    return NULL;
}

/**
 * @brief 添加符号到符号表
 *
 * @param comp     编译器指针
 * @param type     符号类型
 * @param symbol   符号值
 * @param location 内存位置
 * @return 新符号指针，失败返回 NULL
 */
static Symbol* add_symbol(Compiler *comp, SymbolType type, int symbol, int location) {
    if (comp->symbol_count >= MAX_SYMBOLS) {
        set_error(comp, "Symbol table overflow");
        return NULL;
    }
    Symbol *sym = &comp->symbols[comp->symbol_count++];
    sym->type = type;
    sym->symbol = symbol;
    sym->location = location;
    return sym;
}

/**
 * @brief 获取或创建变量
 *
 * 如果变量已存在，返回其地址；否则分配新地址。
 *
 * @param comp    编译器指针
 * @param var_idx 变量索引 (0-25 对应 a-z)
 * @return 变量的内存地址
 */
static int get_or_create_variable(Compiler *comp, int var_idx) {
    Symbol *sym = find_symbol(comp, SYMBOL_VARIABLE, var_idx);
    if (sym) {
        return sym->location;  /* 已存在，返回地址 */
    }

    /* 分配新地址 */
    int loc = alloc_data(comp);
    if (loc >= 0) {
        add_symbol(comp, SYMBOL_VARIABLE, var_idx, loc);
    }
    return loc;
}

/**
 * @brief 获取或创建常量
 *
 * 常量会被存储在数据区，多次使用同一常量只分配一次。
 *
 * @param comp  编译器指针
 * @param value 常量值
 * @return 常量的内存地址
 */
static int get_or_create_constant(Compiler *comp, int value) {
    Symbol *sym = find_symbol(comp, SYMBOL_CONSTANT, value);
    if (sym) {
        return sym->location;  /* 已存在 */
    }

    /* 分配新地址并存储值 */
    int loc = alloc_data(comp);
    if (loc >= 0) {
        add_symbol(comp, SYMBOL_CONSTANT, value, loc);
        comp->memory[loc] = value;  /* 将常量值写入内存 */
    }
    return loc;
}

/**
 * @brief 添加未解决的前向引用
 *
 * 当 goto 跳转到还未定义的行号时，记录下来在第二遍处理。
 *
 * @param comp            编译器指针
 * @param instruction_loc 需要修补的指令地址
 * @param target_line     目标行号
 */
static void add_flag(Compiler *comp, int instruction_loc, int target_line) {
    if (comp->flag_count >= MAX_FLAGS) {
        set_error(comp, "Too many unresolved references");
        return;
    }
    comp->flags[comp->flag_count].instruction_location = instruction_loc;
    comp->flags[comp->flag_count].target_line_number = target_line;
    comp->flag_count++;
}

/**
 * @brief 获取或创建数组
 *
 * 数组在内存中连续存储，基地址在高位，索引增加时地址减小。
 *
 * @param comp    编译器指针
 * @param var_idx 数组变量索引
 * @param size    数组大小
 * @return 数组基地址
 *
 * 内存布局示例 (数组 a, size=3):
 *   地址 97: a(2)
 *   地址 98: a(1)
 *   地址 99: a(0)  ← 基地址
 */
static int get_or_create_array(Compiler *comp, int var_idx, int size) {
    Symbol *sym = find_symbol(comp, SYMBOL_ARRAY, var_idx);
    if (sym) {
        return sym->location;
    }

    /* 分配连续内存空间 */
    int base_loc = comp->data_counter;
    for (int i = 0; i < size; i++) {
        if (alloc_data(comp) < 0) {
            return -1;
        }
    }

    /* 记录数组信息 */
    Symbol *new_sym = add_symbol(comp, SYMBOL_ARRAY, var_idx, base_loc);
    if (new_sym) {
        new_sym->size = size;
    }
    return base_loc;
}

/**
 * @brief 存储字符串常量
 *
 * 字符串以长度前缀格式存储: [length][char1][char2]...
 *
 * @param comp 编译器指针
 * @param str  字符串 (包含引号)
 * @return 字符串起始地址 (长度单元的地址)
 */
static int store_string(Compiler *comp, const char *str) {
    /* 去掉引号 */
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        str++;
        len -= 2;
    }

    /* 检查是否已存在相同字符串 */
    for (int i = 0; i < comp->string_count; i++) {
        if (strncmp(comp->strings[i].text, str, len) == 0 &&
            comp->strings[i].text[len] == '\0') {
            return comp->strings[i].location;
        }
    }

    if (comp->string_count >= MAX_STRINGS) {
        set_error(comp, "Too many string constants");
        return -1;
    }

    /* 存储字符串 */
    StringEntry *entry = &comp->strings[comp->string_count++];
    int start_loc = comp->data_counter;

    /* 先存储长度 */
    comp->memory[comp->data_counter--] = len;

    /* 然后存储每个字符 (ASCII 码) */
    for (int i = 0; i < len && i < MAX_STRING_LEN - 1; i++) {
        comp->memory[comp->data_counter--] = (unsigned char)str[i];
    }

    strncpy(entry->text, str, len);
    entry->text[len] = '\0';
    entry->location = start_loc;

    return start_loc;
}

/* ============================================================================
 *                              表达式编译
 * ============================================================================
 *
 * 表达式编译采用递归下降，与解释器类似，但不是计算值，而是生成 SML 指令。
 *
 * 核心思想: 每个表达式编译后，结果在累加器 (AC) 中
 *
 * 例如编译 "x + y":
 *   1. 编译 x → LOAD x_addr  (x 值进入 AC)
 *   2. 保存 AC → STORE temp
 *   3. 编译 y → LOAD y_addr  (y 值进入 AC)
 *   4. 交换  → STORE temp2; LOAD temp
 *   5. 运算  → ADD temp2     (结果在 AC 中)
 */

/* 前向声明 */
static void compile_expression(Compiler *comp);
static void compile_term(Compiler *comp);
static void compile_power(Compiler *comp);
static void compile_unary(Compiler *comp);
static void compile_primary(Compiler *comp);

/**
 * @brief 编译基本元素
 *
 * 处理: 数字、变量、数组元素(常量索引)、括号表达式
 * 编译后结果在累加器中
 *
 * @param comp 编译器指针
 *
 * 注意: 数组仅支持常量索引，因为 SML 没有间接寻址指令
 */
static void compile_primary(Compiler *comp) {
    Token token = comp->current_token;

    /* ========== 数字字面量 ========== */
    if (token.type == TOKEN_NUMBER || token.type == TOKEN_FLOAT) {
        int value = (int)token.num_value;  /* 截断为整数 */
        int loc = get_or_create_constant(comp, value);
        emit(comp, SML_LOAD * 100 + loc);  /* LOAD 常量地址 */
        advance_token(comp);
    }
    /* ========== 变量或数组 ========== */
    else if (token.type == TOKEN_IDENT) {
        int idx = var_index(token.text[0]);
        if (idx < 0) {
            set_error(comp, "Invalid variable: %s", token.text);
            return;
        }
        advance_token(comp);

        /* 检查是否是数组访问 a(index) */
        if (comp->current_token.type == TOKEN_LPAREN) {
            advance_token(comp);  /* 消费 '(' */

            /* 只支持常量索引 (SML 限制) */
            if (comp->current_token.type != TOKEN_NUMBER) {
                set_error(comp, "Array index must be a constant (SML limitation)");
                return;
            }
            int array_idx = (int)comp->current_token.num_value;
            advance_token(comp);

            if (comp->current_token.type != TOKEN_RPAREN) {
                set_error(comp, "Expected ')' after array index");
                return;
            }
            advance_token(comp);

            /* 查找或创建数组 */
            Symbol *arr = find_symbol(comp, SYMBOL_ARRAY, idx);
            if (!arr) {
                int size = (array_idx + 1 > 10) ? array_idx + 1 : 10;
                get_or_create_array(comp, idx, size);
                arr = find_symbol(comp, SYMBOL_ARRAY, idx);
            }

            if (!arr) {
                set_error(comp, "Failed to create array");
                return;
            }

            /* 边界检查 */
            if (array_idx < 0 || array_idx >= arr->size) {
                set_error(comp, "Array index %d out of bounds (0-%d)",
                          array_idx, arr->size - 1);
                return;
            }

            /* 计算元素地址: 基地址 - 索引 */
            int elem_loc = arr->location - array_idx;
            emit(comp, SML_LOAD * 100 + elem_loc);
        } else {
            /* 普通变量 */
            int loc = get_or_create_variable(comp, idx);
            emit(comp, SML_LOAD * 100 + loc);
        }
    }
    /* ========== 括号表达式 ========== */
    else if (token.type == TOKEN_LPAREN) {
        advance_token(comp);
        compile_expression(comp);
        if (comp->current_token.type != TOKEN_RPAREN) {
            set_error(comp, "Expected ')'");
            return;
        }
        advance_token(comp);
    }
    else {
        set_error(comp, "Unexpected token in expression: %s", token.text);
    }
}

/**
 * @brief 编译一元运算符 (-x, +x)
 *
 * 负号实现: 0 - x
 *
 * @param comp 编译器指针
 */
static void compile_unary(Compiler *comp) {
    if (comp->current_token.type == TOKEN_MINUS) {
        advance_token(comp);
        compile_unary(comp);

        /* 取负: 0 - value
         * 1. 将当前 AC 保存到临时变量
         * 2. 加载 0
         * 3. 减去临时变量 */
        int zero_loc = get_or_create_constant(comp, 0);
        int temp = alloc_data(comp);
        emit(comp, SML_STORE * 100 + temp);     /* 保存 AC */
        emit(comp, SML_LOAD * 100 + zero_loc);  /* 加载 0 */
        emit(comp, SML_SUBTRACT * 100 + temp);  /* 0 - value */
    } else if (comp->current_token.type == TOKEN_PLUS) {
        advance_token(comp);
        compile_unary(comp);  /* 正号不做任何事 */
    } else {
        compile_primary(comp);
    }
}

/**
 * @brief 编译幂运算 (a ^ b)
 *
 * 由于 SML 没有幂指令，通过循环乘法实现。
 * 仅支持正整数指数。
 *
 * @param comp 编译器指针
 *
 * 算法:
 *   result = 1
 *   while (exp > 0) {
 *       result *= base
 *       exp--
 *   }
 */
static void compile_power(Compiler *comp) {
    compile_unary(comp);

    if (!comp->has_error && comp->current_token.type == TOKEN_CARET) {
        advance_token(comp);

        /* 保存底数 */
        int base_loc = alloc_data(comp);
        emit(comp, SML_STORE * 100 + base_loc);

        /* 编译指数 */
        compile_unary(comp);
        int exp_loc = alloc_data(comp);
        emit(comp, SML_STORE * 100 + exp_loc);

        /* 初始化 result = 1 */
        int result_loc = alloc_data(comp);
        int one_loc = get_or_create_constant(comp, 1);
        emit(comp, SML_LOAD * 100 + one_loc);
        emit(comp, SML_STORE * 100 + result_loc);

        /* 循环: while (exp > 0) */
        int loop_start = comp->instruction_counter;

        /* 检查 exp > 0 */
        emit(comp, SML_LOAD * 100 + exp_loc);
        int branch_loc = comp->instruction_counter;
        emit(comp, SML_BRANCHZERO * 100 + 0);  /* 占位，稍后填写 */
        emit(comp, SML_BRANCHNEG * 100 + 0);   /* 占位 */

        /* result *= base */
        emit(comp, SML_LOAD * 100 + result_loc);
        emit(comp, SML_MULTIPLY * 100 + base_loc);
        emit(comp, SML_STORE * 100 + result_loc);

        /* exp-- */
        emit(comp, SML_LOAD * 100 + exp_loc);
        emit(comp, SML_SUBTRACT * 100 + one_loc);
        emit(comp, SML_STORE * 100 + exp_loc);

        /* 跳回循环开始 */
        emit(comp, SML_BRANCH * 100 + loop_start);

        /* 填写循环出口地址 */
        int loop_end = comp->instruction_counter;
        comp->memory[branch_loc] = SML_BRANCHZERO * 100 + loop_end;
        comp->memory[branch_loc + 1] = SML_BRANCHNEG * 100 + loop_end;

        /* 加载结果 */
        emit(comp, SML_LOAD * 100 + result_loc);
    }
}

/**
 * @brief 编译乘除模运算 (*, /, %)
 *
 * @param comp 编译器指针
 *
 * 二元运算编译模式:
 *   1. 编译左操作数 (结果在 AC)
 *   2. 保存 AC 到临时位置
 *   3. 编译右操作数 (结果在 AC)
 *   4. 保存右操作数，加载左操作数
 *   5. 执行运算
 */
static void compile_term(Compiler *comp) {
    compile_power(comp);

    while (!comp->has_error &&
           (comp->current_token.type == TOKEN_STAR ||
            comp->current_token.type == TOKEN_SLASH ||
            comp->current_token.type == TOKEN_PERCENT)) {
        TokenType op = comp->current_token.type;
        advance_token(comp);

        /* 保存左操作数 */
        int temp = alloc_data(comp);
        emit(comp, SML_STORE * 100 + temp);

        /* 编译右操作数 */
        compile_power(comp);

        /* 交换操作数: 右存临时，左入 AC */
        int temp2 = alloc_data(comp);
        emit(comp, SML_STORE * 100 + temp2);   /* 保存右 */
        emit(comp, SML_LOAD * 100 + temp);     /* 加载左 */

        /* 执行运算 */
        if (op == TOKEN_STAR) {
            emit(comp, SML_MULTIPLY * 100 + temp2);
        } else if (op == TOKEN_SLASH) {
            emit(comp, SML_DIVIDE * 100 + temp2);
        } else {
            emit(comp, SML_MOD * 100 + temp2);
        }
    }
}

/**
 * @brief 编译加减运算 (+, -)
 *
 * @param comp 编译器指针
 */
static void compile_expression(Compiler *comp) {
    compile_term(comp);

    while (!comp->has_error &&
           (comp->current_token.type == TOKEN_PLUS ||
            comp->current_token.type == TOKEN_MINUS)) {
        TokenType op = comp->current_token.type;
        advance_token(comp);

        /* 保存左操作数 */
        int temp = alloc_data(comp);
        emit(comp, SML_STORE * 100 + temp);

        /* 编译右操作数 */
        compile_term(comp);

        /* 交换并运算 */
        int temp2 = alloc_data(comp);
        emit(comp, SML_STORE * 100 + temp2);
        emit(comp, SML_LOAD * 100 + temp);

        if (op == TOKEN_PLUS) {
            emit(comp, SML_ADD * 100 + temp2);
        } else {
            emit(comp, SML_SUBTRACT * 100 + temp2);
        }
    }
}

/* ============================================================================
 *                              语句编译
 * ============================================================================
 *
 * 每种语句生成相应的 SML 指令序列。
 */

/**
 * @brief 编译 rem 语句 (注释)
 *
 * 注释不生成任何代码。
 */
static void compile_rem(Compiler *comp) {
    (void)comp;
}

/**
 * @brief 编译 input 语句
 *
 * 语法: input var [, var ...]
 * 生成: READ 指令
 */
static void compile_input(Compiler *comp) {
    advance_token(comp);

    do {
        if (comp->current_token.type == TOKEN_COMMA) {
            advance_token(comp);
        }

        if (comp->current_token.type != TOKEN_IDENT) {
            set_error(comp, "Expected variable after 'input'");
            return;
        }

        int idx = var_index(comp->current_token.text[0]);
        if (idx < 0) {
            set_error(comp, "Invalid variable: %s", comp->current_token.text);
            return;
        }

        int loc = get_or_create_variable(comp, idx);
        emit(comp, SML_READ * 100 + loc);  /* READ 到变量地址 */
        advance_token(comp);

    } while (comp->current_token.type == TOKEN_COMMA);
}

/**
 * @brief 编译 print 语句
 *
 * 语法: print expr | print "string" | print expr, expr, ...
 * 生成: 表达式求值 + WRITE 或 WRITES 指令
 */
static void compile_print(Compiler *comp) {
    advance_token(comp);

    /* 空 print 只输出换行 */
    if (comp->current_token.type == TOKEN_NEWLINE ||
        comp->current_token.type == TOKEN_EOF) {
        emit(comp, SML_NEWLINE * 100 + 0);
        return;
    }

    do {
        if (comp->current_token.type == TOKEN_COMMA) {
            advance_token(comp);
        }

        if (comp->current_token.type == TOKEN_STRING) {
            /* 输出字符串 */
            int str_loc = store_string(comp, comp->current_token.text);
            if (str_loc >= 0) {
                emit(comp, SML_WRITES * 100 + str_loc);
            }
            advance_token(comp);
        } else if (comp->current_token.type != TOKEN_NEWLINE &&
                   comp->current_token.type != TOKEN_EOF &&
                   comp->current_token.type != TOKEN_COMMA) {
            /* 输出表达式 */
            compile_expression(comp);
            if (comp->has_error) return;

            /* 将结果存入临时位置，然后输出 */
            int temp = alloc_data(comp);
            emit(comp, SML_STORE * 100 + temp);
            emit(comp, SML_WRITE * 100 + temp);
        }
    } while (comp->current_token.type == TOKEN_COMMA);

    /* 输出换行 */
    emit(comp, SML_NEWLINE * 100 + 0);
}

/**
 * @brief 编译 let 语句
 *
 * 语法: let var = expr 或 let var(index) = expr
 * 生成: 表达式求值 + STORE 指令
 */
static void compile_let(Compiler *comp) {
    advance_token(comp);

    if (comp->current_token.type != TOKEN_IDENT) {
        set_error(comp, "Expected variable after 'let'");
        return;
    }

    int idx = var_index(comp->current_token.text[0]);
    if (idx < 0) {
        set_error(comp, "Invalid variable: %s", comp->current_token.text);
        return;
    }
    advance_token(comp);

    int loc;
    /* 检查是否是数组赋值 */
    if (comp->current_token.type == TOKEN_LPAREN) {
        advance_token(comp);

        if (comp->current_token.type != TOKEN_NUMBER) {
            set_error(comp, "Array index must be a constant (SML limitation)");
            return;
        }
        int array_idx = (int)comp->current_token.num_value;
        advance_token(comp);

        if (comp->current_token.type != TOKEN_RPAREN) {
            set_error(comp, "Expected ')' after array index");
            return;
        }
        advance_token(comp);

        /* 获取数组元素地址 */
        Symbol *arr = find_symbol(comp, SYMBOL_ARRAY, idx);
        if (!arr) {
            int size = (array_idx + 1 > 10) ? array_idx + 1 : 10;
            get_or_create_array(comp, idx, size);
            arr = find_symbol(comp, SYMBOL_ARRAY, idx);
        }

        if (!arr) {
            set_error(comp, "Failed to create array");
            return;
        }

        if (array_idx < 0 || array_idx >= arr->size) {
            set_error(comp, "Array index %d out of bounds (0-%d)",
                      array_idx, arr->size - 1);
            return;
        }

        loc = arr->location - array_idx;
    } else {
        loc = get_or_create_variable(comp, idx);
    }

    if (comp->current_token.type != TOKEN_ASSIGN) {
        set_error(comp, "Expected '=' in let statement");
        return;
    }
    advance_token(comp);

    /* 编译表达式 (结果在 AC) */
    compile_expression(comp);
    if (comp->has_error) return;

    /* 存储结果 */
    emit(comp, SML_STORE * 100 + loc);
}

/**
 * @brief 编译 goto 语句
 *
 * 语法: goto line_number
 * 生成: BRANCH 指令
 *
 * 如果目标行号尚未定义，记录为前向引用。
 */
static void compile_goto(Compiler *comp) {
    advance_token(comp);

    if (comp->current_token.type != TOKEN_NUMBER) {
        set_error(comp, "Expected line number after 'goto'");
        return;
    }

    int target_line = (int)comp->current_token.num_value;
    Symbol *sym = find_symbol(comp, SYMBOL_LINE, target_line);

    if (sym) {
        /* 目标行已定义，直接生成完整指令 */
        emit(comp, SML_BRANCH * 100 + sym->location);
    } else {
        /* 前向引用，记录待填充 */
        add_flag(comp, comp->instruction_counter, target_line);
        emit(comp, SML_BRANCH * 100 + 0);  /* 地址暂时为 0 */
    }
    advance_token(comp);
}

/**
 * @brief 编译 if 语句
 *
 * 语法: if expr op expr goto line_number
 * 生成: 表达式求值 + 条件跳转指令
 *
 * SML 只有 BRANCHZERO 和 BRANCHNEG，需要组合实现所有比较:
 *   - ==: left - right, BRANCHZERO
 *   - <:  left - right, BRANCHNEG
 *   - >:  right - left, BRANCHNEG
 *   - <=: left - right, BRANCHNEG + BRANCHZERO
 *   - >=: BRANCHZERO, then right - left, BRANCHNEG
 *   - !=: BRANCHNEG (either direction)
 */
static void compile_if(Compiler *comp) {
    advance_token(comp);

    /* 编译左表达式 */
    compile_expression(comp);
    if (comp->has_error) return;

    int temp_left = alloc_data(comp);
    emit(comp, SML_STORE * 100 + temp_left);

    /* 获取比较运算符 */
    TokenType op = comp->current_token.type;
    if (op != TOKEN_EQ && op != TOKEN_NE && op != TOKEN_LT &&
        op != TOKEN_GT && op != TOKEN_LE && op != TOKEN_GE) {
        set_error(comp, "Expected comparison operator in if statement");
        return;
    }
    advance_token(comp);

    /* 编译右表达式 */
    compile_expression(comp);
    if (comp->has_error) return;

    int temp_right = alloc_data(comp);
    emit(comp, SML_STORE * 100 + temp_right);

    /* 计算 left - right */
    emit(comp, SML_LOAD * 100 + temp_left);
    emit(comp, SML_SUBTRACT * 100 + temp_right);

    /* 期望 'goto' */
    if (comp->current_token.type != TOKEN_GOTO) {
        set_error(comp, "Expected 'goto' in if statement");
        return;
    }
    advance_token(comp);

    if (comp->current_token.type != TOKEN_NUMBER) {
        set_error(comp, "Expected line number after 'goto'");
        return;
    }

    int target_line = (int)comp->current_token.num_value;
    Symbol *sym = find_symbol(comp, SYMBOL_LINE, target_line);
    int target_loc = sym ? sym->location : 0;

    /* 根据比较运算符生成跳转指令 */
    switch (op) {
        case TOKEN_EQ:  /* == : 如果 left - right == 0 */
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHZERO * 100 + target_loc);
            break;

        case TOKEN_LT:  /* < : 如果 left - right < 0 */
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            break;

        case TOKEN_GT:  /* > : 如果 right - left < 0 */
            emit(comp, SML_LOAD * 100 + temp_right);
            emit(comp, SML_SUBTRACT * 100 + temp_left);
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            break;

        case TOKEN_LE:  /* <= : 如果 left - right <= 0 */
            if (!sym) {
                add_flag(comp, comp->instruction_counter, target_line);
                add_flag(comp, comp->instruction_counter + 1, target_line);
            }
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            emit(comp, SML_BRANCHZERO * 100 + target_loc);
            break;

        case TOKEN_GE:  /* >= : 如果 left - right >= 0 */
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHZERO * 100 + target_loc);
            emit(comp, SML_LOAD * 100 + temp_right);
            emit(comp, SML_SUBTRACT * 100 + temp_left);
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            break;

        case TOKEN_NE:  /* != : 如果 left - right != 0 */
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            emit(comp, SML_LOAD * 100 + temp_right);
            emit(comp, SML_SUBTRACT * 100 + temp_left);
            if (!sym) add_flag(comp, comp->instruction_counter, target_line);
            emit(comp, SML_BRANCHNEG * 100 + target_loc);
            break;

        default:
            break;
    }

    advance_token(comp);
}

/**
 * @brief 编译 for 语句
 *
 * 语法: for var = start to end [step value]
 * 生成: 初始化代码 + 循环头 (循环体由后续行提供)
 *
 * 循环状态保存在 for_stack 中，供 next 语句使用。
 */
static void compile_for(Compiler *comp) {
    advance_token(comp);

    /* 获取循环变量 */
    if (comp->current_token.type != TOKEN_IDENT) {
        set_error(comp, "Expected variable after 'for'");
        return;
    }
    char loop_var = comp->current_token.text[0];
    int idx = var_index(loop_var);
    if (idx < 0) {
        set_error(comp, "Invalid loop variable");
        return;
    }
    int var_loc = get_or_create_variable(comp, idx);
    advance_token(comp);

    if (comp->current_token.type != TOKEN_ASSIGN) {
        set_error(comp, "Expected '=' in for statement");
        return;
    }
    advance_token(comp);

    /* 编译起始值并存储 */
    compile_expression(comp);
    if (comp->has_error) return;
    emit(comp, SML_STORE * 100 + var_loc);

    if (comp->current_token.type != TOKEN_TO) {
        set_error(comp, "Expected 'to' in for statement");
        return;
    }
    advance_token(comp);

    /* 编译结束值并存储 */
    compile_expression(comp);
    if (comp->has_error) return;
    int end_loc = alloc_data(comp);
    emit(comp, SML_STORE * 100 + end_loc);

    /* 处理可选的 step */
    int step_loc;
    int step_is_negative = 0;
    if (comp->current_token.type == TOKEN_STEP) {
        advance_token(comp);

        if (comp->current_token.type == TOKEN_MINUS) {
            advance_token(comp);
            if (comp->current_token.type == TOKEN_NUMBER) {
                int step_val = -(int)comp->current_token.num_value;
                step_loc = get_or_create_constant(comp, step_val);
                step_is_negative = 1;
                advance_token(comp);
            } else {
                set_error(comp, "Step must be a constant number");
                return;
            }
        } else if (comp->current_token.type == TOKEN_NUMBER) {
            int step_val = (int)comp->current_token.num_value;
            step_loc = get_or_create_constant(comp, step_val);
            step_is_negative = (step_val < 0);
            advance_token(comp);
        } else {
            set_error(comp, "Step must be a constant number");
            return;
        }
    } else {
        step_loc = get_or_create_constant(comp, 1);
        step_is_negative = 0;
    }

    /* 保存循环状态 */
    if (comp->for_depth >= MAX_FOR_DEPTH) {
        set_error(comp, "For loop nested too deep");
        return;
    }
    ForCompileState *state = &comp->for_stack[comp->for_depth++];
    state->var = loop_var;
    state->var_location = var_loc;
    state->end_location = end_loc;
    state->step_location = step_loc;
    state->step_is_negative = step_is_negative;
    state->loop_start = comp->instruction_counter;  /* 循环体起始地址 */
}

/**
 * @brief 编译 next 语句
 *
 * 语法: next var
 * 生成: 更新循环变量 + 条件跳转回循环开始
 */
static void compile_next(Compiler *comp) {
    advance_token(comp);

    if (comp->current_token.type != TOKEN_IDENT) {
        set_error(comp, "Expected variable after 'next'");
        return;
    }
    char loop_var = comp->current_token.text[0];
    advance_token(comp);

    if (comp->for_depth == 0) {
        set_error(comp, "next without for");
        return;
    }

    ForCompileState *state = &comp->for_stack[comp->for_depth - 1];
    if (state->var != loop_var) {
        set_error(comp, "next variable mismatch: expected '%c', got '%c'",
                  state->var, loop_var);
        return;
    }

    /* 更新循环变量: var += step */
    emit(comp, SML_LOAD * 100 + state->var_location);
    emit(comp, SML_ADD * 100 + state->step_location);
    emit(comp, SML_STORE * 100 + state->var_location);

    /* 检查循环条件
     * 正步长: var - end <= 0 则继续
     * 负步长: end - var <= 0 则继续 */
    if (state->step_is_negative) {
        emit(comp, SML_LOAD * 100 + state->end_location);
        emit(comp, SML_SUBTRACT * 100 + state->var_location);
    } else {
        emit(comp, SML_LOAD * 100 + state->var_location);
        emit(comp, SML_SUBTRACT * 100 + state->end_location);
    }

    /* 如果 <= 0，跳回循环开始 */
    emit(comp, SML_BRANCHNEG * 100 + state->loop_start);
    emit(comp, SML_BRANCHZERO * 100 + state->loop_start);

    /* 弹出循环状态 */
    comp->for_depth--;
}

/**
 * @brief 编译 end 语句
 *
 * 生成: HALT 指令
 */
static void compile_end(Compiler *comp) {
    emit(comp, SML_HALT * 100 + 0);
}

/**
 * @brief 编译单行语句
 *
 * @param comp 编译器指针
 * @param line 行起始位置
 */
static void compile_line(Compiler *comp, const char *line) {
    lexer_reset_line(&comp->lexer, line);
    advance_token(comp);

    /* 获取行号 */
    if (comp->current_token.type != TOKEN_NUMBER) {
        return;  /* 空行或无行号 */
    }
    comp->current_line_number = (int)comp->current_token.num_value;

    /* 记录行号对应的指令地址 */
    add_symbol(comp, SYMBOL_LINE, comp->current_line_number, comp->instruction_counter);

    advance_token(comp);

    /* 根据关键字编译 */
    switch (comp->current_token.type) {
        case TOKEN_REM:   compile_rem(comp);   break;
        case TOKEN_INPUT: compile_input(comp); break;
        case TOKEN_PRINT: compile_print(comp); break;
        case TOKEN_LET:   compile_let(comp);   break;
        case TOKEN_GOTO:  compile_goto(comp);  break;
        case TOKEN_IF:    compile_if(comp);    break;
        case TOKEN_FOR:   compile_for(comp);   break;
        case TOKEN_NEXT:  compile_next(comp);  break;
        case TOKEN_END:   compile_end(comp);   break;
        case TOKEN_NEWLINE:
        case TOKEN_EOF:
            break;
        default:
            set_error(comp, "Line %d: Unknown statement: %s",
                      comp->current_line_number, comp->current_token.text);
            break;
    }
}

/* ============================================================================
 *                              第二遍编译: 解决前向引用
 * ============================================================================ */

/**
 * @brief 解决所有前向引用
 *
 * 遍历 flags 表，将所有未填充的跳转地址填充完整。
 */
static void resolve_flags(Compiler *comp) {
    for (int i = 0; i < comp->flag_count; i++) {
        int inst_loc = comp->flags[i].instruction_location;
        int target_line = comp->flags[i].target_line_number;

        /* 查找目标行号 */
        Symbol *sym = find_symbol(comp, SYMBOL_LINE, target_line);
        if (!sym) {
            set_error(comp, "Undefined line number: %d", target_line);
            return;
        }

        /* 修补指令: 保持操作码，填充地址 */
        int instruction = comp->memory[inst_loc];
        int opcode = instruction / 100;
        comp->memory[inst_loc] = opcode * 100 + sym->location;
    }
}

/* ============================================================================
 *                              公开 API
 * ============================================================================ */

/**
 * @brief 初始化编译器
 */
void compiler_init(Compiler *comp) {
    memset(comp, 0, sizeof(Compiler));
    comp->data_counter = MEMORY_SIZE - 1;  /* 数据区从 99 开始 */
}

/**
 * @brief 编译源代码字符串
 */
int compiler_compile(Compiler *comp, const char *source) {
    comp->source = strdup(source);
    if (!comp->source) {
        set_error(comp, "Memory allocation failed");
        return 0;
    }

    lexer_init(&comp->lexer, comp->source);

    /* 第一遍: 逐行编译 */
    const char *line_start = comp->source;
    while (*line_start) {
        while (*line_start == ' ' || *line_start == '\t') {
            line_start++;
        }

        if (*line_start && *line_start != '\n') {
            compile_line(comp, line_start);
            if (comp->has_error) return 0;
        }

        while (*line_start && *line_start != '\n') {
            line_start++;
        }
        if (*line_start == '\n') {
            line_start++;
        }
    }

    /* 第二遍: 解决前向引用 */
    resolve_flags(comp);

    return !comp->has_error;
}

/**
 * @brief 从文件编译
 */
int compiler_compile_file(Compiler *comp, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        set_error(comp, "Cannot open file: %s", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(file);
        set_error(comp, "Memory allocation failed");
        return 0;
    }

    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);

    int result = compiler_compile(comp, content);
    free(content);
    return result;
}

/**
 * @brief 输出 SML 程序到文件
 */
int compiler_output(Compiler *comp, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        set_error(comp, "Cannot create file: %s", filename);
        return 0;
    }

    /* 输出所有 100 个内存单元 */
    for (int i = 0; i < MEMORY_SIZE; i++) {
        fprintf(file, "%+05d\n", comp->memory[i]);
    }

    fclose(file);
    return 1;
}

/**
 * @brief 打印 SML 程序 (调试用)
 */
void compiler_dump(Compiler *comp) {
    printf("=== SML Program ===\n");
    printf("Instructions (0-%d):\n", comp->instruction_counter - 1);

    /* 操作码名称表 */
    const char *op_names[] = {
        [10] = "READ", [11] = "WRITE", [12] = "NEWLINE", [13] = "WRITES",
        [20] = "LOAD", [21] = "STORE",
        [30] = "ADD", [31] = "SUB", [32] = "DIV", [33] = "MUL", [34] = "MOD",
        [40] = "JMP", [41] = "JMPNEG", [42] = "JMPZERO", [43] = "HALT"
    };

    for (int i = 0; i < comp->instruction_counter; i++) {
        int inst = comp->memory[i];
        int opcode = inst / 100;
        int operand = inst % 100;
        const char *name = (opcode >= 10 && opcode <= 43) ? op_names[opcode] : "???";
        printf("  %02d: %+05d  %-8s %02d\n", i, inst, name, operand);
    }

    printf("\nData (%d-99):\n", comp->data_counter + 1);
    for (int i = MEMORY_SIZE - 1; i > comp->data_counter; i--) {
        printf("  %02d: %+05d", i, comp->memory[i]);
        if (comp->memory[i] >= 32 && comp->memory[i] < 127) {
            printf("  '%c'", comp->memory[i]);
        }
        printf("\n");
    }
}

/**
 * @brief 打印符号表 (调试用)
 */
void compiler_dump_symbols(Compiler *comp) {
    printf("=== Symbol Table ===\n");
    for (int i = 0; i < comp->symbol_count; i++) {
        Symbol *sym = &comp->symbols[i];
        const char *type_str;
        switch (sym->type) {
            case SYMBOL_LINE: type_str = "LINE"; break;
            case SYMBOL_VARIABLE: type_str = "VAR"; break;
            case SYMBOL_CONSTANT: type_str = "CONST"; break;
            case SYMBOL_ARRAY: type_str = "ARRAY"; break;
            case SYMBOL_STRING: type_str = "STRING"; break;
            default: type_str = "?"; break;
        }
        if (sym->type == SYMBOL_VARIABLE) {
            printf("  %-6s '%c' -> loc %02d\n", type_str, 'a' + sym->symbol, sym->location);
        } else {
            printf("  %-6s %3d -> loc %02d\n", type_str, sym->symbol, sym->location);
        }
    }
}

void compiler_free(Compiler *comp) {
    if (comp->source) {
        free(comp->source);
        comp->source = NULL;
    }
}

const char* compiler_get_error(Compiler *comp) {
    return comp->error_message;
}

const int* compiler_get_memory(Compiler *comp) {
    return comp->memory;
}
