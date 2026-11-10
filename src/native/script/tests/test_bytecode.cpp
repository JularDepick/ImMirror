#include <vector>

#include "test_framework.h"
#include "test_support.h"

using namespace immirror::script;
using namespace immirror::script::test;

namespace {

/* 手工构造一个原型, 便于逐条校验规则 */
Program MakeProgram(std::vector<uint32_t> code, uint32_t numRegs = 2) {
    Program program;
    program.chunkName = "hand-built";
    Proto proto;
    proto.name = "main";
    proto.line = 1;
    proto.numParams = 0;
    proto.numRegs = numRegs;
    proto.code = std::move(code);
    proto.lines.assign(proto.code.size(), 1);
    program.protos.push_back(std::move(proto));
    program.entry = 0;
    return program;
}

}  /* namespace */

TEST_CASE(bytecode_hand_built_program_runs) {
    Program program = MakeProgram({
        EncodeAsBx(Op::LoadInt, 0, 7),
        EncodeABC(Op::Return, 0, 1, 0),
    });
    BytecodeStatus status;
    CHECK(VerifyProgram(program, status));

    Vm vm;
    const VmResult result = vm.Execute(program, SandboxLimits());
    CHECK(result.ok());
    CHECK_INT(result.results.size(), 1);
    CHECK_INT(result.results[0].intValue, 7);
}

TEST_CASE(bytecode_roundtrip) {
    const CompileResult compiled =
        CompileSource("fn add(a, b) { return a + b; } return add(2, 3), \"ok\";", "round",
                      CompileOptions());
    CHECK(compiled.ok);
    std::vector<uint8_t> bytes;
    BytecodeStatus status;
    CHECK(SerializeProgram(compiled.program, bytes, status));
    CHECK(bytes.size() > 16);

    Program loaded;
    CHECK(DeserializeProgram(bytes.data(), bytes.size(), loaded, status));
    CHECK_INT(loaded.protos.size(), compiled.program.protos.size());

    Vm vm;
    const VmResult result = vm.Execute(loaded, SandboxLimits());
    CHECK(result.ok());
    CHECK_INT(result.results.size(), 2);
    CHECK_INT(result.results[0].intValue, 5);
    CHECK_TEXT(result.results[1].text, "ok");
}

TEST_CASE(bytecode_reject_bad_magic) {
    const CompileResult compiled = CompileSource("return 1;", "magic", CompileOptions());
    CHECK(compiled.ok);
    std::vector<uint8_t> bytes;
    BytecodeStatus status;
    CHECK(SerializeProgram(compiled.program, bytes, status));
    bytes[0] = 'X';
    Program loaded;
    CHECK(!DeserializeProgram(bytes.data(), bytes.size(), loaded, status));
    CHECK(status.error == BytecodeError::BadMagic);
}

TEST_CASE(bytecode_reject_truncated) {
    const CompileResult compiled = CompileSource("return 1 + 2;", "trunc", CompileOptions());
    CHECK(compiled.ok);
    std::vector<uint8_t> bytes;
    BytecodeStatus status;
    CHECK(SerializeProgram(compiled.program, bytes, status));
    Program loaded;
    CHECK(!DeserializeProgram(bytes.data(), bytes.size() / 2, loaded, status));
    CHECK(status.error == BytecodeError::Truncated);
}

TEST_CASE(bytecode_reject_illegal_opcode) {
    Program program = MakeProgram({
        (static_cast<uint32_t>(200) << kOpcodeShift),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    });
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadOpcode);
}

TEST_CASE(bytecode_reject_register_range) {
    Program program = MakeProgram({
        EncodeABC(Op::LoadNil, 9, 0, 0),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    }, 2);
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadRegister);
}

TEST_CASE(bytecode_reject_constant_range) {
    Program program = MakeProgram({
        EncodeABx(Op::LoadK, 0, 3),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    });
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadConstant);
}

TEST_CASE(bytecode_reject_global_constant_kind) {
    Program program = MakeProgram({
        EncodeABx(Op::GetGlobal, 0, 0),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    });
    program.protos[0].constants.push_back(Constant::MakeInt(5));
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadConstantKind);
}

TEST_CASE(bytecode_reject_jump_range) {
    Program program = MakeProgram({
        EncodeAsBx(Op::Jmp, 0, 100),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    });
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadJump);
}

TEST_CASE(bytecode_reject_return_zero_count) {
    Program program = MakeProgram({
        EncodeABC(Op::Return, 0, 0, 0),
    });
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadFrameSize);
}

TEST_CASE(bytecode_reject_missing_return) {
    Program program = MakeProgram({
        EncodeABC(Op::LoadNil, 0, 0, 0),
    });
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::MissingReturn);
}

TEST_CASE(bytecode_reject_zero_frame) {
    Program program = MakeProgram({
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    }, 0);
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadFrameSize);
}

TEST_CASE(bytecode_reject_call_overflow) {
    Program program = MakeProgram({
        EncodeABC(Op::Call, 1, 1, 1),
        EncodeABC(Op::ReturnZero, 0, 0, 0),
    }, 2);
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadRegister);
}

TEST_CASE(bytecode_reject_empty_program) {
    Program program;
    BytecodeStatus status;
    CHECK(!VerifyProgram(program, status));
    CHECK(status.error == BytecodeError::BadProto);
}

TEST_CASE(bytecode_reject_random_bytes) {
    std::vector<uint8_t> garbage(64, 0xA5);
    Program loaded;
    BytecodeStatus status;
    CHECK(!DeserializeProgram(garbage.data(), garbage.size(), loaded, status));
    CHECK(!status.ok());
}

TEST_CASE(bytecode_strip_debug_smaller) {
    const char* source = R"SRC(
        fn fib(n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }
        return fib(8);
    )SRC";
    CompileOptions withLines;
    CompileOptions withoutLines;
    withoutLines.stripLineInfo = true;
    const CompileResult a = CompileSource(source, "a", withLines);
    const CompileResult b = CompileSource(source, "b", withoutLines);
    CHECK(a.ok);
    CHECK(b.ok);
    CHECK(b.program.ByteSize() < a.program.ByteSize());
}

TEST_CASE(bytecode_disassembly_contains_ops) {
    const CompileResult compiled = CompileSource("return 1 + 2;", "disasm", CompileOptions());
    CHECK(compiled.ok);
    const std::string text = DisassembleProgram(compiled.program);
    CHECK(text.find("LOADINT") != std::string::npos);
    CHECK(text.find("ADD") != std::string::npos);
    CHECK(text.find("RETURN") != std::string::npos);
}
