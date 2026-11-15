/* 单行文字裁剪断言: 超出帧宽的部分不生成顶点. */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_frame.h"
#include "ui/layout_render.h"
#include "ui/layout_xml.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static int32_t renderQuads(const std::vector<LayoutDrawCommand>& commands) {
    std::vector<float> vertices(static_cast<size_t>(16384) * kLayoutFloatsPerVertex, 0.0f);
    LayoutRenderStats stats;
    EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(), 16384, &stats);
    return stats.quads;
}

int main() {
    const std::string layout =
        "<Ui><Frame name=\"Root\">"
        "  <Size><AbsDimension x=\"400\" y=\"60\"/></Size>"
        "  <Layers><Layer level=\"artwork\">"
        "    <FontString name=\"Narrow\" fonttype=\"BlackFont10\" text=\"ABCDEFGHIJKLMNOP\">"
        "      <Size><AbsDimension x=\"30\" y=\"20\"/></Size>"
        "    </FontString>"
        "    <FontString name=\"Wide\" fonttype=\"BlackFont10\" text=\"ABCDEFGHIJKLMNOP\">"
        "      <Size><AbsDimension x=\"400\" y=\"20\"/></Size>"
        "    </FontString>"
        "  </Layer></Layers>"
        "</Frame></Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* narrow = document.Find("Narrow");
    LayoutFrame* wide = document.Find("Wide");
    check("找到窄帧", narrow != nullptr);
    check("找到宽帧", wide != nullptr);
    if (narrow == nullptr || wide == nullptr) {
        return 1;
    }

    std::vector<LayoutDrawCommand> narrowCommands;
    CollectDrawCommands(*narrow, narrowCommands);
    std::vector<LayoutDrawCommand> wideCommands;
    CollectDrawCommands(*wide, wideCommands);
    const int32_t narrowQuads = renderQuads(narrowCommands);
    const int32_t wideQuads = renderQuads(wideCommands);
    std::printf("  窄帧 %d 个四元组, 宽帧 %d 个\n", narrowQuads, wideQuads);
    check("窄帧仍有内容", narrowQuads > 0);
    check("窄帧被裁剪", narrowQuads < wideQuads);
    check("宽帧完整绘制", wideQuads > narrowQuads);
    return failures == 0 ? 0 : 1;
}
