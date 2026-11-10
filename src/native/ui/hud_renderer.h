#pragma once

#include <cstdint>

#include "platform/input_state.h"
#include "ui/hud_layout.h"

namespace immirror::ui {

/*
 * 一帧 HUD 的输入状态, 由游戏总装填好后交给渲染层.
 * 摇杆几何直接取自玩家控制器的实际锚点与半径, 命中判定与绘制共用同一份布局,
 * 因此界面上的摇杆与真实输入不会错位.
 */
struct HudFrame {
    int32_t selectedSlot = 0;
    platform::StickVisual stick;
    bool jumpPressed = false;  /* 跳跃按钮按下中, 用于视觉反馈 */
    int32_t nameSlot = -1;     /* 长按显示方块名的槽位, 负值表示不显示 */
};

/*
 * 游戏内 HUD 覆盖层: 准星, 浮动摇杆, 跳跃按钮与方块栏.
 * 目前用纯色几何加内置点阵字绘制, 后续接入原版界面纹理时替换为精灵绘制.
 * 必须在区块绘制之后调用, 内部会临时关闭深度测试与背面剔除并开启混合.
 */
class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    HudRenderer(const HudRenderer&) = delete;
    HudRenderer& operator=(const HudRenderer&) = delete;

    /* atlasTexture 为方块图集句柄, 用于绘制底部方块选择栏. */
    bool Initialize(uint32_t atlasTexture);
    void Shutdown();
    bool Ready() const { return ready_; }

    void SetViewport(int32_t width, int32_t height);

    void Render(const HudFrame& frame);

    /* 底部方块栏的栏位数, 与共享槽位表一致. */
    static constexpr int32_t kSlotCount = kHudSlotCount;

    /* 判断触点是否落在方块栏内, 命中时返回栏位序号, 否则返回 -1. */
    int32_t HitTestSlot(float normalizedX, float normalizedY) const {
        return layout_.HitTestSlot(normalizedX, normalizedY);
    }

    /* 判断触点是否落在跳跃按钮内. */
    bool HitTestJump(float normalizedX, float normalizedY) const {
        return layout_.HitTestJump(normalizedX, normalizedY);
    }

    const HudLayout& Layout() const { return layout_; }

private:
    bool ready_ = false;
    uint32_t program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int32_t uniformAtlasLocation_ = -1;
    HudLayout layout_;
    uint32_t atlasTexture_ = 0;
};

}  /* namespace immirror::ui */
