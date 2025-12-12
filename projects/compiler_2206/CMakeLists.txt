cmake_minimum_required(VERSION 3.10)
project(compiler_2206 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 添加 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/main.c
    src/lexer.c
    src/interpreter.c
    src/compiler.c
    src/sml_vm.c
)

# 头文件 (可选，用于 IDE 显示)
set(HEADERS
    include/token.h
    include/lexer.h
    include/interpreter.h
    include/compiler.h
    include/sml_vm.h
)

# 可执行文件
add_executable(simple ${SOURCES} ${HEADERS})

# 链接数学库
target_link_libraries(simple m)

# 设置输出目录
set_target_properties(simple PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
