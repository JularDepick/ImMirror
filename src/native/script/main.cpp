#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "benchmark.h"
#include "binding.h"
#include "bytecode.h"
#include "compiler.h"
#include "sandbox.h"
#include "vm.h"

/*
 * 主机侧命令行工具. 只在宿主构建中编译, 不进入 Android 库,
 * 提供 编译, 反汇编, 执行 三个子命令, 便于离线验证脚本与字节码.
 */
namespace {

using namespace immirror::script;

const char* const kUsage =
    "用法: immirror-script <命令> [参数]\n"
    "  compile <源码.ims> [-o 输出.imsc] [--strip-debug]\n"
    "      把自有语法源码编译为定长字节码, 不写 -o 时只打印统计\n"
    "  disasm  <字节码.imsc|源码.ims>\n"
    "      反汇编, 打印常量池与每条指令\n"
    "  run     <源码.ims|字节码.imsc> [--limit 指令数] [--memory 字节] [--timeout 毫秒]\n"
    "      在沙盒中执行并打印返回值与统计\n"
    "  bench\n"
    "      运行斐波那契与表操作基准\n";

/* 控制台宿主: 把脚本日志打到标准输出, 世界接口返回固定值 */
class ConsoleWorld : public HostWorld {
public:
    int32_t GetBlock(int32_t x, int32_t y, int32_t z) override {
        (void)x;
        (void)y;
        (void)z;
        return 1;
    }

    bool SetBlock(int32_t x, int32_t y, int32_t z, int32_t blockId) override {
        (void)x;
        (void)y;
        (void)z;
        (void)blockId;
        return true;
    }

    bool PlayerPosition(double& x, double& y, double& z) override {
        x = 0.5;
        y = 64.0;
        z = -2.5;
        return true;
    }

    void Log(const char* message) override { std::printf("[脚本日志] %s\n", message); }
};

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return true;
}

bool ReadBinaryFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return true;
}

bool HasSuffix(const std::string& text, const char* suffix) {
    const size_t length = std::strlen(suffix);
    if (text.size() < length) {
        return false;
    }
    return text.compare(text.size() - length, length, suffix) == 0;
}

bool ParseU64(const char* text, uint64_t& out) {
    if (text == nullptr || text[0] == '\0') {
        return false;
    }
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == nullptr || *end != '\0') {
        return false;
    }
    out = value;
    return true;
}

struct ProgramSummary {
    size_t protos = 0;
    size_t instructions = 0;
    size_t constants = 0;
    size_t bytes = 0;
};

ProgramSummary Summarize(const Program& program) {
    ProgramSummary summary;
    summary.protos = program.protos.size();
    for (const Proto& proto : program.protos) {
        summary.instructions += proto.code.size();
        summary.constants += proto.constants.size();
    }
    summary.bytes = program.ByteSize();
    return summary;
}

void PrintSummary(const Program& program) {
    const ProgramSummary summary = Summarize(program);
    std::printf("字节码统计: 原型 %zu 个, 指令 %zu 条, 常量 %zu 个, 序列化 %zu 字节\n",
                summary.protos, summary.instructions, summary.constants, summary.bytes);
}

/* 按扩展名区分源码与字节码, 非 .ims 一律按字节码处理 */
bool LoadProgram(const std::string& path, Program& program, std::string& error) {
    if (HasSuffix(path, ".ims")) {
        std::string source;
        if (!ReadFile(path, source)) {
            error = "无法读取源码文件: " + path;
            return false;
        }
        const CompileResult compiled = CompileSource(source, path, CompileOptions());
        if (!compiled.ok) {
            error = "编译失败";
            for (const Diagnostic& diagnostic : compiled.diagnostics) {
                error += "\n  ";
                error += FormatDiagnostic(path.c_str(), diagnostic);
            }
            return false;
        }
        program = compiled.program;
        return true;
    }
    std::vector<uint8_t> bytes;
    if (!ReadBinaryFile(path, bytes)) {
        error = "无法读取字节码文件: " + path;
        return false;
    }
    BytecodeStatus status;
    if (!DeserializeProgram(bytes.data(), bytes.size(), program, status)) {
        error = "加载字节码失败: " + status.message + " (" +
                BytecodeErrorName(status.error) + ")";
        return false;
    }
    return true;
}

int CommandCompile(const std::vector<std::string>& args) {
    std::string input;
    std::string output;
    bool stripDebug = false;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-o" && i + 1 < args.size()) {
            output = args[++i];
        } else if (args[i] == "--strip-debug") {
            stripDebug = true;
        } else if (input.empty()) {
            input = args[i];
        } else {
            std::printf("未知参数: %s\n", args[i].c_str());
            return 1;
        }
    }
    if (input.empty()) {
        std::printf("compile 需要源码路径\n");
        return 1;
    }
    std::string source;
    if (!ReadFile(input, source)) {
        std::printf("无法读取源码文件: %s\n", input.c_str());
        return 1;
    }
    CompileOptions options;
    options.stripLineInfo = stripDebug;
    const CompileResult compiled = CompileSource(source, input, options);
    if (!compiled.ok) {
        std::printf("编译失败:\n");
        for (const Diagnostic& diagnostic : compiled.diagnostics) {
            std::printf("  %s\n", FormatDiagnostic(input.c_str(), diagnostic).c_str());
        }
        return 2;
    }
    PrintSummary(compiled.program);
    if (!output.empty()) {
        std::vector<uint8_t> bytes;
        BytecodeStatus status;
        if (!SerializeProgram(compiled.program, bytes, status)) {
            std::printf("序列化失败: %s\n", status.message.c_str());
            return 2;
        }
        std::ofstream stream(output, std::ios::binary | std::ios::trunc);
        if (!stream) {
            std::printf("无法写入输出文件: %s\n", output.c_str());
            return 1;
        }
        stream.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        std::printf("已写出 %s (%zu 字节)\n", output.c_str(), bytes.size());
    }
    return 0;
}

int CommandDisasm(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::printf("disasm 需要脚本路径\n");
        return 1;
    }
    Program program;
    std::string error;
    if (!LoadProgram(args[0], program, error)) {
        std::printf("%s\n", error.c_str());
        return 2;
    }
    std::printf("%s", DisassembleProgram(program).c_str());
    PrintSummary(program);
    return 0;
}

int CommandRun(const std::vector<std::string>& args) {
    std::string input;
    SandboxLimits limits;
    for (size_t i = 0; i < args.size(); ++i) {
        uint64_t value = 0;
        if (args[i] == "--limit" && i + 1 < args.size()) {
            if (!ParseU64(args[++i].c_str(), value)) {
                std::printf("--limit 需要整数\n");
                return 1;
            }
            limits.instructionBudget = value;
        } else if (args[i] == "--memory" && i + 1 < args.size()) {
            if (!ParseU64(args[++i].c_str(), value)) {
                std::printf("--memory 需要整数\n");
                return 1;
            }
            limits.memoryQuotaBytes = value;
        } else if (args[i] == "--timeout" && i + 1 < args.size()) {
            if (!ParseU64(args[++i].c_str(), value)) {
                std::printf("--timeout 需要整数\n");
                return 1;
            }
            limits.timeoutMillis = static_cast<uint32_t>(value);
        } else if (input.empty()) {
            input = args[i];
        } else {
            std::printf("未知参数: %s\n", args[i].c_str());
            return 1;
        }
    }
    if (input.empty()) {
        std::printf("run 需要脚本路径\n");
        return 1;
    }
    Program program;
    std::string error;
    if (!LoadProgram(input, program, error)) {
        std::printf("%s\n", error.c_str());
        return 2;
    }

    BindingRegistry registry;
    RegisterDefaultBindings(registry);
    ConsoleWorld world;
    Vm vm;
    vm.SetBindingRegistry(&registry);
    vm.SetHost(&world);

    const VmResult result = vm.Execute(program, limits);
    if (!result.ok()) {
        std::printf("执行失败: %s (第 %u 行)\n", VmErrorName(result.error), result.line);
        std::printf("  %s\n", result.message.c_str());
        std::printf("统计: 指令 %llu, 耗时 %.3f ms\n",
                    static_cast<unsigned long long>(result.stats.instructions),
                    static_cast<double>(result.stats.elapsedMicros) / 1000.0);
        return 3;
    }
    std::printf("执行成功: %zu 个返回值\n", result.results.size());
    for (size_t i = 0; i < result.results.size(); ++i) {
        std::printf("  [%zu] %s = %s\n", i + 1, ValueTypeName(result.results[i].type),
                    result.results[i].ToDisplayString().c_str());
    }
    std::printf("统计: 指令 %llu, 调用深度 %u, 内存峰值 %llu 字节, 耗时 %.3f ms\n",
                static_cast<unsigned long long>(result.stats.instructions), result.stats.maxCallDepth,
                static_cast<unsigned long long>(result.stats.memoryPeakBytes),
                static_cast<double>(result.stats.elapsedMicros) / 1000.0);
    return 0;
}

}  /* namespace */

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("%s", kUsage);
        return 1;
    }
    const std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    if (command == "compile") {
        return CommandCompile(args);
    }
    if (command == "disasm") {
        return CommandDisasm(args);
    }
    if (command == "run") {
        return CommandRun(args);
    }
    if (command == "bench") {
        return immirror::script::RunBenchmarks();
    }
    if (command == "--help" || command == "-h") {
        std::printf("%s", kUsage);
        return 0;
    }
    std::printf("未知命令: %s\n", command.c_str());
    std::printf("%s", kUsage);
    return 1;
}
