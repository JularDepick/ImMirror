/* 文字描边断言: fontStyle="border" 的文字要画五遍(四周各一遍加深色正文). */
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

/* 把一组命令渲染成顶点, 返回四元组数量. */
static int32_t renderQuads(const std::vector<LayoutDrawCommand>& commands) {
    std::vector<float> vertices(static_cast<size_t>(4096) * kLayoutFloatsPerVertex, 0.0f);
    LayoutRenderStats stats;
    EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(), 4096, &stats);
    return stats.quads;
}

int main() {
    const std::string layout =
        "<Ui>"
        "<Frame name=\"Root\">"
        "  <Size><AbsDimension x=\"200\" y=\"60\"/></Size>"
        "  <Layers><Layer level=\"artwork\">"
        "    <FontString name=\"Plain\" fonttype=\"BlackFont10\" text=\"A\">"
        "      <Size><AbsDimension x=\"40\" y=\"20\"/></Size>"
        "    </FontString>"
        "    <FontString name=\"Bordered\" fontStyle=\"border\" fonttype=\"BlackFont10\" text=\"A\">"
        "      <Size><AbsDimension x=\"40\" y=\"20\"/></Size>"
        "    </FontString>"
        "  </Layer></Layers>"
        "</Frame>"
        "</Ui>";
    LayoutDocument document;
    check("解析布局", document.Parse(layout.data(), layout.size()));
    LayoutFrame* root = document.Root();
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* plain = document.Find("Plain");
    LayoutFrame* bordered = document.Find("Bordered");
    check("找到普通文字", plain != nullptr);
    check("找到描边文字", bordered != nullptr);
    if (plain == nullptr || bordered == nullptr) {
        return 1;
    }
    check("普通文字不描边", !plain->outline);
    check("描边文字带标记", bordered->outline);

    std::vector<LayoutDrawCommand> plainCommands;
    CollectDrawCommands(*plain, plainCommands);
    std::vector<LayoutDrawCommand> borderedCommands;
    CollectDrawCommands(*bordered, borderedCommands);
    const int32_t plainQuads = renderQuads(plainCommands);
    const int32_t borderedQuads = renderQuads(borderedCommands);
    std::printf("  普通 %d 个四元组, 描边 %d 个\n", plainQuads, borderedQuads);
    check("普通文字有四元组", plainQuads > 0);
    check("描边文字是普通文字的五倍", borderedQuads == plainQuads * 5);
    return failures == 0 ? 0 : 1;
}
