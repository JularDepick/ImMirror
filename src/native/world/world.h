#pragma once

#include <cstdint>
#include <memory>
#include <string>
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

    /* 当前世界种子, 落盘与新建世界时用来核对地形来源. */
    uint32_t Seed() const { return seed_; }

    /*
     * 以给定世界坐标为圆心维护视距内的区块; 每帧调用开销应当很小.
     * treeShape 只作用于本次调用中新生成的列, 已加载列不回改;
     * 0 表示矮树, 1 表示标准树, 与设置键 treeshape 的取值一致.
     */
    void Update(const core::Vec3& center, int32_t viewDistanceChunks, uint8_t treeShape = 1);

    voxel::BlockId GetBlock(int32_t x, int32_t y, int32_t z) const;

    /* 流体等级: 0 为源头或满格, 1 到 7 逐级变浅; 非液体格返回 0. */
    uint8_t GetFluidLevel(int32_t x, int32_t y, int32_t z) const;
    /* 设置流体等级; 越界或列未加载时忽略. */
    void SetFluidLevel(int32_t x, int32_t y, int32_t z, uint8_t level);
    bool SetBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id);

    /* 该列最高非空方块的 y 加一, 用于出生点定位. */
    int32_t HeightAt(int32_t x, int32_t z) const;

    const std::vector<ChunkView>& VisibleChunks() const { return visible_; }

    /* 世界内容版本号, 方块变更后递增; 渲染侧据此判断是否需要重建缓冲. */
    uint64_t Version() const { return version_; }

    int32_t LoadedChunkCount() const;

    /*
     * 存档状态: 玩家位置与朝向, 世界种子, 以及世界元数据.
     * worldName 是存档头里的世界名, 旧版本存档读出空串, 游戏侧回落 "default";
     * lastPlayed 是最近一次写档的 Unix 秒级时间戳, 旧版本存档读出 0.
     */
    struct SaveState {
        float playerX = 0.0f;
        float playerY = 0.0f;
        float playerZ = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        uint32_t seed = 0;
        std::string worldName;
        uint64_t lastPlayed = 0;
        /*
         * 世界时间: 存档时刻游戏内已过的总秒数, 20 分钟现实时间为一游戏日.
         * 版本 3 扩展字段, 更早的 v3 存档没有这一段, 读出 0 由游戏侧回落正午.
         */
        float timeOfDaySeconds = 0.0f;
    };

    /* 把全部已加载区块与存档状态写入文件; 目录不存在时返回 false. */
    bool SaveToFile(const char* path, const SaveState& state) const;

    /* 从文件恢复世界; 成功后清空现有区块并按文件内容重建. */
    bool LoadFromFile(const char* path, SaveState& state);

    /*
     * 只读取一个存档文件的头部元数据, 不加载区块:
     * 世界列表按文件逐个取名字与最后游玩时间, 走这条轻量路径.
     * 文件缺失, 魔数不符或版本未知时返回 false.
     */
    static bool PeekHeader(const char* path, SaveState& state);

    /* 世界名安全化: 非字母数字下划线连字的字符折成下划线, 与存档文件名规则一致. */
    static std::string SafeFileName(const std::string& name);

private:
    struct Chunk;
    Chunk* FindChunk(voxel::ChunkCoord coord) const;
    Chunk* EnsureChunk(voxel::ChunkCoord coord, uint8_t treeShape);
    /* 重算一列光场, 并把四个方向的相邻列作为跨列播种来源. */
    void RecomputeLight(Chunk& chunk);

    void RebuildMesh(Chunk& chunk);
    void RebuildVisibleList();

    uint32_t seed_ = 0;
    uint64_t version_ = 1;
    /* 最近一次 Update 下发的树形档, 供后续新建列沿用. */
    uint8_t treeShape_ = 1;
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks_;
    std::vector<ChunkView> visible_;
    int32_t viewDistance_ = 0;
};

/*
 * 世界列表: 扫描目录下全部 .world 存档并输出 JSON 数组.
 * 只读文件头, 不依赖游戏运行时与渲染上下文, 因此大厅与冷启动也能调用.
 * 字段: file, name, lastPlayed, seed, playerX, playerZ.
 */
std::string ListWorldsJson(const char* directory);

}  /* namespace immirror::world */
