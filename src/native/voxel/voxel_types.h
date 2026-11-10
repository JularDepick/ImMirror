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
    float shade = 1.0f;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void Clear() {
        vertices.clear();
        indices.clear();
    }
    bool Empty() const { return vertices.empty(); }
    size_t TriangleCount() const { return indices.size() / 3; }
};

/* 方块静态属性, 由注册表持有, 生存期与进程一致. */
struct BlockInfo {
    const char* name = "air";
    bool solid = false;      /* 参与碰撞 */
    bool opaque = false;     /* 遮挡邻面 */
    uint8_t atlasIndex = 0;  /* 图集格索引, 作为未指定面时的默认值 */
    uint8_t atlasTop = 0xFF;    /* 顶面图集格, 0xFF 表示回落到 atlasIndex */
    uint8_t atlasSide = 0xFF;   /* 侧面图集格, 0xFF 表示回落到 atlasIndex */
    uint8_t atlasBottom = 0xFF; /* 底面图集格, 0xFF 表示回落到 atlasIndex */
};

}  /* namespace immirror::voxel */
