/* 翻转与描边的数学验证. */
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

/* 收集 UV 与像素 x 的对应关系, 用于判断翻转. */
struct Sample {
    float pixelX;
    float u;
};

static void collect(const std::vector<float>& vertices, int32_t count, std::vector<Sample>* out) {
    for (int32_t index = 0; index < count; ++index) {
        const float* vertex = vertices.data() + static_cast<size_t>(index) * kLayoutFloatsPerVertex;
        out->push_back(Sample{(vertex[0] + 1.0f) * 0.5f * 1280.0f, vertex[2]});
    }
}

int main() {
    /* 一 水平翻转: 左侧顶点的 u 应大于右侧顶点. */
    {
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, 100.0f, 100.0f};
        command.texture = "ui/mobile/texture2/common.xml";
        command.sprite = "img_icon_lignt.png";

        std::vector<LayoutDrawCommand> normal;
        normal.push_back(command);
        std::vector<float> normalVertices(static_cast<size_t>(64) * kLayoutFloatsPerVertex, 0.0f);
        LayoutRenderStats normalStats;
        EmitLayoutCommands(normal, 1280.0f, 720.0f, normalVertices.data(), 64, &normalStats);

        LayoutDrawCommand flipped = command;
        flipped.flipU = true;
        std::vector<LayoutDrawCommand> flip;
        flip.push_back(flipped);
        std::vector<float> flipVertices(static_cast<size_t>(64) * kLayoutFloatsPerVertex, 0.0f);
        LayoutRenderStats flipStats;
        EmitLayoutCommands(flip, 1280.0f, 720.0f, flipVertices.data(), 64, &flipStats);

        check("翻转前后块数相同", normalStats.quads == flipStats.quads);

        std::vector<Sample> a;
        std::vector<Sample> b;
        collect(normalVertices, normalStats.vertices, &a);
        collect(flipVertices, flipStats.vertices, &b);
        /* 找最左与最右顶点的 u. */
        float aLeftU = 1.0f;
        float aRightU = 0.0f;
        float bLeftU = 1.0f;
        float bRightU = 0.0f;
        for (const Sample& item : a) {
            if (item.pixelX < 1.0f && item.u < aLeftU) {
                aLeftU = item.u;
            }
            if (item.pixelX > 99.0f && item.u > aRightU) {
                aRightU = item.u;
            }
        }
        for (const Sample& item : b) {
            if (item.pixelX < 1.0f && item.u < bLeftU) {
                bLeftU = item.u;
            }
            if (item.pixelX > 99.0f && item.u > bRightU) {
                bRightU = item.u;
            }
        }
        std::printf("  正常: 左 u=%.4f 右 u=%.4f\n", aLeftU, aRightU);
        std::printf("  翻转: 左 u=%.4f 右 u=%.4f\n", bLeftU, bRightU);
        check("正常时左小右大", aLeftU < aRightU);
        check("翻转后左大右小", bLeftU > bRightU);
    }

    /* 二 描边: 带描边的文字应有五遍绘制(四方向偏移 + 主体). */
    {
        LayoutDrawCommand plain;
        plain.rect = LayoutRect{100.0f, 100.0f, 200.0f, 20.0f};
        plain.text = "AB";
        plain.fontSize = 14.0f;

        std::vector<LayoutDrawCommand> one;
        one.push_back(plain);
        std::vector<float> oneVertices(static_cast<size_t>(512) * kLayoutFloatsPerVertex, 0.0f);
        LayoutRenderStats oneStats;
        EmitLayoutCommands(one, 1280.0f, 720.0f, oneVertices.data(), 512, &oneStats);

        LayoutDrawCommand outlined = plain;
        outlined.outline = true;
        std::vector<LayoutDrawCommand> two;
        two.push_back(outlined);
        std::vector<float> twoVertices(static_cast<size_t>(1024) * kLayoutFloatsPerVertex, 0.0f);
        LayoutRenderStats twoStats;
        EmitLayoutCommands(two, 1280.0f, 720.0f, twoVertices.data(), 1024, &twoStats);

        std::printf("  无描边块数 %d, 有描边块数 %d, 丢弃 %d\n", oneStats.quads, twoStats.quads,
                    twoStats.dropped);
        /*
         * 描边是四方向偏移加主体共五遍, 但偏移后的块可能被容器边界裁掉,
         * 因此只要求"至少四倍", 并确认丢弃数与差额吻合.
         */
        check("描边至少放大四倍", twoStats.quads >= oneStats.quads * 4);
        check("描边块数不超过五倍", twoStats.quads <= oneStats.quads * 5);
        check("差额与丢弃数吻合", twoStats.quads + twoStats.dropped >= oneStats.quads * 5);
    }
    return failures == 0 ? 0 : 1;
}
