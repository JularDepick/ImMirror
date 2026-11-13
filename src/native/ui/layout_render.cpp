#include "ui/layout_render.h"

namespace immirror::ui {

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
        const int32_t order[6] = {0, 1, 2, 0, 2, 3};
        for (int32_t index = 0; index < 6; ++index) {
            float* vertex = out + static_cast<size_t>(offset) * kLayoutFloatsPerVertex;
            vertex[0] = positions[order[index]][0];
            vertex[1] = positions[order[index]][1];
            vertex[2] = kLayoutSolidUv;
            vertex[3] = kLayoutSolidUv;
            vertex[4] = command.red;
            vertex[5] = command.green;
            vertex[6] = command.blue;
            vertex[7] = command.alpha;
            offset += 1;
        }
        if (stats != nullptr) {
            stats->quads += 1;
            if (command.texture != nullptr) {
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
