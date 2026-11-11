#pragma once

#include <cstdint>

#include <android/asset_manager.h>

#include "platform/input_state.h"
#include "ui/hud_layout.h"

namespace immirror::ui {

/* 背包面板的槽位分组, 绘制与命中判定共用同一套几何. */
enum class InventoryRegion : uint8_t {
    kNone = 0,
    kPanel,      /* 面板底板, 命中但没有落在任何控件上 */
    kClose,      /* 关闭按钮 */
    kCrafting,   /* 合成网格 3x3 */
    kResult,     /* 合成产物格 */
    kBackpack,   /* 背包 36 格 */
    kHotbar,     /* 快捷栏 9 格 */
    kNextRecipe, /* 轮换候选配方 */
    kBookRecipe, /* 自动摆出下一张可合成配方 */
};

/* 一次面板命中: 区域与格序号, 非槽位区域的 index 为 -1. */
struct InventoryHit {
    InventoryRegion region = InventoryRegion::kNone;
    int32_t index = -1;
};

/* 背包面板的槽位格数, 与 game::Inventory 的分区一一对应. */
constexpr int32_t kInventoryCraftSlots = 9;
constexpr int32_t kInventoryBackpackSlots = 36;
constexpr int32_t kInventoryHotbarSlots = 9;

/*
 * 面板单格的绘制数据. 关联方块的物品按图集格显示图标;
 * atlasCell 为负时用首字母色块占位, 色块颜色由 tone 决定, 与物品类别对应.
 */
struct InventorySlotView {
    bool hasItem = false;
    int32_t atlasCell = -1;
    char initial = '?';
    uint8_t tone = 0;
    int32_t count = 0;
    bool highlight = false;
};

/*
 * 背包面板的一帧内容, 由游戏总装填好后交给渲染层.
 * 面板只认图标格, 首字母与数量, 不引用物品表, 保持界面层对玩法层零依赖.
 */
struct InventoryPanelFrame {
    InventorySlotView crafting[kInventoryCraftSlots];
    InventorySlotView result;
    InventorySlotView backpack[kInventoryBackpackSlots];
    InventorySlotView hotbar[kInventoryHotbarSlots];
    /* 手指携带中的堆叠, 位置由最近一次点击点给出. */
    InventorySlotView carried;
    bool carriedVisible = false;
    float carriedX = 0.5f;
    float carriedY = 0.5f;
    /* 候选配方的当前序号与总数, 序号从 1 开始, 0 表示没有候选. */
    int32_t candidateIndex = 0;
    int32_t candidateCount = 0;
    /* 是否还有可自动摆出的配方, 决定自动配方按钮的亮度. */
    bool bookAvailable = false;
    /* 底部提示文本, 只放 ASCII 字符, 空指针表示不显示. */
    const char* hint = nullptr;
};

/*
 * 一帧 HUD 的输入状态, 由游戏总装填好后交给渲染层.
 * 摇杆几何直接取自玩家控制器的实际锚点与半径, 命中判定与绘制共用同一份布局,
 * 因此界面上的摇杆与真实输入不会错位.
 */
struct HudFrame {
    int32_t selectedSlot = 0;
    platform::StickVisual stick;
    bool jumpPressed = false;  /* 跳跃按钮按下中, 用于视觉反馈 */
    int32_t nameSlot = -1;     /* 长按显示名称的槽位, 负值表示不显示 */
    float health = 1.0f;       /* 血量比例, 0 到 1, 供左上角血条取长度 */
    float mineProgress = 0.0f; /* 破坏进度, 0 到 1, 归零时不绘制进度环 */

    /* 每格剩余数量, 由物品栏逐帧填入; 数量为 0 时该格变暗, 数字仍显示 0. */
    int32_t slotCounts[kHudSlotCount] = {};
    /*
     * 每格图标. 空格子只画底板, 不复用方块栏的默认图标;
     * slotAtlasCell 为负且 slotInitial 非零时按首字母色块绘制,
     * 两者都没有给定时回落到方块栏共享表的方块图标.
     */
    bool slotHasItem[kHudSlotCount] = {};
    int32_t slotAtlasCell[kHudSlotCount] = {};
    char slotInitial[kHudSlotCount] = {};
    /* 首字母色块的配色下标, 与物品类别对应, 取值 0 到 5. */
    uint8_t slotTone[kHudSlotCount] = {};
    /* 长按显示的名称, 空指针回落到方块栏共享表的方块名. */
    const char* nameText = nullptr;
    /* 背包按钮按下中, 用于视觉反馈. */
    bool bagPressed = false;
    /* 快捷栏上方的操作提示, 空指针表示不显示, 用于数量耗尽这类即时反馈. */
    const char* tipText = nullptr;
    /* 背包面板, 空指针表示不绘制. */
    const InventoryPanelFrame* inventory = nullptr;
};

/*
 * 游戏内 HUD 覆盖层: 准星, 血条, 浮动摇杆, 跳跃按钮, 方块栏与背包面板.
 * 底板优先取界面图集 (generated/atlas_ui.rgba) 的原版精灵, 图集缺失或
 * 精灵未接入时回落到界面视觉规范的角色色; 方块图标仍走方块图集.
 * 文字用内置点阵字.
 * 必须在区块绘制之后调用, 内部会临时关闭深度测试与背面剔除并开启混合.
 */
class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    HudRenderer(const HudRenderer&) = delete;
    HudRenderer& operator=(const HudRenderer&) = delete;

    /*
     * atlasTexture 为方块图集句柄, 用于绘制底部方块选择栏的方块图标;
     * uiAtlasTexture 为界面图集句柄, 用于血条, 快捷栏, 跳跃按钮等原版精灵,
     * 传 0 时界面精灵回落到纯色绘制, 界面不因图集缺失而残缺.
     */
    bool Initialize(uint32_t atlasTexture, uint32_t uiAtlasTexture);
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

    /* 判断触点是否落在背包按钮内, 按钮位于右上角, 面板打开时同样可点. */
    bool HitTestBag(float normalizedX, float normalizedY) const;

    /* 判断触点是否落在背包面板底板范围内. */
    bool HitTestInventoryPanel(float normalizedX, float normalizedY) const;

    /* 背包面板的命中判定: 返回区域与格序号, 未命中返回 kNone. */
    InventoryHit HitTestInventory(float normalizedX, float normalizedY) const;

    const HudLayout& Layout() const { return layout_; }

private:
    bool ready_ = false;
    uint32_t program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int32_t uniformAtlasLocation_ = -1;
    int32_t uniformSpriteLocation_ = -1;
    HudLayout layout_;
    uint32_t atlasTexture_ = 0;
    uint32_t uiAtlasTexture_ = 0;
};

/*
 * 远端玩家的世界坐标占位标记: 一根竖条立在玩家脚底, 相机背后的条目自动跳过.
 * viewProjection 为列主序 4x4 视投影矩阵, 与区块渲染共用同一份, 空指针时不绘制.
 * 渲染管线约定与 HudRenderer::Render 相同: 必须在区块绘制之后调用,
 * 使用共享顶点程序与图集通道, 由本函数自行恢复深度与剔除状态.
 * 远端列表来自 net::RemotePlayerManager, 界面层不依赖玩家层与网络层.
 */
struct RemoteMarkerView {
    const char* const* uids; /* 脱敏昵称数组, 仅用于日志, 不绘制文字 */
    const float* xs;
    const float* ys;
    const float* zs;
    int32_t count;           /* 条目数量, 负数按零处理 */
};

void RenderRemoteMarkers(const RemoteMarkerView& markers, const float* viewProjection,
                         int32_t width, int32_t height);

/*
 * 从安装包资产加载界面图集 (ui/generated/atlas_ui.rgba) 并上传为纹理.
 * 资产缺失或损坏时返回 0, 调用方把 0 传给 Initialize 即回落纯色绘制;
 * 返回的句柄由调用方持有, 在 HUD 生命周期内有效, 释放走调用方的纹理清理.
 * assets 为空指针时同样返回 0, 便于桌面预览等无资产环境的调用方.
 */
uint32_t LoadUiAtlasFromAssets(AAssetManager* assets);

}  /* namespace immirror::ui */
