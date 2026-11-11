#include "voxel/chunk_mesher.h"

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"

namespace immirror::voxel {
namespace {

constexpr int32_t kFaceCount = 6;

/* 图集按 16x16 格切分, 每格在 uv 上占 1/16; 列 = 格索引 % 16, 行 = 格索引 / 16. */
constexpr int32_t kTileGridSize = 64;
constexpr float kTileExtent = 1.0f / static_cast<float>(kTileGridSize);

/* 面未指定图集格时的回落标记, 与 BlockInfo 的约定一致. */
constexpr uint8_t kAtlasFallback = 0xFF;

/* 同一方块顶面, 侧面, 底面的图集格, 已把回落解析成具体格号. */
struct FaceTiles {
    uint8_t top = 0;
    uint8_t side = 0;
    uint8_t bottom = 0;
};

/* 未指定 (0xFF) 时回落到方块默认图集格. */
uint8_t ResolveTile(uint8_t specific, uint8_t fallback) {
    return specific == kAtlasFallback ? fallback : specific;
}

/* 方块属性缓存规模, 足以覆盖注册表全部基础方块, 避免逐面查表. */
constexpr size_t kCachedBlockLimit = 32;

/* 一层方块在一维数组中的步长 (16x16), 相邻层相差这个值. */
constexpr size_t kLayerStride = static_cast<size_t>(kChunkSizeX) * static_cast<size_t>(kChunkSizeZ);

/* 网格初始容量, 按带相邻列剔除后的常见规模预留, 不足时由 vector 自行扩容. */
constexpr size_t kInitialVertexReserve = 3072;
constexpr size_t kInitialIndexReserve = 4608;

/*
 * 单面布局. 四个角给出相对方块原点的偏移, 顺序保证从外部看去是逆时针,
 * shade 是该朝向的固定明暗 (顶面最亮, 底面最暗).
 */
struct FaceLayout {
    float shade;
    float cornerX[4];
    float cornerY[4];
    float cornerZ[4];
};

/*
 * 六个面的顶点表, 顺序固定为 +X, -X, +Y, -Y, +Z, -Z, 与邻居判定数组一一对应.
 * 每个四边形的两个三角形使用 0-1-2 与 0-2-3, 环绕方向一致.
 */
const FaceLayout kFaceLayouts[kFaceCount] = {
    {0.8f, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {0.8f, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 0.0f}},
    {1.0f, {0.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 0.0f}},
    {0.5f, {0.0f, 1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    {0.8f, {0.0f, 1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    {0.8f, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f}},
};

}  /* namespace */

void ChunkMesher::Build(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
                        MeshData& out) const {
    const ChunkColumn* neighbors[4] = {nullptr, nullptr, nullptr, nullptr};
    BuildWithNeighbors(column, coord, registry, neighbors, out);
}

void ChunkMesher::Build(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
                        const NeighborColumns& neighbors, MeshData& out) const {
    const ChunkColumn* list[4] = {neighbors.east, neighbors.west, neighbors.south, neighbors.north};
    BuildWithNeighbors(column, coord, registry, list, out);
}

void ChunkMesher::BuildWithNeighbors(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
                                     const ChunkColumn* const* neighbors, MeshData& out) const {
    /* 顶点使用区块局部坐标, 列坐标只标注调用方来源, 不参与计算. */
    (void)coord;
    out.Clear();

    const int32_t topY = column.TopBlockY();
    if (topY < 0) {
        return;
    }

    /* 常用方块的遮挡与三面图集格先取到局部表, 内层循环只做数组读取. */
    const size_t blockCount = registry.Count();
    const size_t cachedCount = blockCount < kCachedBlockLimit ? blockCount : kCachedBlockLimit;
    FaceTiles cachedTiles[kCachedBlockLimit];
    bool cachedOpaque[kCachedBlockLimit];
    for (size_t i = 0; i < cachedCount; ++i) {
        const BlockInfo& info = registry.Info(static_cast<BlockId>(i));
        cachedTiles[i].top = ResolveTile(info.atlasTop, info.atlasIndex);
        cachedTiles[i].side = ResolveTile(info.atlasSide, info.atlasIndex);
        cachedTiles[i].bottom = ResolveTile(info.atlasBottom, info.atlasIndex);
        cachedOpaque[i] = info.opaque;
    }
    /* 取某方块三个朝向的图集格, 缓存未覆盖的方块直接查注册表. */
    const auto tilesOf = [&registry, &cachedTiles, cachedCount](BlockId id) {
        const size_t idIndex = static_cast<size_t>(id);
        if (idIndex < cachedCount) {
            return cachedTiles[idIndex];
        }
        const BlockInfo& info = registry.Info(id);
        return FaceTiles{ResolveTile(info.atlasTop, info.atlasIndex), ResolveTile(info.atlasSide, info.atlasIndex),
                         ResolveTile(info.atlasBottom, info.atlasIndex)};
    };
    /* 面能否看见: 邻居与自身同类视为内部面, 遮挡方块挡住面, 其余 (空气, 水等) 都要出面. */
    const auto faceVisible = [&registry, &cachedOpaque, cachedCount](BlockId self, BlockId neighbor) {
        if (neighbor == self) {
            return false;
        }
        const size_t index = static_cast<size_t>(neighbor);
        const bool opaque = index < cachedCount ? cachedOpaque[index] : registry.Info(neighbor).opaque;
        return !opaque;
    };

    const std::vector<BlockId>& blocks = column.Blocks();
    out.vertices.reserve(kInitialVertexReserve);
    out.indices.reserve(kInitialIndexReserve);
    /* 下面按 y 层, z 行, x 列的顺序线性遍历方块数组, 索引自增, 不做乘法. */

    for (int32_t y = 0; y <= topY; ++y) {
        for (int32_t z = 0; z < kChunkSizeZ; ++z) {
            size_t index = static_cast<size_t>(y) * kLayerStride + static_cast<size_t>(z) * static_cast<size_t>(kChunkSizeX);
            for (int32_t x = 0; x < kChunkSizeX; ++x, ++index) {
                const BlockId id = blocks[index];
                if (id == block::Air) {
                    continue;
                }
                /*
                 * 先判定六个方向是否需要出面. 水平方向越出区块时查相邻列,
                 * 相邻列缺失就把边界当空气处理, 保证不出现漏洞.
                 * 竖直方向越出世界高度范围的面永远不可见, 直接丢弃.
                 */
                bool needFace[kFaceCount];
                if (x + 1 < kChunkSizeX) {
                    needFace[0] = faceVisible(id, blocks[index + 1]);
                } else {
                    needFace[0] = neighbors[0] != nullptr ? faceVisible(id, neighbors[0]->Get(0, y, z)) : true;
                }
                if (x > 0) {
                    needFace[1] = faceVisible(id, blocks[index - 1]);
                } else {
                    needFace[1] =
                        neighbors[1] != nullptr ? faceVisible(id, neighbors[1]->Get(kChunkSizeX - 1, y, z)) : true;
                }
                needFace[2] = (y + 1 < kWorldHeight) ? faceVisible(id, blocks[index + kLayerStride]) : false;
                needFace[3] = (y > 0) ? faceVisible(id, blocks[index - kLayerStride]) : false;
                if (z + 1 < kChunkSizeZ) {
                    needFace[4] = faceVisible(id, blocks[index + kChunkSizeX]);
                } else {
                    needFace[4] = neighbors[2] != nullptr ? faceVisible(id, neighbors[2]->Get(x, y, 0)) : true;
                }
                if (z > 0) {
                    needFace[5] = faceVisible(id, blocks[index - kChunkSizeX]);
                } else {
                    needFace[5] =
                        neighbors[3] != nullptr ? faceVisible(id, neighbors[3]->Get(x, y, kChunkSizeZ - 1)) : true;
                }
                if (!needFace[0] && !needFace[1] && !needFace[2] && !needFace[3] && !needFace[4] && !needFace[5]) {
                    continue;
                }

                const FaceTiles tiles = tilesOf(id);
                const float originX = static_cast<float>(x);
                const float originY = static_cast<float>(y);
                const float originZ = static_cast<float>(z);

                for (int32_t face = 0; face < kFaceCount; ++face) {
                    if (!needFace[face]) {
                        continue;
                    }
                    const FaceLayout& layout = kFaceLayouts[face];
                    /* 朝向决定用哪一格: +Y 顶面, -Y 底面, 其余四个侧面. */
                    const uint8_t tile = (face == 2) ? tiles.top : ((face == 3) ? tiles.bottom : tiles.side);
                    const float tileU =
                        static_cast<float>(static_cast<int32_t>(tile) % kTileGridSize) * kTileExtent;
                    const float tileV =
                        static_cast<float>(static_cast<int32_t>(tile) / kTileGridSize) * kTileExtent;
                    const uint32_t baseVertex = static_cast<uint32_t>(out.vertices.size());
                    for (int32_t corner = 0; corner < 4; ++corner) {
                        Vertex vertex;
                        vertex.px = originX + layout.cornerX[corner];
                        vertex.py = originY + layout.cornerY[corner];
                        vertex.pz = originZ + layout.cornerZ[corner];
                        vertex.u = tileU + ((corner == 1 || corner == 2) ? kTileExtent : 0.0f);
                        vertex.v = tileV + (corner >= 2 ? kTileExtent : 0.0f);
                        vertex.shade = layout.shade;
                        out.vertices.push_back(vertex);
                    }
                    out.indices.push_back(baseVertex);
                    out.indices.push_back(baseVertex + 1);
                    out.indices.push_back(baseVertex + 2);
                    out.indices.push_back(baseVertex);
                    out.indices.push_back(baseVertex + 2);
                    out.indices.push_back(baseVertex + 3);
                }
            }
        }
    }
}

}  /* namespace immirror::voxel */