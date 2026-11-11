#pragma once

#include <cstdint>
#include <vector>

#include "editor/editor_types.h"
#include "voxel/voxel_types.h"

namespace immirror::editor {

/*
 * 右侧工具条的按钮标识. 界面与总装共用同一套编号, 命中判定直接返回这里的取值.
 */
enum class EditorButton {
    kNone = -1,
    kSave = 0,
    kUndo = 1,
    kRedo = 2,
    kFill = 3,
    kPanel = 4,
    kPlay = 5,
    kBack = 6,
    kAddTrigger = 7,
    kTestTrigger = 8,
    kDeleteTrigger = 9,
};

/* 一个按钮的绘制与命中数据, 由总装按当前模式填好. */
struct EditorButtonState {
    EditorButton id = EditorButton::kNone;
    const char* label = "";
    bool active = false; /* 高亮: 填充模式进行中, 试玩进行中 */
};

/* 属性面板的一行: 标签, 取值文本与是否可用加减号. */
struct EditorRowState {
    const char* label = "";
    char value[32] = {0};
    bool steppable = false;
};

/*
 * 一帧编辑器覆盖层的数据. 全部是只读的展示状态,
 * 绘制与命中判定共用同一份布局, 界面不会与真实输入错位.
 */
struct EditorFrame {
    EditorMode mode = EditorMode::kEdit;
    const char* title = "";

    /* 左上角状态区: 逐行文本, ASCII. */
    const char* statusLines[limits::kStatusLines] = {};
    int32_t statusLineCount = 0;

    /* 右侧工具条. */
    const EditorButtonState* buttons = nullptr;
    int32_t buttonCount = 0;

    /* 底部方块调色板: 选中序号, -1 表示不显示调色板. */
    int32_t selectedPalette = -1;

    /* 准星与世界空间的选中框. */
    const float* viewProjection = nullptr; /* 列主序 16 个浮点, 空表示不画 */
    bool drawTarget = false;
    voxel::BlockPos targetMin{};
    voxel::BlockPos targetMax{};
    bool drawFill = false;
    voxel::BlockPos fillMin{};
    voxel::BlockPos fillMax{};

    /* 属性面板. */
    bool panelVisible = false;
    const char* panelTitle = "";
    const EditorRowState* rows = nullptr;
    int32_t rowCount = 0;
    int32_t highlightedRow = -1;

    /* 屏幕中下方的提示文本, 空表示不显示. */
    const char* hint = nullptr;
};

/* 触摸命中的结果: 按钮, 面板行与调色板序号互斥. */
struct EditorHit {
    EditorButton button = EditorButton::kNone;
    int32_t row = -1;
    int32_t rowDelta = 0;   /* -1 减少, +1 增加, 0 表示整行被点中 */
    int32_t paletteIndex = -1;
};

/*
 * 编辑器覆盖层绘制与命中判定.
 * 与游戏 HUD 一样使用屏幕归一化坐标, 设计坐标长度为屏高的一半并按宽高比等比换算,
 * 因此不同分辨率下按钮的实际像素尺寸一致. 文字沿用内置 5x7 点阵字, 只收录 ASCII.
 */
class EditorHud {
public:
    EditorHud();
    ~EditorHud();

    EditorHud(const EditorHud&) = delete;
    EditorHud& operator=(const EditorHud&) = delete;

    /* atlasTexture 为方块图集, 用于绘制调色板图标; 传 0 时图标退化为纯色块. */
    bool Initialize(uint32_t atlasTexture);
    void Shutdown();
    bool Ready() const { return ready_; }

    void SetViewport(int32_t width, int32_t height);

    /* 必须在区块绘制之后调用: 内部临时关闭深度测试与剔除并开启混合. */
    void Render(const EditorFrame& frame);

    /* 命中判定, 输入为归一化屏幕坐标. */
    EditorHit HitTest(const EditorFrame& frame, float normalizedX, float normalizedY) const;

private:
    /* 面板与按钮的几何, 绘制与命中判定共用. */
    struct Layout {
        float scaleX = 1.0f; /* 屏高/屏宽 */
        float panelX0 = 0.015f;
        float panelX1 = 0.5f;
        float panelTop = 0.215f;
        float panelTitleHeight = 0.032f;
        float rowHeight = 0.04f;
        float buttonX0 = 0.865f;
        float buttonX1 = 0.99f;
        float buttonTop = 0.035f;
        float buttonHeight = 0.052f;
        float buttonGap = 0.008f;
        float paletteCenterY = 0.935f;
        float paletteHalf = 0.026f;
        float palettePitch = 0.061f;
    };

    /* 面板行在给定行数下的纵向范围, 返回该行的上下边界. */
    void RowBounds(const Layout& layout, int32_t index, float& top, float& bottom) const;

    /* 按钮在给定序号下的纵向范围. */
    void ButtonBounds(const Layout& layout, int32_t index, float& top, float& bottom) const;

    /* 调色板某格的横向范围. */
    void PaletteBounds(const Layout& layout, int32_t index, float& left, float& right) const;

    bool ready_ = false;
    uint32_t program_ = 0;
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int32_t uniformAtlasLocation_ = -1;
    uint32_t atlasTexture_ = 0;
    int32_t width_ = 0;
    int32_t height_ = 0;
    /* 每帧重建的顶点数据, 容量固定, 避免逐帧分配. */
    std::vector<float> vertices_;
    Layout layout_;
};

}  /* namespace immirror::editor */
