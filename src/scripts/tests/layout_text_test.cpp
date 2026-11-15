/* 文本对齐定位的离线断言. */
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ui/hud_font.h"
#include "ui/layout_render.h"

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
    LayoutDrawCommand command;
    command.rect = LayoutRect{100.0f, 50.0f, 200.0f, 40.0f};
    command.text = "HELLO";
    const float pixelSize = 2.0f;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    /* 左上: 原点就是矩形左上角. */
    command.justifyH = 0;
    command.justifyV = 0;
    LayoutTextOrigin(command, pixelSize, &x, &y, &width, &height);
    std::printf("宽 %.1f 高 %.1f\n", width, height);
    check("宽度按点阵度量", width > 0.0f);
    check("高度等于字高", near(height, 14.0f));
    check("左上对齐 X", near(x, 100.0f));
    check("左上对齐 Y", near(y, 50.0f));

    /* 居中. */
    command.justifyH = 1;
    command.justifyV = 1;
    LayoutTextOrigin(command, pixelSize, &x, &y, &width, &height);
    check("水平居中", near(x, 100.0f + (200.0f - width) * 0.5f));
    check("垂直居中", near(y, 50.0f + (40.0f - 14.0f) * 0.5f));

    /* 右下. */
    command.justifyH = 2;
    command.justifyV = 2;
    LayoutTextOrigin(command, pixelSize, &x, &y, &width, &height);
    check("右对齐", near(x, 300.0f - width));
    check("底对齐", near(y, 90.0f - 14.0f));

    /* 字形展开: 顶点阶段应把文字逐位画成纯色小方块. */
    command.justifyH = 0;
    command.justifyV = 0;
    command.texture = nullptr;
    command.fontSize = 20.0f;
    command.text = "HELLO";
    std::vector<LayoutDrawCommand> commands{command};
    int32_t expectedBits = 0;
    for (const char* p = "HELLO"; *p != '\0'; ++p) {
        const uint8_t* rows = GlyphRows(*p);
        if (rows == nullptr) {
            continue;
        }
        for (int32_t row = 0; row < kGlyphHeight; ++row) {
            for (int32_t col = 0; col < kGlyphWidth; ++col) {
                if ((rows[row] & static_cast<uint8_t>(1u << (kGlyphWidth - 1 - col))) != 0) {
                    expectedBits += 1;
                }
            }
        }
    }
    std::printf("HELLO 字形方块 %d\n", expectedBits);
    std::vector<float> vertices(8 * 6 * 4096, 0.0f);
    LayoutRenderStats stats;
    const int32_t used = EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(),
                                            static_cast<int32_t>(vertices.size() / kLayoutFloatsPerVertex),
                                            &stats);
    check("字形方块数与点阵一致", stats.quads == expectedBits);
    check("顶点数与方块数对应", used == expectedBits * 6);
    check("纯文本不画底色块", stats.quads == expectedBits);

    return failures == 0 ? 0 : 1;
}
