#include "bytecode.h"

#include <cstdio>
#include <cstring>

namespace immirror::script {

namespace {

const char kMagic[4] = {'I', 'M', 'S', 'C'};
const uint16_t kFormatVersion = 1;

/* 单条指令允许的最大结果个数, 与寄存器数量同量级 */
const uint32_t kMaxResults = kMaxRegisters;

std::string EscapeText(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (char c : text) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20u) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\x%02X",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buffer;
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    return out;
}

std::string FormatConstant(const Constant& constant) {
    switch (constant.kind) {
        case ConstantKind::Nil:
            return "nil";
        case ConstantKind::Bool:
            return constant.boolValue ? "true" : "false";
        case ConstantKind::Int:
            return std::to_string(constant.intValue);
        case ConstantKind::Double: {
            char buffer[40];
            std::snprintf(buffer, sizeof(buffer), "%g", constant.doubleValue);
            return buffer;
        }
        case ConstantKind::String:
            return "\"" + EscapeText(constant.stringValue) + "\"";
        case ConstantKind::Proto:
            return "<原型 " + std::to_string(constant.protoIndex) + ">";
    }
    return "<?>";
}

const char* ConstantKindName(ConstantKind kind) {
    switch (kind) {
        case ConstantKind::Nil: return "nil";
        case ConstantKind::Bool: return "bool";
        case ConstantKind::Int: return "int";
        case ConstantKind::Double: return "double";
        case ConstantKind::String: return "string";
        case ConstantKind::Proto: return "proto";
    }
    return "?";
}

/* 小端写入器, 保证字节码跨平台一致 */
struct ByteWriter {
    std::vector<uint8_t>& out;

    void U8(uint32_t value) { out.push_back(static_cast<uint8_t>(value & 0xFFu)); }

    void U16(uint32_t value) {
        U8(value & 0xFFu);
        U8((value >> 8) & 0xFFu);
    }

    void U32(uint32_t value) {
        U16(value & 0xFFFFu);
        U16((value >> 16) & 0xFFFFu);
    }

    void I64(int64_t value) {
        const uint64_t raw = static_cast<uint64_t>(value);
        for (int i = 0; i < 8; ++i) {
            U8(static_cast<uint32_t>((raw >> (i * 8)) & 0xFFu));
        }
    }

    void F64(double value) {
        uint64_t raw = 0;
        std::memcpy(&raw, &value, sizeof(raw));
        I64(static_cast<int64_t>(raw));
    }

    void Text(const std::string& text) {
        U32(static_cast<uint32_t>(text.size()));
        out.insert(out.end(), text.begin(), text.end());
    }

    void VarU32(uint32_t value) {
        while (value >= 0x80u) {
            U8((value & 0x7Fu) | 0x80u);
            value >>= 7;
        }
        U8(value);
    }

    void VarS32(int32_t value) {
        const uint32_t zigzag = (static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31);
        VarU32(zigzag);
    }
};

/* 小端读取器, 任意越界都置 ok=false 并保留已读内容 */
struct ByteReader {
    const uint8_t* data;
    size_t size;
    size_t pos = 0;
    bool ok = true;

    bool U8(uint32_t& value) {
        if (!ok || pos + 1 > size) {
            ok = false;
            return false;
        }
        value = data[pos++];
        return true;
    }

    bool U16(uint32_t& value) {
        uint32_t low = 0;
        uint32_t high = 0;
        if (!U8(low) || !U8(high)) {
            return false;
        }
        value = low | (high << 8);
        return true;
    }

    bool U32(uint32_t& value) {
        uint32_t low = 0;
        uint32_t high = 0;
        if (!U16(low) || !U16(high)) {
            return false;
        }
        value = low | (high << 16);
        return true;
    }

    bool I64(int64_t& value) {
        uint64_t raw = 0;
        for (int i = 0; i < 8; ++i) {
            uint32_t byte = 0;
            if (!U8(byte)) {
                return false;
            }
            raw |= static_cast<uint64_t>(byte) << (i * 8);
        }
        value = static_cast<int64_t>(raw);
        return true;
    }

    bool F64(double& value) {
        int64_t raw = 0;
        if (!I64(raw)) {
            return false;
        }
        const uint64_t bits = static_cast<uint64_t>(raw);
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool Text(std::string& text) {
        uint32_t length = 0;
        if (!U32(length) || pos + length > size) {
            ok = false;
            return false;
        }
        text.assign(reinterpret_cast<const char*>(data + pos), length);
        pos += length;
        return true;
    }

    bool VarU32(uint32_t& value) {
        uint32_t result = 0;
        uint32_t shift = 0;
        for (;;) {
            uint32_t byte = 0;
            if (!U8(byte)) {
                return false;
            }
            result |= (byte & 0x7Fu) << shift;
            if ((byte & 0x80u) == 0) {
                break;
            }
            shift += 7;
            if (shift > 28) {
                ok = false;
                return false;
            }
        }
        value = result;
        return true;
    }

    bool VarS32(int32_t& value) {
        uint32_t zigzag = 0;
        if (!VarU32(zigzag)) {
            return false;
        }
        value = static_cast<int32_t>((zigzag >> 1) ^ (~(zigzag & 1u) + 1u));
        return true;
    }
};

bool Fail(BytecodeStatus& status, BytecodeError error, const char* text, uint32_t offset) {
    status.error = error;
    status.message = text;
    status.offset = offset;
    return false;
}

/* 校验单个寄存器下标是否落在帧内 */
bool CheckRegister(const Proto& proto, uint32_t reg, const char* what, BytecodeStatus& status,
                   uint32_t pc) {
    if (reg >= proto.numRegs) {
        status.error = BytecodeError::BadRegister;
        status.message = std::string(what) + " 寄存器越界";
        status.offset = pc;
        return false;
    }
    return true;
}

bool CheckConstantIndex(const Proto& proto, uint32_t index, BytecodeStatus& status, uint32_t pc) {
    if (index >= proto.constants.size()) {
        status.error = BytecodeError::BadConstant;
        status.message = "常量下标越界";
        status.offset = pc;
        return false;
    }
    return true;
}

bool CheckStringConstant(const Proto& proto, uint32_t index, BytecodeStatus& status, uint32_t pc) {
    if (!CheckConstantIndex(proto, index, status, pc)) {
        return false;
    }
    if (proto.constants[index].kind != ConstantKind::String) {
        status.error = BytecodeError::BadConstantKind;
        status.message = "操作数引用的常量不是字符串";
        status.offset = pc;
        return false;
    }
    return true;
}

bool CheckJumpTarget(const Proto& proto, uint32_t pc, int32_t sbx, BytecodeStatus& status) {
    const int64_t target = static_cast<int64_t>(pc) + 1 + sbx;
    if (target < 0 || target > static_cast<int64_t>(proto.code.size())) {
        status.error = BytecodeError::BadJump;
        status.message = "跳转目标越界";
        status.offset = pc;
        return false;
    }
    return true;
}

}  /* namespace */

Constant Constant::MakeNil() {
    Constant constant;
    constant.kind = ConstantKind::Nil;
    return constant;
}

Constant Constant::MakeBool(bool value) {
    Constant constant;
    constant.kind = ConstantKind::Bool;
    constant.boolValue = value;
    return constant;
}

Constant Constant::MakeInt(int64_t value) {
    Constant constant;
    constant.kind = ConstantKind::Int;
    constant.intValue = value;
    constant.doubleValue = static_cast<double>(value);
    return constant;
}

Constant Constant::MakeDouble(double value) {
    Constant constant;
    constant.kind = ConstantKind::Double;
    constant.doubleValue = value;
    return constant;
}

Constant Constant::MakeString(std::string value) {
    Constant constant;
    constant.kind = ConstantKind::String;
    constant.stringValue = std::move(value);
    return constant;
}

Constant Constant::MakeProto(uint32_t index) {
    Constant constant;
    constant.kind = ConstantKind::Proto;
    constant.protoIndex = index;
    return constant;
}

bool Constant::operator==(const Constant& other) const {
    if (kind != other.kind) {
        return false;
    }
    switch (kind) {
        case ConstantKind::Nil: return true;
        case ConstantKind::Bool: return boolValue == other.boolValue;
        case ConstantKind::Int: return intValue == other.intValue;
        case ConstantKind::Double: return doubleValue == other.doubleValue;
        case ConstantKind::String: return stringValue == other.stringValue;
        case ConstantKind::Proto: return protoIndex == other.protoIndex;
    }
    return false;
}

size_t Program::ByteSize() const {
    std::vector<uint8_t> buffer;
    BytecodeStatus status;
    SerializeProgram(*this, buffer, status);
    return buffer.size();
}

const char* OpName(Op op) {
    switch (op) {
        case Op::Nop: return "NOP";
        case Op::LoadNil: return "LOADNIL";
        case Op::LoadBool: return "LOADBOOL";
        case Op::LoadInt: return "LOADINT";
        case Op::LoadK: return "LOADK";
        case Op::Move: return "MOVE";
        case Op::GetGlobal: return "GETGLOBAL";
        case Op::SetGlobal: return "SETGLOBAL";
        case Op::NewTable: return "NEWTABLE";
        case Op::GetTable: return "GETTABLE";
        case Op::SetTable: return "SETTABLE";
        case Op::GetField: return "GETFIELD";
        case Op::SetField: return "SETFIELD";
        case Op::Len: return "LEN";
        case Op::Add: return "ADD";
        case Op::Sub: return "SUB";
        case Op::Mul: return "MUL";
        case Op::Div: return "DIV";
        case Op::Mod: return "MOD";
        case Op::Unm: return "UNM";
        case Op::Not: return "NOT";
        case Op::Eq: return "EQ";
        case Op::Ne: return "NE";
        case Op::Lt: return "LT";
        case Op::Le: return "LE";
        case Op::Gt: return "GT";
        case Op::Ge: return "GE";
        case Op::Jmp: return "JMP";
        case Op::JmpIf: return "JMPIF";
        case Op::JmpIfNot: return "JMPIFNOT";
        case Op::Call: return "CALL";
        case Op::Return: return "RETURN";
        case Op::ReturnZero: return "RETURNZERO";
        case Op::ReturnMulti: return "RETURNMULTI";
        case Op::Closure: return "CLOSURE";
        case Op::Count: break;
    }
    return "<非法>";
}

std::string FormatInstruction(const Program& program, const Proto& proto, uint32_t pc) {
    if (pc >= proto.code.size()) {
        return "<越界>";
    }
    const uint32_t word = proto.code[pc];
    const Op op = InstructionOp(word);
    char buffer[96];
    const uint32_t a = OperandA(word);
    const uint32_t b = OperandB(word);
    const uint32_t c = OperandC(word);
    const uint32_t bx = OperandBx(word);
    const int32_t sbx = OperandSBx(word);

    switch (op) {
        case Op::Nop:
            return "";
        case Op::LoadNil:
            std::snprintf(buffer, sizeof(buffer), "R%u", a);
            return buffer;
        case Op::LoadBool:
            std::snprintf(buffer, sizeof(buffer), "R%u, %s", a, b != 0 ? "true" : "false");
            return buffer;
        case Op::LoadInt:
            std::snprintf(buffer, sizeof(buffer), "R%u, %d", a, sbx);
            return buffer;
        case Op::LoadK: {
            const std::string value =
                bx < proto.constants.size() ? FormatConstant(proto.constants[bx]) : "<?>";
            std::snprintf(buffer, sizeof(buffer), "R%u, K%u (%s)", a, bx, value.c_str());
            return buffer;
        }
        case Op::Move:
            std::snprintf(buffer, sizeof(buffer), "R%u, R%u", a, b);
            return buffer;
        case Op::GetGlobal:
        case Op::SetGlobal: {
            const std::string name = bx < proto.constants.size() ? proto.constants[bx].stringValue
                                                                : std::string("<?>");
            std::snprintf(buffer, sizeof(buffer), "R%u, %s", a, name.c_str());
            return buffer;
        }
        case Op::NewTable:
            std::snprintf(buffer, sizeof(buffer), "R%u, 数组 %u, 哈希 %u", a, b, c);
            return buffer;
        case Op::GetTable:
        case Op::SetTable:
            std::snprintf(buffer, sizeof(buffer), "R%u, R%u, R%u", a, b, c);
            return buffer;
        case Op::GetField:
        case Op::SetField: {
            const std::string name = c < proto.constants.size() ? proto.constants[c].stringValue
                                                               : std::string("<?>");
            std::snprintf(buffer, sizeof(buffer), "R%u, R%u, %s", a, b, name.c_str());
            return buffer;
        }
        case Op::Len:
        case Op::Unm:
        case Op::Not:
            std::snprintf(buffer, sizeof(buffer), "R%u, R%u", a, b);
            return buffer;
        case Op::Add:
        case Op::Sub:
        case Op::Mul:
        case Op::Div:
        case Op::Mod:
        case Op::Eq:
        case Op::Ne:
        case Op::Lt:
        case Op::Le:
        case Op::Gt:
        case Op::Ge:
            std::snprintf(buffer, sizeof(buffer), "R%u, R%u, R%u", a, b, c);
            return buffer;
        case Op::Jmp:
            std::snprintf(buffer, sizeof(buffer), "%+d -> %u", sbx,
                          static_cast<unsigned>(static_cast<int64_t>(pc) + 1 + sbx));
            return buffer;
        case Op::JmpIf:
        case Op::JmpIfNot:
            std::snprintf(buffer, sizeof(buffer), "R%u, %+d -> %u", a, sbx,
                          static_cast<unsigned>(static_cast<int64_t>(pc) + 1 + sbx));
            return buffer;
        case Op::Call:
            std::snprintf(buffer, sizeof(buffer), "R%u, 实参 %u, 返回值 %u", a, b, c);
            return buffer;
        case Op::Return:
            std::snprintf(buffer, sizeof(buffer), "R%u, 个数 %u", a, b);
            return buffer;
        case Op::ReturnZero:
            return "";
        case Op::ReturnMulti:
            std::snprintf(buffer, sizeof(buffer), "R%u, 到栈顶", a);
            return buffer;
        case Op::Closure: {
            const std::string name =
                bx < proto.constants.size() && proto.constants[bx].kind == ConstantKind::Proto
                    ? "<原型 " + std::to_string(proto.constants[bx].protoIndex) + ">"
                    : std::string("<?>");
            std::snprintf(buffer, sizeof(buffer), "R%u, K%u %s", a, bx, name.c_str());
            return buffer;
        }
        case Op::Count:
            break;
    }
    (void)program;
    (void)b;
    return "<非法指令>";
}

std::string DisassembleProto(const Program& program, const Proto& proto) {
    std::string out;
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "; 原型 %s 参数 %u 寄存器 %u 指令 %u 常量 %u\n", 
                  proto.name.c_str(), static_cast<unsigned>(proto.numParams),
                  static_cast<unsigned>(proto.numRegs),
                  static_cast<unsigned>(proto.code.size()),
                  static_cast<unsigned>(proto.constants.size()));
    out += buffer;
    for (size_t i = 0; i < proto.constants.size(); ++i) {
        std::snprintf(buffer, sizeof(buffer), ";   K%u = %s (%s)\n", static_cast<unsigned>(i),
                      FormatConstant(proto.constants[i]).c_str(),
                      ConstantKindName(proto.constants[i].kind));
        out += buffer;
    }
    for (size_t pc = 0; pc < proto.code.size(); ++pc) {
        const uint32_t line = pc < proto.lines.size() ? proto.lines[pc] : 0;
        std::snprintf(buffer, sizeof(buffer), "[%04u] 行 %-4u %-10s %s\n",
                      static_cast<unsigned>(pc), line,
                      OpName(InstructionOp(proto.code[pc])),
                      FormatInstruction(program, proto, static_cast<uint32_t>(pc)).c_str());
        out += buffer;
    }
    return out;
}

std::string DisassembleProgram(const Program& program) {
    std::string out = "; 脚本 " + program.chunkName + " 原型数量 " +
                      std::to_string(program.protos.size()) + " 入口 " +
                      std::to_string(program.entry) + "\n";
    for (size_t i = 0; i < program.protos.size(); ++i) {
        out += "\n";
        out += "; ===== 原型 " + std::to_string(i) + " =====\n";
        out += DisassembleProto(program, program.protos[i]);
    }
    return out;
}

const char* BytecodeErrorName(BytecodeError error) {
    switch (error) {
        case BytecodeError::None: return "无错误";
        case BytecodeError::BadMagic: return "文件头魔数不匹配";
        case BytecodeError::BadVersion: return "字节码版本不匹配";
        case BytecodeError::Truncated: return "字节流被截断";
        case BytecodeError::BadOpcode: return "非法操作码";
        case BytecodeError::BadRegister: return "寄存器下标越界";
        case BytecodeError::BadConstant: return "常量下标越界";
        case BytecodeError::BadConstantKind: return "常量类型不匹配";
        case BytecodeError::BadJump: return "跳转目标越界";
        case BytecodeError::BadProto: return "原型下标越界";
        case BytecodeError::BadFrameSize: return "栈帧尺寸非法";
        case BytecodeError::EmptyCode: return "函数体为空";
        case BytecodeError::TooLarge: return "字节码超过容量上限";
        case BytecodeError::MissingReturn: return "函数缺少返回指令";
    }
    return "未知错误";
}

bool SerializeProgram(const Program& program, std::vector<uint8_t>& out, BytecodeStatus& status) {
    out.clear();
    ByteWriter writer{out};
    out.insert(out.end(), kMagic, kMagic + 4);
    writer.U16(kFormatVersion);
    writer.U16(0);
    writer.U32(static_cast<uint32_t>(program.protos.size()));
    writer.U32(program.entry);
    writer.Text(program.chunkName);

    for (const Proto& proto : program.protos) {
        writer.Text(proto.name);
        writer.U32(proto.line);
        writer.U8(proto.numParams);
        writer.U8(proto.numRegs);
        writer.U16(0);
        writer.U32(static_cast<uint32_t>(proto.code.size()));
        for (uint32_t word : proto.code) {
            writer.U32(word);
        }
        writer.U32(static_cast<uint32_t>(proto.constants.size()));
        for (const Constant& constant : proto.constants) {
            writer.U8(static_cast<uint32_t>(constant.kind));
            switch (constant.kind) {
                case ConstantKind::Nil:
                    break;
                case ConstantKind::Bool:
                    writer.U8(constant.boolValue ? 1u : 0u);
                    break;
                case ConstantKind::Int:
                    writer.I64(constant.intValue);
                    break;
                case ConstantKind::Double:
                    writer.F64(constant.doubleValue);
                    break;
                case ConstantKind::String:
                    writer.Text(constant.stringValue);
                    break;
                case ConstantKind::Proto:
                    writer.U32(constant.protoIndex);
                    break;
            }
        }
        writer.U32(static_cast<uint32_t>(proto.children.size()));
        for (uint32_t child : proto.children) {
            writer.U32(child);
        }
        writer.U32(static_cast<uint32_t>(proto.lines.size()));
        uint32_t previous = 0;
        for (size_t i = 0; i < proto.lines.size(); ++i) {
            const uint32_t line = proto.lines[i];
            const int32_t delta = static_cast<int32_t>(line) - static_cast<int32_t>(previous);
            writer.VarS32(delta);
            previous = line;
        }
    }

    if (out.size() > (1u << 24)) {
        status.error = BytecodeError::TooLarge;
        status.message = "字节码体积超过 16 MB 上限";
        out.clear();
        return false;
    }
    status.error = BytecodeError::None;
    status.message.clear();
    return true;
}

bool DeserializeProgram(const uint8_t* data, size_t size, Program& out, BytecodeStatus& status) {
    out = Program();
    status = BytecodeStatus();
    if (data == nullptr || size < 12) {
        return Fail(status, BytecodeError::Truncated, "字节流短于最小头部长度", 0);
    }
    if (std::memcmp(data, kMagic, 4) != 0) {
        return Fail(status, BytecodeError::BadMagic, "文件头魔数不是 IMSC", 0);
    }
    ByteReader reader{data, size};
    reader.pos = 4;
    uint32_t version = 0;
    uint32_t flags = 0;
    if (!reader.U16(version) || !reader.U16(flags)) {
        return Fail(status, BytecodeError::Truncated, "版本字段缺失", 4);
    }
    if (version != kFormatVersion) {
        return Fail(status, BytecodeError::BadVersion, "字节码版本不受支持", 4);
    }
    uint32_t protoCount = 0;
    uint32_t entry = 0;
    if (!reader.U32(protoCount) || !reader.U32(entry)) {
        return Fail(status, BytecodeError::Truncated, "原型数量或入口缺失", 8);
    }
    if (protoCount == 0 || protoCount > 4096) {
        return Fail(status, BytecodeError::BadProto, "原型数量超出合理范围", 8);
    }
    if (!reader.Text(out.chunkName)) {
        return Fail(status, BytecodeError::Truncated, "脚本名缺失", reader.pos);
    }
    out.protos.resize(protoCount);

    for (uint32_t index = 0; index < protoCount; ++index) {
        Proto& proto = out.protos[index];
        uint32_t numParams = 0;
        uint32_t numRegs = 0;
        uint32_t skipped = 0;
        if (!reader.Text(proto.name) || !reader.U32(proto.line) || !reader.U8(numParams) ||
            !reader.U8(numRegs) || !reader.U16(skipped)) {
            return Fail(status, BytecodeError::Truncated, "原型头部缺失", reader.pos);
        }
        proto.numParams = numParams;
        proto.numRegs = numRegs;

        uint32_t codeSize = 0;
        if (!reader.U32(codeSize) || codeSize > (1u << 20)) {
            return Fail(status, BytecodeError::Truncated, "指令数量非法", reader.pos);
        }
        proto.code.resize(codeSize);
        for (uint32_t pc = 0; pc < codeSize; ++pc) {
            if (!reader.U32(proto.code[pc])) {
                return Fail(status, BytecodeError::Truncated, "指令流被截断", reader.pos);
            }
        }

        uint32_t constCount = 0;
        if (!reader.U32(constCount) || constCount > kMaxConstants) {
            return Fail(status, BytecodeError::Truncated, "常量数量非法", reader.pos);
        }
        proto.constants.resize(constCount);
        for (uint32_t i = 0; i < constCount; ++i) {
            Constant& constant = proto.constants[i];
            uint32_t kind = 0;
            if (!reader.U8(kind) || kind > static_cast<uint32_t>(ConstantKind::Proto)) {
                return Fail(status, BytecodeError::BadConstantKind, "常量类型非法", reader.pos);
            }
            constant.kind = static_cast<ConstantKind>(kind);
            switch (constant.kind) {
                case ConstantKind::Nil:
                    break;
                case ConstantKind::Bool: {
                    uint32_t value = 0;
                    if (!reader.U8(value)) {
                        return Fail(status, BytecodeError::Truncated, "布尔常量缺失", reader.pos);
                    }
                    constant.boolValue = value != 0;
                    break;
                }
                case ConstantKind::Int:
                    if (!reader.I64(constant.intValue)) {
                        return Fail(status, BytecodeError::Truncated, "整数常量缺失", reader.pos);
                    }
                    break;
                case ConstantKind::Double:
                    if (!reader.F64(constant.doubleValue)) {
                        return Fail(status, BytecodeError::Truncated, "浮点常量缺失", reader.pos);
                    }
                    break;
                case ConstantKind::String:
                    if (!reader.Text(constant.stringValue)) {
                        return Fail(status, BytecodeError::Truncated, "字符串常量缺失", reader.pos);
                    }
                    break;
                case ConstantKind::Proto:
                    if (!reader.U32(constant.protoIndex)) {
                        return Fail(status, BytecodeError::Truncated, "原型常量缺失", reader.pos);
                    }
                    break;
            }
        }

        uint32_t childCount = 0;
        if (!reader.U32(childCount) || childCount > 4096) {
            return Fail(status, BytecodeError::Truncated, "子原型数量非法", reader.pos);
        }
        proto.children.resize(childCount);
        for (uint32_t i = 0; i < childCount; ++i) {
            if (!reader.U32(proto.children[i])) {
                return Fail(status, BytecodeError::Truncated, "子原型下标缺失", reader.pos);
            }
        }

        uint32_t lineCount = 0;
        if (!reader.U32(lineCount) || lineCount > (1u << 20)) {
            return Fail(status, BytecodeError::Truncated, "行号表数量非法", reader.pos);
        }
        proto.lines.resize(lineCount);
        uint32_t previous = 0;
        for (uint32_t i = 0; i < lineCount; ++i) {
            int32_t delta = 0;
            if (!reader.VarS32(delta)) {
                return Fail(status, BytecodeError::Truncated, "行号表被截断", reader.pos);
            }
            const int64_t line = static_cast<int64_t>(previous) + delta;
            if (line < 0 || line > 100000000) {
                return Fail(status, BytecodeError::BadJump, "行号表数值非法", reader.pos);
            }
            proto.lines[i] = static_cast<uint32_t>(line);
            previous = proto.lines[i];
        }
    }

    if (!reader.ok) {
        return Fail(status, BytecodeError::Truncated, "字节流提前结束", reader.pos);
    }
    if (entry >= out.protos.size()) {
        return Fail(status, BytecodeError::BadProto, "入口原型下标越界", 8);
    }
    out.entry = entry;
    return VerifyProgram(out, status);
}

bool VerifyProgram(const Program& program, BytecodeStatus& status) {
    status = BytecodeStatus();
    if (program.protos.empty()) {
        return Fail(status, BytecodeError::BadProto, "脚本没有任何原型", 0);
    }
    if (program.entry >= program.protos.size()) {
        return Fail(status, BytecodeError::BadProto, "入口原型下标越界", 0);
    }

    for (size_t protoIndex = 0; protoIndex < program.protos.size(); ++protoIndex) {
        const Proto& proto = program.protos[protoIndex];
        if (proto.numRegs == 0 || proto.numRegs > kMaxRegisters) {
            status.error = BytecodeError::BadFrameSize;
            status.message = "原型 " + std::to_string(protoIndex) + " 的栈帧尺寸非法";
            return false;
        }
        if (proto.numParams > kMaxParams || proto.numParams > proto.numRegs) {
            status.error = BytecodeError::BadFrameSize;
            status.message = "原型 " + std::to_string(protoIndex) + " 的参数个数非法";
            return false;
        }
        if (proto.code.empty()) {
            status.error = BytecodeError::EmptyCode;
            status.message = "原型 " + std::to_string(protoIndex) + " 没有指令";
            return false;
        }
        if (!proto.lines.empty() && proto.lines.size() != proto.code.size()) {
            status.error = BytecodeError::BadJump;
            status.message = "原型 " + std::to_string(protoIndex) + " 的行号表长度与指令数不一致";
            return false;
        }
        for (uint32_t child : proto.children) {
            if (child >= program.protos.size()) {
                status.error = BytecodeError::BadProto;
                status.message = "原型 " + std::to_string(protoIndex) + " 的子原型下标越界";
                return false;
            }
        }

        for (size_t pc = 0; pc < proto.code.size(); ++pc) {
            const uint32_t word = proto.code[pc];
            const Op op = InstructionOp(word);
            if (op >= Op::Count) {
                status.error = BytecodeError::BadOpcode;
                status.message = "原型 " + std::to_string(protoIndex) + " 第 " +
                                 std::to_string(pc) + " 条指令操作码非法";
                status.offset = static_cast<uint32_t>(pc);
                return false;
            }
            const uint32_t a = OperandA(word);
            const uint32_t b = OperandB(word);
            const uint32_t c = OperandC(word);
            const uint32_t bx = OperandBx(word);
            const int32_t sbx = OperandSBx(word);

            switch (op) {
                case Op::Nop:
                    break;
                case Op::LoadNil:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::LoadBool:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (b > 1) {
                        status.error = BytecodeError::BadConstantKind;
                        status.message = "布尔立即数只允许 0 或 1";
                        return false;
                    }
                    break;
                case Op::LoadInt:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::LoadK:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckConstantIndex(proto, bx, status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::Move:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, b, "源", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::GetGlobal:
                case Op::SetGlobal:
                    if (!CheckRegister(proto, a, "全局", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckStringConstant(proto, bx, status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::NewTable:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::GetTable:
                case Op::SetTable:
                    if (!CheckRegister(proto, a, "表", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, b, "键", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, c, "值", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::GetField:
                case Op::SetField:
                    if (!CheckRegister(proto, a, "表", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, b, "值", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckStringConstant(proto, c, status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::Len:
                case Op::Unm:
                case Op::Not:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, b, "源", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::Add:
                case Op::Sub:
                case Op::Mul:
                case Op::Div:
                case Op::Mod:
                case Op::Eq:
                case Op::Ne:
                case Op::Lt:
                case Op::Le:
                case Op::Gt:
                case Op::Ge:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, b, "左操作数", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckRegister(proto, c, "右操作数", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::Jmp:
                    if (!CheckJumpTarget(proto, static_cast<uint32_t>(pc), sbx, status)) return false;
                    break;
                case Op::JmpIf:
                case Op::JmpIfNot:
                    if (!CheckRegister(proto, a, "条件", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckJumpTarget(proto, static_cast<uint32_t>(pc), sbx, status)) return false;
                    break;
                case Op::Call:
                    if (!CheckRegister(proto, a, "函数", status, static_cast<uint32_t>(pc))) return false;
                    if (a + b >= proto.numRegs) {
                        status.error = BytecodeError::BadRegister;
                        status.message = "调用实参超出栈帧范围";
                        status.offset = static_cast<uint32_t>(pc);
                        return false;
                    }
                    if (c > kMaxResults) {
                        status.error = BytecodeError::BadFrameSize;
                        status.message = "期望返回值个数超出上限";
                        return false;
                    }
                    break;
                case Op::Return:
                    if (!CheckRegister(proto, a, "起始", status, static_cast<uint32_t>(pc))) return false;
                    if (b == 0 || b > kMaxResults) {
                        status.error = BytecodeError::BadFrameSize;
                        status.message = "RETURN 的返回值个数必须在 1 到上限之间";
                        return false;
                    }
                    if (a + b > proto.numRegs) {
                        status.error = BytecodeError::BadRegister;
                        status.message = "返回值超出栈帧范围";
                        return false;
                    }
                    break;
                case Op::ReturnZero:
                    break;
                case Op::ReturnMulti:
                    if (!CheckRegister(proto, a, "起始", status, static_cast<uint32_t>(pc))) return false;
                    break;
                case Op::Closure:
                    if (!CheckRegister(proto, a, "目标", status, static_cast<uint32_t>(pc))) return false;
                    if (!CheckConstantIndex(proto, bx, status, static_cast<uint32_t>(pc))) return false;
                    if (proto.constants[bx].kind != ConstantKind::Proto) {
                        status.error = BytecodeError::BadConstantKind;
                        status.message = "CLOSURE 引用的常量不是原型";
                        return false;
                    }
                    if (proto.constants[bx].protoIndex >= program.protos.size()) {
                        status.error = BytecodeError::BadProto;
                        status.message = "CLOSURE 引用的原型下标越界";
                        return false;
                    }
                    break;
                case Op::Count:
                    break;
            }
        }

        const Op lastOp = InstructionOp(proto.code.back());
        if (lastOp != Op::Return && lastOp != Op::ReturnZero && lastOp != Op::ReturnMulti) {
            status.error = BytecodeError::MissingReturn;
            status.message = "原型 " + std::to_string(protoIndex) + " 末尾不是 RETURN";
            return false;
        }
    }
    status.error = BytecodeError::None;
    return true;
}

}  /* namespace immirror::script */
