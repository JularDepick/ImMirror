/* 输入断言: 画布映射, 黑边裁切, 按下/抬起状态机. */
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ui/ui_input.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static bool near(float got, float want) {
    return std::fabs(got - want) < 0.6f;
}

int main() {
    /* 同比例: 1280x720 画布铺满 1920x1080 屏幕, 中心点对齐中心点. */
    UiInput input;
    input.SetCanvas(1280.0f, 720.0f);
    input.SetScreen(1920.0f, 1080.0f);
    float x = 0.0f;
    float y = 0.0f;
    check("同比例映射成功", input.ToCanvas(960.0f, 540.0f, &x, &y));
    check("同比例中心 X", near(x, 640.0f));
    check("同比例中心 Y", near(y, 360.0f));
    float rx = 0.0f;
    float ry = 0.0f;
    float rw = 0.0f;
    float rh = 0.0f;
    input.VisibleRect(&rx, &ry, &rw, &rh);
    check("同比例无黑边", near(rw, 1920.0f) && near(rh, 1080.0f));

    /* 宽屏: 2560x1080 时按高度缩放, 左右各留 320 像素黑边. */
    input.SetScreen(2560.0f, 1080.0f);
    input.VisibleRect(&rx, &ry, &rw, &rh);
    check("宽屏留左右黑边", near(rx, 320.0f) && near(rw, 1920.0f));
    check("宽屏中心映射", input.ToCanvas(1280.0f, 540.0f, &x, &y) && near(x, 640.0f) && near(y, 360.0f));
    check("黑边内的点被拒", !input.ToCanvas(100.0f, 540.0f, &x, &y));

    /* 状态机: 一个 100x50 的按钮放在 (10,10). */
    const std::string layout =
        "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"400\" y=\"300\"/></Size>"
        "<Button name=\"Btn\"><Size><AbsDimension x=\"100\" y=\"50\"/></Size>"
        "<NormalTexture name=\"$parentN\" file=\"ui/n.xml\" uvname=\"btn_normal.png\"/>"
        "<PushedTexture name=\"$parentP\" file=\"ui/p.xml\" uvname=\"btn_pushed.png\"/>"
        "</Button></Frame></Ui>";
    WidgetTree tree;
    check("载入布局", tree.Load(layout.data(), layout.size()));
    input.SetCanvas(1280.0f, 720.0f);
    input.SetScreen(1280.0f, 720.0f);
    tree.Resolve(LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    Widget* pressed = input.Down(tree, 60.0f, 30.0f);
    check("按下命中按钮", pressed != nullptr && std::string(pressed->frame->name) == "Btn");
    check("按下态被置位", pressed != nullptr && pressed->pressed);
    std::vector<LayoutDrawCommand> commands;
    tree.CollectDrawCommands(commands);
    bool pushed = false;
    for (const LayoutDrawCommand& command : commands) {
        if (command.sprite != nullptr && std::string(command.sprite) == "btn_pushed.png") {
            pushed = true;
        }
    }
    check("按下时用按下贴图", pushed);

    /* 滑出按钮: 按下态取消. */
    input.Move(tree, 300.0f, 200.0f);
    check("滑出后按下态取消", input.Pressed() == nullptr && !pressed->pressed);

    /* 按下后在同一点抬起: 返回被点击控件. */
    input.Down(tree, 60.0f, 30.0f);
    Widget* clicked = input.Up(tree, 62.0f, 31.0f);
    check("抬起返回被点击控件", clicked != nullptr && std::string(clicked->frame->name) == "Btn");
    check("抬起后按下态清除", !pressed->pressed);

    /* 按下后滑到按钮外抬起: 不算点击. */
    input.Down(tree, 60.0f, 30.0f);
    Widget* missed = input.Up(tree, 300.0f, 200.0f);
    check("按钮外抬起不算点击", missed == nullptr);

    return failures == 0 ? 0 : 1;
}
