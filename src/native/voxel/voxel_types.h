#pragma once

#include <cstdint>
#include <vector>

namespace immirror::voxel {

using BlockId = uint16_t;

/* 区块采用列式布局, 一列覆盖 16x16 水平范围与整个世界高度. */
constexpr int32_t kChunkSizeX = 16;
constexpr int32_t kChunkSizeZ = 16;
constexpr int32_t kWorldHeight = 256;
constexpr int32_t kChunkVolume = kChunkSizeX * kWorldHeight * kChunkSizeZ;

/*
 * 光场等级上限: 天光与方块光两通道共用, 值域 0 到 15.
 * 放在公共头保证列数据, 光场计算与网格采样引用同一语义.
 */
constexpr uint8_t kMaxLightLevel = 15;

struct ChunkCoord {
    int32_t x = 0;
    int32_t z = 0;
};

struct BlockPos {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
};

/* 顶点布局: 位置 3, 图集坐标 2, 明暗 1, 共 24 字节. */
struct Vertex {
    float px = 0.0f;
    float py = 0.0f;
    float pz = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    /* 朝向底色与光照系数的乘积, 光照系数由面外侧邻格光场折算. */
    float shade = 1.0f;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    /*
     * 半透明段在 indices 里的起点与长度. 水面等液体方块的面排在实心面之后,
     * 渲染侧先画 [0, waterIndexBegin), 再按由远到近画 [waterIndexBegin, 末尾).
     */
    size_t waterIndexBegin = 0;
    size_t waterIndexCount = 0;

    void Clear() {
        vertices.clear();
        indices.clear();
        waterIndexBegin = 0;
        waterIndexCount = 0;
    }
    bool Empty() const { return vertices.empty(); }
    size_t TriangleCount() const { return indices.size() / 3; }
};

/*
 * 采集工具. 取值与原版方块定义表的工具字段一致: 0 徒手, 1 斧, 2 镐, 3 铲.
 * 客户端当前还没有工具物品, 该字段只作为数据保留: 后续接入工具后,
 * 由手持工具档位与这里的期望档位比较决定挖掘倍率, 不改变硬度本身的语义.
 */
enum class HarvestTool : uint8_t {
    kHand = 0,
    kAxe = 1,
    kPickaxe = 2,
    kShovel = 3,
};

/*
 * 采集材质族, 决定挖掘音与放置音用哪一族音效.
 * 取值与 audio::SoundId 的分族顺序一一对应, kNone 表示没有音效 (水与基岩).
 * 方块层不引用音频枚举, 以免体素模块反向依赖 JNI 侧的头文件.
 */
enum class HarvestMaterial : uint8_t {
    kNone = 0,
    kGrass,
    kStone,
    kSand,
    kWood,
    kSnow,
    kGravel,
    kCloth,
    kGlass,
    kJar,
    kLadder,
};

/* 方块静态属性, 由注册表持有, 生存期与进程一致. */
struct BlockInfo {
    const char* name = "air";
    bool solid = false;      /* 参与碰撞 */
    bool opaque = false;     /* 遮挡邻面 */
    uint16_t atlasIndex = 0;      /* 图集格索引, 作为未指定面时的默认值 */
    uint16_t atlasTop = 0xFFFF;    /* 顶面图集格, 0xFFFF 表示回落到 atlasIndex */
    uint16_t atlasSide = 0xFFFF;   /* 侧面图集格, 0xFFFF 表示回落到 atlasIndex */
    uint16_t atlasBottom = 0xFFFF; /* 底面图集格, 0xFFFF 表示回落到 atlasIndex */
    /*
     * 原版硬度, 合法范围 -1 到 200, 步长 0.5.
     * -1 表示不可破坏 (空气, 水, 基岩), 既不累计挖掘进度也不响应破坏.
     */
    float hardness = -1.0f;
    HarvestTool tool = HarvestTool::kHand;             /* 采集工具档位 */
    HarvestMaterial material = HarvestMaterial::kNone; /* 采集材质族, 决定音效 */
    /*
     * 原版光照三列折算: 光源强度 0-15 (0 表示不发光), 光衰减 0-15,
     * 遮挡旁边块 0 不遮挡 / 1 遮挡 / 2 按厚度 / 3 遮挡同材质.
     * 天光与方块光传播由 LightField 消费, 网格明暗采样由 chunk_mesher 消费.
     */
    uint8_t lightSource = 0;
    uint8_t lightAtten = 0;
    uint8_t coverMode = 1;
};

}  /* namespace immirror::voxel */