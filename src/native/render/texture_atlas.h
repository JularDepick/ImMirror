#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <android/asset_manager.h>

namespace immirror::render {

/*
 * 方块纹理图集. 图集由资源管线在构建期预生成, 安装包里是裸 RGBA8 数据:
 * 1024x1024 像素, 16x16 格, 每格 64x64 像素, 无文件头, 行优先.
 * 客户端只做读取与上传, 不在运行期生成任何图形.
 */
class TextureAtlas {
public:
    static constexpr int32_t kGridSize = 16;
    static constexpr int32_t kCellPixels = 64;
    static constexpr int32_t kTexturePixels = kGridSize * kCellPixels;
    static constexpr int32_t kCellCount = kGridSize * kGridSize;
    static constexpr size_t kByteSize =
        static_cast<size_t>(kTexturePixels) * static_cast<size_t>(kTexturePixels) * 4u;

    /* 相对 assets 根目录的资产路径. */
    static constexpr const char* kAssetPath = "generated/atlas.rgba";

    /*
     * 从安装包资产读取整张图集.
     * 读取成功后会按格做垂直翻转, 原因见实现内的说明; 失败时返回 false 并清空数据.
     */
    bool LoadFromAssets(AAssetManager* assets);

    /* RGBA8 像素首地址; 未加载时为空指针. */
    const uint8_t* Pixels() const { return pixels_.empty() ? nullptr : pixels_.data(); }

    /* 像素字节数, 上传前用于自检. */
    size_t ByteSize() const { return pixels_.size(); }

    /*
     * 取某一格在图集中的归一化 uv 范围, 依次写入 u0 v0 u1 v1.
     * 网格构建侧若采用别的排布, 以本函数为基准对齐即可.
     */
    static void CellUvRect(uint8_t cellIndex, float* outUv);

private:
    /* 逐格垂直翻转, 见实现文件中的说明. */
    void FlipCellsVertically();

    std::vector<uint8_t> pixels_;
};

/*
 * 图集格位. 与资源管线的 atlas.json 以及 voxel 侧方块注册表的图集格保持一致.
 */
enum AtlasCellIndex : uint8_t {
    kAtlasCellGrassTop = 0,
    kAtlasCellGrassSide = 1,
    kAtlasCellDirt = 2,
    kAtlasCellStone = 3,
    kAtlasCellSand = 4,
    kAtlasCellWater = 5,
    kAtlasCellLogSide = 6,
    kAtlasCellLogTop = 7,
    kAtlasCellLeaves = 8,
    kAtlasCellBedrock = 9,
    kAtlasCellBrick = 10,
    kAtlasCellCobblestone = 11,
    kAtlasCellSnow = 12,
    kAtlasCellIce = 13,
    kAtlasCellGravel = 14,
    kAtlasCellPlanks = 15,
};

}  /* namespace immirror::render */
