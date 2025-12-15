/**
 * @file test_framework.h
 * @brief 轻量级单元测试框架
 *
 * 一个简单的 C 语言单元测试框架，无外部依赖。
 *
 * 使用方法:
 * ```c
 * #include "test_framework.h"
 *
 * void test_example(void) {
 *     ASSERT_EQ(1 + 1, 2);
 *     ASSERT_TRUE(1 < 2);
 *     ASSERT_STR_EQ("hello", "hello");
 * }
 *
 * int main(void) {
 *     TEST_BEGIN();
 *     RUN_TEST(test_example);
 *     TEST_END();
 *     return test_failed;
 * }
 * ```
 *
 * 输出示例:
 *   [RUN ] test_example
 *   [PASS] test_example
 *   ================================
 *   Tests: 1 passed, 0 failed
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 *                              全局状态
 * ============================================================================ */

static int tests_run = 0;      /**< 运行的测试总数 */
static int tests_passed = 0;   /**< 通过的测试数 */
static int test_failed = 0;    /**< 当前测试是否失败 */
static int current_failed = 0; /**< 当前测试的失败标志 */

/* ============================================================================
 *                              测试宏
 * ============================================================================ */

/**
 * @brief 开始测试套件
 */
#define TEST_BEGIN() \
    do { \
        printf("================================\n"); \
        printf("    Simple Compiler Tests\n"); \
        printf("================================\n\n"); \
    } while(0)

/**
 * @brief 结束测试套件，打印汇总
 */
#define TEST_END() \
    do { \
        printf("\n================================\n"); \
        printf("Tests: %d passed, %d failed\n", tests_passed, tests_run - tests_passed); \
        printf("================================\n"); \
    } while(0)

/**
 * @brief 运行单个测试函数
 * @param test_func 测试函数指针
 */
#define RUN_TEST(test_func) \
    do { \
        current_failed = 0; \
        printf("[RUN ] %s\n", #test_func); \
        test_func(); \
        tests_run++; \
        if (current_failed) { \
            printf("[FAIL] %s\n", #test_func); \
            test_failed = 1; \
        } else { \
            printf("[PASS] %s\n", #test_func); \
            tests_passed++; \
        } \
    } while(0)

/**
 * @brief 断言两个整数相等
 */
#define ASSERT_EQ(actual, expected) \
    do { \
        long long _a = (long long)(actual); \
        long long _e = (long long)(expected); \
        if (_a != _e) { \
            printf("  ASSERT_EQ failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Expected: %lld\n", _e); \
            printf("    Actual:   %lld\n", _a); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言两个整数不相等
 */
#define ASSERT_NE(actual, expected) \
    do { \
        long long _a = (long long)(actual); \
        long long _e = (long long)(expected); \
        if (_a == _e) { \
            printf("  ASSERT_NE failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Values should not be equal: %lld\n", _a); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言条件为真
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("  ASSERT_TRUE failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Condition: %s\n", #condition); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言条件为假
 */
#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf("  ASSERT_FALSE failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Condition: %s\n", #condition); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言两个字符串相等
 */
#define ASSERT_STR_EQ(actual, expected) \
    do { \
        const char *_a = (actual); \
        const char *_e = (expected); \
        if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0) { \
            printf("  ASSERT_STR_EQ failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Expected: \"%s\"\n", _e ? _e : "(null)"); \
            printf("    Actual:   \"%s\"\n", _a ? _a : "(null)"); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言指针不为空
 */
#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("  ASSERT_NOT_NULL failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Pointer: %s\n", #ptr); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言指针为空
 */
#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("  ASSERT_NULL failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Pointer: %s\n", #ptr); \
            current_failed = 1; \
        } \
    } while(0)

/**
 * @brief 断言两个浮点数近似相等
 * @param actual 实际值
 * @param expected 期望值
 * @param epsilon 允许的误差
 */
#define ASSERT_FLOAT_EQ(actual, expected, epsilon) \
    do { \
        double _a = (double)(actual); \
        double _e = (double)(expected); \
        double _eps = (double)(epsilon); \
        if (fabs(_a - _e) > _eps) { \
            printf("  ASSERT_FLOAT_EQ failed at %s:%d\n", __FILE__, __LINE__); \
            printf("    Expected: %f\n", _e); \
            printf("    Actual:   %f\n", _a); \
            printf("    Epsilon:  %f\n", _eps); \
            current_failed = 1; \
        } \
    } while(0)

#endif /* TEST_FRAMEWORK_H */
