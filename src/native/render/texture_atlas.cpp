#include "render/texture_atlas.h"

#include <cstring>

#include "core/log.h"

namespace immirror::render {
namespace {

constexpr const char* kTag = "ImMirrorTextureAtlas";

/* 单次资产读取的上限, 避免一次申请过大的连续读取. */
constexpr size_t kReadChunkBytes = 1u << 20;

}  /* namespace */

bool TextureAtlas::LoadFromAssets(AAssetManager* assets) {
    pixels_.clear();
    if (assets == nullptr) {
        IM_LOGE(kTag, "asset manager is null, atlas not loaded");
        return false;
    }

    AAsset* asset = AAssetManager_open(assets, kAssetPath, AASSET_MODE_STREAMING);
    if (asset == nullptr) {
        IM_LOGE(kTag, "asset '%s' missing", kAssetPath);
        return false;
    }

    const off64_t length = AAsset_getLength64(asset);
    if (length != static_cast<off64_t>(kByteSize)) {
        IM_LOGE(kTag, "asset '%s' size mismatch, got %lld expected %zu", kAssetPath,
                static_cast<long long>(length), kByteSize);
        AAsset_close(asset);
        return false;
    }

    pixels_.resize(kByteSize);
    size_t total = 0;
    while (total < kByteSize) {
        const size_t remaining = kByteSize - total;
        const size_t request = remaining > kReadChunkBytes ? kReadChunkBytes : remaining;
        const int32_t read = AAsset_read(asset, pixels_.data() + total, request);
        if (read <= 0) {
            break;
        }
        total += static_cast<size_t>(read);
    }
    AAsset_close(asset);

    if (total != kByteSize) {
        IM_LOGE(kTag, "asset '%s' read %zu of %zu bytes", kAssetPath, total, kByteSize);
        pixels_.clear();
        return false;
    }

    /*
     * 资源管线把图集按图片行序写入 (首行是图案上边缘), 而网格构建侧把侧面上
     * 更高的世界 y 映射到更大的 v, 也就是更大的数据行号. 若原样上传, 草地侧面
     * 的草边会出现在方块底部. 这里逐格翻转 64 行, 让图案在游戏里正立;
     * 翻转只发生在格内, 不改变格位索引.
     * 若以后资源管线改为直接输出自底向上的数据, 将本调用去掉即可.
     */
    FlipCellsVertically();

    IM_LOGI(kTag, "atlas loaded from '%s', %dx%d, %zu bytes", kAssetPath, kTexturePixels, kTexturePixels,
            pixels_.size());
    return true;
}

void TextureAtlas::FlipCellsVertically() {
    if (pixels_.size() != kByteSize) {
        return;
    }
    const size_t rowBytes = static_cast<size_t>(kTexturePixels) * 4u;
    std::vector<uint8_t> rowBuffer(rowBytes);
    for (int32_t cellRow = 0; cellRow < kGridSize; ++cellRow) {
        const int32_t firstRow = cellRow * kCellPixels;
        for (int32_t inner = 0; inner < kCellPixels / 2; ++inner) {
            uint8_t* top = pixels_.data() + static_cast<size_t>(firstRow + inner) * rowBytes;
            uint8_t* bottom =
                pixels_.data() + static_cast<size_t>(firstRow + kCellPixels - 1 - inner) * rowBytes;
            std::memcpy(rowBuffer.data(), top, rowBytes);
            std::memcpy(top, bottom, rowBytes);
            std::memcpy(bottom, rowBuffer.data(), rowBytes);
        }
    }
}

void TextureAtlas::CellUvRect(uint8_t cellIndex, float* outUv) {
    if (outUv == nullptr) {
        return;
    }
    const float cellSize = 1.0f / static_cast<float>(kGridSize);
    const int32_t column = static_cast<int32_t>(cellIndex) % kGridSize;
    const int32_t row = static_cast<int32_t>(cellIndex) / kGridSize;
    outUv[0] = static_cast<float>(column) * cellSize;
    outUv[1] = static_cast<float>(row) * cellSize;
    outUv[2] = outUv[0] + cellSize;
    outUv[3] = outUv[1] + cellSize;
}

}  /* namespace immirror::render */
