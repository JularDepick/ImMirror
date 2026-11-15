/* 锚点数学验证: 九种对齐点在已知参照物下的坐标必须符合定义. */
#include <cstdio>
#include <string>

#include "ui/layout_frame.h"
#include "ui/layout_xml.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

/* 参照物: 100,200 起, 300x400. 各点坐标由定义直接算出. */
struct Case {
    const char* point;
    float expectX;
    float expectY;
};

int main() {
    /*
     * 语义: 自身 point 对齐到参照物的 relativePoint(这里固定 topleft = 100,200).
     * 因此自身位置 = 参照物点 - 自身尺寸在对应方向上的比例:
     * 水平方向 左=0 / 中=宽一半 / 右=整宽; 垂直方向同理. 目标帧 20x10.
     */
    const Case cases[] = {
        {"topleft", 100.0f, 200.0f},
        {"top", 90.0f, 200.0f},
        {"topright", 80.0f, 200.0f},
        {"left", 100.0f, 195.0f},
        {"center", 90.0f, 195.0f},
        {"right", 80.0f, 195.0f},
        {"bottomleft", 100.0f, 190.0f},
        {"bottom", 90.0f, 190.0f},
        {"bottomright", 80.0f, 190.0f},
    };
    for (const Case& item : cases) {
        /* 目标帧尺寸 20x10, 锚点自身 point 对参照物 topleft. */
        const std::string layout =
            std::string("<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"800\" y=\"800\"/></Size>") +
            "<Frames><Frame name=\"Ref\">" +
            "  <Size><AbsDimension x=\"300\" y=\"400\"/></Size>" +
            "  <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">" +
            "    <Offset><AbsDimension x=\"100\" y=\"200\"/></Offset></Anchor></Anchors>" +
            "</Frame>" +
            "<Frame name=\"Probe\">" +
            "  <Size><AbsDimension x=\"20\" y=\"10\"/></Size>" +
            "  <Anchors><Anchor point=\"" + item.point +
            "\" relativePoint=\"topleft\" relativeTo=\"Ref\">" +
            "    <Offset><AbsDimension x=\"0\" y=\"0\"/></Offset></Anchor></Anchors>" +
            "</Frame></Frames></Frame></Ui>";
        LayoutDocument document;
        if (!document.Parse(layout.data(), layout.size())) {
            std::printf("FAIL %s 解析失败\n", item.point);
            failures += 1;
            continue;
        }
        ResolveLayout(*document.Root(), LayoutRect{0.0f, 0.0f, 800.0f, 800.0f});
        LayoutFrame* probe = document.Find("Probe");
        if (probe == nullptr) {
            std::printf("FAIL %s 未找到探针帧\n", item.point);
            failures += 1;
            continue;
        }
        const bool ok = probe->rect.x > item.expectX - 0.5f && probe->rect.x < item.expectX + 0.5f &&
                        probe->rect.y > item.expectY - 0.5f && probe->rect.y < item.expectY + 0.5f;
        std::printf(ok ? "ok   %-12s 位置 (%.0f, %.0f) 期望 (%.0f, %.0f)\n"
                        : "FAIL %-12s 位置 (%.0f, %.0f) 期望 (%.0f, %.0f)\n",
                    item.point, probe->rect.x, probe->rect.y, item.expectX, item.expectY);
        if (!ok) {
            failures += 1;
        }
    }
    return failures == 0 ? 0 : 1;
}
