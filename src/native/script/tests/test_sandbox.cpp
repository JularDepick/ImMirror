#include "test_framework.h"
#include "test_support.h"

using namespace immirror::script;
using namespace immirror::script::test;

TEST_CASE(sandbox_instruction_budget) {
    SandboxLimits limits;
    limits.instructionBudget = 5000;
    limits.timeoutMillis = 10000;
    const Outcome outcome = Run("var i = 0; while (true) { i = i + 1; }", limits);
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::InstructionBudget);
    CHECK(outcome.stats.instructions >= 5000);
}

TEST_CASE(sandbox_instruction_budget_allows_small_script) {
    SandboxLimits limits;
    limits.instructionBudget = 100000;
    const Outcome outcome = Run("var sum = 0; for (var i = 0; i < 10; i += 1) { sum += i; } return sum;",
                                limits);
    CHECK(outcome.ok);
    CHECK_INT(outcome.Int(0), 45);
    CHECK(outcome.stats.instructions > 0);
    CHECK(outcome.stats.instructions < 100000);
}

TEST_CASE(sandbox_timeout) {
    SandboxLimits limits;
    limits.instructionBudget = 1ull << 40;
    limits.timeoutMillis = 1;
    limits.timeoutCheckInterval = 64;
    const Outcome outcome = Run("var i = 0; while (true) { i = i + 1; }", limits);
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::Timeout);
    CHECK(outcome.stats.timedOut);
}

TEST_CASE(sandbox_memory_quota) {
    SandboxLimits limits;
    limits.memoryQuotaBytes = 64ull * 1024ull;
    limits.instructionBudget = 100000000ull;
    limits.timeoutMillis = 10000;
    const Outcome outcome = Run(R"SRC(
        var s = "0123456789";
        for (var i = 0; i < 2000; i = i + 1) { s = s + "0123456789"; }
        return len(s);
    )SRC", limits);
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::MemoryQuota);
}

TEST_CASE(sandbox_call_depth) {
    SandboxLimits limits;
    limits.maxCallDepth = 16;
    limits.instructionBudget = 1000000;
    limits.timeoutMillis = 10000;
    const Outcome outcome = Run("fn f(n) { return f(n + 1); } return f(0);", limits);
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::CallDepth);
}

TEST_CASE(sandbox_stack_slots) {
    SandboxLimits limits;
    limits.maxStackSlots = 32;
    limits.maxCallDepth = 64;
    limits.instructionBudget = 1000000;
    limits.timeoutMillis = 10000;
    const Outcome outcome = Run("fn f(n) { return f(n + 1) + 1; } return f(0);", limits);
    CHECK(!outcome.ok);
    CHECK(outcome.error == VmError::StackOverflow);
}

TEST_CASE(sandbox_stats_reported) {
    const Outcome outcome = Run("var t = { 1, 2, 3 }; return len(t);");
    CHECK(outcome.ok);
    CHECK(outcome.stats.instructions > 0);
    CHECK(outcome.stats.memoryPeakBytes > 0);
    CHECK(outcome.stats.maxCallDepth >= 1);
    CHECK(outcome.stats.tablesCreated >= 1);
}

TEST_CASE(sandbox_budget_reset_between_runs) {
    SandboxLimits small;
    small.instructionBudget = 200;
    const Outcome first = Run("var sum = 0; for (var i = 0; i < 5; i += 1) { sum += i; } return sum;",
                              small);
    CHECK(first.ok);
    const Outcome second = Run("var sum = 0; for (var i = 0; i < 5; i += 1) { sum += i; } return sum;",
                               small);
    CHECK(second.ok);
    CHECK_INT(second.Int(0), 10);
}

TEST_CASE(vm_rejects_invalid_program) {
    Program program;
    Proto proto;
    proto.name = "bad";
    proto.numRegs = 1;
    proto.code.push_back(EncodeABC(Op::LoadNil, 5, 0, 0));
    proto.code.push_back(EncodeABC(Op::ReturnZero, 0, 0, 0));
    proto.lines.assign(proto.code.size(), 1);
    program.protos.push_back(proto);
    program.entry = 0;

    Vm vm;
    const VmResult result = vm.Execute(program, SandboxLimits());
    CHECK(!result.ok());
    CHECK(result.error == VmError::InvalidOpcode);
}

TEST_CASE(error_reports_line_number) {
    const Outcome outcome = Run(R"SRC(
        var a = 1;
        var b = 2;
        return a + missing_field.x;
    )SRC");
    CHECK(!outcome.ok);
    CHECK_INT(outcome.line, 4);
}
