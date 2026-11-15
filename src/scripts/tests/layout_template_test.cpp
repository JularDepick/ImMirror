/* 模板实例化断言: 继承模板的元素要克隆模板子树. */
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
        "<Ui>"
        "<Button name=\"CardTemplate\" virtual=\"true\">"
        "  <Size><AbsDimension x=\"120\" y=\"60\"/></Size>"
        "  <Texture name=\"$parentBg\" file=\"ui/card.xml\" uvname=\"card_bg.png\">"
        "    <Size><AbsDimension x=\"120\" y=\"60\"/></Size>"
        "  </Texture>"
        "  <FontString name=\"$parentTitle\" text=\"T\">"
        "    <Size><AbsDimension x=\"80\" y=\"20\"/></Size>"
        "  </FontString>"
        "</Button>"
        "<Frame name=\"Host\">"
        "  <Size><AbsDimension x=\"400\" y=\"300\"/></Size>"
        "  <Button name=\"Card1\" inherits=\"CardTemplate\">"
        "    <Size><AbsDimension x=\"120\" y=\"60\"/></Size>"
        "  </Button>"
        "</Frame>"
        "</Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));

    LayoutFrame* instance = document.Find("Card1");
    check("找到实例", instance != nullptr);
    if (instance == nullptr) {
        return 1;
    }
    check("实例克隆出子元素", instance->children.size() == 2);

    LayoutFrame* background = document.Find("Card1Bg");
    check("子纹理按实例名展开", background != nullptr);
    check("子纹理带贴图", background != nullptr && background->texture != nullptr &&
                              std::string(background->texture) == "ui/card.xml");
    check("子纹理带精灵", background != nullptr && background->sprite != nullptr &&
                              std::string(background->sprite) == "card_bg.png");

    LayoutFrame* title = document.Find("Card1Title");
    check("子文本按实例名展开", title != nullptr);
    check("子文本带文案", title != nullptr && title->text != nullptr &&
                              std::string(title->text) == "T");

    /* 模板本身仍是 virtual, 不参与绘制. */
    LayoutFrame* templateFrame = document.Find("CardTemplate");
    check("模板帧仍是模板", templateFrame != nullptr && templateFrame->isVirtual);
    return failures == 0 ? 0 : 1;
}
