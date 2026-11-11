#include "editor/project_format.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "core/log.h"
#include "editor/editor_json.h"
#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/terrain_generator.h"
#include "world/world.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditor";

/* 工程格式标识与版本, 与 IMWF 存档的魔数区分开: 这里是文本, 那里是二进制. */
constexpr const char* kProjectFormat = "immirror.project";
constexpr int32_t kProjectFormatVersion = 1;
constexpr const char* kChunkHeader = "# immirror.chunk 1";
constexpr const char* kProjectFileName = "project.json";
constexpr const char* kChunksDirName = "chunks";
constexpr const char* kTriggersDirName = "triggers";
constexpr const char* kExportDirName = "export";

/* 游程文本每行写入的游程个数, 控制单行长度便于阅读与比对. */
constexpr int32_t kRunsPerLine = 12;

/* IMWF 存档结构, 与 world/world.cpp 的二进制布局逐字段一致. */
constexpr char kSaveMagic[4] = {'I', 'M', 'W', 'F'};
constexpr uint32_t kSaveVersion = 1;

struct SaveHeader {
    char magic[4];
    uint32_t version;
    uint32_t seed;
    uint32_t chunkCount;
    float playerX;
    float playerY;
    float playerZ;
    float yaw;
    float pitch;
};

struct ChunkRecord {
    int32_t x;
    int32_t z;
    uint32_t runCount;
};

/* 路径拼接与存在性判断. */
std::string Join(const std::string& base, const char* name) {
    if (base.empty()) {
        return std::string(name);
    }
    if (base.back() == '/') {
        return base + name;
    }
    return base + "/" + name;
}

bool IsDirectory(const std::string& path) {
    struct stat info;
    if (::stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFMT) == S_IFDIR;
}

bool IsRegularFile(const std::string& path) {
    struct stat info;
    if (::stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFMT) == S_IFREG;
}

/* 读取整个文本文件. */
bool ReadTextFile(const std::string& path, std::string& out) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    out.clear();
    char buffer[4096];
    while (true) {
        const size_t read = std::fread(buffer, 1, sizeof(buffer), file);
        if (read > 0) {
            out.append(buffer, read);
        }
        if (read < sizeof(buffer)) {
            break;
        }
    }
    std::fclose(file);
    return true;
}

/* 目录下的文件名列表, 不递归, 跳过 . 与 ... */
bool ListDirectory(const std::string& path, std::vector<std::string>& out) {
    DIR* handle = ::opendir(path.c_str());
    if (handle == nullptr) {
        return false;
    }
    out.clear();
    while (struct dirent* entry = ::readdir(handle)) {
        const char* name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }
        out.emplace_back(name);
    }
    ::closedir(handle);
    std::sort(out.begin(), out.end());
    return true;
}

/* 把名称压成文件名安全的字符集: 非 ASCII 与路径符号替换成下划线. */
std::string SafeFileStem(const std::string& name) {
    std::string out;
    for (const char c : name) {
        const bool alphaNumeric = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                  (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (alphaNumeric) {
            out.push_back(c);
        } else if (c == ' ' || c == '.') {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "project";
    }
    return out;
}

}  /* namespace */

std::string NowTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    std::tm utc{};
#if defined(_WIN32)
    localtime_s(&local, &now);
    gmtime_s(&utc, &now);
#else
    localtime_r(&now, &local);
    gmtime_r(&now, &utc);
#endif
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour,
                  local.tm_min, local.tm_sec);
    /* 时区偏移取本地与 UTC 的差值, 精确到分钟. */
    const int localSeconds = local.tm_hour * 3600 + local.tm_min * 60;
    const int utcSeconds = utc.tm_hour * 3600 + utc.tm_min * 60;
    const int dayDelta = local.tm_mday - utc.tm_mday;
    int offsetSeconds = localSeconds - utcSeconds;
    if (dayDelta > 0) {
        offsetSeconds += 24 * 3600;
    } else if (dayDelta < 0) {
        offsetSeconds -= 24 * 3600;
    }
    const int offsetMinutes = offsetSeconds / 60;
    char zone[8];
    std::snprintf(zone, sizeof(zone), "%c%02d:%02d", offsetMinutes >= 0 ? '+' : '-',
                  std::abs(offsetMinutes) / 60, std::abs(offsetMinutes) % 60);
    return std::string(buffer) + zone;
}

bool MakeDirectories(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::string current;
    size_t index = 0;
    while (index <= path.size()) {
        const size_t slash = path.find('/', index);
        const size_t end = slash == std::string::npos ? path.size() : slash;
        current = path.substr(0, end);
        if (!current.empty() && !IsDirectory(current)) {
            if (::mkdir(current.c_str(), 0700) != 0 && !IsDirectory(current)) {
                return false;
            }
        }
        if (slash == std::string::npos) {
            break;
        }
        index = slash + 1;
    }
    return IsDirectory(path);
}

bool RemoveDirectoryTree(const std::string& path) {
    if (!IsDirectory(path)) {
        return false;
    }
    std::vector<std::string> names;
    if (!ListDirectory(path, names)) {
        return false;
    }
    for (const std::string& name : names) {
        const std::string child = Join(path, name.c_str());
        if (IsDirectory(child)) {
            RemoveDirectoryTree(child);
        } else {
            ::unlink(child.c_str());
        }
    }
    return ::rmdir(path.c_str()) == 0;
}

Project::Project() = default;

Project::~Project() = default;

uint64_t Project::ChunkKey(int32_t x, int32_t z) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
           static_cast<uint64_t>(static_cast<uint32_t>(z));
}

std::string Project::ChunkFileName(int32_t x, int32_t z) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "c.%d.%d.vox", x, z);
    return buffer;
}

std::string Project::TriggerFileName(int32_t id) {
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "t.%d.json", id);
    return buffer;
}

void Project::ResetContents() {
    chunks_.clear();
    triggers_.clear();
    lastExport_.clear();
}

bool Project::EnsureDirectories(std::string& error) {
    if (directory_.empty()) {
        error = "工程目录为空";
        return false;
    }
    if (!MakeDirectories(Join(directory_, kChunksDirName)) ||
        !MakeDirectories(Join(directory_, kTriggersDirName))) {
        error = "无法创建工程子目录: " + directory_;
        return false;
    }
    return true;
}

bool Project::OpenOrCreate(const std::string& directory, const std::string& name, uint32_t seed,
                           std::string& error) {
    if (IsRegularFile(Join(directory, kProjectFileName))) {
        return Load(directory, error);
    }
    directory_ = directory;
    ResetContents();
    meta_ = ProjectMeta{};
    meta_.name = name.empty() ? std::string("未命名工程") : name;
    meta_.seed = seed;
    meta_.createdAt = NowTimestamp();
    meta_.updatedAt = meta_.createdAt;
    /* 出生点取世界原点附近的地表上方, 具体高度在进入编辑器时按地形重新落位. */
    meta_.spawnX = 8.0f;
    meta_.spawnY = 40.0f;
    meta_.spawnZ = 8.0f;
    dirty_ = true;
    if (!EnsureDirectories(error)) {
        return false;
    }
    return Save(error);
}

bool Project::Load(const std::string& directory, std::string& error) {
    const std::string path = Join(directory, kProjectFileName);
    JsonValue root;
    if (!JsonReadFile(path, root, error)) {
        return false;
    }
    if (!root.IsObject()) {
        error = "project.json 顶层不是对象";
        return false;
    }
    if (root.GetString("format", "") != kProjectFormat) {
        error = "project.json 的 format 字段不是 " + std::string(kProjectFormat);
        return false;
    }
    const int64_t version = root.GetInt("formatVersion", 0);
    if (version != kProjectFormatVersion) {
        error = "工程格式版本不支持: " + std::to_string(version);
        return false;
    }

    ProjectMeta meta;
    meta.name = root.GetString("name", "未命名工程");
    meta.author = root.GetString("author", "");
    meta.seed = static_cast<uint32_t>(root.GetInt("seed", static_cast<int64_t>(limits::kDefaultSeed)));
    if (const JsonValue* spawn = root.Find("spawn")) {
        meta.spawnX = static_cast<float>(spawn->GetNumber("x", 8.0));
        meta.spawnY = static_cast<float>(spawn->GetNumber("y", 40.0));
        meta.spawnZ = static_cast<float>(spawn->GetNumber("z", 8.0));
        meta.yaw = static_cast<float>(spawn->GetNumber("yaw", 0.0));
        meta.pitch = static_cast<float>(spawn->GetNumber("pitch", 0.0));
    }
    if (const JsonValue* editor = root.Find("editor")) {
        meta.selectedBlock = static_cast<int32_t>(editor->GetInt("selectedBlock", 1));
        meta.viewDistance =
            static_cast<int32_t>(editor->GetInt("viewDistance", limits::kDefaultViewDistance));
    }
    meta.createdAt = root.GetString("createdAt", "");
    meta.updatedAt = root.GetString("updatedAt", "");

    /* 先在临时容器里装载, 全部成功后再替换当前内容, 避免半个工程覆盖内存. */
    std::unordered_map<uint64_t, Chunk> chunks;
    if (const JsonValue* list = root.Find("chunks")) {
        if (!list->IsArray()) {
            error = "chunks 字段不是数组";
            return false;
        }
        if (static_cast<int32_t>(list->Items().size()) > limits::kMaxChunkFiles) {
            error = "区块数量超过上限";
            return false;
        }
        for (const JsonValue& item : list->Items()) {
            Chunk chunk;
            chunk.coord.x = static_cast<int32_t>(item.GetInt("x", 0));
            chunk.coord.z = static_cast<int32_t>(item.GetInt("z", 0));
            const std::string file = item.GetString("file", ChunkFileName(chunk.coord.x, chunk.coord.z));
            if (!LoadChunkFile(chunk, Join(directory, file.c_str()), error)) {
                return false;
            }
            chunks.emplace(ChunkKey(chunk.coord.x, chunk.coord.z), std::move(chunk));
        }
    }

    std::vector<Trigger> triggers;
    if (const JsonValue* list = root.Find("triggers")) {
        if (!list->IsArray()) {
            error = "triggers 字段不是数组";
            return false;
        }
        for (const JsonValue& item : list->Items()) {
            const int32_t id = static_cast<int32_t>(item.GetInt("id", 0));
            const std::string file = item.GetString("file", TriggerFileName(id));
            JsonValue triggerJson;
            if (!JsonReadFile(Join(directory, file.c_str()), triggerJson, error)) {
                return false;
            }
            Trigger trigger;
            if (!TriggerFromJson(triggerJson, trigger, error)) {
                error = file + ": " + error;
                return false;
            }
            if (static_cast<int32_t>(triggers.size()) < limits::kMaxTriggers) {
                triggers.push_back(std::move(trigger));
            }
        }
    }

    directory_ = directory;
    meta_ = meta;
    chunks_ = std::move(chunks);
    triggers_ = std::move(triggers);
    lastExport_.clear();
    dirty_ = false;
    IM_LOGI(kTag, "project loaded, name=%s chunks=%d triggers=%d", meta_.name.c_str(),
            ChunkCount(), static_cast<int32_t>(triggers_.size()));
    return true;
}

bool Project::Save(std::string& error) {
    if (!EnsureDirectories(error)) {
        return false;
    }
    meta_.updatedAt = NowTimestamp();
    if (meta_.createdAt.empty()) {
        meta_.createdAt = meta_.updatedAt;
    }

    /* 落盘前把差异表按地形重新算一遍, 去掉改回原样的格子, 工程文件保持最小. */
    for (auto& entry : chunks_) {
        if (!entry.second.changedList.empty()) {
            RefreshChunkDiff(entry.second);
        }
    }

    /* 只保存与生成地形不同的区块: 未改动的区块重新生成的结果一致, 不必落盘. */
    std::vector<const Chunk*> saved;
    saved.reserve(chunks_.size());
    for (const auto& entry : chunks_) {
        if (!entry.second.changedList.empty()) {
            saved.push_back(&entry.second);
        }
    }
    std::sort(saved.begin(), saved.end(), [](const Chunk* left, const Chunk* right) {
        if (left->coord.x != right->coord.x) {
            return left->coord.x < right->coord.x;
        }
        return left->coord.z < right->coord.z;
    });

    for (const Chunk* chunk : saved) {
        const std::string path = Join(Join(directory_, kChunksDirName),
                                      ChunkFileName(chunk->coord.x, chunk->coord.z).c_str());
        if (!SaveChunkFile(*chunk, path, error)) {
            return false;
        }
    }
    for (const Trigger& trigger : triggers_) {
        const std::string path =
            Join(Join(directory_, kTriggersDirName), TriggerFileName(trigger.id).c_str());
        if (!JsonWriteFile(path, TriggerToJson(trigger), error)) {
            return false;
        }
    }

    /* 工程描述最后写: 它列出的每个文件此时都已落盘. */
    JsonValue root = JsonValue::MakeObject();
    root.Set("format", JsonValue::MakeString(kProjectFormat));
    root.Set("formatVersion", JsonValue::MakeInt(kProjectFormatVersion));
    root.Set("name", JsonValue::MakeString(meta_.name));
    root.Set("author", JsonValue::MakeString(meta_.author));
    root.Set("seed", JsonValue::MakeInt(static_cast<int64_t>(meta_.seed)));
    JsonValue spawn = JsonValue::MakeObject();
    spawn.Set("x", JsonValue::MakeNumber(static_cast<double>(meta_.spawnX)));
    spawn.Set("y", JsonValue::MakeNumber(static_cast<double>(meta_.spawnY)));
    spawn.Set("z", JsonValue::MakeNumber(static_cast<double>(meta_.spawnZ)));
    spawn.Set("yaw", JsonValue::MakeNumber(static_cast<double>(meta_.yaw)));
    spawn.Set("pitch", JsonValue::MakeNumber(static_cast<double>(meta_.pitch)));
    root.Set("spawn", std::move(spawn));
    JsonValue editor = JsonValue::MakeObject();
    editor.Set("selectedBlock", JsonValue::MakeInt(meta_.selectedBlock));
    editor.Set("viewDistance", JsonValue::MakeInt(meta_.viewDistance));
    root.Set("editor", std::move(editor));
    root.Set("createdAt", JsonValue::MakeString(meta_.createdAt));
    root.Set("updatedAt", JsonValue::MakeString(meta_.updatedAt));

    JsonValue chunkList = JsonValue::MakeArray();
    for (const Chunk* chunk : saved) {
        JsonValue item = JsonValue::MakeObject();
        item.Set("x", JsonValue::MakeInt(chunk->coord.x));
        item.Set("z", JsonValue::MakeInt(chunk->coord.z));
        item.Set("file", JsonValue::MakeString(
                            std::string(kChunksDirName) + "/" +
                            ChunkFileName(chunk->coord.x, chunk->coord.z)));
        chunkList.Push(std::move(item));
    }
    root.Set("chunks", std::move(chunkList));

    JsonValue triggerList = JsonValue::MakeArray();
    for (const Trigger& trigger : triggers_) {
        JsonValue item = JsonValue::MakeObject();
        item.Set("id", JsonValue::MakeInt(trigger.id));
        item.Set("file", JsonValue::MakeString(std::string(kTriggersDirName) + "/" +
                                              TriggerFileName(trigger.id)));
        triggerList.Push(std::move(item));
    }
    root.Set("triggers", std::move(triggerList));
    if (!JsonWriteFile(Join(directory_, kProjectFileName), root, error)) {
        return false;
    }

    /* 清理已经不在工程里的旧文件, 避免删除触发器后留下孤儿文件. */
    std::vector<std::string> names;
    if (ListDirectory(Join(directory_, kChunksDirName), names)) {
        for (const std::string& name : names) {
            bool keep = false;
            for (const Chunk* chunk : saved) {
                if (name == ChunkFileName(chunk->coord.x, chunk->coord.z)) {
                    keep = true;
                    break;
                }
            }
            if (!keep) {
                ::unlink(Join(Join(directory_, kChunksDirName), name.c_str()).c_str());
            }
        }
    }
    if (ListDirectory(Join(directory_, kTriggersDirName), names)) {
        for (const std::string& name : names) {
            bool keep = false;
            for (const Trigger& trigger : triggers_) {
                if (name == TriggerFileName(trigger.id)) {
                    keep = true;
                    break;
                }
            }
            if (!keep) {
                ::unlink(Join(Join(directory_, kTriggersDirName), name.c_str()).c_str());
            }
        }
    }

    dirty_ = false;
    IM_LOGI(kTag, "project saved, name=%s chunks=%d triggers=%d", meta_.name.c_str(),
            static_cast<int32_t>(saved.size()), static_cast<int32_t>(triggers_.size()));
    return true;
}

bool Project::LoadChunkFile(Chunk& chunk, const std::string& path, std::string& error) {
    std::string text;
    if (!ReadTextFile(path, text)) {
        error = "无法读取区块文件: " + path;
        return false;
    }
    chunk.blocks.assign(static_cast<size_t>(limits::kChunkVolume), voxel::block::Air);
    chunk.changed.assign(static_cast<size_t>(limits::kChunkVolume), 0);
    chunk.changedList.clear();

    /* 先按种子生成地形, 记录下来的方块与地形比较后重建差异表. */
    voxel::ChunkColumn generated;
    voxel::TerrainGenerator{}.Generate(chunk.coord, generated, meta_.seed);

    size_t cursor = 0;
    size_t blockIndex = 0;
    while (cursor < text.size() && blockIndex < static_cast<size_t>(limits::kChunkVolume)) {
        /* 跳过行首注释与空白. */
        if (text[cursor] == '#') {
            const size_t lineEnd = text.find('\n', cursor);
            cursor = lineEnd == std::string::npos ? text.size() : lineEnd + 1;
            continue;
        }
        if (text[cursor] == ' ' || text[cursor] == '\n' || text[cursor] == '\r' ||
            text[cursor] == '\t') {
            ++cursor;
            continue;
        }
        const size_t colon = text.find(':', cursor);
        const size_t endToken = text.find_first_of(" \n\r\t", cursor);
        if (colon == std::string::npos || endToken == std::string::npos || colon > endToken) {
            error = "区块文件游程格式不正确: " + path;
            return false;
        }
        const long id = std::strtol(text.substr(cursor, colon - cursor).c_str(), nullptr, 10);
        long count = std::strtol(text.substr(colon + 1, endToken - colon - 1).c_str(), nullptr, 10);
        cursor = endToken;
        if (id < 0 || id > 0xFFFF || count <= 0) {
            error = "区块文件游程取值越界: " + path;
            return false;
        }
        if (blockIndex + static_cast<size_t>(count) > static_cast<size_t>(limits::kChunkVolume)) {
            error = "区块文件游程总长超过一个区块: " + path;
            return false;
        }
        for (long step = 0; step < count; ++step) {
            chunk.blocks[blockIndex + static_cast<size_t>(step)] =
                static_cast<voxel::BlockId>(id);
        }
        blockIndex += static_cast<size_t>(count);
    }
    if (blockIndex != static_cast<size_t>(limits::kChunkVolume)) {
        error = "区块文件游程总长不足一个区块: " + path;
        return false;
    }

    for (size_t index = 0; index < chunk.blocks.size(); ++index) {
        const int32_t x = static_cast<int32_t>(index % voxel::kChunkSizeX);
        const int32_t rest = static_cast<int32_t>(index / voxel::kChunkSizeX);
        const int32_t z = rest % voxel::kChunkSizeZ;
        const int32_t y = rest / voxel::kChunkSizeZ;
        const voxel::BlockId generatedId = generated.Get(x, y, z);
        if (chunk.blocks[index] != generatedId) {
            chunk.changed[index] = 1;
            chunk.changedList.push_back(static_cast<uint32_t>(index));
        }
    }
    return true;
}

bool Project::SaveChunkFile(const Chunk& chunk, const std::string& path, std::string& error) const {
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        error = "无法写入区块文件: " + path;
        return false;
    }
    std::fprintf(file, "%s\n", kChunkHeader);
    std::fprintf(file, "# x=%d z=%d changed=%d\n", chunk.coord.x, chunk.coord.z,
                 static_cast<int32_t>(chunk.changedList.size()));
    std::fprintf(file, "# 顺序为 y 层优先, 再 z 行, 再 x 列, 与 voxel::ChunkColumn::Index 一致\n");
    std::fprintf(file, "# 每条游程写作 方块id:数量, 总格数必须等于 %d\n", limits::kChunkVolume);

    int32_t runsOnLine = 0;
    const size_t total = chunk.blocks.size();
    for (size_t index = 0; index < total;) {
        const voxel::BlockId id = chunk.blocks[index];
        size_t length = 1;
        while (index + length < total && chunk.blocks[index + length] == id &&
               length < 0xFFFFu) {
            ++length;
        }
        std::fprintf(file, "%u:%zu ", static_cast<unsigned>(id), length);
        ++runsOnLine;
        if (runsOnLine >= kRunsPerLine) {
            std::fprintf(file, "\n");
            runsOnLine = 0;
        }
        index += length;
    }
    if (runsOnLine > 0) {
        std::fprintf(file, "\n");
    }
    const bool ok = std::ferror(file) == 0;
    std::fclose(file);
    if (!ok) {
        error = "区块文件写入失败: " + path;
    }
    return ok;
}

void Project::RefreshChunkDiff(Chunk& chunk) {
    voxel::ChunkColumn generated;
    voxel::TerrainGenerator{}.Generate(chunk.coord, generated, meta_.seed);
    chunk.changed.assign(static_cast<size_t>(limits::kChunkVolume), 0);
    chunk.changedList.clear();
    for (size_t index = 0; index < chunk.blocks.size(); ++index) {
        const int32_t x = static_cast<int32_t>(index % voxel::kChunkSizeX);
        const int32_t rest = static_cast<int32_t>(index / voxel::kChunkSizeX);
        const int32_t z = rest % voxel::kChunkSizeZ;
        const int32_t y = rest / voxel::kChunkSizeZ;
        if (chunk.blocks[index] != generated.Get(x, y, z)) {
            chunk.changed[index] = 1;
            chunk.changedList.push_back(static_cast<uint32_t>(index));
        }
    }
}

Project::Chunk* Project::FindChunk(voxel::ChunkCoord coord) {
    const auto it = chunks_.find(ChunkKey(coord.x, coord.z));
    return it == chunks_.end() ? nullptr : &it->second;
}

Project::Chunk* Project::EnsureChunk(voxel::ChunkCoord coord) {
    Chunk* existing = FindChunk(coord);
    if (existing != nullptr) {
        return existing;
    }
    Chunk chunk;
    chunk.coord = coord;
    voxel::ChunkColumn generated;
    voxel::TerrainGenerator{}.Generate(coord, generated, meta_.seed);
    chunk.blocks = generated.Blocks();
    chunk.blocks.resize(static_cast<size_t>(limits::kChunkVolume), voxel::block::Air);
    chunk.changed.assign(static_cast<size_t>(limits::kChunkVolume), 0);
    const auto inserted = chunks_.emplace(ChunkKey(coord.x, coord.z), std::move(chunk));
    return &inserted.first->second;
}

voxel::BlockId Project::GetBlock(int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y >= voxel::kWorldHeight) {
        return voxel::block::Air;
    }
    const int32_t chunkX = x >= 0 ? x / voxel::kChunkSizeX : (x - voxel::kChunkSizeX + 1) / voxel::kChunkSizeX;
    const int32_t chunkZ = z >= 0 ? z / voxel::kChunkSizeZ : (z - voxel::kChunkSizeZ + 1) / voxel::kChunkSizeZ;
    Chunk* chunk = EnsureChunk(voxel::ChunkCoord{chunkX, chunkZ});
    const int32_t localX = x - chunkX * voxel::kChunkSizeX;
    const int32_t localZ = z - chunkZ * voxel::kChunkSizeZ;
    const size_t index = voxel::ChunkColumn::Index(localX, y, localZ);
    if (index >= chunk->blocks.size()) {
        return voxel::block::Air;
    }
    return chunk->blocks[index];
}

bool Project::SetBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id) {
    if (y < 0 || y >= voxel::kWorldHeight) {
        return false;
    }
    if (static_cast<int32_t>(chunks_.size()) >= limits::kMaxChunkFiles) {
        /* 工程规模上限保护: 只在新建区块时拒绝, 已有区块仍可编辑. */
        const int32_t chunkX =
            x >= 0 ? x / voxel::kChunkSizeX : (x - voxel::kChunkSizeX + 1) / voxel::kChunkSizeX;
        const int32_t chunkZ =
            z >= 0 ? z / voxel::kChunkSizeZ : (z - voxel::kChunkSizeZ + 1) / voxel::kChunkSizeZ;
        if (FindChunk(voxel::ChunkCoord{chunkX, chunkZ}) == nullptr) {
            IM_LOGW(kTag, "project chunk budget reached, edit rejected");
            return false;
        }
    }
    const int32_t chunkX = x >= 0 ? x / voxel::kChunkSizeX : (x - voxel::kChunkSizeX + 1) / voxel::kChunkSizeX;
    const int32_t chunkZ = z >= 0 ? z / voxel::kChunkSizeZ : (z - voxel::kChunkSizeZ + 1) / voxel::kChunkSizeZ;
    Chunk* chunk = EnsureChunk(voxel::ChunkCoord{chunkX, chunkZ});
    const int32_t localX = x - chunkX * voxel::kChunkSizeX;
    const int32_t localZ = z - chunkZ * voxel::kChunkSizeZ;
    const size_t index = voxel::ChunkColumn::Index(localX, y, localZ);
    if (index >= chunk->blocks.size() || chunk->blocks[index] == id) {
        return false;
    }
    chunk->blocks[index] = id;

    /*
     * 差异表只在落盘前统一重算: 每次改一格就重跑一次地形生成太贵,
     * 这里先按"被改过"标记, 投影时多写一次同值方块的代价可以忽略.
     */
    if (chunk->changed[index] == 0) {
        chunk->changed[index] = 1;
        chunk->changedList.push_back(static_cast<uint32_t>(index));
    }
    dirty_ = true;
    return true;
}

int32_t Project::ProjectToWorld(world::World& world, int32_t budget) {
    int32_t applied = 0;
    for (auto& entry : chunks_) {
        Chunk& chunk = entry.second;
        if (chunk.changedList.empty()) {
            continue;
        }
        const int32_t originX = chunk.coord.x * voxel::kChunkSizeX;
        const int32_t originZ = chunk.coord.z * voxel::kChunkSizeZ;
        for (const uint32_t flat : chunk.changedList) {
            const int32_t localX = static_cast<int32_t>(flat % voxel::kChunkSizeX);
            const int32_t rest = static_cast<int32_t>(flat / voxel::kChunkSizeX);
            const int32_t localZ = rest % voxel::kChunkSizeZ;
            const int32_t y = rest / voxel::kChunkSizeZ;
            const int32_t worldX = originX + localX;
            const int32_t worldZ = originZ + localZ;
            const voxel::BlockId wanted = chunk.blocks[flat];
            if (world.GetBlock(worldX, y, worldZ) == wanted) {
                continue;
            }
            if (applied >= budget) {
                return applied;
            }
            /* 区块还没加载时写入会失败, 该格留到下一帧再试. */
            if (world.SetBlock(worldX, y, worldZ, wanted)) {
                ++applied;
            }
        }
    }
    return applied;
}

bool Project::ExportWorld(const std::string& path, std::string& error) {
    /* 导出前先把工程状态整理成待写出的区块列表, 只写有差异的区块. */
    std::vector<const Chunk*> saved;
    for (const auto& entry : chunks_) {
        if (!entry.second.changedList.empty()) {
            saved.push_back(&entry.second);
        }
    }
    if (saved.empty()) {
        error = "工程还没有任何方块改动, 没有可导出的内容";
        return false;
    }
    std::sort(saved.begin(), saved.end(), [](const Chunk* left, const Chunk* right) {
        if (left->coord.x != right->coord.x) {
            return left->coord.x < right->coord.x;
        }
        return left->coord.z < right->coord.z;
    });
    if (!MakeDirectories(path.substr(0, path.rfind('/')))) {
        error = "无法创建导出目录: " + path;
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        error = "无法写入导出文件: " + path;
        return false;
    }
    SaveHeader header{};
    std::memcpy(header.magic, kSaveMagic, sizeof(header.magic));
    header.version = kSaveVersion;
    header.seed = meta_.seed;
    header.chunkCount = static_cast<uint32_t>(saved.size());
    header.playerX = meta_.spawnX;
    header.playerY = meta_.spawnY;
    header.playerZ = meta_.spawnZ;
    header.yaw = meta_.yaw;
    header.pitch = meta_.pitch;
    if (std::fwrite(&header, sizeof(header), 1, file) != 1) {
        std::fclose(file);
        error = "导出文件头写入失败";
        return false;
    }
    for (const Chunk* chunk : saved) {
        /* 先统计游程数, 再写记录头与游程数据, 与原生存档布局一致. */
        std::vector<std::pair<uint16_t, voxel::BlockId>> runs;
        runs.reserve(256);
        const size_t total = chunk->blocks.size();
        for (size_t index = 0; index < total;) {
            const voxel::BlockId id = chunk->blocks[index];
            size_t length = 1;
            while (index + length < total && chunk->blocks[index + length] == id &&
                   length < 0xFFFFu) {
                ++length;
            }
            runs.emplace_back(static_cast<uint16_t>(length), id);
            index += length;
        }
        ChunkRecord record{};
        record.x = chunk->coord.x;
        record.z = chunk->coord.z;
        record.runCount = static_cast<uint32_t>(runs.size());
        if (std::fwrite(&record, sizeof(record), 1, file) != 1) {
            std::fclose(file);
            error = "导出区块记录写入失败";
            return false;
        }
        for (const auto& run : runs) {
            if (std::fwrite(&run.first, sizeof(run.first), 1, file) != 1 ||
                std::fwrite(&run.second, sizeof(run.second), 1, file) != 1) {
                std::fclose(file);
                error = "导出方块游程写入失败";
                return false;
            }
        }
    }
    std::fclose(file);
    lastExport_ = path;
    IM_LOGI(kTag, "project exported to %s chunks=%d", path.c_str(),
            static_cast<int32_t>(saved.size()));
    return true;
}

Trigger* Project::FindTrigger(int32_t id) {
    for (Trigger& trigger : triggers_) {
        if (trigger.id == id) {
            return &trigger;
        }
    }
    return nullptr;
}

const Trigger* Project::FindTrigger(int32_t id) const {
    for (const Trigger& trigger : triggers_) {
        if (trigger.id == id) {
            return &trigger;
        }
    }
    return nullptr;
}

Trigger* Project::AddTrigger() {
    if (static_cast<int32_t>(triggers_.size()) >= limits::kMaxTriggers) {
        return nullptr;
    }
    int32_t nextId = 1;
    for (const Trigger& trigger : triggers_) {
        if (trigger.id >= nextId) {
            nextId = trigger.id + 1;
        }
    }
    Trigger trigger;
    trigger.id = nextId;
    trigger.name = "触发器 " + std::to_string(nextId);
    trigger.region = Region{};
    TriggerAction action;
    action.type = TriggerActionType::kLog;
    action.itemId = TriggerActionItemId(TriggerActionType::kLog);
    action.text = "trigger fired";
    trigger.actions.push_back(action);
    trigger.RebuildScript();
    triggers_.push_back(trigger);
    dirty_ = true;
    return &triggers_.back();
}

bool Project::RemoveTrigger(int32_t id) {
    for (auto it = triggers_.begin(); it != triggers_.end(); ++it) {
        if (it->id == id) {
            triggers_.erase(it);
            dirty_ = true;
            return true;
        }
    }
    return false;
}

int64_t Project::ChangedBlockCount() const {
    int64_t total = 0;
    for (const auto& entry : chunks_) {
        total += static_cast<int64_t>(entry.second.changedList.size());
    }
    return total;
}

bool ListProjects(const std::string& root, std::vector<ProjectSummary>& out, std::string& error) {
    out.clear();
    if (!IsDirectory(root)) {
        error = "工程根目录不存在: " + root;
        return false;
    }
    std::vector<std::string> names;
    if (!ListDirectory(root, names)) {
        error = "无法读取工程根目录: " + root;
        return false;
    }
    for (const std::string& name : names) {
        const std::string directory = Join(root, name.c_str());
        if (!IsDirectory(directory) || !IsRegularFile(Join(directory, kProjectFileName))) {
            continue;
        }
        JsonValue root_json;
        std::string readError;
        if (!JsonReadFile(Join(directory, kProjectFileName), root_json, readError)) {
            IM_LOGW(kTag, "skip broken project %s: %s", directory.c_str(), readError.c_str());
            continue;
        }
        ProjectSummary summary;
        summary.directory = directory;
        summary.name = root_json.GetString("name", name);
        summary.seed = static_cast<uint32_t>(root_json.GetInt("seed", 0));
        summary.updatedAt = root_json.GetString("updatedAt", "");
        if (const JsonValue* chunks = root_json.Find("chunks")) {
            if (chunks->IsArray()) {
                summary.chunkCount = static_cast<int32_t>(chunks->Items().size());
            }
        }
        if (const JsonValue* triggers = root_json.Find("triggers")) {
            if (triggers->IsArray()) {
                summary.triggerCount = static_cast<int32_t>(triggers->Items().size());
            }
        }
        out.push_back(std::move(summary));
    }
    std::sort(out.begin(), out.end(), [](const ProjectSummary& left, const ProjectSummary& right) {
        return left.updatedAt > right.updatedAt;
    });
    return true;
}

bool CreateProject(const std::string& root, const std::string& name, uint32_t seed,
                   std::string& directoryOut, std::string& error) {
    if (!MakeDirectories(root)) {
        error = "无法创建工程根目录: " + root;
        return false;
    }
    /* 目录名取安全字符并加序号去重, 显示名保持原样存在工程描述里. */
    const std::string stem = SafeFileStem(name);
    std::string directory;
    for (int32_t index = 1; index < 1000; ++index) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "-%03d", index);
        directory = Join(root, (stem + buffer).c_str());
        if (!IsDirectory(directory)) {
            break;
        }
        directory.clear();
    }
    if (directory.empty()) {
        error = "工程目录名已经被占满: " + root;
        return false;
    }
    if (!MakeDirectories(directory)) {
        error = "无法创建工程目录: " + directory;
        return false;
    }
    Project project;
    if (!project.OpenOrCreate(directory, name, seed, error)) {
        RemoveDirectoryTree(directory);
        return false;
    }
    directoryOut = directory;
    IM_LOGI(kTag, "project created at %s name=%s", directory.c_str(), name.c_str());
    return true;
}

bool DeleteProject(const std::string& directory, std::string& error) {
    if (!IsDirectory(directory)) {
        error = "工程目录不存在: " + directory;
        return false;
    }
    if (!IsRegularFile(Join(directory, kProjectFileName))) {
        /* 只允许删除带工程描述的目录, 避免误删其他内容. */
        error = "目标目录不是工程目录: " + directory;
        return false;
    }
    if (!RemoveDirectoryTree(directory)) {
        error = "删除工程目录失败: " + directory;
        return false;
    }
    IM_LOGI(kTag, "project deleted at %s", directory.c_str());
    return true;
}

}  /* namespace immirror::editor */
