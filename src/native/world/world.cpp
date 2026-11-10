#include "world/world.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <utility>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/chunk_mesher.h"
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

void World::Update(const core::Vec3& center, int32_t viewDistanceChunks) {
    if (viewDistanceChunks < 1) {
        viewDistanceChunks = 1;
    }
    viewDistance_ = viewDistanceChunks;

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
                EnsureChunk(coord);
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

World::Chunk* World::EnsureChunk(voxel::ChunkCoord coord) {
    Chunk* existing = FindChunk(coord);
    if (existing != nullptr) {
        return existing;
    }
    auto chunk = std::make_unique<Chunk>();
    chunk->coord = coord;
    voxel::TerrainGenerator{}.Generate(coord, chunk->column, seed_);
    chunk->meshDirty = true;
    Chunk* raw = chunk.get();
    chunks_.emplace(ChunkKey(coord.x, coord.z), std::move(chunk));
    RebuildMesh(*raw);

    /* 新列加入后, 已存在的相邻列边界遮挡关系发生变化, 需要一并重建. */
    if (Chunk* n = FindChunk({coord.x + 1, coord.z})) RebuildMesh(*n);
    if (Chunk* n = FindChunk({coord.x - 1, coord.z})) RebuildMesh(*n);
    if (Chunk* n = FindChunk({coord.x, coord.z + 1})) RebuildMesh(*n);
    if (Chunk* n = FindChunk({coord.x, coord.z - 1})) RebuildMesh(*n);

    version_ += 1;
    return raw;
}

void World::RebuildMesh(Chunk& chunk) {
    /* 提供四个方向的相邻列, 让区块交界处的被遮挡面被剔除. */
    voxel::NeighborColumns neighbors;
    if (Chunk* n = FindChunk({chunk.coord.x + 1, chunk.coord.z})) neighbors.east = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x - 1, chunk.coord.z})) neighbors.west = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z + 1})) neighbors.south = &n->column;
    if (Chunk* n = FindChunk({chunk.coord.x, chunk.coord.z - 1})) neighbors.north = &n->column;
    voxel::ChunkMesher{}.Build(chunk.column, chunk.coord, voxel::BlockRegistry::Instance(), neighbors,
                               chunk.mesh);
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
    RebuildMesh(*chunk);

    /* 边界方块影响相邻列的剔除结果, 一并重建. */
    if (localX == 0) { if (Chunk* n = FindChunk({coord.x - 1, coord.z})) RebuildMesh(*n); }
    if (localX == voxel::kChunkSizeX - 1) { if (Chunk* n = FindChunk({coord.x + 1, coord.z})) RebuildMesh(*n); }
    if (localZ == 0) { if (Chunk* n = FindChunk({coord.x, coord.z - 1})) RebuildMesh(*n); }
    if (localZ == voxel::kChunkSizeZ - 1) { if (Chunk* n = FindChunk({coord.x, coord.z + 1})) RebuildMesh(*n); }

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

/* 存档格式: 文件头 + 区块记录, 方块数据用游程编码存储. */
constexpr char kSaveMagic[4] = {'I', 'M', 'W', 'F'};
constexpr uint32_t kSaveVersion = 1;
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
        header.version != kSaveVersion || header.chunkCount > kMaxChunks) {
        std::fclose(file);
        return false;
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

    /* 相邻列的边界面按加载后的完整数据重新计算一次. */
    for (auto& entry : chunks_) {
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

}  /* namespace immirror::world */
