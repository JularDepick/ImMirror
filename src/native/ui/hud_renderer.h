#pragma once

#include <cstdint>

namespace immirror::ui {

/*
 * 游戏内 HUD 覆盖层: 准星, 虚拟摇杆与跳跃按钮.
 * 目前用纯色几何绘制, 后续接入原版界面纹理时替换为精灵绘制.
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

    /* 摇杆量取自玩家控制器, 范围 -1 到 1; active 表示当前有手指按在摇杆区.
     * selectedSlot 是当前选中的方块栏位, 范围 0 到 kSlotCount-1. */
    void Render(float stickX, float stickY, bool stickActive, int32_t selectedSlot);

    /* 底部方块栏的栏位数. */
    static constexpr int32_t kSlotCount = 9;

    /* 判断触点是否落在方块栏内, 命中时返回栏位序号, 否则返回 -1. */
    int32_t HitTestSlot(float normalizedX, float normalizedY) const;

private:
    bool ready_ = false;
    uint32_t program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int32_t uniformAtlasLocation_ = -1;
    int32_t viewportWidth_ = 0;
    int32_t viewportHeight_ = 0;
    uint32_t atlasTexture_ = 0;
};

}  /* namespace immirror::ui */