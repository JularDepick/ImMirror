/* 流式布局断言: LayoutBox 按样式把子元素依次排开. */
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

int main() {
    const std::string layout =
        "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"400\" y=\"200\"/></Size>"
        "  <Frames><LayoutBox name=\"Box\" layoutstyle=\"horizontal_left\" spacex=\"10\">"
        "    <Size><AbsDimension x=\"400\" y=\"60\"/></Size>"
        "    <Frames>"
        "      <Frame name=\"A\"><Size><AbsDimension x=\"50\" y=\"40\"/></Size></Frame>"
        "      <Frame name=\"B\"><Size><AbsDimension x=\"50\" y=\"40\"/></Size></Frame>"
        "      <Frame name=\"C\"><Size><AbsDimension x=\"50\" y=\"40\"/></Size></Frame>"
        "    </Frames>"
        "  </LayoutBox></Frames>"
        "</Frame></Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* box = document.Find("Box");
    check("找到流式容器", box != nullptr);
    check("容器带样式", box != nullptr && box->layoutStyle != nullptr);
    LayoutFrame* a = document.Find("A");
    LayoutFrame* b = document.Find("B");
    LayoutFrame* c = document.Find("C");
    check("找到三个子元素", a != nullptr && b != nullptr && c != nullptr);
    if (a == nullptr || b == nullptr || c == nullptr || box == nullptr) {
        return 1;
    }
    std::printf("  A=%.0f B=%.0f C=%.0f (间距 %.0f)\n", a->rect.x, b->rect.x, c->rect.x,
                box->spaceX);
    check("子元素水平排开", a->rect.x < b->rect.x && b->rect.x < c->rect.x);
    check("间距等于 spacex", b->rect.x - a->rect.x == 60.0f);
    check("子元素同高", a->rect.y == b->rect.y && b->rect.y == c->rect.y);
    return failures == 0 ? 0 : 1;
}
