#include "ui/layout_render.h"

#include <cstring>
#include <string>

#include "ui/generated/ui_atlas_gen.h"
#include "ui/generated/ui_sprite_index_gen.h"

namespace immirror::ui {
namespace {

/* 取贴图路径的文件名主干: 去掉目录与扩展名. */
std::string StemOf(const char* path) {
    if (path == nullptr) {
        return std::string();
    }
    const char* slash = std::strrchr(path, '/');
    const char* start = slash == nullptr ? path : slash + 1;
    const char* dot = std::strrchr(start, '.');
    const size_t length = dot == nullptr ? std::strlen(start) : static_cast<size_t>(dot - start);
    return std::string(start, length);
}

}  /* namespace */

bool UiSpriteRectUv(const char* spriteName, float* u0, float* v0, float* u1, float* v1) {
    if (spriteName == nullptr || spriteName[0] == '\0') {
        return false;
    }
    /*
     * 子图集索引按名字排序, 用二分查找.
     * 名字带扩展名时去掉再比对, 索引里存的是不带扩展名的精灵名.
     */
    std::string key(spriteName);
    if (key.size() > 4 && key.compare(key.size() - 4, 4, ".png") == 0) {
        key.resize(key.size() - 4);
    }
    int32_t low = 0;
    int32_t high = gen::kUiSpriteRectCount - 1;
    while (low <= high) {
        const int32_t middle = low + (high - low) / 2;
        const int32_t order = key.compare(gen::kUiSpriteRects[middle].name);
        if (order == 0) {
            const gen::UiSpriteRect& rect = gen::kUiSpriteRects[middle];
            if (u0 != nullptr) {
                *u0 = rect.u0;
            }
            if (v0 != nullptr) {
                *v0 = rect.v0;
            }
            if (u1 != nullptr) {
                *u1 = rect.u1;
            }
            if (v1 != nullptr) {
                *v1 = rect.v1;
            }
            return true;
        }
        if (order < 0) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
    }
    return false;
}

bool UiSpriteUv(const char* texturePath, float* u0, float* v0, float* u1, float* v1) {
    const std::string stem = StemOf(texturePath);
    if (stem.empty()) {
        return false;
    }
    for (const gen::UiSpriteInfo& sprite : gen::kUiSprites) {
        if (sprite.name == nullptr || stem != sprite.name) {
            continue;
        }
        if (u0 != nullptr) {
            *u0 = sprite.u0;
        }
        if (v0 != nullptr) {
            *v0 = sprite.v0;
        }
        if (u1 != nullptr) {
            *u1 = sprite.u1;
        }
        if (v1 != nullptr) {
            *v1 = sprite.v1;
        }
        return true;
    }
    return false;
}

int32_t EmitLayoutCommands(const std::vector<LayoutDrawCommand>& commands, float screenWidth,
                           float screenHeight, float* out, int32_t capacityVertices,
                           LayoutRenderStats* stats) {
    if (out == nullptr || capacityVertices <= 0 || !(screenWidth > 0.0f) || !(screenHeight > 0.0f)) {
        return 0;
    }
    int32_t offset = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (offset + 6 > capacityVertices) {
            if (stats != nullptr) {
                stats->dropped += 1;
            }
            continue;
        }
        /* 像素矩形转裁剪空间: 左上角 (-1, 1), 右下角 (1, -1). */
        const float left = command.rect.x / screenWidth * 2.0f - 1.0f;
        const float right = (command.rect.x + command.rect.width) / screenWidth * 2.0f - 1.0f;
        const float top = 1.0f - command.rect.y / screenHeight * 2.0f;
        const float bottom =
            1.0f - (command.rect.y + command.rect.height) / screenHeight * 2.0f;
        const float positions[4][2] = {{left, bottom}, {right, bottom}, {right, top}, {left, top}};
        /*
         * 贴图通道: 素材名能在界面图集里查到就写真实 uv, 查不到保持纯色约定.
         * uv 顺序与四角对应: 左下, 右下, 右上, 左上.
         */
        float u0 = kLayoutSolidUv;
        float v0 = kLayoutSolidUv;
        float u1 = kLayoutSolidUv;
        float v1 = kLayoutSolidUv;
        bool textured = false;
        if (command.texture != nullptr) {
            /* 带精灵名时按名字取子图集矩形, 否则按整图名去查. */
            textured = command.sprite != nullptr && command.sprite[0] != '\0'
                           ? UiSpriteRectUv(command.sprite, &u0, &v0, &u1, &v1)
                           : UiSpriteUv(command.texture, &u0, &v0, &u1, &v1);
        }
        /*
         * uv 朝向与 HUD 的四边形一致: 左下角取 (u0, v0), 右上角取 (u1, v1).
         * 提交前调用方还要给 u 加界面图集的通道偏移, 与 PushUiQuad 同一处理.
         */
        const float uvs[4][2] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
        const int32_t order[6] = {0, 1, 2, 0, 2, 3};
        for (int32_t index = 0; index < 6; ++index) {
            float* vertex = out + static_cast<size_t>(offset) * kLayoutFloatsPerVertex;
            vertex[0] = positions[order[index]][0];
            vertex[1] = positions[order[index]][1];
            vertex[2] = uvs[order[index]][0];
            vertex[3] = uvs[order[index]][1];
            vertex[4] = command.red;
            vertex[5] = command.green;
            vertex[6] = command.blue;
            vertex[7] = command.alpha;
            offset += 1;
        }
        if (stats != nullptr) {
            stats->quads += 1;
            if (textured) {
                stats->textured += 1;
            }
        }
    }
    if (stats != nullptr) {
        stats->vertices = offset;
    }
    return offset;
}

}  /* namespace immirror::ui */
