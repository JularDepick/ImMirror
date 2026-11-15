/* 平铺与文字对齐的数学验证: 给定参数, 验证块数与对齐坐标算得对不对. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/hud_font.h"
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

/* 跑一次渲染, 返回顶点数组与统计. */
static void render(const std::vector<LayoutDrawCommand>& commands, std::vector<float>* vertices,
                   LayoutRenderStats* stats) {
    vertices->assign(static_cast<size_t>(4096) * kLayoutFloatsPerVertex, 0.0f);
    EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices->data(), 4096, stats);
}

int main() {
    /* 一 平铺: 区域 100x100, 每块 30x30, 应铺成 4x4 = 16 块. */
    {
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, 100.0f, 100.0f};
        command.texture = "ui/mobile/texture2/common.xml";
        command.sprite = "img_icon_lignt.png";
        command.tileWidth = 30.0f;
        command.tileHeight = 30.0f;
        std::vector<LayoutDrawCommand> commands;
        commands.push_back(command);
        std::vector<float> vertices;
        LayoutRenderStats stats;
        render(commands, &vertices, &stats);
        std::printf("  平铺块数 %d\n", stats.quads);
        check("平铺切成 16 块", stats.quads == 16);

        /* 末块应被截断: 所有顶点的 x 不应超过区域右边界 100. */
        float maxX = 0.0f;
        float maxY = 0.0f;
        for (int32_t index = 0; index < stats.vertices; ++index) {
            const float* vertex = vertices.data() + static_cast<size_t>(index) * kLayoutFloatsPerVertex;
            /* 顶点坐标是裁剪空间, 换算回像素: x = (ndc + 1) / 2 * 1280. */
            const float pixelX = (vertex[0] + 1.0f) * 0.5f * 1280.0f;
            const float pixelY = (1.0f - vertex[1]) * 0.5f * 720.0f;
            if (pixelX > maxX) {
                maxX = pixelX;
            }
            if (pixelY > maxY) {
                maxY = pixelY;
            }
        }
        std::printf("  平铺覆盖到 %.1f x %.1f\n", maxX, maxY);
        check("平铺不超出区域右边界", maxX <= 100.5f);
        check("平铺不超出区域下边界", maxY <= 100.5f);
        check("平铺填满区域", maxX >= 99.5f && maxY >= 99.5f);
    }

    /* 二 文字对齐: 帧 200 宽, 三种对齐下的起始 x 应不同且符合公式. */
    {
        const char* text = "ABCD";
        const float pixelSize = 2.0f;
        const float textWidth = MeasureText(text, pixelSize);
        std::printf("  文字宽度 %.1f\n", textWidth);
        check("文字宽度为正", textWidth > 0.0f);

        LayoutDrawCommand left;
        left.rect = LayoutRect{100.0f, 50.0f, 200.0f, 20.0f};
        left.text = text;
        left.fontSize = 14.0f;
        left.justifyH = 0;
        float x = 0.0f;
        float y = 0.0f;
        LayoutTextOrigin(left, pixelSize, &x, &y, nullptr, nullptr);
        std::printf("  左对齐 x=%.1f (期望 100.0)\n", x);
        check("左对齐贴左边", x == 100.0f);

        LayoutDrawCommand center = left;
        center.justifyH = 1;
        LayoutTextOrigin(center, pixelSize, &x, &y, nullptr, nullptr);
        const float expectCenter = 100.0f + (200.0f - textWidth) * 0.5f;
        std::printf("  居中 x=%.1f (期望 %.1f)\n", x, expectCenter);
        check("居中对齐符合公式", x > expectCenter - 0.5f && x < expectCenter + 0.5f);

        LayoutDrawCommand right = left;
        right.justifyH = 2;
        LayoutTextOrigin(right, pixelSize, &x, &y, nullptr, nullptr);
        const float expectRight = 100.0f + 200.0f - textWidth;
        std::printf("  右对齐 x=%.1f (期望 %.1f)\n", x, expectRight);
        check("右对齐符合公式", x > expectRight - 0.5f && x < expectRight + 0.5f);
    }
    return failures == 0 ? 0 : 1;
}
