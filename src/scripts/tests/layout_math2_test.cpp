/* 布局数学验证: 双锚点拉伸、相对尺寸、九宫边角块尺寸. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_frame.h"
#include "ui/layout_render.h"
#include "ui/layout_xml.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static bool near(float a, float b) { return a > b - 0.5f && a < b + 0.5f; }

int main() {
    /* 一 双锚点拉伸: 左右各贴父帧左右并内缩 20, 宽度应为 800-40=760. */
    {
        const std::string layout =
            "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"800\" y=\"600\"/></Size>"
            "<Frames><Frame name=\"Stretch\">"
            "  <Anchors>"
            "    <Anchor point=\"left\" relativePoint=\"left\" relativeTo=\"$parent\">"
            "      <Offset><AbsDimension x=\"20\" y=\"0\"/></Offset></Anchor>"
            "    <Anchor point=\"right\" relativePoint=\"right\" relativeTo=\"$parent\">"
            "      <Offset><AbsDimension x=\"-20\" y=\"0\"/></Offset></Anchor>"
            "    <Anchor point=\"top\" relativePoint=\"top\" relativeTo=\"$parent\">"
            "      <Offset><AbsDimension x=\"0\" y=\"10\"/></Offset></Anchor>"
            "    <Anchor point=\"bottom\" relativePoint=\"bottom\" relativeTo=\"$parent\">"
            "      <Offset><AbsDimension x=\"0\" y=\"-10\"/></Offset></Anchor>"
            "  </Anchors>"
            "</Frame></Frames></Frame></Ui>";
        LayoutDocument document;
        check("双锚点布局解析", document.Parse(layout.data(), layout.size()));
        ResolveLayout(*document.Root(), LayoutRect{0.0f, 0.0f, 800.0f, 600.0f});
        LayoutFrame* frame = document.Find("Stretch");
        check("找到拉伸帧", frame != nullptr);
        if (frame != nullptr) {
            std::printf("  双锚点: x=%.0f y=%.0f %.0fx%.0f\n", frame->rect.x, frame->rect.y,
                        frame->rect.width, frame->rect.height);
            check("拉伸宽度符合公式", near(frame->rect.width, 760.0f));
            check("拉伸高度符合公式", near(frame->rect.height, 580.0f));
            check("拉伸左边界符合公式", near(frame->rect.x, 20.0f));
            check("拉伸上边界符合公式", near(frame->rect.y, 10.0f));
        }
    }

    /* 二 相对尺寸: 父帧 400x300, RelDimension 0.5/0.25 -> 200x75. */
    {
        const std::string layout =
            "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"400\" y=\"300\"/></Size>"
            "<Frames><Frame name=\"Half\">"
            "  <Size><RelDimension x=\"0.5\" y=\"0.25\"/></Size>"
            "</Frame></Frames></Frame></Ui>";
        LayoutDocument document;
        check("相对尺寸布局解析", document.Parse(layout.data(), layout.size()));
        ResolveLayout(*document.Root(), LayoutRect{0.0f, 0.0f, 400.0f, 300.0f});
        LayoutFrame* frame = document.Find("Half");
        check("找到相对尺寸帧", frame != nullptr);
        if (frame != nullptr) {
            std::printf("  相对尺寸: %.0fx%.0f\n", frame->rect.width, frame->rect.height);
            check("相对宽符合公式", near(frame->rect.width, 200.0f));
            check("相对高符合公式", near(frame->rect.height, 75.0f));
        }
    }

    /* 三 九宫边角块: 内边距 10, 边角块的像素尺寸应等于 10. */
    {
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, 100.0f, 100.0f};
        command.texture = "ui/mobile/texture2/common.xml";
        command.sprite = "img_icon_lignt.png";
        command.insetLeft = 10.0f;
        command.insetRight = 10.0f;
        command.insetTop = 10.0f;
        command.insetBottom = 10.0f;
        std::vector<LayoutDrawCommand> commands;
        commands.push_back(command);
        std::vector<float> vertices(static_cast<size_t>(512) * kLayoutFloatsPerVertex, 0.0f);
        LayoutRenderStats stats;
        EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(), 512, &stats);
        std::printf("  九宫块数 %d\n", stats.quads);
        check("切成九块", stats.quads == 9);
        /* 收集所有顶点的像素 x, 左上角块应覆盖 [0,10]. */
        bool hasTopLeftBlock = false;
        for (int32_t index = 0; index < stats.vertices; ++index) {
            const float* vertex = vertices.data() + static_cast<size_t>(index) * kLayoutFloatsPerVertex;
            const float pixelX = (vertex[0] + 1.0f) * 0.5f * 1280.0f;
            const float pixelY = (1.0f - vertex[1]) * 0.5f * 720.0f;
            if (near(pixelX, 0.0f) && near(pixelY, 0.0f)) {
                hasTopLeftBlock = true;
            }
        }
        check("左上角块存在", hasTopLeftBlock);

        /*
         * 边角块的像素尺寸必须等于内边距: 收集全部顶点的像素范围,
         * 检查是否存在 x 落在 [0,10] 与 [90,100] 这两个区间的顶点.
         */
        bool hasLeftEdge = false;
        bool hasRightEdge = false;
        bool hasCenterLeft = false;
        for (int32_t index = 0; index < stats.vertices; ++index) {
            const float* vertex = vertices.data() + static_cast<size_t>(index) * kLayoutFloatsPerVertex;
            const float pixelX = (vertex[0] + 1.0f) * 0.5f * 1280.0f;
            if (near(pixelX, 10.0f)) {
                hasCenterLeft = true;
            }
            if (near(pixelX, 90.0f)) {
                hasRightEdge = true;
            }
            if (near(pixelX, 0.0f)) {
                hasLeftEdge = true;
            }
        }
        check("左边角块从 0 开始", hasLeftEdge);
        check("中间块从内边距 10 开始", hasCenterLeft);
        check("中间块到 90 结束", hasRightEdge);
    }
    return failures == 0 ? 0 : 1;
}
