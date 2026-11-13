/* 界面脚本解析覆盖率: 逐个文件解析, 统计成功率与失败原因. */
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "ui/script/lua_parser.h"

using namespace immirror::ui::script;

static bool ReadFile(const char* path, std::string& out) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    char buffer[8192];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        out.append(buffer, got);
    }
    std::fclose(file);
    return true;
}

int main(int argc, char** argv) {
    const char* directory = argc > 1 ? argv[1] : "../../assets/project/ui/mobile";
    int files = 0;
    int parsed = 0;
    int64_t statements = 0;
    int64_t functions = 0;
    int64_t tables = 0;
    std::vector<std::string> failures;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
            continue;
        }
        files += 1;
        std::string text;
        if (!ReadFile(entry.path().string().c_str(), text)) {
            failures.push_back(entry.path().filename().string() + " :: 读不到");
            continue;
        }
        ParseStats stats;
        std::string error;
        const std::vector<NodePtr> nodes = ParseScript(text.data(), text.size(), &stats, &error);
        if (nodes.empty() && !error.empty()) {
            failures.push_back(entry.path().filename().string() + " :: " + error);
            continue;
        }
        parsed += 1;
        statements += stats.statements;
        functions += stats.functions;
        tables += stats.tables;
    }
    std::printf("脚本文件 %d, 解析成功 %d, 失败 %d\n", files, parsed,
                static_cast<int>(failures.size()));
    std::printf("语句 %lld, 函数 %lld, 表 %lld\n", static_cast<long long>(statements),
                static_cast<long long>(functions), static_cast<long long>(tables));
    for (size_t index = 0; index < failures.size() && index < 12; ++index) {
        std::printf("  失败 %s\n", failures[index].c_str());
    }
    return failures.empty() ? 0 : 1;
}
