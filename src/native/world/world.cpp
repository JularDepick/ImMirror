#include "world/world.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <algorithm>
#include <utility>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/chunk_mesher.h"
#include "voxel/light_field.h"
#include "voxel/generated/biome_data_gen.h"
#include "voxel/terrain_generator.h"

namespace immirror::world {
namespace {

/* 每帧最多新建的区块列数, 避免首次进入世界时一次性生成造成卡顿. */
constexpr int32_t kChunkBuildBudgetPerFrame = 2;

uint64_t ChunkKey(int32_t x, int32_t z) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
           static_cast<uint64_t>(static_cast<uint32_t>(z));
}

int32_t FloorDiv(int32_t value, int32_t divisor) {
    const int32_t quotient = value / divisor;
    return (value % divisor != 0 && ((value < 0) != (divisor < 0))) ? quotient - 1 : quotient;
}

}  /* namespace */

/*
 * 列光场计算器: 主循环单线程访问, 无状态除复用的 BFS 队列外,
 * 各列重算按调用顺序串行执行.
 */
voxel::LightField& ColumnLightField() {
    static voxel::LightField field;
    return field;
}

/* 单个区块列: 方块数据 + 已构建网格. */
struct World::Chunk {
    voxel::ChunkCoord coord;
    voxel::ChunkColumn column;
    voxel::MeshData mesh;
    uint64_t revision = 0;
    bool meshDirty = true;
};

World::World(uint32_t seed) : seed_(seed) {}

World::~World() = default;

void World::SetSeed(uint32_t seed) {
    if (seed_ == seed) {
        return;
    }
    seed_ = seed;
    chunks_.clear();
    visible_.clear();
    version_ += 1;
}

void World::Update(const core::Vec3& center, int32_t viewDistanceChunks, uint8_t treeShape) {
    if (viewDistanceChunks < 1) {
        viewDistanceChunks = 1;
    }
    viewDistance_ = viewDistanceChunks;
    /* 树形档只影响本帧新建列, 已加载列保持生成时的树形, 与数据表一致. */
    treeShape_ = treeShape;

    const int32_t centerX = FloorDiv(static_cast<int32_t>(std::floor(center.x)), voxel::kChunkSizeX);
    const int32_t centerZ = FloorDiv(static_cast<int32_t>(std::floor(center.z)), voxel::kChunkSizeZ);

    /* 先卸载超出视距的列, 记下坐标以便随后修补相邻列的边界面. */
    std::vector<voxel::ChunkCoord> removed;
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const voxel::ChunkCoord coord = it->second->coord;
        if (std::abs(coord.x - centerX) > viewDistance_ || std::abs(coord.z - centerZ) > viewDistance_) {
            removed.push_back(coord);
            it = chunks_.erase(it);
            version_ += 1;
        } else {
            ++it;
        }
    }
    for (const voxel::ChunkCoord& coord : removed) {
        if (Chunk* n = FindChunk({coord.x + 1, coord.z})) RebuildMesh(*n);
        if (Chunk* n = FindChunk({coord.x - 1, coord.z})) RebuildMesh(*n);
        if (Chunk* n = FindChunk({coord.x, coord.z + 1})) RebuildMesh(*n);
        if (Chunk* n = FindChunk({coord.x, coord.z - 1})) RebuildMesh(*n);
    }

    /* 再按预算补齐缺失的列, 由近及远. */
    int32_t budget = kChunkBuildBudgetPerFrame;
    for (int32_t ring = 0; ring <= viewDistance_ && budget > 0; ++ring) {
        for (int32_t dz = -ring; dz <= ring && budget > 0; ++dz) {
            for (int32_t dx = -ring; dx <= ring && budget > 0; ++dx) {
                if (ring > 0 && std::abs(dx) != ring && std::abs(dz) != ring) {
                    continue;
                }
                const voxel::ChunkCoord coord{centerX + dx, centerZ + dz};
                if (FindChunk(coord) != nullptr) {
                    continue;
                }
                EnsureChunk(coord, treeShape_);
                budget -= 1;
            }
        }
    }

    RebuildVisibleList();
}

World::Chunk* World::FindChunk(voxel::ChunkCoord coord) const {
    const auto it = chunks_.find(ChunkKey(coord.x, coord.z));
    return it == chunks_.end() ? nullptr : it->second.get();
}

World::Chunk* World::EnsureChunk(voxel::ChunkCoord coord, uint8_t treeShape) {
    Chunk* existing = FindChunk(coord);
    if (existing != nullptr) {
        return existing;
    }
    auto chunk = std::make_unique<Chunk>();
    chunk->coord = coord;
    voxel::TerrainGenerator{}.Generate(coord, chunk->column, seed_, treeShape);
    /* 生成后全量重算本列光场, 网格构建按光照采样顶点明暗. */
    RecomputeLight(*chunk);
    chunk->meshDirty = true;
    Chunk* raw = chunk.get();
    chunks_.emplace(ChunkKey(coord.x, coord.z), std::move(chunk));
    RebuildMesh(*raw);

    /*
     * 新列加入后, 已存在的相邻列边界面从全亮回落切换到真实光场,
     * 需要先重算相邻列光场再重建网格.
     */
    if (Chunk* n = FindChunk({coord.x + 1, coord.z})) {
        RecomputeLight(*n);
        RebuildMesh(*n);
    }
    if (Chunk* n = FindChunk({coord.x - 1, coord.z})) {
        RecomputeLight(*n);
        RebuildMesh(*n);
    }
    if (Chunk* n = FindChunk({coord.x, coord.z + 1})) {
        RecomputeLight(*n);
        RebuildMesh(*n);
    }
    if (Chunk* n = FindChunk({coord.x, coord.z - 1})) {
        RecomputeLight(*n);
        RebuildMesh(*n);
    }

    version_ += 1;
    return raw;
}

/*
 * 光场重算入口.
 * 四个方向的相邻列作为跨列播种来源传给光照场: 边界面的亮度由邻列按同一套
 * 扣减规则折算后参与本列扩散, 跨区块的火把与洞口因此亮度连续.
 * 缺失方向的邻列传空指针, 该方向按没有外部光源处理.
 */
void World::RecomputeLight(Chunk& chunk) {
    const voxel::ChunkColumn* neighbors[4] = {nullptr, nullptr, nullptr, nullptr};
    if (Chunk* n = FindChunk({chunk.coord.x + 1, chunk.coord.z})) neighbors[0] = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x - 1, chunk.coord.z})) neighbors[1] = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z + 1})) neighbors[2] = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z - 1})) neighbors[3] = &n->column;
    ColumnLightField().Recompute(chunk.column, neighbors, voxel::BlockRegistry::Instance());
}

void World::RebuildMesh(Chunk& chunk) {
    /* 提供四个方向的相邻列, 让区块交界处的被遮挡面被剔除. */
    voxel::NeighborColumns neighbors;
    if (Chunk* n = FindChunk({chunk.coord.x + 1, chunk.coord.z})) neighbors.east = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x - 1, chunk.coord.z})) neighbors.west = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z + 1})) neighbors.south = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z - 1})) neighbors.north = &n->column;
    /*
     * 群系草色: 按区块中心所在的群系带取草颜色, 归一化后传给网格构建,
     * 草方块顶面与树叶按它染色; 取不到带时按白色不染色.
     */
    float tint[3] = {1.0f, 1.0f, 1.0f};
    const int32_t band = voxel::TerrainGenerator{}.BiomeAt(
        seed_, chunk.coord.x * voxel::kChunkSizeX + voxel::kChunkSizeX / 2,
        chunk.coord.z * voxel::kChunkSizeZ + voxel::kChunkSizeZ / 2);
    float waterTint[3] = {1.0f, 1.0f, 1.0f};
    if (band >= 0 && band < voxel::kBiomeBandCount) {
        const voxel::BiomeBand& info = voxel::kBiomeBands[band];
        tint[0] = static_cast<float>(info.grassR) / 255.0f;
        tint[1] = static_cast<float>(info.grassG) / 255.0f;
        tint[2] = static_cast<float>(info.grassB) / 255.0f;
        /* 群系水色: 与草色同一来源, 液体面按它着色. */
        waterTint[0] = static_cast<float>(info.waterR) / 255.0f;
        waterTint[1] = static_cast<float>(info.waterG) / 255.0f;
        waterTint[2] = static_cast<float>(info.waterB) / 255.0f;
    }
    voxel::ChunkMesher{}.Build(chunk.column, chunk.coord, voxel::BlockRegistry::Instance(), neighbors,
                               chunk.mesh, tint, waterTint);
    chunk.meshDirty = false;
    chunk.revision += 1;
}

voxel::BlockId World::GetBlock(int32_t x, int32_t y, int32_t z) const {
    if (y < 0 || y >= voxel::kWorldHeight) {
        return 0;
    }
    Chunk* chunk = FindChunk(voxel::ChunkCoord{FloorDiv(x, voxel::kChunkSizeX), FloorDiv(z, voxel::kChunkSizeZ)});
    if (chunk == nullptr) {
        return 0;
    }
    return chunk->column.Get(x - chunk->coord.x * voxel::kChunkSizeX, y,
                             z - chunk->coord.z * voxel::kChunkSizeZ);
}

bool World::SetBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id) {
    if (y < 0 || y >= voxel::kWorldHeight) {
        return false;
    }
    const voxel::ChunkCoord coord{FloorDiv(x, voxel::kChunkSizeX), FloorDiv(z, voxel::kChunkSizeZ)};
    Chunk* chunk = FindChunk(coord);
    if (chunk == nullptr) {
        return false;
    }
    const int32_t localX = x - coord.x * voxel::kChunkSizeX;
    const int32_t localZ = z - coord.z * voxel::kChunkSizeZ;
    if (chunk->column.Get(localX, y, localZ) == id) {
        return false;
    }
    chunk->column.Set(localX, y, localZ, id);
    /* 方块增删改变遮光与光源分布, 全列重算光场后再重建网格. */
    RecomputeLight(*chunk);
    RebuildMesh(*chunk);

    /*
     * 边界方块影响相邻列的光照与剔除结果, 相邻列先重算光场
     * 再重建, 其余方向的相邻列光场不受影响, 只需重建网格.
     */
    if (localX == 0) {
        if (Chunk* n = FindChunk({coord.x - 1, coord.z})) {
            RecomputeLight(*n);
            RebuildMesh(*n);
        }
    }
    if (localX == voxel::kChunkSizeX - 1) {
        if (Chunk* n = FindChunk({coord.x + 1, coord.z})) {
            RecomputeLight(*n);
            RebuildMesh(*n);
        }
    }
    if (localZ == 0) {
        if (Chunk* n = FindChunk({coord.x, coord.z - 1})) {
            RecomputeLight(*n);
            RebuildMesh(*n);
        }
    }
    if (localZ == voxel::kChunkSizeZ - 1) {
        if (Chunk* n = FindChunk({coord.x, coord.z + 1})) {
            RecomputeLight(*n);
            RebuildMesh(*n);
        }
    }

    version_ += 1;
    RebuildVisibleList();
    return true;
}

int32_t World::HeightAt(int32_t x, int32_t z) const {
    for (int32_t y = voxel::kWorldHeight - 1; y >= 0; --y) {
        if (GetBlock(x, y, z) != 0) {
            return y + 1;
        }
    }
    return 0;
}

int32_t World::LoadedChunkCount() const {
    return static_cast<int32_t>(chunks_.size());
}

void World::RebuildVisibleList() {
    visible_.clear();
    visible_.reserve(chunks_.size());
    for (const auto& entry : chunks_) {
        const Chunk& chunk = *entry.second;
        if (chunk.mesh.Empty()) {
            continue;
        }
        ChunkView view;
        view.coord = chunk.coord;
        view.mesh = &chunk.mesh;
        view.originX = static_cast<float>(chunk.coord.x * voxel::kChunkSizeX);
        view.originZ = static_cast<float>(chunk.coord.z * voxel::kChunkSizeZ);
        view.revision = chunk.revision;
        visible_.push_back(view);
    }
}


namespace {

/*
 * 存档格式: 文件头 + 区块记录, 方块数据用游程编码存储.
 * 版本历史:
 *   1: 首个多世界之前的格式, 头内只有玩家姿态与种子;
 *   3: 追加世界名 (长度前缀 UTF-8 字节串) 与最后游玩时间, 支持世界列表.
 * 版本 2 从未在 world.cpp 落过盘 (容器状态走独立容器文件), 跳过不设防.
 */
constexpr char kSaveMagic[4] = {'I', 'M', 'W', 'F'};
constexpr uint32_t kSaveVersion = 3;
constexpr uint32_t kMaxChunks = 65536;

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

bool ReadRuns(FILE* file, uint32_t runCount, std::vector<voxel::BlockId>& blocks) {
    for (uint32_t i = 0; i < runCount; ++i) {
        uint16_t count = 0;
        uint16_t id = 0;
        if (std::fread(&count, sizeof(count), 1, file) != 1) {
            return false;
        }
        if (std::fread(&id, sizeof(id), 1, file) != 1) {
            return false;
        }
        if (blocks.size() + count > static_cast<size_t>(voxel::kChunkVolume)) {
            return false;
        }
        blocks.insert(blocks.end(), count, id);
    }
    return blocks.size() == static_cast<size_t>(voxel::kChunkVolume);
}

}  /* namespace */

bool World::SaveToFile(const char* path, const SaveState& state) const {
    if (path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }
    SaveHeader header{};
    std::memcpy(header.magic, kSaveMagic, sizeof(header.magic));
    header.version = kSaveVersion;
    header.seed = seed_;
    header.chunkCount = static_cast<uint32_t>(chunks_.size());
    header.playerX = state.playerX;
    header.playerY = state.playerY;
    header.playerZ = state.playerZ;
    header.yaw = state.yaw;
    header.pitch = state.pitch;
    if (std::fwrite(&header, sizeof(header), 1, file) != 1) {
        std::fclose(file);
        return false;
    }
    /* 版本 3 附加段: 世界名按长度前缀写入, 再写最后游玩时间. */
    const std::string& worldName = state.worldName;
    const uint32_t nameLength = static_cast<uint32_t>(worldName.size());
    if (std::fwrite(&nameLength, sizeof(nameLength), 1, file) != 1 ||
        (!worldName.empty() &&
         std::fwrite(worldName.data(), worldName.size(), 1, file) != 1)) {
        std::fclose(file);
        return false;
    }
    const uint64_t lastPlayed = state.lastPlayed;
    if (std::fwrite(&lastPlayed, sizeof(lastPlayed), 1, file) != 1) {
        std::fclose(file);
        return false;
    }
    /* 版本 3 扩展段尾: 世界时间总秒数, 与任务 10 的日夜循环配套. */
    const uint32_t timeOfDay = static_cast<uint32_t>(state.timeOfDaySeconds);
    if (std::fwrite(&timeOfDay, sizeof(timeOfDay), 1, file) != 1) {
        std::fclose(file);
        return false;
    }

    for (const auto& entry : chunks_) {
        const Chunk& chunk = *entry.second;
        const std::vector<voxel::BlockId>& blocks = chunk.column.Blocks();
        /* 先统计游程数量, 再写记录头与游程数据. */
        std::vector<std::pair<uint16_t, voxel::BlockId>> runs;
        runs.reserve(64);
        for (size_t i = 0; i < blocks.size();) {
            const voxel::BlockId id = blocks[i];
            uint32_t length = 1;
            while (i + length < blocks.size() && blocks[i + length] == id && length < 0xFFFF) {
                ++length;
            }
            runs.emplace_back(static_cast<uint16_t>(length), id);
            i += length;
        }
        ChunkRecord record{};
        record.x = chunk.coord.x;
        record.z = chunk.coord.z;
        record.runCount = static_cast<uint32_t>(runs.size());
        if (std::fwrite(&record, sizeof(record), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        for (const auto& run : runs) {
            if (std::fwrite(&run.first, sizeof(run.first), 1, file) != 1 ||
                std::fwrite(&run.second, sizeof(run.second), 1, file) != 1) {
                std::fclose(file);
                return false;
            }
        }
    }
    std::fclose(file);
    return true;
}

bool World::LoadFromFile(const char* path, SaveState& state) {
    if (path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    SaveHeader header{};
    if (std::fread(&header, sizeof(header), 1, file) != 1 ||
        std::memcmp(header.magic, kSaveMagic, sizeof(header.magic)) != 0 ||
        header.chunkCount > kMaxChunks) {
        std::fclose(file);
        return false;
    }
    /*
     * 版本兼容: 只接受 1 与 3. 版本 1 是旧档, 头后没有世界名与游玩时间,
     * 两个新字段回落空名与 0; 版本 2 从未落过盘, 与未知版本一并拒绝.
     */
    if (header.version != 1 && header.version != kSaveVersion) {
        std::fclose(file);
        return false;
    }

    /* 版本 3 的元数据紧跟文件头, 先于区块数据读出. */
    state.worldName.clear();
    state.lastPlayed = 0;
    if (header.version >= kSaveVersion) {
        uint32_t nameLength = 0;
        if (std::fread(&nameLength, sizeof(nameLength), 1, file) != 1 || nameLength > 256) {
            std::fclose(file);
            return false;
        }
        if (nameLength > 0) {
            state.worldName.resize(nameLength);
            if (std::fread(&state.worldName[0], nameLength, 1, file) != 1) {
                std::fclose(file);
                return false;
            }
        }
        if (std::fread(&state.lastPlayed, sizeof(state.lastPlayed), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        /*
         * 世界时间是 v3 的后扩字段: 早期 v3 档没有这一段. 试探读取前先记下
         * 文件位置, 读失败时回退, 否则会吞掉紧随其后的 ChunkRecord.x 四个字节,
         * 导致后续所有区块记录错位.
         */
        uint32_t timeOfDay = 0;
        const long timeFieldPos = std::ftell(file);
        if (std::fread(&timeOfDay, sizeof(timeOfDay), 1, file) == 1) {
            state.timeOfDaySeconds = static_cast<float>(timeOfDay);
        } else {
            if (timeFieldPos >= 0) {
                std::fseek(file, timeFieldPos, SEEK_SET);
            }
            state.timeOfDaySeconds = 0.0f;
        }
    }

    chunks_.clear();
    visible_.clear();
    seed_ = header.seed;

    for (uint32_t i = 0; i < header.chunkCount; ++i) {
        ChunkRecord record{};
        if (std::fread(&record, sizeof(record), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        auto chunk = std::make_unique<Chunk>();
        chunk->coord = voxel::ChunkCoord{record.x, record.z};
        std::vector<voxel::BlockId> blocks;
        blocks.reserve(voxel::kChunkVolume);
        if (!ReadRuns(file, record.runCount, blocks)) {
            std::fclose(file);
            return false;
        }
        chunk->column.AssignBlocks(blocks);
        Chunk* raw = chunk.get();
        chunks_.emplace(ChunkKey(record.x, record.z), std::move(chunk));
        RebuildMesh(*raw);
    }
    std::fclose(file);

    /* 相邻列的边界面按加载后的完整数据重新计算一次光照与网格. */
    for (auto& entry : chunks_) {
        RecomputeLight(*entry.second);
        RebuildMesh(*entry.second);
    }
    version_ += 1;
    RebuildVisibleList();

    state.playerX = header.playerX;
    state.playerY = header.playerY;
    state.playerZ = header.playerZ;
    state.yaw = header.yaw;
    state.pitch = header.pitch;
    state.seed = header.seed;
    return true;
}

/*
 * 只读头部: 与 LoadFromFile 同一套魔数与版本检查, 读到玩家姿态与元数据即返回,
 * 不触碰区块数据, 供世界列表逐文件扫描.
 */
bool World::PeekHeader(const char* path, SaveState& state) {
    if (path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    SaveHeader header{};
    if (std::fread(&header, sizeof(header), 1, file) != 1 ||
        std::memcmp(header.magic, kSaveMagic, sizeof(header.magic)) != 0 ||
        header.chunkCount > kMaxChunks ||
        (header.version != 1 && header.version != kSaveVersion)) {
        std::fclose(file);
        return false;
    }
    state.playerX = header.playerX;
    state.playerY = header.playerY;
    state.playerZ = header.playerZ;
    state.yaw = header.yaw;
    state.pitch = header.pitch;
    state.seed = header.seed;
    state.worldName.clear();
    state.lastPlayed = 0;
    if (header.version >= kSaveVersion) {
        uint32_t nameLength = 0;
        if (std::fread(&nameLength, sizeof(nameLength), 1, file) != 1 || nameLength > 256) {
            std::fclose(file);
            return false;
        }
        if (nameLength > 0) {
            state.worldName.resize(nameLength);
            if (std::fread(&state.worldName[0], nameLength, 1, file) != 1) {
                std::fclose(file);
                return false;
            }
        }
        if (std::fread(&state.lastPlayed, sizeof(state.lastPlayed), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        uint32_t timeOfDay = 0;
        if (std::fread(&timeOfDay, sizeof(timeOfDay), 1, file) == 1) {
            state.timeOfDaySeconds = static_cast<float>(timeOfDay);
        }
    }
    std::fclose(file);
    return true;
}

/*
 * 世界名安全化: 文件系统非法字符与控制字符折成下划线, 其余字节原样保留,
 * 因此中文与其余多字节字符的世界名可以正常落成文件名 (UTF-8 文件系统支持).
 * 折损后为空时回落 "default", 保证任意输入都能得到一个可用文件名.
 */
std::string World::SafeFileName(const std::string& name) {
    std::string safe;
    safe.reserve(name.size());
    for (const char c : name) {
        const unsigned char byte = static_cast<unsigned char>(c);
        /* 控制字符与 Windows 文件系统保留字符一律折成下划线. */
        const bool control = byte < 0x20u || byte == 0x7Fu;
        const bool reserved = c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
                              c == '"' || c == '<' || c == '>' || c == '|';
        safe += (control || reserved) ? '_' : c;
    }
    /* 首尾空格与点会被部分文件系统吞掉, 统一折成下划线避免名字漂移. */
    for (size_t i = 0; i < safe.size(); ++i) {
        if (safe[i] == ' ') {
            safe[i] = '_';
        }
    }
    while (!safe.empty() && safe.front() == '.') {
        safe.front() = '_';
    }
    if (safe.empty()) {
        safe = "default";
    }
    return safe;
}


namespace {

/* JSON 字符串转义: 只处理会破坏 JSON 结构的字符, 其余字节原样保留. */
std::string EscapeListJson(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (const char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

}  /* namespace */

std::string ListWorldsJson(const char* directory) {
    if (directory == nullptr) {
        return "[]";
    }
    DIR* dir = ::opendir(directory);
    if (dir == nullptr) {
        return "[]";
    }
    std::string json = "[";
    bool firstEntry = true;
    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        const std::string fileName = entry->d_name;
        if (fileName.size() <= 6 || fileName.compare(fileName.size() - 6, 6, ".world") != 0) {
            continue;
        }
        const std::string path = std::string(directory) + "/" + fileName;
        World::SaveState state;
        if (!World::PeekHeader(path.c_str(), state)) {
            continue;
        }
        std::string name = state.worldName;
        if (name.empty()) {
            name = fileName.substr(0, fileName.size() - 6);
        }
        /* 早期版本把带 .world 后缀的文件名当世界名写入, 读取时兼容剥离. */
        if (name.size() > 6 && name.compare(name.size() - 6, 6, ".world") == 0) {
            name.resize(name.size() - 6);
        }
        if (!firstEntry) {
            json += ",";
        }
        firstEntry = false;
        json += "{\"file\":\"" + EscapeListJson(fileName) + "\",\"name\":\"" + EscapeListJson(name) +
                "\",\"lastPlayed\":" + std::to_string(state.lastPlayed) +
                ",\"seed\":" + std::to_string(state.seed) +
                ",\"playerX\":" + std::to_string(state.playerX) +
                ",\"playerZ\":" + std::to_string(state.playerZ) + "}";
    }
    ::closedir(dir);
    json += "]";
    return json;
}

}  /* namespace immirror::world */
