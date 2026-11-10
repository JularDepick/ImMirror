#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/vec3.h"
#include "voxel/voxel_types.h"

namespace immirror::world {

/* 可见区块视图: 区块坐标与已构建好的网格, 渲染侧只读. */
struct ChunkView {
    voxel::ChunkCoord coord;
    const voxel::MeshData* mesh = nullptr;
    float originX = 0.0f;  /* 区块原点在世界坐标中的位置 */
    float originZ = 0.0f;
    uint64_t revision = 0; /* 该区块网格修订号, 每次重建递增, 渲染侧据此增量同步 */
};

/*
 * 体素世界. 负责区块的生成, 加载, 卸载, 方块读写与网格重建,
 * 渲染与玩家逻辑只通过本接口访问世界数据.
 */
class World {
public:
    explicit World(uint32_t seed = 20260101u);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void SetSeed(uint32_t seed);

    /* 以给定世界坐标为圆心维护视距内的区块; 每帧调用开销应当很小. */
    void Update(const core::Vec3& center, int32_t viewDistanceChunks);

    voxel::BlockId GetBlock(int32_t x, int32_t y, int32_t z) const;
    bool SetBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id);

    /* 该列最高非空方块的 y 加一, 用于出生点定位. */
    int32_t HeightAt(int32_t x, int32_t z) const;

    const std::vector<ChunkView>& VisibleChunks() const { return visible_; }

    /* 世界内容版本号, 方块变更后递增; 渲染侧据此判断是否需要重建缓冲. */
    uint64_t Version() const { return version_; }

    int32_t LoadedChunkCount() const;

    /* 存档状态: 玩家位置与朝向, 以及世界种子. */
    struct SaveState {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float playerZ = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        uint32_t seed = 0;
    };

    /* 把全部已加载区块与存档状态写入文件; 目录不存在时返回 false. */
    bool SaveToFile(const char* path, const SaveState& state) const;

    /* 从文件恢复世界; 成功后清空现有区块并按文件内容重建. */
    bool LoadFromFile(const char* path, SaveState& state);

private:
    struct Chunk;
    Chunk* FindChunk(voxel::ChunkCoord coord) const;
    Chunk* EnsureChunk(voxel::ChunkCoord coord);
    void RebuildMesh(Chunk& chunk);
    void RebuildVisibleList();

    uint32_t seed_ = 0;
    uint64_t version_ = 1;
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks_;
    std::vector<ChunkView> visible_;
    int32_t viewDistance_ = 0;
};

}  /* namespace immirror::world */
