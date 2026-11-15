/*
 * 布局转储: 把一份真实布局解算成绘制命令, 按 TSV 输出, 供与原版 XML 逐项比对.
 * 用法: layout_dump <布局.xml> [模板.xml] [宽 高] [--all]
 *   默认只转储可见帧(经控件树, 受 hidden 与 virtual 影响);
 *   --all 转储全部帧(忽略 hidden, 仍跳过 virtual), 用于几何基线比对.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_render.h"
#include "ui/layout_xml.h"
#include "ui/widget_tree.h"

using namespace immirror::ui;

static bool ReadFile(const char* path, std::string& out) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    char buffer[8192];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        out.append(buffer, got);
    }
    std::fclose(file);
    return true;
}

static std::string Escape(const char* text) {
    std::string out;
    if (text == nullptr) {
        return out;
    }
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '\t') {
            out += "\\t";
        } else if (*p == '\n') {
            out += "\\n";
        } else {
            out += *p;
        }
    }
    return out;
}

static const char* LayerName(LayerLevel level) {
    switch (level) {
        case LayerLevel::kBackground: return "background";
        case LayerLevel::kBorders: return "borders";
        case LayerLevel::kArtwork: return "artwork";
        case LayerLevel::kOverlay: return "overlay";
        case LayerLevel::kHighlight: return "highlight";
    }
    return "?";
}

static int withSprite = 0;
static int withText = 0;
static int total = 0;

static void Print(const LayoutDrawCommand& command) {
    if (command.sprite != nullptr && command.sprite[0] != '\0') {
        withSprite += 1;
    }
    if (command.text != nullptr && command.text[0] != '\0') {
        withText += 1;
    }
    total += 1;
    std::printf("%d\t%d\t%s\t%s\t%.1f\t%.1f\t%.1f\t%.1f\t%s\t%s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\n",
                total - 1, command.depth, LayerName(command.layer), Escape(command.name).c_str(),
                command.rect.x, command.rect.y, command.rect.width, command.rect.height,
                Escape(command.texture).c_str(), Escape(command.sprite).c_str(),
                Escape(command.text).c_str(), command.red, command.green, command.blue,
                command.alpha);
}

/* 全量模式: 逐帧输出, 忽略 hidden, 跳过 virtual. */
static void WalkAll(const LayoutFrame& frame, int depth) {
    if (frame.isVirtual) {
        return;
    }
    if (frame.rect.width > 0.0f && frame.rect.height > 0.0f) {
        LayoutDrawCommand command;
        command.rect = frame.rect;
        command.layer = frame.layer;
        command.texture = frame.texture != nullptr ? frame.texture : frame.normalTexture;
        command.sprite = frame.sprite != nullptr ? frame.sprite : frame.normalSprite;
        command.text = frame.text;
        command.name = frame.name;
        command.depth = depth;
        command.red = frame.red;
        command.green = frame.green;
        command.blue = frame.blue;
        command.alpha = frame.alpha;
        Print(command);
    }
    for (const LayoutFrame* child : frame.children) {
        if (child != nullptr) {
            WalkAll(*child, depth + 1);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "用法: layout_dump <布局.xml> [模板.xml] [宽 高] [--all]\n");
        return 2;
    }
    const char* layoutPath = argv[1];
    const char* templatePath = argc > 2 && argv[2][0] != '\0' ? argv[2] : nullptr;
    bool allFrames = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--all") == 0) {
            allFrames = true;
        }
    }
    const float width = argc > 4 && argv[3][0] != '-' ? static_cast<float>(std::atof(argv[3])) : 1280.0f;
    const float height = argc > 4 && argv[4][0] != '-' ? static_cast<float>(std::atof(argv[4])) : 720.0f;

    std::string layout;
    if (!ReadFile(layoutPath, layout)) {
        std::fprintf(stderr, "读不到布局: %s\n", layoutPath);
        return 2;
    }

    LayoutTemplates templates;
    if (templatePath != nullptr) {
        std::string text;
        if (!ReadFile(templatePath, text)) {
            std::fprintf(stderr, "读不到模板: %s\n", templatePath);
            return 2;
        }
        ParseTemplates(text.data(), text.size(), templates);
    }

    SetSpriteSizeResolver(&UiSpriteSize);
    SetTextureSizeResolver(&UiPageSize);
    std::printf("idx\tdepth\tlayer\tname\tx\ty\tw\th\ttexture\tsprite\ttext\tr\tg\tb\ta\n");
    if (allFrames) {
        LayoutDocument document;
        if (!templates.empty()) {
            document.SetTemplates(&templates);
        }
        if (!document.Parse(layout.data(), layout.size())) {
            std::fprintf(stderr, "布局解析失败: %s\n", document.Error().c_str());
            return 1;
        }
        LayoutFrame* root = document.Root();
        if (root == nullptr) {
            std::fprintf(stderr, "没有根帧\n");
            return 1;
        }
        ResolveLayout(*root, LayoutRect{0.0f, 0.0f, width, height});
        WalkAll(*root, 0);
        std::fprintf(stderr, "全量帧: 输出 %d, 带精灵 %d, 带文字 %d, 模板 %zu\n", total, withSprite,
                     withText, templates.size());
        return 0;
    }

    WidgetTree tree;
    if (!templates.empty()) {
        tree.SetTemplates(&templates);
    }
    if (!tree.Load(layout.data(), layout.size())) {
        std::fprintf(stderr, "布局解析失败: %s\n", tree.Error().c_str());
        return 1;
    }
    tree.Resolve(LayoutRect{0.0f, 0.0f, width, height});
    std::vector<LayoutDrawCommand> commands;
    tree.CollectDrawCommands(commands);
    for (const LayoutDrawCommand& command : commands) {
        Print(command);
    }
    std::fprintf(stderr, "控件 %d, 可见绘制命令 %d, 带精灵 %d, 带文字 %d, 模板 %zu\n",
                 tree.WidgetCount(), total, withSprite, withText, templates.size());
    return 0;
}
