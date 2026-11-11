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
#include "editor/undo_stack.h"
#include "voxel/block_registry.h"
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
    out += "\n";
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
        if (c == '|' || c == '\n' || c == '\r') {
            out.push_back('/');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

/* 按首个空白切分动词与参数, 动词统一小写; 空命令返回假. */
bool SplitVerb(const std::string& command, std::string& verb, std::string& rest) {
    const size_t start = command.find_first_not_of(" \t\n");
    if (start == std::string::npos) {
        return false;
    }
    const size_t space = command.find_first_of(" \t\n", start);
    if (space == std::string::npos) {
        verb = command.substr(start);
        rest.clear();
    } else {
        verb = command.substr(start, space - start);
        const size_t restStart = command.find_first_not_of(" \t\n", space);
        rest = restStart == std::string::npos ? std::string() : command.substr(restStart);
    }
    std::transform(verb.begin(), verb.end(), verb.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return true;
}

/* 字符串转整数, 非法或越界返回假; 允许前后空白与负号. */
bool ParseInt32(const std::string& text, int32_t& out) {
    const size_t start = text.find_first_not_of(" \t\n");
    if (start == std::string::npos) {
        return false;
    }
    const size_t end = text.find_last_not_of(" \t\n");
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
        const size_t start = rest.find_first_not_of(" \t\n", cursor);
        if (start == std::string::npos) {
            break;
        }
        size_t end = rest.find_first_of(" \t\n", start);
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
/* ---------------------------------------------------------------- 命令实现 */

std::string CommandPing() {
    return OkReply("pong");
}

std::string CommandInit(const std::string& dataPath) {
    CommandState& state = State();
    state.dataPath = dataPath;
    state.initialized = true;
    IM_LOGI(kTag, "editor command init, dataPath=%s", dataPath.c_str());
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "编辑器已初始化");
    AppendField(out, "dataPath", SanitizeField(dataPath));
    AppendField(out, "root", SanitizeField(ProjectRoot(dataPath)));
    return out;
}

std::string CommandStatus() {
    CommandState& state = State();
    std::string error;
    if (!RequireInit(error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", state.hasProject ? "工程已打开" : "编辑器就绪");
    AppendFieldInt(out, "open", state.hasProject ? 1 : 0);
    if (state.hasProject && state.project != nullptr) {
        AppendField(out, "project", SanitizeField(state.project->Directory()));
        AppendField(out, "name", SanitizeField(state.project->Name()));
        AppendFieldInt(out, "seed", static_cast<int32_t>(state.project->Seed()));
        AppendFieldInt(out, "chunks", state.project->ChunkCount());
        AppendFieldInt(out, "triggers",
                       static_cast<int32_t>(state.project->Triggers().size()));
        AppendFieldInt(out, "edits", static_cast<int32_t>(state.project->ChangedBlockCount()));
        AppendFieldInt(out, "dirty", state.project->Dirty() ? 1 : 0);
    }
    AppendField(out, "tool", state.tool);
    AppendField(out, "block", state.paletteKey);
    AppendField(out, "blockLabel", SanitizeField(PaletteNameZh(state.paletteIndex)));
    AppendFieldInt(out, "blockId", static_cast<int32_t>(PaletteBlock(state.paletteIndex)));
    AppendField(out, "cursor", Int3(state.cursorX, state.cursorY, state.cursorZ));
    AppendFieldInt(out, "selectionSet", state.selectionSet ? 1 : 0);
    if (state.selectionSet) {
        const int32_t sizeX = std::abs(state.selectionBx - state.selectionAx) + 1;
        const int32_t sizeY = std::abs(state.selectionBy - state.selectionAy) + 1;
        const int32_t sizeZ = std::abs(state.selectionBz - state.selectionAz) + 1;
        AppendField(out, "selectionA",
                    Int3(state.selectionAx, state.selectionAy, state.selectionAz));
        AppendField(out, "selectionB",
                    Int3(state.selectionBx, state.selectionBy, state.selectionBz));
        AppendField(out, "selectionSize", Int3(sizeX, sizeY, sizeZ));
    }
    AppendFieldInt(out, "undo", state.undo.UndoDepth());
    AppendFieldInt(out, "redo", state.undo.RedoDepth());
    AppendField(out, "dataPath", SanitizeField(state.dataPath));
    return out;
}

std::string CommandProjects() {
    std::string error;
    if (!RequireInit(error)) {
        return ErrorReply(kCodeError, error);
    }
    std::vector<ProjectSummary> projects;
    if (!ListProjects(ProjectRoot(State().dataPath), projects, error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", projects.empty() ? std::string("还没有工程")
                                                 : std::string("工程清单已生成"));
    for (const ProjectSummary& summary : projects) {
        AppendRow(out, "project", ProjectSummaryRow(summary));
    }
    return out;
}

std::string CommandNew(const std::string& name) {
    std::string error;
    if (!RequireInit(error)) {
        return ErrorReply(kCodeError, error);
    }
    if (name.empty()) {
        return ErrorReply(kCodeError, "工程名不能为空");
    }
    std::string directory;
    if (!CreateProject(ProjectRoot(State().dataPath), name, limits::kDefaultSeed, directory,
                       error)) {
        return ErrorReply(kCodeError, error);
    }
    IM_LOGI(kTag, "editor command new, dir=%s", directory.c_str());
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "工程已创建");
    AppendField(out, "project", SanitizeField(directory));
    return out;
}

std::string CommandOpen(const std::string& directory) {
    std::string error;
    if (!RequireInit(error)) {
        return ErrorReply(kCodeError, error);
    }
    if (directory.empty()) {
        return ErrorReply(kCodeError, "工程目录为空");
    }
    CommandState& state = State();
    auto project = std::make_unique<Project>();
    if (!project->Load(directory, error)) {
        return ErrorReply(kCodeError, error);
    }
    state.project = std::move(project);
    state.hasProject = true;
    state.projectName = state.project->Name();
    state.selectionSet = false;
    state.undo.Clear();
    state.paletteIndex =
        PaletteIndexOf(static_cast<voxel::BlockId>(state.project->Meta().selectedBlock));
    if (state.paletteIndex < 0) {
        state.paletteIndex = 0;
    }
    state.paletteKey = PaletteKey(state.paletteIndex);
    state.tool = "place";
    IM_LOGI(kTag, "editor command open, dir=%s chunks=%d", directory.c_str(),
            state.project->ChunkCount());
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "工程已打开");
    AppendField(out, "project", SanitizeField(directory));
    AppendField(out, "name", SanitizeField(state.project->Name()));
    AppendFieldInt(out, "seed", static_cast<int32_t>(state.project->Seed()));
    AppendFieldInt(out, "chunks", state.project->ChunkCount());
    AppendFieldInt(out, "triggers", static_cast<int32_t>(state.project->Triggers().size()));
    AppendFieldInt(out, "edits", static_cast<int32_t>(state.project->ChangedBlockCount()));
    return out;
}

std::string CommandSave() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string saveError;
    if (!State().project->Save(saveError)) {
        return ErrorReply(kCodeError, saveError);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "工程已保存");
    AppendFieldInt(out, "chunks", State().project->ChunkCount());
    AppendFieldInt(out, "edits", static_cast<int32_t>(State().project->ChangedBlockCount()));
    return out;
}

std::string CommandTool(const std::string& token) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    if (token != "place" && token != "remove") {
        return ErrorReply(kCodeError, "未知工具: " + token);
    }
    State().tool = token;
    return OkReply(token == "place" ? "切换到放置工具" : "切换到删除工具");
}

std::string CommandBlock(const std::string& key) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    for (int32_t index = 0; index < PaletteCount(); ++index) {
        if (key == PaletteKey(index)) {
            State().paletteIndex = index;
            State().paletteKey = PaletteKey(index);
            return OkReply("已选方块: " + SanitizeField(PaletteNameZh(index)));
        }
    }
    return ErrorReply(kCodeError, "未知方块键: " + key);
}

std::string CommandCursor(const std::vector<std::string>& tokens) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    if (tokens.size() != 3 || !ParseInt32(tokens[0], x) || !ParseInt32(tokens[1], y) ||
        !ParseInt32(tokens[2], z)) {
        return ErrorReply(kCodeError, "cursor 需要 x y z 三个整数");
    }
    if (y < 0 || y >= voxel::kWorldHeight) {
        return ErrorReply(kCodeError, "y 超出世界高度范围");
    }
    CommandState& state = State();
    state.cursorX = x;
    state.cursorY = y;
    state.cursorZ = z;
    return OkReply("光标已移动");
}

std::string CommandApply() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    CommandState& state = State();
    const voxel::BlockId target =
        state.tool == "remove" ? voxel::block::Air : PaletteBlock(state.paletteIndex);
    const int32_t x = state.cursorX;
    const int32_t y = state.cursorY;
    const int32_t z = state.cursorZ;
    if (y < 0 || y >= voxel::kWorldHeight) {
        return ErrorReply(kCodeError, "y 超出世界高度范围");
    }
    Project& project = *state.project;
    const voxel::BlockId before = project.GetBlock(x, y, z);
    if (before == target) {
        return OkReply("目标已经是当前方块, 没有改动");
    }
    if (!project.SetBlock(x, y, z, target)) {
        return ErrorReply(kCodeError, "方块写入被拒绝");
    }
    EditBatch batch;
    batch.label = state.tool == "remove" ? "remove" : "place";
    batch.edits.push_back(BlockEdit{x, y, z, before, target});
    state.undo.Push(std::move(batch));
    IM_LOGI(kTag, "editor command apply, %s at %d %d %d", batch.label.c_str(), x, y, z);
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", state.tool == "remove" ? "已删除方块" : "已放置方块");
    AppendFieldInt(out, "changed", 1);
    AppendField(out, "cursor", Int3(x, y, z));
    return out;
}

std::string CommandSel(const std::string& rest) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    CommandState& state = State();
    if (rest == "clear") {
        state.selectionSet = false;
        return OkReply("已清空选区");
    }
    const std::vector<std::string> tokens = SplitTokens(rest);
    if (tokens.size() == 1 && tokens[0] == "a") {
        state.selectionSet = false;
        state.selectionAx = state.cursorX;
        state.selectionAy = state.cursorY;
        state.selectionAz = state.cursorZ;
        return OkReply("A 点已标记");
    }
    if (tokens.size() == 1 && tokens[0] == "b") {
        state.selectionBx = state.cursorX;
        state.selectionBy = state.cursorY;
        state.selectionBz = state.cursorZ;
        state.selectionSet = true;
        return OkReply("B 点已标记");
    }
    return ErrorReply(kCodeError, "sel 需要 a, b 或 clear 子命令");
}

std::string CommandFill() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    CommandState& state = State();
    if (!state.selectionSet) {
        return ErrorReply(kCodeError, "选区未设置, 先标记 A 与 B 两点");
    }
    const int32_t minX = std::min(state.selectionAx, state.selectionBx);
    const int32_t maxX = std::max(state.selectionAx, state.selectionBx);
    const int32_t minY = std::max(0, std::min(state.selectionAy, state.selectionBy));
    const int32_t maxY =
        std::min(voxel::kWorldHeight - 1, std::max(state.selectionAy, state.selectionBy));
    const int32_t minZ = std::min(state.selectionAz, state.selectionBz);
    const int32_t maxZ = std::max(state.selectionAz, state.selectionBz);
    const int64_t volume = static_cast<int64_t>(maxX - minX + 1) *
                           static_cast<int64_t>(maxY - minY + 1) *
                           static_cast<int64_t>(maxZ - minZ + 1);
    if (volume > limits::kMaxFillBlocks) {
        return ErrorReply(kCodeError, "选区体积超出填充上限");
    }
    const voxel::BlockId target = PaletteBlock(state.paletteIndex);
    Project& project = *state.project;
    EditBatch batch;
    batch.label = "fill";
    batch.edits.reserve(static_cast<size_t>(volume));
    for (int32_t y = minY; y <= maxY; ++y) {
        for (int32_t z = minZ; z <= maxZ; ++z) {
            for (int32_t x = minX; x <= maxX; ++x) {
                const voxel::BlockId before = project.GetBlock(x, y, z);
                if (before != target) {
                    batch.edits.push_back(BlockEdit{x, y, z, before, target});
                }
            }
        }
    }
    if (batch.edits.empty()) {
        return OkReply("选区已经是当前方块, 没有改动");
    }
    for (const BlockEdit& edit : batch.edits) {
        project.SetBlock(edit.x, edit.y, edit.z, edit.after);
    }
    const int32_t changed = static_cast<int32_t>(batch.edits.size());
    state.undo.Push(std::move(batch));
    IM_LOGI(kTag, "editor command fill, blocks=%d", changed);
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "填充完成");
    AppendFieldInt(out, "changed", changed);
    AppendField(out, "selectionSize",
                Int3(maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1));
    return out;
}

std::string CommandUndo() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    CommandState& state = State();
    if (!state.undo.Undo(state.project.get(), &UndoApplyToProject)) {
        return ErrorReply(kCodeError, "没有可撤销的操作");
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "已撤销");
    AppendFieldInt(out, "undo", state.undo.UndoDepth());
    AppendFieldInt(out, "redo", state.undo.RedoDepth());
    return out;
}

std::string CommandRedo() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    CommandState& state = State();
    if (!state.undo.Redo(state.project.get(), &UndoApplyToProject)) {
        return ErrorReply(kCodeError, "没有可重做的操作");
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "已重做");
    AppendFieldInt(out, "undo", state.undo.UndoDepth());
    AppendFieldInt(out, "redo", state.undo.RedoDepth());
    return out;
}

std::string CommandExport(const std::string& target) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    if (target != "playtest") {
        return ErrorReply(kCodeError, "未知导出目标: " + target);
    }
    CommandState& state = State();
    const std::string path = GameWorldPath(
        state.dataPath,
        state.project->Name().empty() ? std::string("project") : state.project->Name());
    if (!state.project->ExportWorld(path, error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "世界已导出");
    AppendField(out, "world", SanitizeField(path));
    return out;
}

std::string CommandPalette() {
    std::string error;
    if (!RequireInit(error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "调色板");
    for (int32_t index = 0; index < PaletteCount(); ++index) {
        std::string fields;
        fields += SanitizeField(PaletteKey(index));
        fields += "|";
        fields += SanitizeField(PaletteNameZh(index));
        fields += "|";
        fields += std::to_string(static_cast<int32_t>(PaletteBlock(index)));
        AppendRow(out, "block", fields);
    }
    return out;
}

std::string CommandTriggers() {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "触发器清单");
    for (const Trigger& trigger : State().project->Triggers()) {
        std::string fields;
        fields += std::to_string(trigger.id);
        fields += "|";
        fields += SanitizeField(trigger.name);
        fields += "|";
        fields += SanitizeField(TriggerEventNameZh(trigger.eventType));
        fields += "|";
        fields += std::to_string(static_cast<int32_t>(trigger.actions.size()));
        fields += "|";
        fields += trigger.enabled ? "1" : "0";
        AppendRow(out, "trigger", fields);
    }
    return out;
}

std::string CommandTriggerAdd(const std::string& eventToken) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    TriggerEventType eventType = TriggerEventType::kEnterRegion;
    if (!ParseTriggerEventKey(eventToken, eventType)) {
        return ErrorReply(kCodeError, "未知触发器事件: " + eventToken);
    }
    CommandState& state = State();
    Trigger* trigger = state.project->AddTrigger();
    if (trigger == nullptr) {
        return ErrorReply(kCodeError, "触发器数量已达上限");
    }
    trigger->eventType = eventType;
    trigger->RebuildScript();
    state.project->MarkDirty();
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", "触发器已添加");
    AppendFieldInt(out, "id", trigger->id);
    return out;
}

std::string CommandTriggerRemove(const std::string& idToken) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    int32_t id = 0;
    if (!ParseInt32(idToken, id)) {
        return ErrorReply(kCodeError, "触发器 id 必须是整数");
    }
    if (!State().project->RemoveTrigger(id)) {
        return ErrorReply(kCodeError, "触发器不存在: " + idToken);
    }
    return OkReply("触发器已删除");
}

std::string CommandTriggerRun(const std::string& idToken) {
    std::string error;
    if (!RequireProject(error)) {
        return ErrorReply(kCodeError, error);
    }
    int32_t id = 0;
    if (!ParseInt32(idToken, id)) {
        return ErrorReply(kCodeError, "触发器 id 必须是整数");
    }
    const Trigger* trigger = State().project->FindTrigger(id);
    if (trigger == nullptr) {
        return ErrorReply(kCodeError, "触发器不存在: " + idToken);
    }
    /*
     * 命令层不接入脚本运行时: 试跑只回放动作摘要与源码,
     * 真正的事件驱动执行由编辑器外壳里的 TriggerRuntime 承担.
     */
    std::string actions;
    for (const TriggerAction& action : trigger->actions) {
        if (!actions.empty()) {
            actions += "; ";
        }
        actions += action.DescribeAscii();
    }
    if (actions.empty()) {
        actions = "no action";
    }
    std::string out;
    AppendFieldInt(out, "code", kCodeOk);
    AppendField(out, "message", SanitizeField(actions));
    AppendFieldInt(out, "id", trigger->id);
    AppendField(out, "script", SanitizeField(trigger->script));
    return out;
}

}  /* namespace */

std::string HandleEditorCommand(const std::string& command) {
    std::string verb;
    std::string rest;
    if (!SplitVerb(command, verb, rest)) {
        return ErrorReply(kCodeUnknownCommand, "空命令");
    }
    if (verb == "ping") {
        return CommandPing();
    }
    if (verb == "init") {
        return CommandInit(rest);
    }
    if (verb == "status") {
        return CommandStatus();
    }
    if (verb == "projects") {
        return CommandProjects();
    }
    if (verb == "new") {
        return CommandNew(rest);
    }
    if (verb == "open") {
        return CommandOpen(rest);
    }
    if (verb == "save") {
        return CommandSave();
    }
    if (verb == "tool") {
        return CommandTool(rest);
    }
    if (verb == "block") {
        return CommandBlock(rest);
    }
    if (verb == "cursor") {
        return CommandCursor(SplitTokens(rest));
    }
    if (verb == "apply") {
        return CommandApply();
    }
    if (verb == "sel") {
        return CommandSel(rest);
    }
    if (verb == "fill") {
        return CommandFill();
    }
    if (verb == "undo") {
        return CommandUndo();
    }
    if (verb == "redo") {
        return CommandRedo();
    }
    if (verb == "export") {
        return CommandExport(rest.empty() ? std::string("playtest") : rest);
    }
    if (verb == "palette") {
        return CommandPalette();
    }
    if (verb == "triggers") {
        return CommandTriggers();
    }
    if (verb == "trigger_add") {
        return CommandTriggerAdd(rest);
    }
    if (verb == "trigger_remove") {
        return CommandTriggerRemove(rest);
    }
    if (verb == "trigger_run") {
        return CommandTriggerRun(rest);
    }
    return ErrorReply(kCodeUnknownCommand, "未知命令: " + verb);
}

void ResetEditorCommandStateForTest() {
    State() = CommandState{};
}

}  /* namespace immirror::editor */
