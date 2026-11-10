#include "world/world.h"

#include <cmath>
#include <cstdlib>

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

}  /* namespace immirror::world */
