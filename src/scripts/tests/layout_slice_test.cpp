/* 九宫切片与按精灵名反查模板的离线断言. */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/generated/ui_sprite_index_gen.h"
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

int main() {
    SetSpriteSizeResolver(&UiSpriteSize);

    /* 从真实索引里挑一个足够大的精灵, 保证 200x200 的目标矩形能切出九宫. */
    const char* sprite = nullptr;
    for (int32_t index = 0; index < gen::kUiSpriteRectCount; ++index) {
        const gen::UiSpriteRect& rect = gen::kUiSpriteRects[index];
        if (rect.w >= 60.0f && rect.h >= 60.0f) {
            sprite = rect.name;
            break;
        }
    }
    check("索引里找到可用精灵", sprite != nullptr);
    if (sprite == nullptr) {
        return 1;
    }
    std::printf("使用精灵: %s\n", sprite);

    const std::string templatesXml =
        std::string("<Ui><Texture name=\"SliceTpl\" virtual=\"true\" file=\"ui/t.xml\" uvname=\"") +
        sprite + "\" DrawType=\"ninesquare\"><NineSquare ox=\"10\" oy=\"10\" tilew=\"20\" tileh=\"20\"/></Texture></Ui>";
    LayoutTemplates templates;
    check("模板解析成功", ParseTemplates(templatesXml.data(), templatesXml.size(), templates));
    const auto found = templates.find("SliceTpl");
    check("模板存在", found != templates.end());
    check("模板带切片参数", found != templates.end() && found->second.insetLeft > 0.0f);

    const std::string layoutXml =
        std::string("<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"400\" y=\"400\"/></Size>") +
        "<Texture name=\"Inherited\" inherits=\"SliceTpl\"><Size><AbsDimension x=\"200\" y=\"200\"/></Size></Texture>" +
        "<Texture name=\"Direct\" file=\"ui/t.xml\" uvname=\"" + sprite +
        "\"><Size><AbsDimension x=\"200\" y=\"200\"/></Size></Texture></Frame></Ui>";
    LayoutDocument document;
    document.SetTemplates(&templates);
    check("布局解析成功", document.Parse(layoutXml.data(), layoutXml.size()));
    LayoutFrame* root = document.Root();
    check("有根帧", root != nullptr);
    if (root == nullptr) {
        return 1;
    }
    ResolveLayout(*root, LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});

    LayoutFrame* inherited = document.Find("Inherited");
    LayoutFrame* direct = document.Find("Direct");
    check("继承元素带切片", inherited != nullptr && inherited->insetLeft > 0.0f);
    check("直接元素按精灵名拿到切片", direct != nullptr && direct->insetLeft > 0.0f);

    std::vector<LayoutDrawCommand> commands;
    CollectDrawCommands(*root, commands);
    std::printf("绘制命令 %zu\n", commands.size());
    check("两条命令", commands.size() == 2);

    std::vector<float> vertices(8 * 6 * 32, 0.0f);
    LayoutRenderStats stats;
    const int32_t used = EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(),
                                            static_cast<int32_t>(vertices.size() / kLayoutFloatsPerVertex),
                                            &stats);
    std::printf("四边形 %d, 顶点 %d\n", stats.quads, used);
    check("两条命令各切九块", stats.quads == 18);
    check("顶点数对应", used == 18 * 6);
    check("没有丢弃", stats.dropped == 0);
    return failures == 0 ? 0 : 1;
}
