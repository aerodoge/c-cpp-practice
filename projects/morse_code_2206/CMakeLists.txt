cmake_minimum_required(VERSION 3.16)
project(morse_code VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 库
add_library(morse_lib
    src/morse_code.cpp
)
target_include_directories(morse_lib PUBLIC include)

# 主程序
add_executable(morse_code src/main.cpp)
target_link_libraries(morse_code PRIVATE morse_lib)
