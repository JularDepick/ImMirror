#include "voxel/chunk_mesher.h"

#include <cstring>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"

namespace immirror::voxel {
namespace {

constexpr int32_t kFaceCount = 6;

/*
 * 六向面外法线方向表, 顺序与 kFaceLayouts 的面序一致:
 * +X, -X, +Y, -Y, +Z, -Z; 光照采样按该方向取面外侧邻格.
 */
constexpr int32_t kDirX[6] = {1, -1, 0, 0, 0, 0};
constexpr int32_t kDirY[6] = {0, 0, 1, -1, 0, 0};
constexpr int32_t kDirZ[6] = {0, 0, 0, 0, 1, -1};

/* 图集按 16x16 格切分, 每格在 uv 上占 1/16; 列 = 格索引 % 16, 行 = 格索引 / 16. */
constexpr int32_t kTileGridSize = 64;
constexpr float kTileExtent = 1.0f / static_cast<float>(kTileGridSize);

/* 面未指定图集格时的回落标记, 与 BlockInfo 的约定一致. */
constexpr uint16_t kAtlasFallback = 0xFFFF;

/* 同一方块顶面, 侧面, 底面的图集格, 已把回落解析成具体格号. */
struct FaceTiles {
    uint16_t top = 0;
    uint16_t side = 0;
    uint16_t bottom = 0;
};

/* 未指定 (0xFFFF) 时回落到方块默认图集格. */
uint16_t ResolveTile(uint16_t specific, uint16_t fallback) {
    return specific == kAtlasFallback ? fallback : specific;
}

/* 方块属性缓存规模, 足以覆盖注册表全部基础方块, 避免逐面查表. */
constexpr size_t kCachedBlockLimit = 32;

/*
 * 液体判定按名字: 客户端把液体方块建成非实心且非遮挡, 花草树木也有同样的属性,
 * 只看属性会把水与花混为一谈. 液体键都含 water/lava 或带 _flow/_still 后缀.
 */
bool IsLiquidName(const char* name) {
    if (name == nullptr) {
        return false;
    }
    if (std::strstr(name, "water") != nullptr || std::strstr(name, "lava") != nullptr) {
        return true;
    }
    const size_t length = std::strlen(name);
    const char* flowSuffix = "_flow";
    const char* stillSuffix = "_still";
    const size_t flowLength = std::strlen(flowSuffix);
    const size_t stillLength = std::strlen(stillSuffix);
    if (length > flowLength && std::strcmp(name + length - flowLength, flowSuffix) == 0) {
        return true;
    }
    return length > stillLength && std::strcmp(name + length - stillLength, stillSuffix) == 0;
}

/* 花草类: 非实心, 非遮挡, 也不是液体; 这类方块按两片交叉面绘制, 不画立方体. */
bool IsPlantLike(const BlockInfo& info) {
    return !info.solid && !info.opaque && !IsLiquidName(info.name);
}

/*
 * 非整块形状的包围盒. 每个形状给一到两个盒子, 盒内按立方体生成六个面.
 * 这些形状不做邻居面剔除: 体积小, 面数有限, 剔除逻辑的复杂度不划算.
 */
struct ShapeBox {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

/* 返回该形状的盒子数, 盒子写进 out; 整块形状返回 0 表示走常规路径. */
int32_t ShapeBoxesOf(BlockShape shape, ShapeBox* out) {
    switch (shape) {
        case BlockShape::kSlab:
            out[0] = ShapeBox{0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f};
            return 1;
        case BlockShape::kStair:
            /* 下半整格, 上半只占 z 前半, 与楼梯的踏面一致. */
            out[0] = ShapeBox{0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f};
            out[1] = ShapeBox{0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f};
            return 2;
        case BlockShape::kFence:
            out[0] = ShapeBox{0.375f, 0.0f, 0.375f, 0.625f, 1.0f, 0.625f};
            return 1;
        case BlockShape::kPane:
            out[0] = ShapeBox{0.4375f, 0.0f, 0.0f, 0.5625f, 1.0f, 1.0f};
            return 1;
        case BlockShape::kCarpet:
            out[0] = ShapeBox{0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f};
            return 1;
        case BlockShape::kCube:
        default:
            return 0;
    }
}

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
                        MeshData& out, const float* tint) const {
    const ChunkColumn* neighbors[4] = {nullptr, nullptr, nullptr, nullptr};
    BuildWithNeighbors(column, coord, registry, neighbors, out, tint);
}

void ChunkMesher::Build(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
                        const NeighborColumns& neighbors, MeshData& out,
                        const float* tint) const {
    const ChunkColumn* list[4] = {neighbors.east, neighbors.west, neighbors.south, neighbors.north};
    BuildWithNeighbors(column, coord, registry, list, out, tint);
}

void ChunkMesher::BuildWithNeighbors(const ChunkColumn& column, ChunkCoord coord,
                                     const BlockRegistry& registry, const ChunkColumn* const* neighbors,
                                     MeshData& out, const float* tint) const {
    /* 顶点使用区块局部坐标, 列坐标只标注调用方来源, 不参与计算. */
    (void)coord;
    /* 群系草色: 缺省白色即不染色; 染色只作用于草方块顶面与树叶. */
    const float tintR = tint != nullptr ? tint[0] : 1.0f;
    const float tintG = tint != nullptr ? tint[1] : 1.0f;
    const float tintB = tint != nullptr ? tint[2] : 1.0f;
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
    bool cachedLiquid[kCachedBlockLimit];
    for (size_t i = 0; i < cachedCount; ++i) {
        const BlockInfo& info = registry.Info(static_cast<BlockId>(i));
        cachedTiles[i].top = ResolveTile(info.atlasTop, info.atlasIndex);
        cachedTiles[i].side = ResolveTile(info.atlasSide, info.atlasIndex);
        cachedTiles[i].bottom = ResolveTile(info.atlasBottom, info.atlasIndex);
        cachedOpaque[i] = info.opaque;
        /* 液体判定: 既不实心也不遮挡, 且名字属于水或岩浆系. */
        cachedLiquid[i] = !info.solid && !info.opaque && IsLiquidName(info.name);
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

    /*
     * 面外侧邻格光照采样: 光照系数取两通道较大值, 归一化到 0.25 到
     * 1.0 区间, 下限防纯黑. 采样格在列边界外时转向对应方向的相邻列
     * 取对侧边界格, 相邻列缺失才回落, 天光按全亮, 方块光按 0,
     * 避免边缘黑缝; 相邻列就位后随网格重建自然修正.
     */
    const auto sampleLight = [&column, neighbors](int32_t lx, int32_t ly, int32_t lz, int32_t face,
                                                  uint8_t* outSky, uint8_t* outBlock) {
        int32_t x = lx + kDirX[face];
        int32_t y = ly + kDirY[face];
        int32_t z = lz + kDirZ[face];
        const ChunkColumn* source = &column;
        if (x >= kChunkSizeX) {
            source = neighbors[0];
            x = 0;
        } else if (x < 0) {
            source = neighbors[1];
            x = kChunkSizeX - 1;
        } else if (z >= kChunkSizeZ) {
            source = neighbors[2];
            z = 0;
        } else if (z < 0) {
            source = neighbors[3];
            z = kChunkSizeZ - 1;
        }
        uint8_t sky = kMaxLightLevel;
        uint8_t blockLight = 0;
        if (source != nullptr && y >= 0 && y < kWorldHeight) {
            sky = source->SkyLightAt(x, y, z);
            blockLight = source->BlockLightAt(x, y, z);
        } else if (y < 0) {
            sky = 0;
        }
        *outSky = sky;
        *outBlock = blockLight;
    };
    const auto lightAt = [&sampleLight](int32_t lx, int32_t ly, int32_t lz, int32_t face) {
        uint8_t sky = kMaxLightLevel;
        uint8_t blockLight = 0;
        sampleLight(lx, ly, lz, face, &sky, &blockLight);
        const uint8_t level = sky > blockLight ? sky : blockLight;
        return 0.25f + 0.75f * static_cast<float>(level) / static_cast<float>(kMaxLightLevel);
    };

    /* 液体面单独收集, 网格末尾拼接到实心面之后, 渲染侧据此分两趟绘制. */
    const auto isLiquid = [&registry, &cachedLiquid, cachedCount](BlockId id) {
        const size_t index = static_cast<size_t>(id);
        if (index < cachedCount) {
            return cachedLiquid[index];
        }
        const BlockInfo& info = registry.Info(id);
        return !info.solid && !info.opaque && IsLiquidName(info.name);
    };
    /* 花草类按交叉双面绘制, 与花草的平面片一致. */
    const auto isPlant = [&registry](BlockId id) { return IsPlantLike(registry.Info(id)); };
    std::vector<uint32_t> liquidIndices;

    const std::vector<BlockId>& blocks = column.Blocks();
    out.vertices.reserve(kInitialVertexReserve);
    out.indices.reserve(kInitialIndexReserve);
    liquidIndices.reserve(512);
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
                 * 非整块形状: 按包围盒逐盒生成六个面, 不做邻居剔除.
                 * 台阶, 楼梯, 栅栏, 玻璃板, 地毯属于这一类, 体积小面数有限.
                 */
                {
                    ShapeBox boxes[2];
                    const int32_t boxCount = ShapeBoxesOf(registry.Info(id).shape, boxes);
                    if (boxCount > 0) {
                        const FaceTiles shapedTiles = tilesOf(id);
                        const uint16_t topTile =
                            shapedTiles.top == kAtlasFallback ? shapedTiles.side : shapedTiles.top;
                        const uint16_t sideTile =
                            shapedTiles.side == kAtlasFallback ? shapedTiles.top : shapedTiles.side;
                        const uint16_t bottomTile =
                            shapedTiles.bottom == kAtlasFallback ? sideTile : shapedTiles.bottom;
                        for (int32_t box = 0; box < boxCount; ++box) {
                            const ShapeBox& shape = boxes[box];
                            const float light = lightAt(x, y, z, 2);
                            for (int32_t face = 0; face < kFaceCount; ++face) {
                                const FaceLayout& layout = kFaceLayouts[face];
                                const uint16_t tile =
                                    face == 2 ? topTile : (face == 3 ? bottomTile : sideTile);
                                if (tile == kAtlasFallback) {
                                    continue;
                                }
                                const float tileU =
                                    static_cast<float>(static_cast<int32_t>(tile) % kTileGridSize) * kTileExtent;
                                const float tileV =
                                    static_cast<float>(static_cast<int32_t>(tile) / kTileGridSize) * kTileExtent;
                                const uint32_t base = static_cast<uint32_t>(out.vertices.size());
                                for (int32_t corner = 0; corner < 4; ++corner) {
                                    /* 角点先按单位立方体取, 再线性映射进形状盒. */
                                    const float unitX = layout.cornerX[corner];
                                    const float unitY = layout.cornerY[corner];
                                    const float unitZ = layout.cornerZ[corner];
                                    Vertex vertex;
                                    vertex.px = static_cast<float>(x) + shape.minX +
                                                (shape.maxX - shape.minX) * unitX;
                                    vertex.py = static_cast<float>(y) + shape.minY +
                                                (shape.maxY - shape.minY) * unitY;
                                    vertex.pz = static_cast<float>(z) + shape.minZ +
                                                (shape.maxZ - shape.minZ) * unitZ;
                                    vertex.u = tileU + ((corner == 1 || corner == 2) ? kTileExtent : 0.0f);
                                    vertex.v = tileV + (corner >= 2 ? kTileExtent : 0.0f);
                                    vertex.shade = layout.shade * light;
                                    out.vertices.push_back(vertex);
                                }
                                out.indices.push_back(base);
                                out.indices.push_back(base + 1);
                                out.indices.push_back(base + 2);
                                out.indices.push_back(base);
                                out.indices.push_back(base + 2);
                                out.indices.push_back(base + 3);
                            }
                        }
                        continue;
                    }
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

                /*
                 * 花草: 两片沿对角线交叉的竖直面, 正反各一遍, 因此不受背面剔除影响.
                 * 贴图取侧面格, 明暗取该格的天光与方块光, 比整方块更接近观感.
                 */
                if (isPlant(id)) {
                    const uint16_t tile = tiles.side == kAtlasFallback ? tiles.top : tiles.side;
                    if (tile == kAtlasFallback) {
                        continue;
                    }
                    const float tileU = static_cast<float>(static_cast<int32_t>(tile) % kTileGridSize) * kTileExtent;
                    const float tileV = static_cast<float>(static_cast<int32_t>(tile) / kTileGridSize) * kTileExtent;
                    const float light = lightAt(x, y, z, 2);
                    const float shade = 0.92f * light;
                    /*
                     * 草类植物叠群系草色, 花类保持原色 (花有自己的颜色);
                     * 所有植物都吃方块光暖色, 火把旁边的花不会发蓝.
                     */
                    const char* plantName = registry.Info(id).name;
                    const bool tintPlant =
                        plantName != nullptr && std::strstr(plantName, "grass") != nullptr;
                    uint8_t plantSky = kMaxLightLevel;
                    uint8_t plantBlock = 0;
                    sampleLight(x, y, z, 2, &plantSky, &plantBlock);
                    const float inset = 0.12f;
                    const float low = inset;
                    const float high = 1.0f - inset;
                    /* 两条对角线的四个角点 (水平面内). */
                    const float diagonalX0 = originX + low;
                    const float diagonalZ0 = originZ + low;
                    const float diagonalX1 = originX + high;
                    const float diagonalZ1 = originZ + high;
                    const float topY = originY + 1.0f;
                    const float corners[2][4][3] = {
                        {{diagonalX0, originY, diagonalZ0}, {diagonalX1, originY, diagonalZ1},
                         {diagonalX1, topY, diagonalZ1}, {diagonalX0, topY, diagonalZ0}},
                        {{diagonalX0, originY, diagonalZ1}, {diagonalX1, originY, diagonalZ0},
                         {diagonalX1, topY, diagonalZ0}, {diagonalX0, topY, diagonalZ1}},
                    };
                    for (int32_t plane = 0; plane < 2; ++plane) {
                        for (int32_t side = 0; side < 2; ++side) {
                            const uint32_t base = static_cast<uint32_t>(out.vertices.size());
                            for (int32_t corner = 0; corner < 4; ++corner) {
                                /* 反面按逆序绕行, 保证两面都能通过背面剔除. */
                                const int32_t source = side == 0 ? corner : (corner == 0 ? 0 : 4 - corner);
                                Vertex vertex;
                                vertex.px = corners[plane][source][0];
                                vertex.py = corners[plane][source][1];
                                vertex.pz = corners[plane][source][2];
                                vertex.u = tileU + ((source == 1 || source == 2) ? kTileExtent : 0.0f);
                                vertex.v = tileV + (source >= 2 ? kTileExtent : 0.0f);
                                vertex.shade = shade;
                                if (tintPlant) {
                                    vertex.red = tintR;
                                    vertex.green = tintG;
                                    vertex.blue = tintB;
                                }
                                if (plantBlock > 0) {
                                    const float warm = static_cast<float>(plantBlock) /
                                                       static_cast<float>(kMaxLightLevel);
                                    vertex.red *= 1.0f + 0.18f * warm;
                                    vertex.green *= 1.0f + 0.04f * warm;
                                    vertex.blue *= 1.0f - 0.25f * warm;
                                }
                                out.vertices.push_back(vertex);
                            }
                            out.indices.push_back(base);
                            out.indices.push_back(base + 1);
                            out.indices.push_back(base + 2);
                            out.indices.push_back(base);
                            out.indices.push_back(base + 2);
                            out.indices.push_back(base + 3);
                        }
                    }
                    continue;
                }

                /*
                 * 群系染色: 草方块只染顶面, 树叶整块染, 其余方块保持白色.
                 * 判定按方块名做一次, 面循环里只比较两个布尔.
                 */
                const char* blockName = registry.Info(id).name;
                const bool tintLeaves =
                    blockName != nullptr && std::strstr(blockName, "leaves") != nullptr;
                const bool tintGrassTop =
                    blockName != nullptr && std::strstr(blockName, "grass") != nullptr;

                /*
                 * 液体表面: 上方是空气时把这一格整体压到八成七高,
                 * 水面因此低于方块顶沿, 看起来是液面而不是一整块实心方块.
                 */
                float liquidScale = 1.0f;
                if (isLiquid(id) && y + 1 < kWorldHeight &&
                    blocks[index + kLayerStride] == block::Air) {
                    liquidScale = 0.875f;
                }

                for (int32_t face = 0; face < kFaceCount; ++face) {
                    if (!needFace[face]) {
                        continue;
                    }
                    const FaceLayout& layout = kFaceLayouts[face];
                    /* 面外侧邻格的光照系数与朝向底色相乘, 写入 shade 通道. */
                    const float lightFactor = lightAt(x, y, z, face);
                    /* 朝向决定用哪一格: +Y 顶面, -Y 底面, 其余四个侧面. */
                    /* 格位是 16 位: 全量图集 64x64 共 4096 格, 8 位会在 256 格处截断. */
                    const uint16_t tile = (face == 2) ? tiles.top : ((face == 3) ? tiles.bottom : tiles.side);
                    const float tileU =
                        static_cast<float>(static_cast<int32_t>(tile) % kTileGridSize) * kTileExtent;
                    const float tileV =
                        static_cast<float>(static_cast<int32_t>(tile) / kTileGridSize) * kTileExtent;
                    const uint32_t baseVertex = static_cast<uint32_t>(out.vertices.size());
                    for (int32_t corner = 0; corner < 4; ++corner) {
                        Vertex vertex;
                        vertex.px = originX + layout.cornerX[corner];
                        vertex.py = originY + layout.cornerY[corner] * liquidScale;
                        vertex.pz = originZ + layout.cornerZ[corner];
                        vertex.u = tileU + ((corner == 1 || corner == 2) ? kTileExtent : 0.0f);
                        vertex.v = tileV + (corner >= 2 ? kTileExtent : 0.0f);
                        vertex.shade = layout.shade * lightFactor;
                        /* 草方块顶面与树叶按群系草色染色, 其余顶点保持白色. */
                        if (tintLeaves || (tintGrassTop && face == 2)) {
                            vertex.red = tintR;
                            vertex.green = tintG;
                            vertex.blue = tintB;
                        }
                        /*
                         * 方块光暖色调制: 火把一类光源附近的方块往暖色偏,
                         * 越亮越明显, 上限约三成; 天光照明的露天方块不受影响.
                         */
                        uint8_t skyLevel = kMaxLightLevel;
                        uint8_t blockLevel = 0;
                        sampleLight(x, y, z, face, &skyLevel, &blockLevel);
                        if (blockLevel > 0) {
                            const float warm = static_cast<float>(blockLevel) /
                                               static_cast<float>(kMaxLightLevel);
                            vertex.red *= 1.0f + 0.18f * warm;
                            vertex.green *= 1.0f + 0.04f * warm;
                            vertex.blue *= 1.0f - 0.25f * warm;
                        }
                        out.vertices.push_back(vertex);
                    }
                    std::vector<uint32_t>& target = isLiquid(id) ? liquidIndices : out.indices;
                    target.push_back(baseVertex);
                    target.push_back(baseVertex + 1);
                    target.push_back(baseVertex + 2);
                    target.push_back(baseVertex);
                    target.push_back(baseVertex + 2);
                    target.push_back(baseVertex + 3);
                }
            }
        }
    }

    /* 半透明段接在实心面之后: 起点与长度记在网格上, 顶点仍是同一份. */
    out.waterIndexBegin = out.indices.size();
    out.waterIndexCount = liquidIndices.size();
    out.indices.insert(out.indices.end(), liquidIndices.begin(), liquidIndices.end());
}

}  /* namespace immirror::voxel */