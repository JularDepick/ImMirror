/* 多行文本断言: 超过帧宽时按宽度断行. */
#include <cstdio>
#include <string>

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

int main() {
    const std::string layout =
        "<Ui>"
        "<Frame name=\"Wrap\">"
        "  <Size><AbsDimension x=\"50\" y=\"60\"/></Size>"
        "  <Layers><Layer level=\"artwork\">"
        "    <FontString name=\"WrapLong\" multiLine=\"true\" fonttype=\"BlackFont10\" text=\"ABCDEFGHIJKLMNOPQRST\">"
        "      <Size><AbsDimension x=\"50\" y=\"60\"/></Size>"
        "    </FontString>"
        "    <FontString name=\"WrapShort\" multiLine=\"true\" fonttype=\"BlackFont10\" text=\"A\">"
        "      <Size><AbsDimension x=\"50\" y=\"60\"/></Size>"
        "    </FontString>"
        "  </Layer></Layers>"
        "</Frame>"
        "</Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* longText = document.Find("WrapLong");
    LayoutFrame* shortText = document.Find("WrapShort");
    check("找到长文本", longText != nullptr);
    check("找到短文本", shortText != nullptr);
    if (longText == nullptr || shortText == nullptr) {
        return 1;
    }
    check("长文本带多行标记", longText->multiLine);

    std::vector<LayoutDrawCommand> commands;
    CollectDrawCommands(*root, commands);
    int longCount = 0;
    int shortCount = 0;
    float firstY = 0.0f;
    float lastY = 0.0f;
    bool descending = true;
    for (const LayoutDrawCommand& command : commands) {
        if (command.name == nullptr) {
            continue;
        }
        const std::string name(command.name);
        if (name == "WrapLong") {
            if (longCount == 0) {
                firstY = command.rect.y;
            } else if (command.rect.y >= lastY) {
                descending = false;
            }
            lastY = command.rect.y;
            longCount += 1;
        } else if (name == "WrapShort") {
            shortCount += 1;
        }
    }
    check("长文本断成多行", longCount > 1);
    check("短文本保持一行", shortCount == 1);
    check("后续行向下排列", descending);
    check("行间有偏移", longCount > 1 && firstY != lastY);
    return failures == 0 ? 0 : 1;
}
