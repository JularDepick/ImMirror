#include "ui/layout_render.h"

#include <cstring>
#include <string>

#include "ui/generated/ui_atlas_gen.h"

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
            textured = UiSpriteUv(command.texture, &u0, &v0, &u1, &v1);
        }
        const float uvs[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
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
