/* 交互断言: 点击派发与按下态贴图. */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/script/widget_host.h"

using namespace immirror::ui;
using namespace immirror::ui::script;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    const std::string layout =
        "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"400\" y=\"300\"/></Size>"
        "<Button name=\"OkBtn\">"
        "<Size><AbsDimension x=\"120\" y=\"40\"/></Size>"
        "<NormalTexture name=\"$parentNormal\" file=\"ui/n.xml\" uvname=\"btn_normal.png\"/>"
        "<PushedTexture name=\"$parentPushed\" file=\"ui/p.xml\" uvname=\"btn_pushed.png\"/>"
        "<OnClick>OkBtn_OnClick()</OnClick>"
        "</Button>"
        "<FontString name=\"Tip\" text=\"BEFORE\"><Size><AbsDimension x=\"200\" y=\"20\"/></Size></FontString>"
        "</Frame></Ui>";
    const std::string script =
        "function OkBtn_OnClick() local t = getglobal(\"Tip\") if t ~= nil then t:SetText(\"AFTER\") end end";

    WidgetHost host;
    check("载入布局", host.Load(layout.data(), layout.size()));
    const LayoutRect screen{0.0f, 0.0f, 400.0f, 300.0f};
    check("执行脚本", host.RunScript(script.data(), script.size(), screen));

    Widget* button = host.Tree().Find("OkBtn");
    Widget* tip = host.Tree().Find("Tip");
    check("找到按钮", button != nullptr);
    check("找到文本控件", tip != nullptr);
    if (button == nullptr || tip == nullptr) {
        return 1;
    }
    check("OnClick 片段被解析", button->frame->onClick != nullptr);
    check("初始文本", tip->text.empty());

    /* 命中测试: 按钮中心应当命中按钮本身或其子树. */
    Widget* hit = host.Tree().HitTest(60.0f, 20.0f);
    check("命中测试有结果", hit != nullptr);
    check("命中落在按钮子树", hit != nullptr && (hit == button || hit->frame->parent == button->frame));

    /* 点击派发. */
    const int32_t handled = host.Click(button);
    std::printf("点击处理数 %d, 文本 [%s]\n", handled, tip->text.c_str());
    check("点击执行了处理函数", handled == 1);
    check("处理函数改写了文本", tip->text == "AFTER");

    /* 按下态: 绘制贴图应切到 PushedTexture. */
    std::vector<LayoutDrawCommand> commands;
    host.CollectDrawCommands(commands);
    bool normalUsed = false;
    for (const LayoutDrawCommand& command : commands) {
        if (command.sprite != nullptr && std::string(command.sprite) == "btn_normal.png") {
            normalUsed = true;
        }
    }
    check("弹起态用普通贴图", normalUsed);

    host.Tree().SetPressed(button, true);
    commands.clear();
    host.CollectDrawCommands(commands);
    bool pushedUsed = false;
    for (const LayoutDrawCommand& command : commands) {
        if (command.sprite != nullptr && std::string(command.sprite) == "btn_pushed.png") {
            pushedUsed = true;
        }
    }
    check("按下态用按下贴图", pushedUsed);

    return failures == 0 ? 0 : 1;
}
