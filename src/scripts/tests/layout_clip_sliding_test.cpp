/* 滑动容器裁剪断言: 超出容器矩形的子元素不产出绘制命令. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
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

static int32_t countNamed(const std::vector<LayoutDrawCommand>& commands, const char* name) {
    int32_t total = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (command.name != nullptr && std::string(command.name) == name) {
            total += 1;
        }
    }
    return total;
}

int main() {
    const std::string layout =
        "<Ui><Frame name=\"Root\">"
        "  <Size><AbsDimension x=\"1280\" y=\"720\"/></Size>"
        "  <Frames>"
        "    <SlidingFrame name=\"Box\" slidingY=\"true\">"
        "      <Size><AbsDimension x=\"200\" y=\"150\"/></Size>"
        "      <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">"
        "        <Offset><AbsDimension x=\"100\" y=\"100\"/></Offset></Anchor></Anchors>"
        "      <Frames>"
        "        <Texture name=\"Inside\"><Size><AbsDimension x=\"40\" y=\"30\"/></Size>"
        "          <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">"
        "            <Offset><AbsDimension x=\"10\" y=\"10\"/></Offset></Anchor></Anchors>"
        "          <Color r=\"255\" g=\"0\" b=\"0\"/></Texture>"
        "        <Texture name=\"Outside\"><Size><AbsDimension x=\"40\" y=\"30\"/></Size>"
        "          <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">"
        "            <Offset><AbsDimension x=\"400\" y=\"300\"/></Offset></Anchor></Anchors>"
        "          <Color r=\"0\" g=\"255\" b=\"0\"/></Texture>"
        "      </Frames>"
        "    </SlidingFrame>"
        "  </Frames>"
        "</Frame></Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* box = document.Find("Box");
    check("找到滑动容器", box != nullptr);
    check("滑动容器带裁剪标记", box != nullptr && box->clipChildren);

    std::vector<LayoutDrawCommand> commands;
    CollectDrawCommands(*root, commands);
    const int32_t inside = countNamed(commands, "Inside");
    const int32_t outside = countNamed(commands, "Outside");
    std::printf("  容器内 %d 条, 容器外 %d 条\n", inside, outside);
    check("容器内元素被绘制", inside == 1);
    check("容器外元素被裁掉", outside == 0);
    return failures == 0 ? 0 : 1;
}
