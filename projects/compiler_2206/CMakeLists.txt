# ============================================================================
#                     Simple 语言编译器/解释器 - CMake 构建配置
# ============================================================================
#
# 本文件定义了项目的构建规则，使用 CMake 跨平台构建系统。
#
# 构建步骤:
#   mkdir build && cd build
#   cmake ..
#   make
#
# 或使用单行命令:
#   cmake -B build -S . && cmake --build build
#
# 构建选项:
#   cmake -DCMAKE_BUILD_TYPE=Debug ..    # 调试模式 (带调试符号)
#   cmake -DCMAKE_BUILD_TYPE=Release ..  # 发布模式 (优化)
#
# ============================================================================

# ----------------------------------------------------------------------------
# CMake 版本要求
# ----------------------------------------------------------------------------
# CMake 3.10+ 提供了现代 CMake 特性:
#   - target_include_directories() 的 PUBLIC/PRIVATE 支持
#   - 更好的生成器表达式支持
#   - 改进的 find_package() 功能
cmake_minimum_required(VERSION 3.10)

# ----------------------------------------------------------------------------
# 项目定义
# ----------------------------------------------------------------------------
# project() 命令定义项目名称和使用的语言
# - compiler_2206: 项目名称，对应生成的 Makefile/工程文件
# - C: 指定项目语言为 C (非 C++)
project(compiler_2206 C)

# ----------------------------------------------------------------------------
# C 语言标准
# ----------------------------------------------------------------------------
# C11 标准特性:
#   - 匿名结构体和联合体
#   - 静态断言 _Static_assert
#   - 泛型选择 _Generic
#   - 对齐说明符 _Alignas/_Alignof
# CMAKE_C_STANDARD_REQUIRED=ON 确保编译器必须支持 C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------------
# 编译器警告选项
# ----------------------------------------------------------------------------
# 仅对 GCC 和 Clang 启用严格警告:
#   -Wall:     启用常见警告 (未使用变量、隐式声明等)
#   -Wextra:   启用额外警告 (未使用参数、符号比较等)
#   -pedantic: 严格遵循 ISO C 标准，禁止编译器扩展
#
# 这些选项帮助发现潜在的 bug 和不规范代码
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ----------------------------------------------------------------------------
# 头文件搜索路径
# ----------------------------------------------------------------------------
# include_directories() 添加头文件搜索路径
# ${CMAKE_SOURCE_DIR} = 项目根目录 (CMakeLists.txt 所在目录)
#
# 这样源文件中可以直接写:
#   #include "lexer.h"
# 而不是:
#   #include "../include/lexer.h"
include_directories(${CMAKE_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
# 源文件列表
# ----------------------------------------------------------------------------
# 定义项目的所有 .c 源文件
# 各文件职责:
#   main.c        - 程序入口，命令行解析，各模式调度
#   lexer.c       - 词法分析器，将源码转换为 Token 流
#   interpreter.c - 解释器，直接执行 Simple 源码
#   compiler.c    - 编译器，将 Simple 编译为 SML 机器码
#   sml_vm.c      - SML 虚拟机，执行编译后的机器码
set(SOURCES
    src/main.c
    src/lexer.c
    src/interpreter.c
    src/compiler.c
    src/sml_vm.c
)

# ----------------------------------------------------------------------------
# 头文件列表 (可选)
# ----------------------------------------------------------------------------
# 将头文件添加到目标中，主要用于:
#   1. IDE 项目视图中显示头文件
#   2. 某些 IDE 的代码补全和跳转功能
# 对编译本身没有影响 (编译器通过 include_directories 找到头文件)
set(HEADERS
    include/token.h
    include/lexer.h
    include/interpreter.h
    include/compiler.h
    include/sml_vm.h
)

# ----------------------------------------------------------------------------
# 可执行文件目标
# ----------------------------------------------------------------------------
# add_executable() 定义要构建的可执行文件:
#   - simple: 生成的可执行文件名
#   - ${SOURCES}: 源文件列表
#   - ${HEADERS}: 头文件列表 (可选，用于 IDE)
#
# 生成的可执行文件:
#   - Linux/macOS: simple
#   - Windows: simple.exe
add_executable(simple ${SOURCES} ${HEADERS})

# ----------------------------------------------------------------------------
# 链接库
# ----------------------------------------------------------------------------
# target_link_libraries() 指定链接的库
#   - m: 数学库 (libm)，提供 pow(), sqrt() 等数学函数
#
# Linux/macOS 需要显式链接数学库
# Windows 的 MSVC 编译器自动包含数学函数，但添加此行不会造成问题
target_link_libraries(simple m)

# ----------------------------------------------------------------------------
# 输出目录配置
# ----------------------------------------------------------------------------
# 设置可执行文件的输出位置
# RUNTIME_OUTPUT_DIRECTORY: 可执行文件输出目录
# ${CMAKE_BINARY_DIR}: 构建目录 (如 build/)
#
# 这确保 simple 直接输出到 build/ 目录，而不是 build/src/ 子目录
set_target_properties(simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
#                              测试配置
# ============================================================================
# 启用 CTest 测试框架
# 运行测试: cd build && ctest 或 make test
enable_testing()

# ----------------------------------------------------------------------------
# 单元测试可执行文件
# ----------------------------------------------------------------------------
# 测试源文件不包含 main.c (避免重复定义 main)
set(TEST_SOURCES_WITHOUT_MAIN
    src/lexer.c
    src/interpreter.c
    src/compiler.c
    src/sml_vm.c
)

# 词法分析器测试
add_executable(test_lexer
    tests/test_lexer.c
    ${TEST_SOURCES_WITHOUT_MAIN}
)
target_include_directories(test_lexer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(test_lexer m)

# 编译器测试
add_executable(test_compiler
    tests/test_compiler.c
    ${TEST_SOURCES_WITHOUT_MAIN}
)
target_include_directories(test_compiler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(test_compiler m)

# SML 虚拟机测试
add_executable(test_sml_vm
    tests/test_sml_vm.c
    ${TEST_SOURCES_WITHOUT_MAIN}
)
target_include_directories(test_sml_vm PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(test_sml_vm m)

# 性能基准测试
add_executable(benchmark
    tests/benchmark.c
    ${TEST_SOURCES_WITHOUT_MAIN}
)
target_include_directories(benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(benchmark m)

# ----------------------------------------------------------------------------
# 注册单元测试到 CTest
# ----------------------------------------------------------------------------
add_test(NAME unit_test_lexer COMMAND test_lexer)
add_test(NAME unit_test_compiler COMMAND test_compiler)
add_test(NAME unit_test_sml_vm COMMAND test_sml_vm)

# ----------------------------------------------------------------------------
# 集成测试 (使用完整的 simple 可执行文件)
# ----------------------------------------------------------------------------
# 测试解释器模式
add_test(
    NAME integration_interpreter_sum
    COMMAND simple ${CMAKE_SOURCE_DIR}/examples/sum.simple
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 测试编译模式
add_test(
    NAME integration_compile_factorial
    COMMAND simple -c ${CMAKE_SOURCE_DIR}/examples/factorial.simple
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 测试编译运行模式
add_test(
    NAME integration_compile_run_countdown
    COMMAND simple -r ${CMAKE_SOURCE_DIR}/examples/countdown.simple
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ============================================================================
#                              安装配置 (可选)
# ============================================================================
# install() 定义 'make install' 的行为
# 默认安装到 /usr/local/bin (可通过 CMAKE_INSTALL_PREFIX 修改)
#
# 使用方法:
#   cmake -DCMAKE_INSTALL_PREFIX=/custom/path ..
#   make install
install(TARGETS simple DESTINATION bin)

# 安装示例文件
install(DIRECTORY examples/ DESTINATION share/simple/examples)

# 安装文档
install(FILES README.md DESTINATION share/simple)
install(DIRECTORY docs/ DESTINATION share/simple/docs)
