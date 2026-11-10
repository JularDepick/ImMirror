# 脚本运行时 (自有 DSL + 寄存器式虚拟机)

本模块实现里程碑 M7 的脚本运行时, 由自有语法文本编译为定长字节码并在沙盒中执行, 不嵌入任何第三方脚本语言。字节码随资源包下发, 客户端只解释执行, 不执行机器码。

## 一、目录与职责

```
script/
├── CMakeLists.txt          # 模块构建: Android 只收集运行期源文件
├── README.md               # 本文件
├── ast.h                   # 语法树节点定义
├── ast.cpp                 # 语法树深拷贝
├── lexer.h                 # 词法分析接口
├── lexer.cpp               # 词法分析实现
├── parser.h                # 语法分析接口
├── parser.cpp              # 递归下降语法分析实现
├── bytecode.h              # 指令集, 常量池, 原型, 序列化接口
├── bytecode.cpp            # 指令编码, 反汇编, 序列化, 静态校验
├── compiler.h              # 编译入口
├── compiler.cpp            # 源码到字节码的编译实现
├── value.h                 # 值, 表, 区域分配器, 字符串驻留
├── value.cpp               # 值系统实现
├── sandbox.h               # 沙盒限制, 错误码, 统计
├── sandbox.cpp             # 沙盒实现
├── binding.h               # 白名单 native 函数注册
├── binding.cpp             # 内置与世界 binding 实现
├── vm.h                    # 虚拟机接口
├── vm.cpp                  # 虚拟机实现
├── benchmark.h             # 性能基准入口声明
├── main.cpp                # 主机命令行工具, 不进入 Android 库
└── tests/
    ├── CMakeLists.txt      # 测试与基准构建
    ├── test_framework.h    # 极简断言框架
    ├── test_support.h      # 编译执行辅助
    ├── test_main.cpp       # 测试入口
    ├── test_language.cpp   # 语言特性用例
    ├── test_sandbox.cpp    # 沙盒与错误用例
    ├── test_bytecode.cpp   # 字节码校验与序列化用例
    └── bench.cpp           # 斐波那契与表操作基准
```

四层结构如下: 词法与语法分析产出语法树; 编译器把语法树翻译为寄存器式定长字节码; 虚拟机解释执行字节码; 沙盒在虚拟机外围限制指令数, 内存与时间。宿主通过白名单 binding 暴露能力, 脚本无法触碰文件与网络。

## 二、语法参考

### 2.1 注释

```
// 行注释, 到行尾结束
/* 块注释, 不嵌套,
   可以跨行 */
```

### 2.2 字面量

| 类型 | 写法 | 说明 |
|:---:|:---|:---|
| nil | `nil` | 空值 |
| 布尔 | `true` `false` | 逻辑值 |
| 整数 | `42` `-7` `0x1F` | 64 位有符号整数, 支持十进制与十六进制 |
| 浮点 | `1.5` `2.0e3` | 双精度, 必须有小数点或指数 |
| 字符串 | `"文本"` `'文本'` | UTF-8 字节串, 单双引号等价 |
| 表 | `{ 1, 2, name = "x" }` | 见 2.7 |

字符串转义: `\n` `\t` `\r` `\0` `\\` `\'` `\"` `\xNN` `\uXXXX`。其中 `\uXXXX` 只支持 BMP 码点, 按 UTF-8 编码写入。

### 2.3 变量与作用域

```
var a = 1;              // 可变局部变量
var x, y = 1, 2;        // 一次声明多个
var q, r = divmod(7, 2);// 函数返回多个值时按位置绑定, 不足补 nil
const limit = 10;       // 只读局部变量, 赋值会在编译期报错
counter = 0;            // 未声明的名字是全局变量
```

局部变量在块内有效, 每次进入 `{}` 形成新作用域, 离开时释放其寄存器。全局变量存放在每次执行独立的全局表中。

### 2.4 函数

```
fn add(a, b) {
    return a + b;
}

fn divmod(a, b) {
    return a / b, a % b;     // 多返回值
}

fn noop() { }                 // 无返回值

var f = fn (x) { return x * 2; };  // 匿名函数表达式
var t = { double = fn (x) { return x * 2; } };
t.double(3);
```

约定:
- 主代码块顶层的 `fn` 声明登记为全局函数, 因此函数之间可以相互调用, 也支持互递归。
- 函数体内部的 `fn` 声明是局部闭包, 只在当前作用域可见。
- 嵌套函数暂不支持捕获外层局部变量, 编译期会给出明确错误, 需要共享状态时改为传参或使用表。

### 2.5 控制流

```
if (a > b) {
    work();
} else if (a == b) {
    tie();
} else {
    other();
}

while (i < 10) {
    i = i + 1;
    if (i == 3) { continue; }
    if (i == 8) { break; }
}

for (var i = 0; i < 10; i = i + 1) {
    total = total + i;
}

for (var i = 10; i > 0; i -= 1) { }   // 递减循环与复合赋值
```

`for` 的初始化子句可以是 `var` 声明或赋值语句, 条件子句可省略, 步进子句是赋值语句。编译器把 `for` 展开成等价的 `while` 结构, 因此 `break` 与 `continue` 语义与 `while` 一致。

`break` 与 `continue` 只能出现在循环体内, 否则编译报错。

### 2.6 运算符

优先级由低到高:

| 优先级 | 运算符 | 结合性 |
|:---:|:---|:---:|
| 1 | `or` `||` | 左 |
| 2 | `and` `&&` | 左 |
| 3 | `==` `!=` | 左 |
| 4 | `<` `<=` `>` `>=` | 左 |
| 5 | `+` `-` | 左 |
| 6 | `*` `/` `%` | 左 |
| 7 | `-` `!` `not` | 右, 一元 |
| 8 | `()` `[]` `. ` 调用 | 左, 后缀 |

语义要点:
- `and` 与 `or` 短路求值, 返回操作数本身而不是布尔值, 与常见脚本语言一致。
- 真值判定: 只有 `nil` 与 `false` 为假, `0` 与空串为真。
- `+` 有一侧为字符串时执行拼接, 数值与布尔会被转成文本。
- 两个整数相除是截断除法, 例如 `7 / 2 == 3`; 有一侧是浮点时按浮点除。
- 整数溢出自动提升为浮点, 不会产生未定义行为。
- 除以零返回运行时错误, 浮点除零同样报错。
- 比较运算支持数值与数值, 字符串与字符串; 类型不匹配返回运行时错误。
- `==` 与 `!=` 在类型不同时返回 `false` / `true`, 不做隐式转换, 但整数与整数值浮点可相等。
- 字符串没有下标运算, 取长度请用 `len(s)`。

### 2.7 表

```
var t = { 1, 2, 3 };                 // 位置字段, 键从 1 开始
var u = { name = "abc", hp = 10 };   // 具名字段
var v = { ["k" + "1"] = 1 };          // 计算键
t[4] = 40;                            // 下标访问
t.hp = 100;                           // 点访问
var a = t[1];
var b = u.name;
len(t);                               // 数组部分从 1 开始的连续非 nil 长度
```

表内部由数组部分与哈希部分构成: 连续的 1 到 N 整数键落在数组部分, 其余键落在哈希部分。`nil` 不能作为键, 赋值为 `nil` 表示删除该键但仍占位。

### 2.8 全局函数

运行时默认注册以下白名单函数, 全部由宿主注入, 详见第七节:

| 名称 | 参数 | 返回 | 说明 |
|:---:|:---|:---:|:---|
| `log` | 0 到 4 个 | 无 | 输出到宿主日志通道 |
| `type` | 1 个 | 字符串 | 返回 nil/bool/int/double/string/table/closure |
| `tostring` | 1 个 | 字符串 | 文本形式 |
| `tonumber` | 1 个 | 数值或 nil | 字符串解析失败返回 nil |
| `len` | 1 个 | 整数 | 字符串字节数或表的数组长度 |
| `abs` `floor` `ceil` `sqrt` | 1 个数值 | 数值 | 数学函数 |
| `min` `max` | 2 个数值 | 数值 | 取小/取大 |
| `world_get_block` | 3 个整数 | 整数 | 读取方块 id |
| `world_set_block` | 4 个整数 | 布尔 | 写入方块 id |
| `player_position` | 无 | 3 个浮点 | 玩家坐标 x, y, z |

## 三、字节码格式

### 3.1 指令编码

每条指令固定 32 位, 高 8 位为操作码, 其余 24 位按三种格式解释:

```
iABC : 31..24 操作码 | 23..16 A | 15..8 B | 7..0 C
iABx : 31..24 操作码 | 23..16 A | 15..0 Bx
iAsBx: 31..24 操作码 | 23..16 A | 15..0 sBx (有符号 16 位)
```

因此单个函数最多 250 个寄存器, 最多 65535 个常量, 跳转范围为正负 32767 条指令。

### 3.2 指令表

| 操作码 | 格式 | 操作数 | 语义 |
|:---:|:---:|:---:|:---|
| NOP | iABC | 无 | 空操作, 保留给补丁使用 |
| LOADNIL | iABC | A | R[A] = nil |
| LOADBOOL | iABC | A, B | R[A] = (B != 0) |
| LOADINT | iAsBx | A, sBx | R[A] = sBx, 16 位立即数 |
| LOADK | iABx | A, Bx | R[A] = K[Bx] |
| MOVE | iABC | A, B | R[A] = R[B] |
| GETGLOBAL | iABx | A, Bx | R[A] = 全局表[K[Bx]] |
| SETGLOBAL | iABx | A, Bx | 全局表[K[Bx]] = R[A] |
| NEWTABLE | iABC | A, B, C | R[A] = 新表, B 与 C 是数组与哈希容量提示 |
| GETTABLE | iABC | A, B, C | R[A] = R[B][R[C]] |
| SETTABLE | iABC | A, B, C | R[A][R[B]] = R[C] |
| GETFIELD | iABC | A, B, C | R[A] = R[B].K[C], K[C] 必须是字符串常量 |
| SETFIELD | iABC | A, B, C | R[A].K[C] = R[B], K[C] 必须是字符串常量 |
| LEN | iABC | A, B | R[A] = 长度(R[B]) |
| ADD | iABC | A, B, C | R[A] = R[B] + R[C], 字符串相加为拼接 |
| SUB | iABC | A, B, C | R[A] = R[B] - R[C] |
| MUL | iABC | A, B, C | R[A] = R[B] * R[C] |
| DIV | iABC | A, B, C | R[A] = R[B] / R[C] |
| MOD | iABC | A, B, C | R[A] = R[B] % R[C] |
| UNM | iABC | A, B | R[A] = -R[B] |
| NOT | iABC | A, B | R[A] = 逻辑非(R[B]) |
| EQ | iABC | A, B, C | R[A] = 相等(R[B], R[C]) |
| NE | iABC | A, B, C | R[A] = 不等(R[B], R[C]) |
| LT | iABC | A, B, C | R[A] = 小于(R[B], R[C]) |
| LE | iABC | A, B, C | R[A] = 小于等于(R[B], R[C]) |
| GT | iABC | A, B, C | R[A] = 大于(R[B], R[C]) |
| GE | iABC | A, B, C | R[A] = 大于等于(R[B], R[C]) |
| JMP | iAsBx | sBx | pc = pc + sBx |
| JMPIF | iAsBx | A, sBx | R[A] 为真时 pc = pc + sBx |
| JMPIFNOT | iAsBx | A, sBx | R[A] 为假时 pc = pc + sBx |
| CALL | iABC | A, B, C | 调用 R[A], B 个实参在 R[A+1] 起, C 个返回值写回 R[A] 起, C 为 0 表示全部结果 |
| RETURN | iABC | A, B | 返回 R[A] 起的 B 个值, B 至少为 1 |
| RETURNZERO | 无 | 无 | 返回 0 个值 |
| RETURNMULTI | iABC | A | 返回 R[A] 到栈顶的全部值, 只由 return 调用表达式生成 |
| CLOSURE | iABx | A, Bx | R[A] = 由原型常量 K[Bx] 构造的闭包 |

### 3.3 常量池与原型

每个原型独立持有常量池。常量类型为 nil, bool, int, double, string, proto 六种, 编译期按类型与取值去重。原型记录函数名, 定义行, 参数个数, 栈帧寄存器数, 指令序列, 行号表, 子原型下标表。

行号表与指令一一对应, 用于错误定位; 编译选项 `stripLineInfo` 为真时不写行号表, 字节码随之变小。

### 3.4 序列化

全部多字节整数为小端序:

| 区段 | 长度 | 说明 |
|:---:|:---:|:---|
| 魔数 | 4 | `IMSC` |
| 版本 | 2 | 当前为 1 |
| 保留 | 2 | 必须为 0 |
| 原型数量 | 4 | 至少为 1 |
| 入口原型 | 4 | 主原型下标 |
| 脚本名 | 4 + N | 长度加 UTF-8 字节 |
| 原型循环体 | 变长 | 见下 |

单个原型依次写入: 名称, 定义行, 参数个数, 寄存器个数, 保留, 指令条数, 指令字数组, 常量个数, 常量数组, 子原型个数, 子原型下标数组, 行号个数, 行号差值。行号差值为 zigzag 编码的变长整数, 重复行只占 1 字节。

加载时必须通过静态校验, 校验项包括: 魔数, 版本, 截断, 操作码范围, 寄存器下标, 常量下标与类型, 跳转目标, 原型下标, 栈帧尺寸, 入口下标, 末尾必须是返回指令。校验失败返回错误码与消息, 不会崩溃。

### 3.5 栈帧与调用约定

寄存器栈是一维数组。调用时函数位于 R[A], 实参位于 R[A+1] 起, 新帧的基址为当前帧基址加 A 加 1, 因此被调函数看到的是连续的参数区块。返回时结果复制回调用者 R[A] 起的位置, 期望个数不足补 nil, 多余丢弃。

多返回值有两种来源: 显式 `return a, b` 编译为 RETURN 指令; `return f()` 编译为 CALL 后接 RETURNMULTI, 结果个数由运行时确定。

### 3.6 内存管理

本实现选择"区域分配 + 帧结束回收": 一次 `Execute` 内的全部表, 闭包与运行期字符串都从区域分配器申请, 执行结束时整体重置, 重置时按登记顺序调用析构函数。

选择理由:
- 脚本执行是短周期任务, 没有跨执行的堆对象存活问题。
- 不需要写屏障, 也不需要处理循环引用。
- 回收点明确, 时序可预期, 不会因为回收时机引入帧卡顿。
- 与"内存配额按单次执行统计"的沙盒模型天然一致。

代价与约束: 执行期间不回收垃圾, 因此单次执行的内存上限等于峰值分配量; 需要长期驻留的状态由宿主保存在 binding 一侧, 而不是钉在脚本堆上。

字符串驻留: 同内容的字符串共用同一个 `StringObject`, 哈希由 FNV-1a 计算, 常量字符串在每次执行开始时统一驻留并缓存到按原型索引的表中, 运行期拼接触发驻留查找。

## 四、沙盒

`SandboxLimits` 的默认值:

| 限制 | 默认值 | 触发后的错误码 |
|:---:|:---:|:---|
| 指令预算 | 2000000 条每次执行 | InstructionBudget |
| 内存配额 | 4 MiB | MemoryQuota |
| 执行超时 | 500 毫秒 | Timeout |
| 调用深度 | 64 层 | CallDepth |
| 寄存器栈 | 4096 个槽位 | StackOverflow |
| 超时检测间隔 | 每 256 条指令 | 无 |

统计信息 `SandboxStats` 记录指令数, 内存峰值, 最大调用深度, 表与字符串创建次数, 耗时微秒。

超时检测按指令间隔抽样读时钟, 避免每条指令都做一次系统调用。内存配额在三个位置检查: 表内部存储增长, 字符串驻留, 以及算术拼接与原生调用之后。

除配额之外, 沙盒还提供两类保护:
- 加载期静态校验: 非法操作码, 越界寄存器与常量, 越界跳转在进入执行前就被拒绝。
- 调用期边界检查: 新帧写入前检查栈容量, 返回值数量写回前检查目标区间, 指令指针越界立即报错。

脚本没有文件, 网络, 反射或动态代码加载能力, 能看到的只有全局表里的白名单函数。

## 五、错误处理

模块内部不使用异常做控制流。编译期通过 `Diagnostic` 结构累积错误, 包含行号, 列号与中文消息; 运行期通过 `VmError` 枚举加消息文本返回, 由 `VmResult` 携带错误码, 消息, 行号与统计。

运行期错误码:

| 错误码 | 含义 |
|:---:|:---|
| InstructionBudget | 指令预算耗尽 |
| Timeout | 执行超时 |
| MemoryQuota | 内存配额超限 |
| CallDepth | 调用深度超限 |
| StackOverflow | 栈空间不足 |
| RegisterRange | 寄存器下标越界 |
| InvalidOpcode | 非法指令 |
| ConstantRange | 常量下标越界 |
| TypeMismatch | 操作数类型不符 |
| DivisorZero | 除数为零 |
| NotCallable | 调用了非函数值 |
| NotIndexable | 对非表值取下标 |
| ArityMismatch | 实参个数不符 |
| BindingRejected | binding 拒绝调用 |
| BindingType | binding 参数类型不符 |
| NativeFailure | 原生函数内部失败 |
| Internal | 内部状态异常 |

原生函数返回约定: 非负数为结果个数, 负数为 `-VmError` 的数值, 失败时通过 `NativeCall::errorMessage` 附消息。

## 六、宿主接口

### 6.1 白名单注册

`BindingRegistry` 只登记显式注册的函数, 注册阶段校验签名合法性, 重复注册返回错误。签名由最小实参个数, 最大实参个数与逐参数类型组成; 数值类型允许整数与浮点互通, 其余类型严格匹配。

```
BindingRegistry registry;
std::string error;
registry.RegisterFunction("world_get_block", &MyGetBlock,
                          { ValueType::Int, ValueType::Int, ValueType::Int }, 3,
                          "读取方块 id", error);
```

### 6.2 宿主世界

`HostWorld` 是引擎侧实现的抽象: 读方块, 写方块, 读玩家坐标, 写日志。虚拟机把 `Vm::SetHost` 安装的指针原样传给 `NativeCall::host`, binding 自行转换为 `HostWorld*`。未挂载宿主时世界函数返回 `BindingRejected`, 不会崩溃。

## 七、命令行工具

工具只在主机构建中编译, 不进入 Android 库:

```
immirror-script compile 源码.ims [-o 输出.imsc] [--strip-debug]
immirror-script disasm  字节码.imsc|源码.ims
immirror-script run     字节码.imsc|源码.ims [--limit 指令数] [--memory 字节] [--timeout 毫秒]
immirror-script bench
```

扩展名 `.ims` 视为源码, 其余视为字节码。退出码 0 表示成功, 1 表示用法错误, 2 表示编译或加载失败, 3 表示运行时错误。

### 7.1 构建与测试

```
# 主机工具与测试 (独立配置, 默认开启主机工具)
cmake -S single-branch/client/src/native/script -B temp/build-script -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build temp/build-script -j 8

# 单元测试加基准
temp/build-script/tests/immirror-script-tests.exe

# 只跑单元测试
temp/build-script/tests/immirror-script-tests.exe --no-bench
```

作为 `src/native` 子目录构建时, `IMMIRROR_SCRIPT_HOST_TOOLS` 默认关闭, 只把运行期源文件汇总进 `immirror_core`, 命令行与测试不参与 Android 产物。

### 7.2 实测数据

单元测试: 59 个用例, 199 个断言, 全部通过。

性能基准 (Windows x86_64, Release, 取三轮最优):

| 基准 | 指令数 | 耗时 | 每百万指令耗时 | 吞吐 |
|:---:|:---:|:---:|:---:|:---:|
| 斐波那契 fib(24) 累计和 | 2913370 | 27.841 ms | 9.556 ms | 104.6 MIPS |
| 表读写 1000 项乘 200 轮 | 3017814 | 14.393 ms | 4.769 ms | 209.7 MIPS |

同一台机器多次实测, 每百万指令耗时在 9.5 到 11.7 ms 与 4.6 到 5.3 ms 之间波动, 上表取其中较优的一轮。

字节码体积示例:

| 源码 | 指令数 | 原型数 | 序列化体积 |
|:---|:---:|:---:|:---:|
| `return 1;` | 3 | 1 | 73 字节 |
| `fn add(a, b) { return a + b; } return add(2, 3);` | 13 | 2 | 171 字节 |
| 表构造加字段读写加 len | 15 | 1 | 142 字节 |

虚拟机内部实现细节对性能影响最大的三点: 常量字符串在执行前预驻留, 指令循环内不做寄存器边界检查而是依赖加载期校验, 整数与浮点在加法处先走整数快路径。

## 八、扩展手册

### 8.1 新增一条指令

1. 在 `bytecode.h` 的 `enum class Op` 中, 在 `Count` 之前加入新操作码, 并写清格式与操作数含义。
2. 在 `bytecode.cpp` 的 `OpName` 中加入名称。
3. 在 `bytecode.cpp` 的 `FormatInstruction` 中加入反汇编文本。
4. 在 `bytecode.cpp` 的 `VerifyProgram` 中加入操作数校验: 寄存器下标, 常量下标与类型, 跳转目标, 栈帧关系。
5. 在 `vm.cpp` 的 `Run` 中加入执行分支, 失败路径只设置 `error_` 与 `errorMessage_` 后 break。
6. 在 `compiler.cpp` 中按需发射新指令, 并在发射前确保写到的寄存器已经被 `Reserve` 占用。
7. 在 `tests/test_bytecode.cpp` 中增加手工构造的正例与反例。
8. 更新本文件第三节的指令表。

### 8.2 新增一个 binding

1. 在 `binding.cpp` 的匿名命名空间中写静态函数, 签名固定为 `int32_t 名称(NativeCall& call)`。
2. 从 `call.args` 读取参数, 自行再做一次类型与范围检查, 用 `Fail` 返回 `-VmError` 与消息。
3. 把结果写入 `call.results`, 返回结果个数; 需要字符串时调用 `call.makeString`, 不要直接持有内存。
4. 在 `RegisterStandardBindings` 或 `RegisterWorldBindings` 中登记名称, 逐参数类型与最大实参个数。
5. 在 `tests/test_language.cpp` 中增加正例与失败路径用例, 更新本文件 2.8 节的函数表。

### 8.3 新增一种编译期语法

1. 在 `lexer.h` 的 `TokenType` 加词法单元, 在 `lexer.cpp` 的关键字表或符号分支中识别, 在 `TokenTypeName` 补名称。
2. 在 `ast.h` 的 `StmtKind` 或 `ExprKind` 加节点种类, 复用现有字段, 必要时在 `ast.cpp` 的克隆函数中同步。
3. 在 `parser.cpp` 增加解析函数, 接入 `ParseStatement` 或优先级链; 保证失败路径返回 false, 循环中每次至少前进一个词法单元。
4. 在 `compiler.cpp` 增加编译分支, 表达式必须把结果写入传入的 target 寄存器, 并用 `RegScope` 自动归还临时寄存器。
5. 增加单元测试, 更新本文件第二节。

## 九、已知限制

- 嵌套函数不捕获外层局部变量, 编译期报错; 顶层 `fn` 是全局函数, 不受此限制。
- 不支持变长参数, 不支持 `for ... in` 遍历, 不支持位运算与三目运算符。
- 字符串不支持下标与切片, `len` 按字节计。
- 整数与浮点混合运算统一转浮点, 超大整数比较使用精确算法, 不会因为尾数丢失给出错误结论。
- 每次执行都会重置全局环境, 跨执行的游戏状态必须放在宿主侧。
- 没有垃圾回收之外的压缩或分代策略, 单次执行的内存峰值等于分配总量。
- 字节码不携带源码文本, 调试只依赖行号表。
