# CMakeLists.txt
# 支票保护系统 - CMake构建配置

cmake_minimum_required(VERSION 3.16)
project(CheckProtection
    VERSION 1.0.0
    DESCRIPTION "支票保护系统 - Check Protection System"
    LANGUAGES CXX
)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# 源文件
set(SOURCES
    check_protection.cpp
    main.cpp
)

# 头文件
set(HEADERS
    check_protection.hpp
)

# 创建可执行文件
add_executable(check_protection ${SOURCES} ${HEADERS})

# 输出编译信息
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
