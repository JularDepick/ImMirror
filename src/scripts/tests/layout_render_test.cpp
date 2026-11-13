/* 布局绘制命令转顶点: 顶点数, 坐标范围与颜色传递. */
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ui/layout_render.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static void near(const char* label, float got, float want) {
    if (std::fabs(got - want) > 0.001f) {
        std::printf("FAIL %-22s got %.4f want %.4f\n", label, got, want);
        failures += 1;
    } else {
        std::printf("ok   %-22s %.4f\n", label, got);
    }
}

int main() {
    constexpr float kW = 1280.0f;
    constexpr float kH = 720.0f;
    static float buffer[6 * 8 * 8];

    /* 单个矩形: 铺满屏幕时四角正好是裁剪空间四角. */
    {
        std::vector<LayoutDrawCommand> commands;
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, kW, kH};
        command.red = 1.0f;
        command.green = 0.5f;
        command.blue = 0.25f;
        command.alpha = 0.75f;
        commands.push_back(command);
        LayoutRenderStats stats;
        const int32_t vertices = EmitLayoutCommands(commands, kW, kH, buffer, 48, &stats);
        check("一个矩形六个顶点", vertices == 6);
        check("统计一致", stats.quads == 1 && stats.vertices == 6);
        /* 第一个顶点是左下角. */
        near("左下 x", buffer[0], -1.0f);
        near("左下 y", buffer[1], -1.0f);
        near("颜色 r", buffer[4], 1.0f);
        near("颜色 g", buffer[5], 0.5f);
        near("透明度", buffer[7], 0.75f);
        /* 第三个顶点是右上角. */
        near("右上 x", buffer[2 * 8], 1.0f);
        near("右上 y", buffer[2 * 8 + 1], 1.0f);
    }
    /* 居中矩形: 坐标落在中心附近. */
    {
        std::vector<LayoutDrawCommand> commands;
        LayoutDrawCommand command;
        command.rect = LayoutRect{560.0f, 320.0f, 160.0f, 80.0f};
        commands.push_back(command);
        LayoutRenderStats stats;
        EmitLayoutCommands(commands, kW, kH, buffer, 48, &stats);
        near("中心矩形左", buffer[0], 560.0f / kW * 2.0f - 1.0f);
        near("中心矩形上", buffer[2 * 8 + 1], 1.0f - 320.0f / kH * 2.0f);
    }
    /* 多条命令与容量不足. */
    {
        std::vector<LayoutDrawCommand> commands;
        for (int32_t index = 0; index < 4; ++index) {
            LayoutDrawCommand command;
            command.rect = LayoutRect{static_cast<float>(index) * 10.0f, 0.0f, 10.0f, 10.0f};
            commands.push_back(command);
        }
        LayoutRenderStats stats;
        const int32_t vertices = EmitLayoutCommands(commands, kW, kH, buffer, 12, &stats);
        check("容量只够两条", vertices == 12);
        check("超出记入丢弃", stats.dropped == 2);
    }
    /* 贴图名能在图集里查到: 写入真实 uv. */
    {
        std::vector<LayoutDrawCommand> commands;
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, 10.0f, 10.0f};
        command.texture = "ui/hud/slot_normal.png";
        commands.push_back(command);
        LayoutRenderStats stats;
        EmitLayoutCommands(commands, kW, kH, buffer, 48, &stats);
        check("贴图命令被统计", stats.textured == 1);
        check("uv 不再是纯色约定", buffer[2] > 0.0f && buffer[3] > 0.0f);
    }
    /* 查不到的贴图名回落到纯色. */
    {
        std::vector<LayoutDrawCommand> commands;
        LayoutDrawCommand command;
        command.rect = LayoutRect{0.0f, 0.0f, 10.0f, 10.0f};
        command.texture = "ui/no_such_sprite.png";
        commands.push_back(command);
        LayoutRenderStats stats;
        EmitLayoutCommands(commands, kW, kH, buffer, 48, &stats);
        check("未知贴图不计入", stats.textured == 0);
        near("未知贴图回落纯色", buffer[2], -1.0f);
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
