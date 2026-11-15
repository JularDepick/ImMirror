/* 命中测试断言: 可点击元素的中心点能命中, 画布外不命中. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/widget_tree.h"

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
        "  <Size><AbsDimension x=\"1280\" y=\"720\"/></Size>"
        "  <Frames>"
        "    <Button name=\"BtnA\">"
        "      <Size><AbsDimension x=\"120\" y=\"40\"/></Size>"
        "      <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">"
        "        <Offset><AbsDimension x=\"100\" y=\"80\"/></Offset></Anchor></Anchors>"
        "      <NormalTexture file=\"ui/white.png\"/>"
        "    </Button>"
        "    <Button name=\"BtnB\">"
        "      <Size><AbsDimension x=\"80\" y=\"30\"/></Size>"
        "      <Anchors><Anchor point=\"topleft\" relativePoint=\"topleft\" relativeTo=\"$parent\">"
        "        <Offset><AbsDimension x=\"300\" y=\"200\"/></Offset></Anchor></Anchors>"
        "      <NormalTexture file=\"ui/white.png\"/>"
        "    </Button>"
        "  </Frames>"
        "</Frame></Ui>";
    WidgetTree tree;
    check("载入布局", tree.Load(layout.data(), layout.size()));
    tree.Resolve(LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    Widget* btnA = tree.Find("BtnA");
    Widget* btnB = tree.Find("BtnB");
    check("找到 BtnA", btnA != nullptr);
    check("找到 BtnB", btnB != nullptr);
    if (btnA == nullptr || btnB == nullptr) {
        return 1;
    }
    check("BtnA 有尺寸", btnA->frame->rect.width > 0.0f);

    /* 中心点命中自己. */
    Widget* hitA = tree.HitTest(btnA->frame->rect.x + btnA->frame->rect.width * 0.5f,
                                btnA->frame->rect.y + btnA->frame->rect.height * 0.5f);
    check("BtnA 中心命中", hitA == btnA);
    Widget* hitB = tree.HitTest(btnB->frame->rect.x + btnB->frame->rect.width * 0.5f,
                                btnB->frame->rect.y + btnB->frame->rect.height * 0.5f);
    check("BtnB 中心命中", hitB == btnB);
    check("空白处不命中按钮", tree.HitTest(700.0f, 600.0f) != btnA);
    check("画布外不命中", tree.HitTest(5000.0f, 5000.0f) == nullptr);

    /* 隐藏后不命中. */
    tree.Hide(btnA);
    Widget* hiddenHit = tree.HitTest(btnA->frame->rect.x + btnA->frame->rect.width * 0.5f,
                                     btnA->frame->rect.y + btnA->frame->rect.height * 0.5f);
    check("隐藏后不命中", hiddenHit != btnA);
    tree.Show(btnA);
    Widget* shownHit = tree.HitTest(btnA->frame->rect.x + btnA->frame->rect.width * 0.5f,
                                    btnA->frame->rect.y + btnA->frame->rect.height * 0.5f);
    check("重新显示后命中", shownHit == btnA);
    return failures == 0 ? 0 : 1;
}
