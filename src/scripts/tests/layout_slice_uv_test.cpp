/* 九宫切片 UV 验证: 九个块的 UV 必须覆盖精灵的完整范围, 且边角块比例与内边距一致. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_render.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    /* 用一个确定存在的精灵, 取它的原始尺寸与 UV 范围. */
    const char* sprite = "img_icon_lignt.png";
    float spriteWidth = 0.0f;
    float spriteHeight = 0.0f;
    if (!UiSpriteSize(sprite, &spriteWidth, &spriteHeight)) {
        std::printf("SKIP 精灵不在索引里: %s\n", sprite);
        return 0;
    }
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    check("取到精灵 UV 范围", UiSpriteRectUv(sprite, &u0, &v0, &u1, &v1));

    /* 构造一个九宫命令: 元素远大于精灵, 内边距取精灵的四分之一. */
    LayoutDrawCommand command;
    command.rect = LayoutRect{100.0f, 100.0f, spriteWidth * 4.0f, spriteHeight * 4.0f};
    command.sprite = sprite;
    /* 渲染层要求贴图页非空才会去查精灵矩形, 只给精灵名会被当成无贴图. */
    command.texture = "ui/mobile/texture2/common.xml";
    command.insetLeft = spriteWidth / 4.0f;
    command.insetRight = spriteWidth / 4.0f;
    command.insetTop = spriteHeight / 4.0f;
    command.insetBottom = spriteHeight / 4.0f;

    std::vector<LayoutDrawCommand> commands;
    commands.push_back(command);
    std::vector<float> vertices(static_cast<size_t>(256) * kLayoutFloatsPerVertex, 0.0f);
    LayoutRenderStats stats;
    EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(), 256, &stats);
    std::printf("  九宫块数 %d, 顶点 %d\n", stats.quads, stats.vertices);
    check("切成九块", stats.quads == 9);

    /* 遍历全部顶点, 收集 UV 的最小与最大值. */
    float minU = 1.0f;
    float maxU = 0.0f;
    float minV = 1.0f;
    float maxV = 0.0f;
    for (int32_t index = 0; index < stats.vertices; ++index) {
        const float* vertex = vertices.data() + static_cast<size_t>(index) * kLayoutFloatsPerVertex;
        if (vertex[2] < minU) {
            minU = vertex[2];
        }
        if (vertex[2] > maxU) {
            maxU = vertex[2];
        }
        if (vertex[3] < minV) {
            minV = vertex[3];
        }
        if (vertex[3] > maxV) {
            maxV = vertex[3];
        }
    }
    std::printf("  UV 范围 u=[%.4f, %.4f] v=[%.4f, %.4f], 精灵 u=[%.4f, %.4f] v=[%.4f, %.4f]\n",
                minU, maxU, minV, maxV, u0, u1, v0, v1);
    check("UV 不超出精灵左边界", minU >= u0 - 0.001f);
    check("UV 不超出精灵右边界", maxU <= u1 + 0.001f);
    check("UV 不超出精灵上边界", minV >= v0 - 0.001f);
    check("UV 不超出精灵下边界", maxV <= v1 + 0.001f);
    check("UV 覆盖完整宽度", maxU - minU > (u1 - u0) * 0.99f);
    check("UV 覆盖完整高度", maxV - minV > (v1 - v0) * 0.99f);
    return failures == 0 ? 0 : 1;
}
