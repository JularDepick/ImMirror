/* 选中态断言: 按钮 Checked 后应改用 CheckedTexture. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/widget_tree.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

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
        "  <Size><AbsDimension x=\"400\" y=\"200\"/></Size>"
        "  <Frames>"
        "    <Button name=\"Tab\">"
        "      <Size><AbsDimension x=\"100\" y=\"40\"/></Size>"
        "      <NormalTexture file=\"ui/white.png\"/>"
        "      <PushedTexture file=\"ui/ui2.png\"/>"
        "      <CheckedTexture file=\"ui/login_bg.png\"/>"
        "    </Button>"
        "  </Frames>"
        "</Frame></Ui>";
    WidgetTree tree;
    check("载入布局", tree.Load(layout.data(), layout.size()));
    tree.Resolve(LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    Widget* tab = tree.Find("Tab");
    check("找到按钮", tab != nullptr);
    if (tab == nullptr) {
        return 1;
    }
    check("按钮带选中贴图", tab->frame->checkedTexture != nullptr);
    check("默认未选中", !tab->checked);

    std::vector<LayoutDrawCommand> plain;
    tree.CollectDrawCommands(plain);
    const LayoutDrawCommand* plainTab = pick(plain, "Tab");
    check("未选中用普通贴图", plainTab != nullptr && plainTab->texture != nullptr &&
                              std::string(plainTab->texture) == "ui/white.png");

    tab->checked = true;
    std::vector<LayoutDrawCommand> selected;
    tree.CollectDrawCommands(selected);
    const LayoutDrawCommand* selectedTab = pick(selected, "Tab");
    check("选中后用选中贴图", selectedTab != nullptr && selectedTab->texture != nullptr &&
                                std::string(selectedTab->texture) == "ui/login_bg.png");
    return failures == 0 ? 0 : 1;
}
