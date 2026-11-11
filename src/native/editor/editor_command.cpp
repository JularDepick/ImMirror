#include "editor/editor_command.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <vector>

#include "core/log.h"
#include "editor/editor_session.h"
#include "editor/editor_types.h"
#include "editor/project_format.h"
#include "editor/trigger_model.h"
#include "voxel/voxel_types.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditorCmd";

/* 命令执行结果的统一取值, 与 EditorReply 的 code 语义一致. */
constexpr int kCodeOk = 0;
constexpr int kCodeError = 1;
constexpr int kCodeUnknownCommand = 2;

/*
 * 进程内编辑器状态, 设计说明见 editor/COMMANDS.md.
 * 命令层把工程对象持有到下一条 open 为止; 没有打开工程时其余编辑命令直接拒绝.
 */
struct CommandState {
    bool initialized = false;          /* init 命令是否已经执行 */
    std::string dataPath;              /* init 记录的应用内部存储目录 */
    bool hasProject = false;           /* 是否已打开工程 */
    std::string projectName;           /* 工程显示名, open 时随工程刷新 */
    std::unique_ptr<Project> project;  /* 打开的工程对象, 拥有全部工程数据 */
    std::string tool = "place";        /* 当前工具: place 或 remove */
    int32_t paletteIndex = 0;          /* 当前调色板序号 */
    std::string paletteKey;            /* 当前调色板键, 随序号联动 */
    int32_t cursorX = 0;               /* 命令层光标, apply 与 sel 的目标 */
    int32_t cursorY = 64;
    int32_t cursorZ = 0;
    bool selectionSet = false;         /* 是否已经框选 A 与 B 两点 */
    int32_t selectionAx = 0;           /* A 点坐标 */
    int32_t selectionAy = 0;
    int32_t selectionAz = 0;
    int32_t selectionBx = 0;           /* B 点坐标 */
    int32_t selectionBy = 0;
    int32_t selectionBz = 0;
    UndoStack undo;                    /* 命令层自己的撤销栈, 与外壳同一套类型 */
};

/* 进程内单例. 全部命令由 Java 主线程串行调用, 因此不加锁. */
CommandState& State() {
    static CommandState state;
    return state;
}

/* 撤销栈写回: 只写工程数据, 命令层没有世界与渲染对象可投影. */
void UndoApplyToProject(void* user, int32_t x, int32_t y, int32_t z, voxel::BlockId id) {
    auto* project = static_cast<Project*>(user);
    if (project != nullptr) {
        project->SetBlock(x, y, z, id);
    }
}

/* 行追加辅助: 键=值 一行. */
void AppendField(std::string& out, const char* key, const std::string& value) {
    out += key;
    out += "=";
    out += value;
    out += "
";
}

void AppendFieldInt(std::string& out, const char* key, int32_t value) {
    AppendField(out, key, std::to_string(value));
}

/* 列表行辅助: 键=字段|字段|... 一行. */
void AppendRow(std::string& out, const char* key, const std::string& fields) {
    AppendField(out, key, fields);
}

/* 逗号分隔的整数三元组, Java 侧用 EditorReply.triple 解析. */
std::string Int3(int32_t x, int32_t y, int32_t z) {
    return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
}

/* 名称里的分隔符与换行替换掉, 保证 Java 侧按行与竖线切分不会错位. */
std::string SanitizeField(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        if (c == '|' || c == '
' || c == '') {
            out.push_back('/');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

/* 按首个空白切分动词与参数, 动词统一小写; 空命令返回假. */
bool SplitVerb(const std::string& command, std::string& verb, std::string& rest) {
    const size_t start = command.find_first_not_of(" 	
");
    if (start == std::string::npos) {
        return false;
    }
    const size_t space = command.find_first_of(" 	
", start);
    if (space == std::string::npos) {
        verb = command.substr(start);
        rest.clear();
    } else {
        verb = command.substr(start, space - start);
        const size_t restStart = command.find_first_not_of(" 	
", space);
        rest = restStart == std::string::npos ? std::string() : command.substr(restStart);
    }
    std::transform(verb.begin(), verb.end(), verb.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return true;
}

/* 字符串转整数, 非法或越界返回假; 允许前后空白与负号. */
bool ParseInt32(const std::string& text, int32_t& out) {
    const size_t start = text.find_first_not_of(" 	
");
    if (start == std::string::npos) {
        return false;
    }
    const size_t end = text.find_last_not_of(" 	
");
    const std::string body = text.substr(start, end - start + 1);
    if (body.empty()) {
        return false;
    }
    errno = 0;
    char* endPointer = nullptr;
    const long long value = std::strtoll(body.c_str(), &endPointer, 10);
    if (errno != 0 || endPointer != body.c_str() + body.size()) {
        return false;
    }
    if (value < -2147483648LL || value > 2147483647LL) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

/* 空白分隔的参数切分, 连续空白视为一个分隔符. */
std::vector<std::string> SplitTokens(const std::string& rest) {
    std::vector<std::string> tokens;
    size_t cursor = 0;
    while (cursor < rest.size()) {
        const size_t start = rest.find_first_not_of(" 	
", cursor);
        if (start == std::string::npos) {
            break;
        }
        size_t end = rest.find_first_of(" 	
", start);
        if (end == std::string::npos) {
            end = rest.size();
        }
        tokens.emplace_back(rest.substr(start, end - start));
        cursor = end;
    }
    return tokens;
}

/* 统一成功回复, code=0 加 message. */
std::string OkReply(const std::string& message) {
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", message);
    return out;
}

/* 统一失败回复. */
std::string ErrorReply(int code, const std::string& message) {
    std::string out;
    AppendFieldInt(out, "code", code);
    AppendField(out, "message", message);
    return out;
}

/* 工程摘要行: 目录|名称|种子|区块数|触发器数|更新时间, projects 命令使用. */
std::string ProjectSummaryRow(const ProjectSummary& summary) {
    std::string fields;
    fields += SanitizeField(summary.directory);
    fields += "|";
    fields += SanitizeField(summary.name);
    fields += "|";
    fields += std::to_string(summary.seed);
    fields += "|";
    fields += std::to_string(summary.chunkCount);
    fields += "|";
    fields += std::to_string(summary.triggerCount);
    fields += "|";
    fields += SanitizeField(summary.updatedAt);
    return fields;
}

/* 未初始化与未打开工程的公共检查, 命中时 error 给出原因并返回假. */
bool RequireInit(std::string& error) {
    if (!State().initialized) {
        error = "编辑器尚未初始化, 先执行 init";
        return false;
    }
    return true;
}

bool RequireProject(std::string& error) {
    if (!RequireInit(error)) {
        return false;
    }
    if (!State().hasProject || State().project == nullptr) {
        error = "尚未打开工程";
        return false;
    }
    return true;
}