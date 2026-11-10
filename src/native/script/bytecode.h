#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common.h"

namespace immirror::script {

/*
 * 指令集. 每条指令固定 32 位, 高 8 位为操作码, 其余 24 位按三种格式解释:
 *   iABC  : A(8) B(8) C(8)      三个 8 位操作数, 通常为寄存器或小常量下标
 *   iABx  : A(8) Bx(16)         16 位无符号操作数, 通常为常量池下标
 *   iAsBx : A(8) sBx(16)        16 位有符号操作数, 用于相对跳转
 * 因此单个函数最多 250 个寄存器 (留出余量给调用返回值), 最多 65535 个常量.
 */
enum class Op : uint8_t {
    Nop = 0,
    LoadNil,    /* iABC  A: 目标寄存器 */
    LoadBool,   /* iABC  A: 目标, B: 0 或 1 */
    LoadInt,    /* iAsBx A: 目标, sBx: 立即数 */
    LoadK,      /* iABx  A: 目标, Bx: 常量下标 */
    Move,       /* iABC  A: 目标, B: 源 */
    GetGlobal,  /* iABx  A: 目标, Bx: 名字常量下标 */
    SetGlobal,  /* iABx  A: 源, Bx: 名字常量下标 */
    NewTable,   /* iABC  A: 目标, B: 数组部分提示, C: 哈希部分提示 */
    GetTable,   /* iABC  A: 目标, B: 表, C: 键寄存器 */
    SetTable,   /* iABC  A: 表, B: 键寄存器, C: 值寄存器 */
    GetField,   /* iABC  A: 目标, B: 表, C: 字段名常量下标 */
    SetField,   /* iABC  A: 表, B: 值寄存器, C: 字段名常量下标 */
    Len,        /* iABC  A: 目标, B: 源 */
    Add,        /* iABC  A: 目标, B, C: 操作数 */
    Sub,
    Mul,
    Div,
    Mod,
    Unm,        /* iABC  A: 目标, B: 操作数 */
    Not,        /* iABC  A: 目标, B: 操作数 */
    Eq,         /* iABC  A: 目标, B, C: 操作数 */
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Jmp,        /* iAsBx sBx: 相对当前指令的偏移 */
    JmpIf,      /* iAsBx A: 条件寄存器, sBx */
    JmpIfNot,   /* iAsBx A: 条件寄存器, sBx */
    Call,       /* iABC  A: 函数寄存器, B: 实参个数, C: 期望返回值个数, 0 表示全部 */
    Return,     /* iABC  A: 起始寄存器, B: 返回值个数, B 至少为 1 */
    ReturnZero, /* 无操作数, 返回 0 个值 */
    ReturnMulti,/* iABC  A: 起始寄存器, 返回到栈顶为止的全部值 */
    Closure,    /* iABx  A: 目标, Bx: 原型常量下标 */
    Count
};

/* 指令字的高 8 位为操作码, 其余位含义见上 */
constexpr uint32_t kOpcodeShift = 24;
constexpr uint32_t kOperandAShift = 16;
constexpr uint32_t kOperandBShift = 8;

/* 编码与解码辅助函数, 全部为纯函数, 便于测试与反汇编复用 */
constexpr uint32_t EncodeABC(Op op, uint32_t a, uint32_t b, uint32_t c) {
    return (static_cast<uint32_t>(op) << kOpcodeShift) |
           ((a & 0xFFu) << kOperandAShift) |
           ((b & 0xFFu) << kOperandBShift) |
           (c & 0xFFu);
}

constexpr uint32_t EncodeABx(Op op, uint32_t a, uint32_t bx) {
    return (static_cast<uint32_t>(op) << kOpcodeShift) |
           ((a & 0xFFu) << kOperandAShift) |
           (bx & 0xFFFFu);
}

constexpr uint32_t EncodeAsBx(Op op, uint32_t a, int32_t sbx) {
    return (static_cast<uint32_t>(op) << kOpcodeShift) |
           ((a & 0xFFu) << kOperandAShift) |
           (static_cast<uint32_t>(static_cast<uint16_t>(sbx)) & 0xFFFFu);
}

inline Op InstructionOp(uint32_t word) {
    return static_cast<Op>((word >> kOpcodeShift) & 0xFFu);
}

inline uint32_t OperandA(uint32_t word) { return (word >> kOperandAShift) & 0xFFu; }
inline uint32_t OperandB(uint32_t word) { return (word >> kOperandBShift) & 0xFFu; }
inline uint32_t OperandC(uint32_t word) { return word & 0xFFu; }
inline uint32_t OperandBx(uint32_t word) { return word & 0xFFFFu; }

inline int32_t OperandSBx(uint32_t word) {
    return static_cast<int32_t>(static_cast<int16_t>(word & 0xFFFFu));
}

/* 寄存器与参数上限, 编译期与校验期共用 */
constexpr uint32_t kMaxRegisters = 250;
constexpr uint32_t kMaxParams = 32;
constexpr uint32_t kMaxConstants = 65535;

/* 跳转偏移单位为"指令条数", 取值范围与 16 位有符号数一致 */
constexpr int32_t kMaxJumpOffset = 32767;

/* 常量池条目类型 */
enum class ConstantKind : uint8_t {
    Nil = 0,
    Bool,
    Int,
    Double,
    String,
    Proto,
};

/* 常量池条目. 为保持结构简单, 全部字段并列存放, 由 kind 决定有效字段 */
struct Constant {
    ConstantKind kind = ConstantKind::Nil;
    bool boolValue = false;
    int64_t intValue = 0;
    double doubleValue = 0.0;
    std::string stringValue;
    uint32_t protoIndex = 0;

    static Constant MakeNil();
    static Constant MakeBool(bool value);
    static Constant MakeInt(int64_t value);
    static Constant MakeDouble(double value);
    static Constant MakeString(std::string value);
    static Constant MakeProto(uint32_t index);

    bool operator==(const Constant& other) const;
};

/* 函数原型, 一个脚本由若干原型组成, 下标 0 为主原型 */
struct Proto {
    std::string name;                 /* 调试用函数名 */
    uint32_t line = 0;                /* 定义所在行 */
    uint32_t numParams = 0;
    uint32_t numRegs = 0;             /* 栈帧大小, 由编译期确定 */
    std::vector<uint32_t> code;
    std::vector<Constant> constants;
    std::vector<uint32_t> lines;      /* 与 code 等长, 指令到源码行号 */
    std::vector<uint32_t> children;   /* 子原型在 Program 中的下标 */
};

/* 一个完整的可执行脚本 */
struct Program {
    std::string chunkName;
    std::vector<Proto> protos;
    uint32_t entry = 0;

    /* 序列化后的字节数, 用于向宿主报告字节码体积 */
    size_t ByteSize() const;
};

/* 操作码名称, 反汇编与日志共用 */
const char* OpName(Op op);

/* 指令是否带有需要校验的跳转操作数 */
bool IsJumpInstruction(Op op);

/* 反汇编单条指令, 输出形如 "ADD  R3, R1, R2" 的可读文本 */
std::string FormatInstruction(const Program& program, const Proto& proto, uint32_t pc);

/* 反汇编整个原型, 每行带源码行号与指令序号 */
std::string DisassembleProto(const Program& program, const Proto& proto);

/* 反汇编整个脚本 */
std::string DisassembleProgram(const Program& program);

/* 序列化与加载错误码 */
enum class BytecodeError : uint8_t {
    None = 0,
    BadMagic,
    BadVersion,
    Truncated,
    BadOpcode,
    BadRegister,
    BadConstant,
    BadConstantKind,
    BadJump,
    BadProto,
    BadFrameSize,
    EmptyCode,
    TooLarge,
    MissingReturn,
};

const char* BytecodeErrorName(BytecodeError error);

/* 加载结果, message 为可直接打印的中文说明 */
struct BytecodeStatus {
    BytecodeError error = BytecodeError::None;
    std::string message;
    uint32_t offset = 0;

    bool ok() const { return error == BytecodeError::None; }
};

/* 序列化脚本到字节流, 主机编译期与打包流程使用 */
bool SerializeProgram(const Program& program, std::vector<uint8_t>& out, BytecodeStatus& status);

/* 从字节流加载并静态校验, 校验失败一律返回错误而不是崩溃 */
bool DeserializeProgram(const uint8_t* data, size_t size, Program& out, BytecodeStatus& status);

/* 对内存中的脚本做静态校验, 直接编译结果也走同一套规则 */
bool VerifyProgram(const Program& program, BytecodeStatus& status);

}  /* namespace immirror::script */
