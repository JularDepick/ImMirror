/* 注释帧断言: 布局里被注释掉的帧不应进入控件树. */
#include <cstdio>
#include <string>

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
        "<Ui><Frame name=\"Root\">"
        "  <Size><AbsDimension x=\"400\" y=\"300\"/></Size>"
        "  <Frames>"
        "    <Frame name=\"Kept\"><Size><AbsDimension x=\"50\" y=\"50\"/></Size></Frame>"
        "    <!--Frame name=\"Dropped\"><Size><AbsDimension x=\"50\" y=\"50\"/></Size></Frame-->"
        "    <Frame name=\"AlsoKept\"><Size><AbsDimension x=\"60\" y=\"60\"/></Size></Frame>"
        "  </Frames>"
        "</Frame></Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    check("保留第一个帧", document.Find("Kept") != nullptr);
    check("保留第三个帧", document.Find("AlsoKept") != nullptr);
    check("注释里的帧被忽略", document.Find("Dropped") == nullptr);
    return failures == 0 ? 0 : 1;
}
