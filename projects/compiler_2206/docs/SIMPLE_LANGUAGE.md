# Simple 语言规范

## 概述

Simple 是一种类 BASIC 的简单高级语言，支持基本的顺序、选择和循环结构。

## 程序结构

- 每行一条语句
- 每条语句以行号开头（10, 20, 30...）
- 行号用于 `goto` 跳转

```
10 rem 这是注释
20 input x
30 print x
40 end
```

## 关键字

| 关键字 | 说明 | 示例 |
|--------|------|------|
| `rem` | 注释 | `10 rem 这是注释` |
| `input` | 读取输入 | `20 input x` 或 `20 input x, y, z` |
| `print` | 输出 | `30 print x` 或 `30 print "hello"` |
| `let` | 赋值 | `40 let x = 10 + 5` |
| `goto` | 无条件跳转 | `50 goto 100` |
| `if` | 条件跳转 | `60 if x > 0 goto 100` |
| `for` | 循环开始 | `70 for i = 1 to 10` 或 `70 for i = 1 to 10 step 2` |
| `next` | 循环结束 | `80 next i` |
| `end` | 程序结束 | `90 end` |

## 数据类型

### 1. 整数
```
let x = 42
let y = -10
```

### 2. 浮点数
```
let pi = 3.14159
let f = -2.5
```

### 3. 字符串
```
print "Hello, World!"
let s = "message"
```

### 4. 数组
```
let a(0) = 10
let a(1) = 20
print a(0)
```

## 变量命名

- 单个小写字母: `a` - `z`（26个变量）
- 数组使用括号: `a(0)`, `a(1)`, ...

## 运算符

### 算术运算符
| 运算符 | 说明 | 示例 |
|--------|------|------|
| `+` | 加法 | `let x = a + b` |
| `-` | 减法 | `let x = a - b` |
| `*` | 乘法 | `let x = a * b` |
| `/` | 除法 | `let x = a / b` |
| `%` | 取模 | `let x = a % b` |
| `^` | 幂运算 | `let x = a ^ 2` |

### 关系运算符
| 运算符 | 说明 |
|--------|------|
| `==` | 等于 |
| `!=` | 不等于 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |

## 语句详解

### 1. rem - 注释
```
10 rem 计算两数之和
```

### 2. input - 输入
```
10 input x           // 读取单个变量
20 input x, y, z     // 读取多个变量
```

### 3. print - 输出
```
10 print x           // 输出变量
20 print "result:"   // 输出字符串
30 print x, y, z     // 输出多个值
```

### 4. let - 赋值
```
10 let x = 10
20 let y = x + 5 * 2
30 let a(0) = 100    // 数组赋值
```

### 5. goto - 无条件跳转
```
10 goto 50
...
50 print "jumped here"
```

### 6. if - 条件跳转
```
10 if x == 0 goto 50
20 if x > y goto 100
30 if x != y goto 80
```

### 7. for/next - 循环
```
10 for i = 1 to 10
20   print i
30 next i

// 带步长
10 for i = 0 to 100 step 5
20   print i
30 next i

// 倒序
10 for i = 10 to 1 step -1
20   print i
30 next i
```

### 8. end - 结束
```
100 end
```

## 示例程序

### 示例 1: 两数相加
```
10 rem 计算两数之和
20 input a
30 input b
40 let c = a + b
50 print c
60 end
```

### 示例 2: 求最大值
```
10 rem 找出两数中较大的
20 input x
30 input y
40 if x >= y goto 70
50 print y
60 goto 80
70 print x
80 end
```

### 示例 3: 1到N求和
```
10 rem 计算 1+2+...+n
20 input n
30 let s = 0
40 for i = 1 to n
50   let s = s + i
60 next i
70 print s
80 end
```

### 示例 4: 阶乘
```
10 rem 计算 n!
20 input n
30 let f = 1
40 for i = 1 to n
50   let f = f * i
60 next i
70 print f
80 end
```

### 示例 5: 数组操作
```
10 rem 数组求和
20 let a(0) = 10
30 let a(1) = 20
40 let a(2) = 30
50 let s = a(0) + a(1) + a(2)
60 print s
70 end
```

## 语法错误

解释器/编译器会检测以下错误：

1. **语法错误**: 关键字拼写错误、缺少操作数
2. **未定义变量**: 使用未初始化的变量
3. **除零错误**: 除法运算中除数为零
4. **跳转错误**: goto 目标行号不存在
5. **类型错误**: 字符串参与算术运算
6. **数组越界**: 访问超出数组范围的元素

## SML 指令映射（编译器用）

| Simple 语句 | SML 指令 |
|-------------|----------|
| `input x` | `10xx` (READ) |
| `print x` | `11xx` (WRITE) |
| `let x = ...` | `20xx` (LOAD), `21xx` (STORE), 算术指令 |
| `goto n` | `40xx` (BRANCH) |
| `if ... goto` | `41xx` (BRANCHNEG), `42xx` (BRANCHZERO) |
| `end` | `4300` (HALT) |
