/* 绘制路径同步断言: 布局树与控件树两条路径产出的命令参数必须一致. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_frame.h"
#include "ui/layout_xml.h"
#include "ui/widget_tree.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

/* 按名字找命令, 找不到返回空指针. */
static const LayoutDrawCommand* pick(const std::vector<LayoutDrawCommand>& commands,
                                    const char* name) {
    for (const LayoutDrawCommand& command : commands) {
        if (command.name != nullptr && std::string(command.name) == name) {
            return &command;
        }
    }
    return nullptr;
}

int main() {
    const std::string layout =
        "<Ui><Frame name=\"Root\">"
        "  <Size><AbsDimension x=\"1280\" y=\"720\"/></Size>"
        "  <Layers><Layer level=\"artwork\">"
        "    <Texture name=\"Tiled\" file=\"ui/white.png\">"
        "      <Size><AbsDimension x=\"200\" y=\"200\"/></Size>"
        "      <NineSquare ox=\"0\" oy=\"0\" tilew=\"20\" tileh=\"20\"/>"
        "    </Texture>"
    "    <FontString name=\"Bordered\" fontStyle=\"border\" multiLine=\"true\" text=\"ABC\">"
        "      <Size><AbsDimension x=\"80\" y=\"40\"/></Size>"
        "    </FontString>"
        "  </Layer></Layers>"
        "</Frame></Ui>";

    /* 路径一: 布局树. */
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});
    std::vector<LayoutDrawCommand> fromLayout;
    CollectDrawCommands(*root, fromLayout);

    /* 路径二: 控件树. */
    WidgetTree tree;
    check("控件树载入", tree.Load(layout.data(), layout.size()));
    tree.Resolve(LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});
    std::vector<LayoutDrawCommand> fromWidget;
    tree.CollectDrawCommands(fromWidget);

    std::printf("  布局树 %d 条, 控件树 %d 条\n", static_cast<int>(fromLayout.size()),
                static_cast<int>(fromWidget.size()));
    check("两条路径命令数一致", fromLayout.size() == fromWidget.size());

    const LayoutDrawCommand* tiledLayout = pick(fromLayout, "Tiled");
    const LayoutDrawCommand* tiledWidget = pick(fromWidget, "Tiled");
    check("布局树找到平铺元素", tiledLayout != nullptr);
    check("控件树找到平铺元素", tiledWidget != nullptr);
    if (tiledLayout != nullptr && tiledWidget != nullptr) {
        check("平铺尺寸一致", tiledLayout->tileWidth == tiledWidget->tileWidth &&
                              tiledLayout->tileHeight == tiledWidget->tileHeight);
        check("平铺尺寸非零", tiledWidget->tileWidth > 0.0f);
    }

    const LayoutDrawCommand* textLayout = pick(fromLayout, "Bordered");
    const LayoutDrawCommand* textWidget = pick(fromWidget, "Bordered");
    check("布局树找到描边文字", textLayout != nullptr);
    check("控件树找到描边文字", textWidget != nullptr);
    if (textLayout != nullptr && textWidget != nullptr) {
        check("描边标记一致", textLayout->outline == textWidget->outline);
        check("控件树带描边标记", textWidget->outline);
        check("多行标记一致", textLayout->multiLine == textWidget->multiLine);
        check("控件树带多行标记", textWidget->multiLine);
    }
    return failures == 0 ? 0 : 1;
}
