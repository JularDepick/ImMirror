#include "ui/hud_renderer.h"

#include "ui/layout_render.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

#include "core/log.h"
#include "ui/generated/ui_atlas_gen.h"
#include "ui/generated/ui_resources_gen.h"
#include "ui/hud_font.h"
#include "ui/hud_slots.h"

namespace immirror::ui {
namespace {

constexpr const char* kTag = "ImMirrorHud";

/*
 * 顶点: 屏幕归一化坐标 (2) + 纹理坐标 (2) + 颜色 (4).
 * uv 的取值范围区分三路来源, 见 kUiChannel 的说明.
 */
constexpr int32_t kFloatsPerVertex = 8;

/*
 * 顶点缓冲容量, 按顶点数计. HUD 常态用量不足一千;
 * 背包面板一屏要画 55 个格子, 每格含九宫底板, 图标与数量点阵,
 * 因此预留到三万两千, 保证整包都摆满时也不会截断.
 */
constexpr int32_t kVertexCapacity = 32768;

/* 方块图集为 64x64 格. */
constexpr float kAtlasGrid = 64.0f;

/*
 * 纹理通道编码: 顶点 uv 复用一对 float, 按 uv.x 的取值范围区分三路来源.
 * uv.x < 0 纯色矩形, 不采样
 * 0 <= uv.x < kUiChannel 方块图集, 采样 uAtlas
 * uv.x >= kUiChannel 界面图集, 采样 uSprite, 采样前减回偏移
 * 偏移取 2 而不是 1, 留出余量避免界面 uv 恰好压在 1 上时判错通道;
 * float 在 2 到 3 区间的分辨率约 2.4e-7, 对 1/2048 的图集步长没有精度损失.
 */
constexpr float kUiChannel = 2.0f;

/* 摇杆环粗细, 以屏宽为 1. */
constexpr float kStickRingThickness = 0.004f;
/* 滑块半径相对摇杆环半径的比例. */
constexpr float kKnobRadiusRatio = 0.3f;

/* 跳跃按钮按下时的缩放, 与亮度一起构成按压反馈. */
constexpr float kJumpPressedScale = 0.9f;

/*
 * 规范配色, 取自 .agent/UI_VISUAL_SPEC.md 第三章的角色色表.
 * 四个分量依次为红绿蓝与透明度, 数值已从十六进制归一.
 */
/* #465677: HUD 底板, img_item 的槽位底色, 与素材一致取不透明 */
constexpr float kHudBoardColor[4] = {0x46 / 255.0f, 0x56 / 255.0f, 0x77 / 255.0f, 1.0f};
/* #20180F: 提示条底, board_tips_icon 的深棕黑, 同样不透明 */
constexpr float kHudTipsColor[4] = {0x20 / 255.0f, 0x18 / 255.0f, 0x0F / 255.0f, 1.0f};
/* #D61919: 血条实心段, img_blood_strip_red */
constexpr float kHudBloodColor[4] = {0xD6 / 255.0f, 0x19 / 255.0f, 0x19 / 255.0f, 1.0f};
/* 界面精灵的默认色调: 全白不透明, 保证精灵以原色显示. */
constexpr float kSpriteTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
/* 界面精灵的不可用色调: 压到半透明灰, 用于不可按的按钮底板. */
constexpr float kRecipeIdleTint[4] = {0.62f, 0.65f, 0.72f, 1.0f};

/*
 * 一个素材像素对应的归一化屏宽, 与 Java 侧一个素材像素等于一个 dp 的标尺呼应;
 * 九宫边界按它换算成设计长度, 换算方式见 PushNineSlice.
 */
constexpr float kHudArtPixel = 0.00075f;

/*
 * 血条展示几何, 以屏宽为 1 的归一化坐标给出.
 * 血条只是 HUD 的展示件, 不参与命中判定, 因此几何留在这里, 不改动布局表.
 */
constexpr float kHealthCenterX = 0.22f;
constexpr float kHealthCenterY = 0.075f;
constexpr float kHealthHalfWidth = 0.18f;
constexpr float kHealthHalfHeight = 0.014f;
/* 血条实心段相对底槽的内缩, 让底槽描边露出一圈. */
constexpr float kHealthFillInset = 2.0f;

/*
 * 生存状态条: PlayerFoodLevelBar 与 ChickenEnergyFrame.
 * 饥饿条与血条同宽, 紧接血条下方一行 (行距 30 像素, 按 720 设计高折算);
 * 体力条在快捷栏上方 (269x25, 锚 PlayShortcut 上移 11), 宽度略短于血条.
 * 三条同用一套底板色, 靠填充色区分: 血红, 饥饿橙黄, 体力青蓝.
 */
constexpr float kHungerFillColor[4] = {0xE8 / 255.0f, 0xA0 / 255.0f, 0x1B / 255.0f, 1.0f};
constexpr float kStaminaFillColor[4] = {0x3C / 255.0f, 0xC0 / 255.0f, 0xF1 / 255.0f, 1.0f};
/* 269 设计像素: 按 kHudArtPixel 标尺折算, 半宽 = 269 * 标尺 / 2. */
constexpr float kStaminaHalfWidth = 269.0f * kHudArtPixel / 2.0f;
constexpr float kStaminaHalfHeight = 0.011f;
constexpr float kStaminaCenterX = 0.135f;
constexpr float kStaminaCenterY = 0.865f - 0.055f;

/*
 * 状态条左侧图标: 血条 icon_heart 20x16, 饥饿条 icon_chicken_leg 19x20,
 * 两条同在 300x31 的框内. 用几何近似: 心形取两圆加下三角, 鸡腿取圆头加斜柄,
 * 不额外引入图集精灵, 色块本身即为可读性标记.
 */
constexpr float kVitalHeartColor[4] = {0xD6 / 255.0f, 0x19 / 255.0f, 0x19 / 255.0f, 1.0f};
constexpr float kVitalFoodColor[4] = {0xE8 / 255.0f, 0xA0 / 255.0f, 0x1B / 255.0f, 1.0f};
constexpr float kVitalStrokeColor[4] = {0x00 / 255.0f, 0x00 / 255.0f, 0x00 / 255.0f, 0.75f};

/* 纯色填充时传给九宫的纹理坐标首项, 顶点着色器按负数判定走纯色分支. */
constexpr float kSolidUv[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

/*
 * 破坏进度环: 准星外围的正方形, 边长与线宽均以屏宽为 1.
 * 进度沿周长从左上角顺时针填充, 底衬常显, 归零时整圈不绘制.
 */
constexpr float kMineRingHalfSize = 0.022f;
constexpr float kMineRingThickness = 0.0035f;

/* 方块名标签: 点阵像素边长, 内边距与离方块栏的间距, 均以屏宽为 1. */
constexpr float kNamePixelSize = 0.0035f;
constexpr float kNamePadding = 0.012f;
constexpr float kNamePlateGap = 0.012f;

/* 槽位数量: 点阵像素边长与贴齐槽位右下角的内缩比例, 均以屏宽为 1. */
constexpr float kCountPixelSize = 0.0026f;
constexpr float kCountInsetRatio = 0.82f;

/* 操作提示条离方块栏的间距与点阵像素边长, 以屏宽为 1; 间距留出方块名标签的位置. */
constexpr float kTipGap = 0.08f;
constexpr float kTipPixelSize = 0.0032f;

/* 数量为 0 时的槽位配色: 图标压到近乎半透明, 底板同时压深, 与有货的格区分. */
constexpr float kSlotEmptyTint[4] = {0.55f, 0.58f, 0.66f, 0.28f};
constexpr float kSlotEmptyBoard[4] = {0x2A / 255.0f, 0x31 / 255.0f, 0x42 / 255.0f, 1.0f};
/* 数量文字配色: 有货用亮白, 耗尽用暗白, 保证数字仍然可读. */
constexpr float kCountColor[4] = {1.0f, 1.0f, 1.0f, 0.95f};
constexpr float kCountEmptyColor[4] = {1.0f, 1.0f, 1.0f, 0.55f};
/* 数量底板: 深色半透明, 白色数字压在雪块这类亮色图标上也能看清. */
constexpr float kCountBackdrop[4] = {0.0f, 0.0f, 0.0f, 0.45f};

/*
 * 背包按钮: 固定在右上角, 面板打开与关闭共用同一个按钮.
 * 与血条, 跳跃按钮一样只在展示层绘制, 命中判定从同一份几何取值.
 */
constexpr float kBagCenterX = 0.93f;
constexpr float kBagCenterY = 0.064f;
constexpr float kBagHalfSize = 0.042f;

/*
 * 背包面板几何, 归一化屏幕坐标, 长度以屏宽为 1.
 * 自上而下的分区: 标题行, 合成区 (3x3 网格, 产物格与两个按钮), 背包四行,
 * 提示条与快捷栏. 面板底板覆盖大部分屏幕, 下层 HUD 仍按原样绘制.
 */
constexpr float kPanelLeft = 0.035f;
constexpr float kPanelRight = 0.965f;
constexpr float kPanelTop = 0.05f;
constexpr float kPanelBottom = 0.95f;

constexpr float kPanelTitleX = 0.12f;
constexpr float kPanelTitleY = 0.095f;
constexpr float kPanelTitlePixel = 0.009f;

constexpr float kPanelCloseX = 0.915f;
constexpr float kPanelCloseY = 0.095f;
constexpr float kPanelCloseHalf = 0.034f;

constexpr float kCraftOriginX = 0.13f;
constexpr float kCraftOriginY = 0.19f;
constexpr float kCraftPitchX = 0.082f;
constexpr float kCraftPitchY = 0.082f;

constexpr float kResultX = 0.47f;
constexpr float kResultY = 0.272f;
constexpr float kResultHalf = 0.046f;

constexpr float kRecipeButtonX = 0.67f;
constexpr float kNextButtonY = 0.19f;
constexpr float kBookButtonY = 0.272f;
constexpr float kRecipeButtonHalfW = 0.12f;
constexpr float kRecipeButtonHalfH = 0.030f;
/* 按钮文字的像素边长, 按住 "NEXT 1/16" 这类最长文案也能放进按钮宽度. */
constexpr float kRecipeButtonPixel = 0.0042f;

constexpr float kBackpackOriginX = 0.10f;
constexpr float kBackpackOriginY = 0.47f;
constexpr float kBackpackPitchX = 0.095f;
constexpr float kBackpackPitchY = 0.088f;
constexpr float kHotbarY = 0.865f;
constexpr float kHintY = 0.79f;
constexpr float kHintPixel = 0.0050f;

constexpr float kPanelSlotHalf = 0.038f;

/* 面板底板与遮罩配色: 遮罩压暗场景, 底板沿用 HUD 底板色的深色版本. */
constexpr float kPanelScrim[4] = {0.0f, 0.0f, 0.0f, 0.55f};
constexpr float kPanelBoard[4] = {0x24 / 255.0f, 0x2C / 255.0f, 0x3C / 255.0f, 1.0f};

/* 首字母占位块配色, 下标与物品类别一一对应, 0 为未分类. */
constexpr float kToneColors[6][4] = {
    {0.42f, 0.55f, 0.72f, 1.0f},
    {0.72f, 0.48f, 0.42f, 1.0f},
    {0.52f, 0.66f, 0.42f, 1.0f},
    {0.45f, 0.52f, 0.62f, 1.0f},
    {0.62f, 0.58f, 0.40f, 1.0f},
    {0.55f, 0.45f, 0.62f, 1.0f},
};

/* 有货格子的图标亮度与高亮描边色, 与下方快捷栏的取值保持一致. */
constexpr float kSlotTintColor[4] = {1.0f, 1.0f, 1.0f, 0.92f};
constexpr float kSlotBorderColor[4] = {1.0f, 0.92f, 0.55f, 1.0f};

/* 耐久条配色: 底槽半透明黑, 前景按剩余量由绿转黄再转红. */
constexpr float kDurabilityTrack[4] = {0.0f, 0.0f, 0.0f, 0.55f};
constexpr float kDurabilityGood[4] = {0.35f, 0.85f, 0.35f, 1.0f};
constexpr float kDurabilityWarn[4] = {0.95f, 0.80f, 0.25f, 1.0f};
constexpr float kDurabilityLow[4] = {0.92f, 0.28f, 0.24f, 1.0f};

/* 文案配色: 标题与按钮文字用亮白, 提示条用略暗的白. */
constexpr float kPanelTextColor[4] = {1.0f, 1.0f, 1.0f, 0.96f};
constexpr float kPanelHintColor[4] = {1.0f, 1.0f, 1.0f, 0.72f};
constexpr float kPanelButtonColor[4] = {0x3A / 255.0f, 0x62 / 255.0f, 0x9E / 255.0f, 1.0f};
constexpr float kPanelButtonIdleColor[4] = {0x2E / 255.0f, 0x3A / 255.0f, 0x50 / 255.0f, 1.0f};

const char* const kVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
out vec2 vTexCoord;
out vec4 vColor;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

const char* const kFragmentShader = R"(#version 300 es
precision mediump float;
uniform sampler2D uAtlas;
uniform sampler2D uSprite;
in vec2 vTexCoord;
in vec4 vColor;
out vec4 fragColor;
void main() {
    if (vTexCoord.x < 0.0) {
        fragColor = vColor;
        return;
    }
    /* uv.x 带通道偏移的走界面图集, 其余走方块图集, 判定见 kUiChannel 的说明. */
    if (vTexCoord.x >= 2.0) {
        fragColor = texture(uSprite, vec2(vTexCoord.x - 2.0, vTexCoord.y)) * vColor;
        return;
    }
    fragColor = texture(uAtlas, vTexCoord) * vColor;
}
)";

/* 读出并清空 GL 错误队列, 返回 true 表示记录了至少一个错误. */
bool DrainGlError(const char* stage) {
    bool failed = false;
    for (int32_t guard = 0; guard < 8; ++guard) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            break;
        }
        IM_LOGE(kTag, "gl error 0x%x at %s", static_cast<unsigned int>(error), stage);
        failed = true;
    }
    return failed;
}

/* 当前线程是否有可用的图形上下文, 上下文销毁后任何 GL 调用都是未定义行为. */
bool HasCurrentContext() {
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

uint32_t CompileShader(GLenum type, const char* source) {
    const uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char message[512];
        glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
        IM_LOGE(kTag, "shader compile failed: %s", message);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool HasRoom(int32_t offset, int32_t vertices) {
    return offset + vertices <= kVertexCapacity;
}

/*
 * 纯色矩形. 中心与半宽半高都是设计坐标, scale 为 高/宽,
 * 水平方向乘上它以后横纵像素长度相等. 用于准星, 按钮与标签底板.
 */
void PushRect(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
              float halfHeight, float scale, const float color[4]) {
    if (!HasRoom(offset, 6)) {
        return;
    }
    const float positions[4][2] = {
        {centerX - halfWidth, centerY - halfHeight},
        {centerX + halfWidth, centerY - halfHeight},
        {centerX + halfWidth, centerY + halfHeight},
        {centerX - halfWidth, centerY + halfHeight},
    };
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0] * scale;
        target[1] = positions[order[index]][1];
        target[2] = -1.0f;
        target[3] = -1.0f;
        target[4] = color[0];
        target[5] = color[1];
        target[6] = color[2];
        target[7] = color[3];
        offset += 1;
    }
}

/* 纯色三角形, 用于跳跃按钮上的向上箭头. */
void PushTriangle(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
                  float height, float scale, const float color[4]) {
    if (!HasRoom(offset, 3)) {
        return;
    }
    const float positions[3][2] = {
        {centerX, centerY + height * 0.5f},
        {centerX + halfWidth, centerY - height * 0.5f},
        {centerX - halfWidth, centerY - height * 0.5f},
    };
    for (int32_t index = 0; index < 3; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[index][0] * scale;
        target[1] = positions[index][1];
        target[2] = -1.0f;
        target[3] = -1.0f;
        target[4] = color[0];
        target[5] = color[1];
        target[6] = color[2];
        target[7] = color[3];
        offset += 1;
    }
}

/* 带图集纹理的矩形, cell 为图集格序号. */
void PushSprite(float* out, int32_t& offset, float centerX, float centerY, float halfSize,
                float scale, int32_t cell, const float color[4]) {
    if (!HasRoom(offset, 6)) {
        return;
    }
    const float left = centerX - halfSize;
    const float bottom = centerY - halfSize;
    const float right = centerX + halfSize;
    const float top = centerY + halfSize;
    const float column = static_cast<float>(cell % 16);
    const float row = static_cast<float>(cell / 16);
    const float u0 = column / kAtlasGrid;
    const float u1 = (column + 1.0f) / kAtlasGrid;
    /* 图集上传时已按格垂直翻转, 数据首行即格内底边, 因此 v 随行号增大. */
    const float v0 = row / kAtlasGrid;
    const float v1 = (row + 1.0f) / kAtlasGrid;
    const float positions[4][2] = {
        {left, bottom},
        {right, bottom},
        {right, top},
        {left, top},
    };
    const float uvs[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0] * scale;
        target[1] = positions[order[index]][1];
        target[2] = uvs[order[index]][0];
        target[3] = uvs[order[index]][1];
        target[4] = color[0];
        target[5] = color[1];
        target[6] = color[2];
        target[7] = color[3];
        offset += 1;
    }
}

/* 单个四边形, 顶点顺序为左下, 右下, 右上, 左上, 纹理坐标按素材方向传入. */
void PushQuad(float* out, int32_t& offset, float x0, float y0, float x1, float y1, float u0,
              float v0, float u1, float v1, float scale, const float color[4]) {
    if (!HasRoom(offset, 6)) {
        return;
    }
    const float positions[4][2] = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
    /* v0 给下边, v1 给上边, 与 PushSprite 的图集方向一致. */
    const float uvs[4][2] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0] * scale;
        target[1] = positions[order[index]][1];
        target[2] = uvs[order[index]][0];
        target[3] = uvs[order[index]][1];
        target[4] = color[0];
        target[5] = color[1];
        target[6] = color[2];
        target[7] = color[3];
        offset += 1;
    }
}

/*
 * 界面图集的运行期状态: 句柄是否有效由 Initialize 维护, 一个素材像素对应的
 * 设计长度由 Render 按当前布局刷新, 绘制函数只读这两个值.
 * 匿名命名空间的文件级状态对 HUD 单例足够, 渲染全部发生在主线程.
 */
bool uiAtlasAvailable = false;
float artPixelUnit = 0.0f;

/* 生成的格位表条目恒定有效, 这里只挡枚举越界回落项的负尺寸. */
bool spriteAvailable(const gen::UiSpriteInfo& sprite) {
    return sprite.width > 0.0f && sprite.height > 0.0f;
}

/*
 * 界面图集矩形: 坐标与 uv 直接来自生成的格位表 ui_atlas_gen.h.
 * 内部给 uv.x 加上通道偏移, 采样时由片元着色器切到界面图集;
 * uiAtlas 纹理无效时按纯色绘制, 调用方不用关心图集是否在线.
 */
void PushUiQuad(float* out, int32_t& offset, float x0, float y0, float x1, float y1, float u0,
                float v0, float u1, float v1, float scale, const float color[4]) {
    if (uiAtlasAvailable) {
        u0 += kUiChannel;
        u1 += kUiChannel;
        PushQuad(out, offset, x0, y0, x1, y1, u0, v0, u1, v1, scale, color);
        return;
    }
    PushRect(out, offset, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, (x1 - x0) * 0.5f, (y1 - y0) * 0.5f,
             scale, color);
}

/*
 * 界面精灵: 整体拉伸素材, 以中心与半宽半高摆放, 保持素材宽高比.
 * sprite 取自生成的格位表, uv 与尺寸都从表内取, 不手写坐标.
 */
void PushUiSprite(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
                  float halfHeight, float scale, const gen::UiSpriteInfo& sprite,
                  const float color[4]) {
    if (!spriteAvailable(sprite)) {
        return;
    }
    PushUiQuad(out, offset, centerX - halfWidth, centerY - halfHeight, centerX + halfWidth,
               centerY + halfHeight, sprite.u0, sprite.v0, sprite.u1, sprite.v1, scale, color);
}

/*
 * 界面九宫: 与 PushNineSlice 同一套分块逻辑, 数据源换成界面图集格位.
 * 边界之和超过目标尺寸时同样按比例压缩, 保证描边不被画花.
 */
void PushUiNineSlice(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
                     float halfHeight, float scale, const gen::UiSpriteInfo& sprite,
                     const float color[4]) {
    if (!spriteAvailable(sprite)) {
        return;
    }
    if (!uiAtlasAvailable) {
        /* 图集不在线时退化为纯色底板, 与 PushNineSlice 的纯色分支一致. */
        PushRect(out, offset, centerX, centerY, halfWidth, halfHeight, scale, color);
        return;
    }
    if (!HasRoom(offset, 54)) {
        return;
    }
    const float left = centerX - halfWidth;
    const float right = centerX + halfWidth;
    const float bottom = centerY - halfHeight;
    const float top = centerY + halfHeight;
    const float width = halfWidth * 2.0f;
    const float height = halfHeight * 2.0f;

    float insetLeft = sprite.left * artPixelUnit;
    float insetRight = sprite.right * artPixelUnit;
    float insetTop = sprite.top * artPixelUnit;
    float insetBottom = sprite.bottom * artPixelUnit;
    const float horizontal = insetLeft + insetRight;
    if (horizontal >= width && horizontal > 0.0f) {
        const float factor = width * 0.98f / horizontal;
        insetLeft *= factor;
        insetRight *= factor;
    }
    const float vertical = insetTop + insetBottom;
    if (vertical >= height && vertical > 0.0f) {
        const float factor = height * 0.98f / vertical;
        insetTop *= factor;
        insetBottom *= factor;
    }

    const float xs[4] = {left, left + insetLeft, right - insetRight, right};
    const float ys[4] = {bottom, bottom + insetBottom, top - insetTop, top};
    /* 上下边界按素材像素占精灵高度的比例切分 uv, 与 PushNineSlice 的换算一致. */
    const float sourceU[4] = {
        sprite.u0,
        sprite.u0 + (sprite.u1 - sprite.u0) * sprite.left / sprite.width,
        sprite.u1 - (sprite.u1 - sprite.u0) * sprite.right / sprite.width,
        sprite.u1,
    };
    const float sourceV[4] = {
        sprite.v0,
        sprite.v0 + (sprite.v1 - sprite.v0) * sprite.bottom / sprite.height,
        sprite.v1 - (sprite.v1 - sprite.v0) * sprite.top / sprite.height,
        sprite.v1,
    };
    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 3; ++column) {
            if (xs[column + 1] <= xs[column] || ys[row + 1] <= ys[row]) {
                continue;
            }
            const float u0 = sourceU[column] + kUiChannel;
            const float u1 = sourceU[column + 1] + kUiChannel;
            PushQuad(out, offset, xs[column], ys[row], xs[column + 1], ys[row + 1], u0, sourceV[row],
                     u1, sourceV[row + 1], scale, color);
        }
    }
}

/*
 * 九宫底板: 九个四边形拼成一块面板, 四角原位, 四边单向拉伸, 中心双向拉伸.
 *
 * asset 取自生成的参数表 ui_resources_gen.h, 四边边界是素材像素;
 * unit 为一个素材像素对应的设计长度, 由调用方按屏宽换算, 因此同一边界在不同
 * 分辨率下描边视觉粗细一致. uv 为素材在纹理内的归一化范围, 顺序为
 * (u0, 左上 v, u1, 右下 v); uv 首项为负时九个四边形都按纯色填充,
 * 用于界面图集尚未接入、只用规范色画底板的阶段.
 */
void PushNineSlice(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
                   float halfHeight, float scale, const gen::UiAsset& asset, float unit,
                   const float uv[4], const float color[4]) {
    if (!HasRoom(offset, 54)) {
        return;
    }
    const float left = centerX - halfWidth;
    const float right = centerX + halfWidth;
    const float bottom = centerY - halfHeight;
    const float top = centerY + halfHeight;
    const float width = halfWidth * 2.0f;
    const float height = halfHeight * 2.0f;

    float insetLeft = asset.left * unit;
    float insetRight = asset.right * unit;
    float insetTop = asset.top * unit;
    float insetBottom = asset.bottom * unit;

    /* 边界之和超过目标尺寸时按比例压缩, 避免九块互相重叠把描边画花. */
    const float horizontal = insetLeft + insetRight;
    if (horizontal >= width && horizontal > 0.0f) {
        const float factor = width * 0.98f / horizontal;
        insetLeft *= factor;
        insetRight *= factor;
    }
    const float vertical = insetTop + insetBottom;
    if (vertical >= height && vertical > 0.0f) {
        const float factor = height * 0.98f / vertical;
        insetTop *= factor;
        insetBottom *= factor;
    }

    const float xs[4] = {left, left + insetLeft, right - insetRight, right};
    const float ys[4] = {bottom, bottom + insetBottom, top - insetTop, top};

    const bool textured = uv[0] >= 0.0f;
    float sourceU[4] = {uv[0], uv[0], uv[2], uv[2]};
    float sourceV[4] = {uv[3], uv[3], uv[1], uv[1]};
    if (textured) {
        /* 上下与左右的边界按素材像素占原图的比例切分纹理坐标. */
        const float spanU = uv[2] - uv[0];
        const float spanV = uv[3] - uv[1];
        sourceU[0] = uv[0];
        sourceU[1] = uv[0] + spanU * asset.left / asset.width;
        sourceU[2] = uv[2] - spanU * asset.right / asset.width;
        sourceU[3] = uv[2];
        sourceV[0] = uv[3];
        sourceV[1] = uv[3] - spanV * asset.bottom / asset.height;
        sourceV[2] = uv[1] + spanV * asset.top / asset.height;
        sourceV[3] = uv[1];
    } else {
        for (int32_t index = 0; index < 4; ++index) {
            sourceU[index] = -1.0f;
            sourceV[index] = -1.0f;
        }
    }

    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 3; ++column) {
            if (xs[column + 1] <= xs[column] || ys[row + 1] <= ys[row]) {
                continue;
            }
            PushQuad(out, offset, xs[column], ys[row], xs[column + 1], ys[row + 1],
                     sourceU[column], sourceV[row], sourceU[column + 1], sourceV[row + 1], scale,
                     color);
        }
    }
}

/*
 * 粗线段: 两端外扩半粗细围成四边形, 主要供远端玩家的世界坐标标记使用.
 * 与 PushRect 一致使用设计坐标, scale 负责横向等比.
 */
void PushLine(float* out, int32_t& offset, float x0, float y0, float x1, float y1,
              float thickness, float scale, const float color[4]) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001f || !HasRoom(offset, 6)) {
        return;
    }
    const float nx = -dy / length * thickness * 0.5f;
    const float ny = dx / length * thickness * 0.5f;
    const float positions[4][2] = {
        {x0 + nx, y0 + ny}, {x1 + nx, y1 + ny}, {x1 - nx, y1 - ny}, {x0 - nx, y0 - ny}};
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0] * scale;
        target[1] = positions[order[index]][1];
        target[2] = -1.0f;
        target[3] = -1.0f;
        target[4] = color[0];
        target[5] = color[1];
        target[6] = color[2];
        target[7] = color[3];
        offset += 1;
    }
}

/* 圆环, thickness 等于 radius 时退化为实心圆. 横纵等比, 屏幕上始终是正圆. */
void PushRing(float* out, int32_t& offset, float centerX, float centerY, float radius,
              float thickness, float scale, const float color[4], int32_t segments) {
    const float innerRadius = radius - thickness;
    for (int32_t segment = 0; segment < segments; ++segment) {
        if (!HasRoom(offset, 6)) {
            return;
        }
        const float a0 = 6.28318530718f * static_cast<float>(segment) / static_cast<float>(segments);
        const float a1 =
            6.28318530718f * static_cast<float>(segment + 1) / static_cast<float>(segments);
        const float quad[4][2] = {
            {centerX + std::cos(a0) * innerRadius, centerY + std::sin(a0) * innerRadius},
            {centerX + std::cos(a1) * innerRadius, centerY + std::sin(a1) * innerRadius},
            {centerX + std::cos(a1) * radius, centerY + std::sin(a1) * radius},
            {centerX + std::cos(a0) * radius, centerY + std::sin(a0) * radius},
        };
        const int32_t order[6] = {0, 1, 2, 0, 2, 3};
        for (int32_t index = 0; index < 6; ++index) {
            float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
            target[0] = quad[order[index]][0] * scale;
            target[1] = quad[order[index]][1];
            target[2] = -1.0f;
            target[3] = -1.0f;
            target[4] = color[0];
            target[5] = color[1];
            target[6] = color[2];
            target[7] = color[3];
            offset += 1;
        }
    }
}

/*
 * 方形进度环. 四条边按周长展开, 从左上角起顺时针填充:
 * 上边向右, 右边向下, 下边向左, 左边向上. 底衬整圈常显, 已填充段加亮.
 * 全部由纯色四边形拼成, 不依赖图集纹理; 进度为 0 时整圈不绘制.
 */
void PushSquareProgress(float* out, int32_t& offset, float centerX, float centerY, float halfSize,
                        float thickness, float progress, float scale, const float track[4],
                        const float fill[4]) {
    if (progress <= 0.0f) {
        return;
    }
    const float clamped = progress > 1.0f ? 1.0f : progress;
    const float halfThickness = thickness * 0.5f;
    const float left = centerX - halfSize;
    const float right = centerX + halfSize;
    const float bottom = centerY - halfSize;
    const float top = centerY + halfSize;

    /* 底衬: 四条边各一条细矩形, 首尾相接围成方框. */
    PushRect(out, offset, centerX, top, halfSize, halfThickness, scale, track);
    PushRect(out, offset, centerX, bottom, halfSize, halfThickness, scale, track);
    PushRect(out, offset, left, centerY, halfThickness, halfSize, scale, track);
    PushRect(out, offset, right, centerY, halfThickness, halfSize, scale, track);

    /* 已填充段按累计长度逐边推进, 每条边长等于两倍半边长. */
    const float side = halfSize * 2.0f;
    float remaining = clamped * side * 4.0f;
    for (int32_t edge = 0; edge < 4 && remaining > 0.0f; ++edge) {
        const float length = remaining < side ? remaining : side;
        const float halfLength = length * 0.5f;
        if (edge == 0) {
            /* 上边: 从左上角向右. */
            PushRect(out, offset, left + halfLength, top, halfLength, halfThickness, scale, fill);
        } else if (edge == 1) {
            /* 右边: 从右上角向下. */
            PushRect(out, offset, right, top - halfLength, halfThickness, halfLength, scale, fill);
        } else if (edge == 2) {
            /* 下边: 从右下角向左. */
            PushRect(out, offset, right - halfLength, bottom, halfLength, halfThickness, scale,
                     fill);
        } else {
            /* 左边: 从左下角向上. */
            PushRect(out, offset, left, bottom + halfLength, halfThickness, halfLength, scale, fill);
        }
        remaining -= length;
    }
}

/* 点阵文字. left 与 top 为文本左上角的设计坐标, 逐像素铺小矩形. */
void PushText(float* out, int32_t& offset, float left, float top, float pixelSize,
              const char* text, float scale, const float color[4]) {
    if (text == nullptr) {
        return;
    }
    float cursorX = left;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        const uint8_t* rows = GlyphRows(*cursor);
        if (rows != nullptr) {
            for (int32_t row = 0; row < kGlyphHeight; ++row) {
                for (int32_t column = 0; column < kGlyphWidth; ++column) {
                    const bool lit = ((rows[row] >> (kGlyphWidth - 1 - column)) & 1u) != 0u;
                    if (!lit) {
                        continue;
                    }
                    const float centerX = cursorX + (static_cast<float>(column) + 0.5f) * pixelSize;
                    const float centerY = top - (static_cast<float>(row) + 0.5f) * pixelSize;
                    PushRect(out, offset, centerX, centerY, pixelSize * 0.5f, pixelSize * 0.5f,
                             scale, color);
                }
            }
        }
        cursorX += static_cast<float>(kGlyphWidth + kGlyphSpacing) * pixelSize;
    }
}

void ToUpper(const char* source, char* target, size_t capacity) {
    if (target == nullptr || capacity == 0) {
        return;
    }
    size_t index = 0;
    if (source != nullptr) {
        for (; source[index] != '\0' && index + 1 < capacity; ++index) {
            const char character = source[index];
            target[index] = (character >= 'a' && character <= 'z')
                                ? static_cast<char>(character - 'a' + 'A')
                                : character;
        }
    }
    target[index] = '\0';
}

/* 设计坐标下的矩形命中判定, 横纵半边长分别给出. */
bool InsideBox(float x, float y, float centerX, float centerY, float halfWidth, float halfHeight) {
    return x >= centerX - halfWidth && x <= centerX + halfWidth && y >= centerY - halfHeight &&
           y <= centerY + halfHeight;
}

/*
 * 远端玩家占位标记的配色: 亮青色, 与方块世界的暖色地形拉开对比.
 * 半透明, 避免遮挡背后场景; 正式形象接入前只做"这里有人"的视觉提示.
 */
constexpr float kRemoteMarkerColor[4] = {0.35f, 0.95f, 0.90f, 0.80f};

/* 远端标记绘制与 HUD 共享的着色程序与图集句柄, 由 HudRenderer::Initialize 装填. */
uint32_t program_shared = 0;
uint32_t atlasTexture_shared = 0;

/* 背包面板的几何结果, 绘制与命中判定都从这里取坐标, 两处不再各写一套数字. */
struct InventoryPanelGeometry {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;
    float slotHalf = 0.0f;
    float craftX[3] = {};
    float craftY[3] = {};
    float resultX = 0.0f;
    float resultY = 0.0f;
    float resultHalf = 0.0f;
    float backpackX[9] = {};
    float backpackY[4] = {};
    float hotbarX[9] = {};
    float hotbarY = 0.0f;
    float closeX = 0.0f;
    float closeY = 0.0f;
    float closeHalf = 0.0f;
    float titleX = 0.0f;
    float titleY = 0.0f;
    float nextX = 0.0f;
    float nextY = 0.0f;
    float bookX = 0.0f;
    float bookY = 0.0f;
    float buttonHalfW = 0.0f;
    float buttonHalfH = 0.0f;
    float hintX = 0.0f;
    float hintY = 0.0f;
    float bagX = 0.0f;
    float bagY = 0.0f;
    float bagHalf = 0.0f;
};

InventoryPanelGeometry MakeInventoryPanelGeometry(const HudLayout& layout) {
    InventoryPanelGeometry geometry;
    geometry.centerX = layout.DesignX((kPanelLeft + kPanelRight) * 0.5f);
    geometry.centerY = layout.DesignY((kPanelTop + kPanelBottom) * 0.5f);
    geometry.halfWidth = layout.DesignLength((kPanelRight - kPanelLeft) * 0.5f);
    geometry.halfHeight = layout.DesignLength((kPanelBottom - kPanelTop) * 0.5f);
    geometry.slotHalf = layout.DesignLength(kPanelSlotHalf);
    for (int32_t index = 0; index < 3; ++index) {
        geometry.craftX[index] = layout.DesignX(kCraftOriginX + static_cast<float>(index) * kCraftPitchX);
        geometry.craftY[index] = layout.DesignY(kCraftOriginY + static_cast<float>(index) * kCraftPitchY);
    }
    geometry.resultX = layout.DesignX(kResultX);
    geometry.resultY = layout.DesignY(kResultY);
    geometry.resultHalf = layout.DesignLength(kResultHalf);
    for (int32_t index = 0; index < 9; ++index) {
        geometry.backpackX[index] =
            layout.DesignX(kBackpackOriginX + static_cast<float>(index) * kBackpackPitchX);
        geometry.hotbarX[index] = geometry.backpackX[index];
    }
    for (int32_t row = 0; row < 4; ++row) {
        geometry.backpackY[row] = layout.DesignY(kBackpackOriginY + static_cast<float>(row) * kBackpackPitchY);
    }
    geometry.hotbarY = layout.DesignY(kHotbarY);
    geometry.closeX = layout.DesignX(kPanelCloseX);
    geometry.closeY = layout.DesignY(kPanelCloseY);
    geometry.closeHalf = layout.DesignLength(kPanelCloseHalf);
    geometry.titleX = layout.DesignX(kPanelTitleX);
    geometry.titleY = layout.DesignY(kPanelTitleY);
    geometry.nextX = layout.DesignX(kRecipeButtonX);
    geometry.nextY = layout.DesignY(kNextButtonY);
    geometry.bookX = layout.DesignX(kRecipeButtonX);
    geometry.bookY = layout.DesignY(kBookButtonY);
    geometry.buttonHalfW = layout.DesignLength(kRecipeButtonHalfW);
    geometry.buttonHalfH = layout.DesignLength(kRecipeButtonHalfH);
    geometry.hintX = layout.DesignX(0.5f);
    geometry.hintY = layout.DesignY(kHintY);
    geometry.bagX = layout.DesignX(kBagCenterX);
    geometry.bagY = layout.DesignY(kBagCenterY);
    geometry.bagHalf = layout.DesignLength(kBagHalfSize);
    return geometry;
}

/* 居中对齐的点阵文字, 用于按钮与提示条. */
void PushCenteredText(float* out, int32_t& offset, float centerX, float centerY, float pixelSize,
                      const char* text, float scale, const float color[4]) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const float width = MeasureText(text, pixelSize);
    const float height = static_cast<float>(kGlyphHeight) * pixelSize;
    PushText(out, offset, centerX - width * 0.5f, centerY + height * 0.5f, pixelSize, text, scale,
             color);
}

/*
 * 面板单格: 九宫底板, 图标或首字母占位, 右下角数量与高亮描边.
 * 有图标格子的物品按图集格绘制, 其余物品用类别配色块加首字母占位,
 * 没有独立图标资源时也能一眼区分物品.
 */
void PushInventorySlot(float* out, int32_t& offset, float centerX, float centerY, float half,
                       float scale, const InventorySlotView& view, float artUnit) {
    /*
     * 底板取img_item_box 九宫, 高亮格换成 img_item_box_selected;
     * 界面图集不在线时回落到纯色九宫, 与快捷栏共用同一套回落逻辑.
     */
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, centerX, centerY, half * 1.05f, half * 1.05f, scale,
                        gen::UiSpriteOf(view.highlight ? gen::UiSprite::SlotSelected
                                                       : gen::UiSprite::SlotNormal),
                        kSpriteTint);
    } else {
        PushNineSlice(out, offset, centerX, centerY, half * 1.05f, half * 1.05f, scale,
                      gen::UiAssetOf(view.highlight ? gen::UiWidget::SlotItemFrameOn
                                                    : gen::UiWidget::SlotItemFrame),
                      artUnit, kSolidUv, kHudBoardColor);
    }
    if (view.hasItem) {
        if (view.atlasCell >= 0) {
            PushSprite(out, offset, centerX, centerY, half, scale, view.atlasCell,
                       view.highlight ? kPanelTextColor : kSlotTintColor);
        } else {
            const float tileHalf = half * 0.72f;
            PushRect(out, offset, centerX, centerY, tileHalf, tileHalf, scale,
                     kToneColors[view.tone < 6 ? view.tone : 0]);
            /* 首字母字号按格子高度折算, 五乘七点阵最多占到格高的六成. */
            const float pixel = half * 0.18f;
            char label[2] = {view.initial == '\0' ? '?' : view.initial, '\0'};
            PushCenteredText(out, offset, centerX, centerY, pixel, label, scale, kPanelTextColor);
        }
    }
    /*
     * 耐久条: 带耐久的工具在格子底部画一条细条, 长度按剩余比例,
     * 颜色随剩余量由绿转黄再转红, 玩家不打开背包也能看出工具快坏了.
     */
    if (view.hasItem && view.maxDurability > 0 && view.durability >= 0) {
        const float barHalf = half * 0.78f;
        const float barY = centerY - half * 0.78f;
        const float barThickness = half * 0.09f;
        const float ratio = static_cast<float>(view.durability) /
                            static_cast<float>(view.maxDurability);
        const float clamped = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
        /* 底槽先铺满, 再按比例压一条彩色前景, 空槽部分也能看出满长. */
        PushRect(out, offset, centerX, barY, barHalf, barThickness, scale, kDurabilityTrack);
        if (clamped > 0.0f) {
            const float filledHalf = barHalf * clamped;
            const float left = centerX - barHalf + filledHalf;
            const float* color = clamped > 0.5f
                                     ? kDurabilityGood
                                     : (clamped > 0.25f ? kDurabilityWarn : kDurabilityLow);
            PushRect(out, offset, left, barY, filledHalf, barThickness, scale, color);
        }
    }
    /*
     * 附魔标记: 带附魔的物品在右上角点一个亮色小方块,
     * 在格子层面就能与普通物品区分, 不必打开说明.
     */
    if (view.hasItem && view.enchanted) {
        const float markHalf = half * 0.16f;
        const float markColor[4] = {0.55f, 0.90f, 0.98f, 0.95f};
        PushRect(out, offset, centerX + half * 0.70f, centerY + half * 0.70f, markHalf, markHalf,
                 scale, markColor);
    }
    if (view.hasItem && view.count > 1) {
        char countText[8];
        std::snprintf(countText, sizeof(countText), "%d", view.count);
        const float countPixel = half * 0.10f;
        const float countWidth = MeasureText(countText, countPixel);
        const float countRight = centerX + half * kCountInsetRatio;
        const float countBottom = centerY - half * kCountInsetRatio;
        const float countTop = countBottom + static_cast<float>(kGlyphHeight) * countPixel;
        const float backdropPad = countPixel * 0.5f;
        PushRect(out, offset, countRight - countWidth * 0.5f, (countBottom + countTop) * 0.5f,
                 countWidth * 0.5f + backdropPad, (countTop - countBottom) * 0.5f + backdropPad,
                 scale, kCountBackdrop);
        PushText(out, offset, countRight - countWidth, countTop, countPixel, countText, scale,
                 kCountColor);
    }
    if (view.highlight) {
        const float edge = half * 0.07f;
        PushRect(out, offset, centerX, centerY + half, half, edge, scale, kSlotBorderColor);
        PushRect(out, offset, centerX, centerY - half, half, edge, scale, kSlotBorderColor);
        PushRect(out, offset, centerX - half, centerY, edge, half, scale, kSlotBorderColor);
        PushRect(out, offset, centerX + half, centerY, edge, half, scale, kSlotBorderColor);
    }
}

/* 背包面板: 遮罩, 底板, 标题与关闭按钮, 合成区, 背包四行, 快捷栏与提示条. */
void DrawInventoryPanel(float* out, int32_t& offset, const HudLayout& layout,
                        const InventoryPanelFrame& frame, float scale) {
    const InventoryPanelGeometry geometry = MakeInventoryPanelGeometry(layout);
    const float artUnit = layout.DesignLength(kHudArtPixel);

    /* 遮罩铺满整屏, 让面板之外的场景退到后面. */
    PushRect(out, offset, 0.0f, 0.0f, layout.DesignLength(0.5f), 1.0f, scale, kPanelScrim);

    /*
     * 面板底板取board_bottom_blue_big 九宫 (texture2/common 的 60x60 大板),
     * 界面图集不在线时回落到深色纯色底板.
     */
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, geometry.centerX, geometry.centerY, geometry.halfWidth,
                        geometry.halfHeight, scale, gen::UiSpriteOf(gen::UiSprite::PanelBlue),
                        kSpriteTint);
    } else {
        PushNineSlice(out, offset, geometry.centerX, geometry.centerY, geometry.halfWidth,
                      geometry.halfHeight, scale, gen::UiAssetOf(gen::UiWidget::PanelSurfaceBlueLarge),
                      artUnit, kSolidUv, kPanelBoard);
    }

    const float titlePixel = layout.DesignLength(kPanelTitlePixel);
    PushText(out, offset, geometry.titleX,
             geometry.titleY + static_cast<float>(kGlyphHeight) * titlePixel * 0.5f, titlePixel,
             "BAG", scale, kPanelTextColor);

    /*
     * 关闭按钮: 取btn_mini_orange 三片底板加 X 字, 面板内唯一必然可用的出口;
     * 界面图集不在线时回落到纯色底板.
     */
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, geometry.closeX, geometry.closeY, geometry.closeHalf,
                        geometry.closeHalf, scale, gen::UiSpriteOf(gen::UiSprite::BtnCompactOrange),
                        kSpriteTint);
    } else {
        PushNineSlice(out, offset, geometry.closeX, geometry.closeY, geometry.closeHalf,
                      geometry.closeHalf, scale, gen::UiAssetOf(gen::UiWidget::ButtonCompactOrange),
                      artUnit, kSolidUv, kPanelButtonColor);
    }
    PushCenteredText(out, offset, geometry.closeX, geometry.closeY,
                     layout.DesignLength(0.0075f), "X", scale, kPanelTextColor);

    /* 合成网格 3x3 与产物格. */
    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 3; ++column) {
            PushInventorySlot(out, offset, geometry.craftX[column], geometry.craftY[row],
                              geometry.slotHalf, scale, frame.crafting[row * 3 + column], artUnit);
        }
    }
    PushInventorySlot(out, offset, geometry.resultX, geometry.resultY, geometry.resultHalf, scale,
                      frame.result, artUnit);

    /* 两个配方按钮: NEXT 轮换候选, AUTO 自动摆出一张可合成配方. */
    char nextText[16];
    if (frame.candidateCount > 1) {
        std::snprintf(nextText, sizeof(nextText), "NEXT %d/%d", frame.candidateIndex,
                      frame.candidateCount);
    } else {
        std::snprintf(nextText, sizeof(nextText), "NEXT");
    }
    const float buttonPixel = layout.DesignLength(kRecipeButtonPixel);
    /*
     * 两个配方按钮取btn_mini_blue 三片底板: NEXT 轮换候选常亮,
     * AUTO 自动摆出可合成配方, 不可用时压暗底板; 图集不在线时回落纯色.
     */
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, geometry.nextX, geometry.nextY, geometry.buttonHalfW,
                        geometry.buttonHalfH, scale, gen::UiSpriteOf(gen::UiSprite::BtnCompactBlue),
                        kSpriteTint);
        PushUiNineSlice(out, offset, geometry.bookX, geometry.bookY, geometry.buttonHalfW,
                        geometry.buttonHalfH, scale, gen::UiSpriteOf(gen::UiSprite::BtnCompactBlue),
                        frame.bookAvailable ? kSpriteTint : kRecipeIdleTint);
    } else {
        PushNineSlice(out, offset, geometry.nextX, geometry.nextY, geometry.buttonHalfW,
                      geometry.buttonHalfH, scale, gen::UiAssetOf(gen::UiWidget::ButtonCompactBlue),
                      artUnit, kSolidUv, kPanelButtonColor);
        PushNineSlice(out, offset, geometry.bookX, geometry.bookY, geometry.buttonHalfW,
                      geometry.buttonHalfH, scale, gen::UiAssetOf(gen::UiWidget::ButtonCompactBlue),
                      artUnit, kSolidUv,
                      frame.bookAvailable ? kPanelButtonColor : kPanelButtonIdleColor);
    }
    PushCenteredText(out, offset, geometry.nextX, geometry.nextY, buttonPixel, nextText, scale,
                     kPanelTextColor);
    PushCenteredText(out, offset, geometry.bookX, geometry.bookY, buttonPixel, "AUTO", scale,
                     kPanelTextColor);

    /* 背包 9 列 4 行, 顺序与物品栏 9 到 44 号槽一致. */
    for (int32_t row = 0; row < 4; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            PushInventorySlot(out, offset, geometry.backpackX[column], geometry.backpackY[row],
                              geometry.slotHalf, scale, frame.backpack[row * 9 + column], artUnit);
        }
    }

    /* 快捷栏一行, 与屏幕上方的快捷栏内容同源. */
    for (int32_t column = 0; column < 9; ++column) {
        PushInventorySlot(out, offset, geometry.hotbarX[column], geometry.hotbarY,
                          geometry.slotHalf, scale, frame.hotbar[column], artUnit);
    }

    if (frame.hint != nullptr && frame.hint[0] != '\0') {
        char hint[48];
        ToUpper(frame.hint, hint, sizeof(hint));
        PushCenteredText(out, offset, geometry.hintX, geometry.hintY,
                         layout.DesignLength(kHintPixel), hint, scale, kPanelHintColor);
    }

    /* 手指携带中的堆叠画在最后, 保证压在其它格子之上. */
    if (frame.carriedVisible && frame.carried.hasItem) {
        const float carryHalf = geometry.slotHalf * 1.15f;
        PushInventorySlot(out, offset, layout.DesignX(frame.carriedX),
                          layout.DesignY(frame.carriedY), carryHalf, scale, frame.carried, artUnit);
    }
}

/*
 * 以下为容器面板 (熔炉与储物箱) 的几何与绘制, 与背包面板同源同风格.
 * 为保持既有代码区不动, 本段以新的匿名命名空间块追加在原匿名块之后.
 */
namespace {

/* 容器面板几何, 归一化屏幕坐标, 复用背包面板的底板范围. */
constexpr float kContainerPanelLeft = kPanelLeft;
constexpr float kContainerPanelRight = kPanelRight;
constexpr float kContainerPanelTop = kPanelTop;
constexpr float kContainerPanelBottom = kPanelBottom;
constexpr float kContainerSlotHalf = kPanelSlotHalf;

/* 熔炉三格: 输入, 燃料与产出, 纵向排在面板上部. */
constexpr float kFurnaceInputX = 0.30f;
constexpr float kFurnaceInputY = 0.22f;
constexpr float kFurnaceFuelX = 0.30f;
constexpr float kFurnaceFuelY = 0.40f;
constexpr float kFurnaceOutputX = 0.62f;
constexpr float kFurnaceOutputY = 0.31f;
constexpr float kFurnaceOutputHalf = 0.046f;
/* 熔炼进度条: 在输入与产出之间横向延伸. */
constexpr float kFurnaceProgressLeft = 0.38f;
constexpr float kFurnaceProgressRight = 0.54f;
constexpr float kFurnaceProgressY = 0.31f;
constexpr float kFurnaceProgressHalfH = 0.008f;
/* 燃料余量条: 燃料格右侧的竖条. */
constexpr float kFurnaceFlameX = 0.37f;
constexpr float kFurnaceFlameY = 0.40f;
constexpr float kFurnaceFlameHalfW = 0.010f;
constexpr float kFurnaceFlameHalfH = 0.026f;

/* 储物箱 27 格: 9 列 3 行, 位于面板上部, 与背包区的行距一致. */
constexpr float kChestOriginX = 0.10f;
constexpr float kChestOriginY = 0.17f;
constexpr float kChestPitchX = 0.095f;
constexpr float kChestPitchY = 0.088f;

/* 容器面板的几何结果, 绘制与命中判定共用. */
struct ContainerPanelGeometry {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;
    float slotHalf = 0.0f;
    float inputX = 0.0f;
    float inputY = 0.0f;
    float fuelX = 0.0f;
    float fuelY = 0.0f;
    float outputX = 0.0f;
    float outputY = 0.0f;
    float outputHalf = 0.0f;
    float chestX[9] = {};
    float chestY[3] = {};
    float backpackX[9] = {};
    float backpackY[4] = {};
    float hotbarX[9] = {};
    float hotbarY = 0.0f;
    float closeX = 0.0f;
    float closeY = 0.0f;
    float closeHalf = 0.0f;
    float titleX = 0.0f;
    float titleY = 0.0f;
};
ContainerPanelGeometry MakeContainerPanelGeometry(const HudLayout& layout) {
    ContainerPanelGeometry geometry;
    geometry.centerX = layout.DesignX((kContainerPanelLeft + kContainerPanelRight) * 0.5f);
    geometry.centerY = layout.DesignY((kContainerPanelTop + kContainerPanelBottom) * 0.5f);
    geometry.halfWidth = layout.DesignLength((kContainerPanelRight - kContainerPanelLeft) * 0.5f);
    geometry.halfHeight = layout.DesignLength((kContainerPanelBottom - kContainerPanelTop) * 0.5f);
    geometry.slotHalf = layout.DesignLength(kContainerSlotHalf);
    geometry.inputX = layout.DesignX(kFurnaceInputX);
    geometry.inputY = layout.DesignY(kFurnaceInputY);
    geometry.fuelX = layout.DesignX(kFurnaceFuelX);
    geometry.fuelY = layout.DesignY(kFurnaceFuelY);
    geometry.outputX = layout.DesignX(kFurnaceOutputX);
    geometry.outputY = layout.DesignY(kFurnaceOutputY);
    geometry.outputHalf = layout.DesignLength(kFurnaceOutputHalf);
    for (int32_t index = 0; index < 9; ++index) {
        geometry.chestX[index] =
            layout.DesignX(kChestOriginX + static_cast<float>(index) * kChestPitchX);
        geometry.backpackX[index] =
            layout.DesignX(kBackpackOriginX + static_cast<float>(index) * kBackpackPitchX);
        geometry.hotbarX[index] = geometry.backpackX[index];
    }
    for (int32_t index = 0; index < 3; ++index) {
        geometry.chestY[index] =
            layout.DesignY(kChestOriginY + static_cast<float>(index) * kChestPitchY);
    }
    for (int32_t index = 0; index < 4; ++index) {
        geometry.backpackY[index] =
            layout.DesignY(kBackpackOriginY + static_cast<float>(index) * kBackpackPitchY);
    }
    geometry.hotbarY = layout.DesignY(kHotbarY);
    geometry.closeX = layout.DesignX(kPanelCloseX);
    geometry.closeY = layout.DesignY(kPanelCloseY);
    geometry.closeHalf = layout.DesignLength(kPanelCloseHalf);
    geometry.titleX = layout.DesignX(kPanelTitleX);
    geometry.titleY = layout.DesignY(kPanelTitleY);
    return geometry;
}
/* 容器面板的遮罩, 底板, 标题与关闭按钮, 熔炉与储物箱面板共用. */
void DrawContainerPanelBase(float* out, int32_t& offset, const HudLayout& layout,
                            const ContainerPanelGeometry& geometry, float scale,
                            const char* title) {
    const float artUnit = layout.DesignLength(kHudArtPixel);
    PushRect(out, offset, 0.0f, 0.0f, layout.DesignLength(0.5f), 1.0f, scale, kPanelScrim);
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, geometry.centerX, geometry.centerY, geometry.halfWidth,
                        geometry.halfHeight, scale, gen::UiSpriteOf(gen::UiSprite::PanelBlue),
                        kSpriteTint);
    } else {
        PushNineSlice(out, offset, geometry.centerX, geometry.centerY, geometry.halfWidth,
                      geometry.halfHeight, scale,
                      gen::UiAssetOf(gen::UiWidget::PanelSurfaceBlueLarge), artUnit, kSolidUv,
                      kPanelBoard);
    }
    const float titlePixel = layout.DesignLength(kPanelTitlePixel);
    PushText(out, offset, geometry.titleX,
             geometry.titleY + static_cast<float>(kGlyphHeight) * titlePixel * 0.5f, titlePixel,
             title, scale, kPanelTextColor);
    if (uiAtlasAvailable) {
        PushUiNineSlice(out, offset, geometry.closeX, geometry.closeY, geometry.closeHalf,
                        geometry.closeHalf, scale, gen::UiSpriteOf(gen::UiSprite::BtnCompactOrange),
                        kSpriteTint);
    } else {
        PushNineSlice(out, offset, geometry.closeX, geometry.closeY, geometry.closeHalf,
                      geometry.closeHalf, scale, gen::UiAssetOf(gen::UiWidget::ButtonCompactOrange),
                      artUnit, kSolidUv, kPanelButtonColor);
    }
    PushCenteredText(out, offset, geometry.closeX, geometry.closeY,
                     layout.DesignLength(0.0075f), "X", scale, kPanelTextColor);
}

/* 容器面板的背包四行与快捷栏一行, 与背包面板同源. */
void DrawContainerInventoryGrids(float* out, int32_t& offset,
                                 const ContainerPanelGeometry& geometry, float scale,
                                 const InventorySlotView* backpack,
                                 const InventorySlotView* hotbar, float artUnit) {
    for (int32_t row = 0; row < 4; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            PushInventorySlot(out, offset, geometry.backpackX[column], geometry.backpackY[row],
                              geometry.slotHalf, scale, backpack[row * 9 + column], artUnit);
        }
    }
    for (int32_t column = 0; column < 9; ++column) {
        PushInventorySlot(out, offset, geometry.hotbarX[column], geometry.hotbarY,
                          geometry.slotHalf, scale, hotbar[column], artUnit);
    }
}

/* 容器面板的携带堆叠, 画在最后. */
void DrawContainerCarried(float* out, int32_t& offset, const HudLayout& layout,
                          const ContainerPanelGeometry& geometry, float scale,
                          const InventorySlotView& carried, bool visible, float x, float y) {
    if (!visible || !carried.hasItem) {
        return;
    }
    const float carryHalf = geometry.slotHalf * 1.15f;
    PushInventorySlot(out, offset, layout.DesignX(x), layout.DesignY(y), carryHalf, scale,
                      carried, layout.DesignLength(kHudArtPixel));
}
/* 熔炉面板: 三格, 进度条, 燃料余量条, 背包与快捷栏. */
void DrawFurnacePanel(float* out, int32_t& offset, const HudLayout& layout,
                      const FurnacePanelFrame& frame, float scale) {
    const ContainerPanelGeometry geometry = MakeContainerPanelGeometry(layout);
    const float artUnit = layout.DesignLength(kHudArtPixel);
    DrawContainerPanelBase(out, offset, layout, geometry, scale, "FURNACE");

    PushInventorySlot(out, offset, geometry.inputX, geometry.inputY, geometry.slotHalf, scale,
                      frame.input, artUnit);
    PushInventorySlot(out, offset, geometry.fuelX, geometry.fuelY, geometry.slotHalf, scale,
                      frame.fuel, artUnit);
    PushInventorySlot(out, offset, geometry.outputX, geometry.outputY, geometry.outputHalf,
                      scale, frame.output, artUnit);

    /* 熔炼进度条: 底槽加填充, 填充比例取 frame.progress. */
    const float progressLeft = layout.DesignX(kFurnaceProgressLeft);
    const float progressRight = layout.DesignX(kFurnaceProgressRight);
    const float progressY = layout.DesignY(kFurnaceProgressY);
    const float progressHalfW = (progressRight - progressLeft) * 0.5f;
    const float progressHalfH = layout.DesignLength(kFurnaceProgressHalfH);
    PushRect(out, offset, (progressLeft + progressRight) * 0.5f, progressY, progressHalfW,
             progressHalfH, scale, kPanelButtonIdleColor);
    float fillRatio = frame.progress;
    if (fillRatio < 0.0f) fillRatio = 0.0f;
    if (fillRatio > 1.0f) fillRatio = 1.0f;
    const float fillHalfW = progressHalfW * fillRatio;
    if (fillHalfW > 0.0f) {
        PushRect(out, offset, progressLeft + fillHalfW, progressY, fillHalfW,
                 progressHalfH * 0.7f, scale, kSlotBorderColor);
    }

    /* 燃料余量条: 竖条底槽加自下而上的填充. */
    const float flameX = layout.DesignX(kFurnaceFlameX);
    const float flameY = layout.DesignY(kFurnaceFlameY);
    const float flameHalfW = layout.DesignLength(kFurnaceFlameHalfW);
    const float flameHalfH = layout.DesignLength(kFurnaceFlameHalfH);
    PushRect(out, offset, flameX, flameY, flameHalfW, flameHalfH, scale, kPanelButtonIdleColor);
    float fuelRatio = frame.fuelRatio;
    if (fuelRatio < 0.0f) fuelRatio = 0.0f;
    if (fuelRatio > 1.0f) fuelRatio = 1.0f;
    const float fillHalfH = flameHalfH * fuelRatio;
    if (fillHalfH > 0.0f) {
        PushRect(out, offset, flameX, flameY - flameHalfH + fillHalfH, flameHalfW * 0.7f,
                 fillHalfH, scale, kPanelButtonColor);
    }

    DrawContainerInventoryGrids(out, offset, geometry, scale, frame.backpack, frame.hotbar,
                                artUnit);
    DrawContainerCarried(out, offset, layout, geometry, scale, frame.carried,
                         frame.carriedVisible, frame.carriedX, frame.carriedY);
}

/* 储物箱面板: 27 格容器, 背包与快捷栏. */
void DrawChestPanel(float* out, int32_t& offset, const HudLayout& layout,
                    const ChestPanelFrame& frame, float scale) {
    const ContainerPanelGeometry geometry = MakeContainerPanelGeometry(layout);
    const float artUnit = layout.DesignLength(kHudArtPixel);
    DrawContainerPanelBase(out, offset, layout, geometry, scale, "CHEST");
    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            PushInventorySlot(out, offset, geometry.chestX[column], geometry.chestY[row],
                              geometry.slotHalf, scale, frame.chest[row * 9 + column], artUnit);
        }
    }
    DrawContainerInventoryGrids(out, offset, geometry, scale, frame.backpack, frame.hotbar,
                                artUnit);
    DrawContainerCarried(out, offset, layout, geometry, scale, frame.carried,
                         frame.carriedVisible, frame.carriedX, frame.carriedY);
}

}  /* namespace */
}  /* namespace */

HudRenderer::HudRenderer() = default;

HudRenderer::~HudRenderer() = default;

bool HudRenderer::Initialize(uint32_t atlasTexture, uint32_t uiAtlasTexture) {
    if (ready_) {
        return true;
    }
    atlasTexture_ = atlasTexture;
    uiAtlasTexture_ = uiAtlasTexture;
    uiAtlasAvailable = uiAtlasTexture != 0;
    const uint32_t vertexShader = CompileShader(GL_VERTEX_SHADER, kVertexShader);
    if (vertexShader == 0) {
        return false;
    }
    const uint32_t fragmentShader = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char message[512];
        glGetProgramInfoLog(program_, sizeof(message), nullptr, message);
        IM_LOGE(kTag, "program link failed: %s", message);
        glDeleteProgram(program_);
        program_ = 0;
        return false;
    }
    uniformAtlasLocation_ = glGetUniformLocation(program_, "uAtlas");
    uniformSpriteLocation_ = glGetUniformLocation(program_, "uSprite");
    /* 远端标记绘制复用同一份程序与图集, 生命周期跟随 HudRenderer. */
    program_shared = program_;
    atlasTexture_shared = atlasTexture_;
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, kVertexCapacity * kFloatsPerVertex * sizeof(float), nullptr,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(4 * sizeof(float)));
    glBindVertexArray(0);
    ready_ = true;
    IM_LOGI(kTag, "hud renderer ready, atlas=%u", atlasTexture_);
    return true;
}

void HudRenderer::Shutdown() {
    if (!ready_) {
        return;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    program_shared = 0;
    atlasTexture_shared = 0;
    ready_ = false;
}

void HudRenderer::SetViewport(int32_t width, int32_t height) {
    layout_ = HudLayout(width, height);
}

/*
 * 顶部罗盘.
 * 以玩家朝向为中心, 左右各显示 45 度: 每 15 度一根刻度, 四个正方向
 * (北东南西) 加字母标注, 中心一条竖线表示正前方.
 * 整条罗盘只由纯色矩形与点阵字组成, 不依赖界面图集,
 * 因此图集不在线时照样可读.
 */
void DrawCompass(float* out, int32_t& offset, const HudLayout& layout, float yawRadians,
                 float scale) {
    constexpr float kCenterYNorm = 0.94f;     /* 归一化屏幕纵坐标, 贴顶 */
    constexpr float kHalfWidthNorm = 0.22f;   /* 横条半宽, 以屏宽为 1 */
    constexpr float kHalfHeightNorm = 0.014f; /* 横条半高 */
    constexpr float kHalfFov = 0.7853982f;    /* 左右各 45 度 */
    constexpr float kTwoPi = 6.2831853f;
    constexpr float kPi = 3.14159265f;

    const float centerX = 0.0f;
    const float centerY = layout.DesignY(kCenterYNorm);
    const float halfWidth = layout.DesignLength(kHalfWidthNorm);
    const float halfHeight = layout.DesignLength(kHalfHeightNorm);
    const float track[4] = {0.0f, 0.0f, 0.0f, 0.32f};
    PushRect(out, offset, centerX, centerY, halfWidth, halfHeight, scale, track);

    /* 朝向归一到 0 到 2pi, 正北为 0. */
    float heading = std::fmod(yawRadians, kTwoPi);
    if (heading < 0.0f) {
        heading += kTwoPi;
    }
    /* 24 根刻度对应 15 度一格, 每 6 格一个正方向. */
    for (int32_t step = 0; step < 24; ++step) {
        const float bearing = static_cast<float>(step) * (kTwoPi / 24.0f);
        float delta = bearing - heading;
        while (delta > kPi) {
            delta -= kTwoPi;
        }
        while (delta < -kPi) {
            delta += kTwoPi;
        }
        if (std::fabs(delta) > kHalfFov) {
            continue;
        }
        const bool cardinal = (step % 6) == 0;
        /*
         * 引擎的朝向以 yaw 增大为左转 (视线方向 -sin/-cos), 方位角以顺时针增大,
         * 因此刻度相对中心的位置取负号: 面朝北时东在右侧, 面朝西时北在右侧.
         */
        const float x = centerX - (delta / kHalfFov) * halfWidth;
        const float tickHalf = layout.DesignLength(cardinal ? 0.0022f : 0.0012f);
        const float tickHeight = layout.DesignLength(cardinal ? 0.009f : 0.005f);
        const float color[4] = {1.0f, 1.0f, 1.0f, cardinal ? 0.85f : 0.45f};
        PushRect(out, offset, x, centerY, tickHalf, tickHeight, scale, color);
        if (!cardinal) {
            continue;
        }
        const char* label = "N";
        if (step == 6) {
            label = "E";
        } else if (step == 12) {
            label = "S";
        } else if (step == 18) {
            label = "W";
        }
        const float textSize = layout.DesignLength(0.0016f);
        const float textY = centerY - halfHeight - layout.DesignLength(0.007f);
        PushCenteredText(out, offset, x, textY, textSize, label, scale, color);
    }
    /* 中心竖线: 正前方指示, 用暖色与刻度区分. */
    const float marker[4] = {1.0f, 0.86f, 0.35f, 0.95f};
    PushRect(out, offset, centerX, centerY, layout.DesignLength(0.0008f), halfHeight * 0.9f, scale,
             marker);
}

/*
 * 战斗框: 顶部罗盘下方一条细长血条加目标名称.
 * 只在最近攻击过且目标仍存活时绘制, 名称走点阵字, 血条按血量比例填充.
 * 与罗盘同一套设计坐标, 不依赖界面图集.
 */
void DrawBattleFrame(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                     float scale) {
    if (!frame.targetVisible) {
        return;
    }
    constexpr float kCenterYNorm = 0.075f;
    constexpr float kHalfWidthNorm = 0.16f;
    constexpr float kHalfHeightNorm = 0.011f;
    const float centerX = 0.0f;
    const float centerY = layout.DesignY(kCenterYNorm);
    const float halfWidth = layout.DesignLength(kHalfWidthNorm);
    const float halfHeight = layout.DesignLength(kHalfHeightNorm);
    const float track[4] = {0.0f, 0.0f, 0.0f, 0.45f};
    PushRect(out, offset, centerX, centerY, halfWidth, halfHeight, scale, track);
    const float health = frame.targetHealth < 0.0f ? 0.0f
                         : (frame.targetHealth > 1.0f ? 1.0f : frame.targetHealth);
    if (health > 0.0f) {
        const float fill[4] = {0.86f, 0.28f, 0.26f, 0.95f};
        PushRect(out, offset, centerX - halfWidth + halfWidth * health, centerY, halfWidth * health,
                 halfHeight, scale, fill);
    }
    if (frame.targetName != nullptr) {
        const float textSize = layout.DesignLength(0.0015f);
        const float textY = centerY + halfHeight + layout.DesignLength(0.006f);
        const float white[4] = {1.0f, 1.0f, 1.0f, 0.9f};
        PushCenteredText(out, offset, centerX, textY, textSize, frame.targetName, scale, white);
    }
}

/*
 * 伤害飘字: 战斗框上方一行动态数字, 按进度向上位移并淡出.
 * 点阵字只有 ASCII 字形, 因此只画数值本身, 不带单位.
 */
void DrawDamagePopup(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                     float scale) {
    if (!frame.damageVisible || frame.damageText == nullptr) {
        return;
    }
    constexpr float kRiseNorm = 0.055f;
    const float progress = frame.damageProgress < 0.0f
                               ? 0.0f
                               : (frame.damageProgress > 1.0f ? 1.0f : frame.damageProgress);
    /* 从命中点起向上飘, 坐标由总装按世界投影给出. */
    const float x = layout.DesignX(frame.damageAnchorX);
    const float y = layout.DesignY(frame.damageAnchorY - kRiseNorm * progress);
    /* 后三分之一淡出, 前段保持清晰. */
    const float alpha = progress > 0.66f ? (1.0f - progress) / 0.34f : 1.0f;
    const float color[4] = {1.0f, 0.86f, 0.40f, alpha < 0.0f ? 0.0f : alpha};
    const float textSize = layout.DesignLength(0.0022f);
    PushCenteredText(out, offset, x, y, textSize, frame.damageText, scale, color);
}

/*
 * 暂停面板的按钮矩形, 设计坐标 (原点屏幕中心, 纵轴向上).
 * 绘制与命中判定共用这一份, 两边不会错位.
 */
struct PauseButtonRect {
    float centerX;
    float centerY;
    float halfWidth;
    float halfHeight;
};

constexpr PauseButtonRect kPauseResumeButton = {0.0f, 0.04f, 0.32f, 0.075f};
constexpr PauseButtonRect kPauseQuitButton = {0.0f, -0.13f, 0.32f, 0.075f};

/* 暂停面板: 整屏压暗, 中间一块底板与两个按钮, 文字用点阵字. */
void DrawPauseMenu(float* out, int32_t& offset, const HudLayout& layout, float scale) {
    if (!(scale > 0.0f)) {
        return;
    }
    /* 整屏压暗: 设计坐标的半宽取自缩放系数的倒数, 覆盖整个视口. */
    const float dim[4] = {0.0f, 0.0f, 0.0f, 0.55f};
    PushRect(out, offset, 0.0f, 0.0f, 1.0f / scale, 1.0f, scale, dim);

    const float panelHalfW = 0.46f;
    const float panelHalfH = 0.28f;
    const float board[4] = {0.10f, 0.12f, 0.18f, 0.92f};
    PushRect(out, offset, 0.0f, 0.0f, panelHalfW, panelHalfH, scale, board);

    const float title[4] = {1.0f, 1.0f, 1.0f, 0.95f};
    PushCenteredText(out, offset, 0.0f, 0.17f, 0.022f, "PAUSED", scale, title);

    const float buttonFill[4] = {0.22f, 0.42f, 0.58f, 0.95f};
    const float buttonText[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    PushRect(out, offset, kPauseResumeButton.centerX, kPauseResumeButton.centerY,
             kPauseResumeButton.halfWidth, kPauseResumeButton.halfHeight, scale, buttonFill);
    PushCenteredText(out, offset, kPauseResumeButton.centerX, kPauseResumeButton.centerY, 0.016f,
                     "RESUME", scale, buttonText);

    const float quitFill[4] = {0.52f, 0.24f, 0.24f, 0.95f};
    PushRect(out, offset, kPauseQuitButton.centerX, kPauseQuitButton.centerY,
             kPauseQuitButton.halfWidth, kPauseQuitButton.halfHeight, scale, quitFill);
    PushCenteredText(out, offset, kPauseQuitButton.centerX, kPauseQuitButton.centerY, 0.016f,
                     "QUIT", scale, buttonText);
}

/*
 * 护甲指示: 血条右侧一排小方块, 每块代表两成减伤, 满减伤显示五块.
 * 只画纯色矩形, 不依赖界面图集.
 */
void DrawArmorPips(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                   float scale) {
    if (!(frame.armor > 0.01f)) {
        return;
    }
    constexpr float kStartXNorm = 0.30f;
    constexpr float kStepXNorm = 0.018f;
    constexpr float kCenterYNorm = 0.185f;
    constexpr float kHalfNorm = 0.007f;
    const int32_t filled = frame.armor > 1.0f ? 5 : static_cast<int32_t>(frame.armor * 5.0f + 0.5f);
    for (int32_t index = 0; index < 5; ++index) {
        const float x = layout.DesignX(kStartXNorm + kStepXNorm * static_cast<float>(index));
        const float y = layout.DesignY(kCenterYNorm);
        const float half = layout.DesignLength(kHalfNorm);
        if (index < filled) {
            const float on[4] = {0.62f, 0.78f, 0.92f, 0.95f};
            PushRect(out, offset, x, y, half, half, scale, on);
        } else {
            const float off[4] = {0.0f, 0.0f, 0.0f, 0.35f};
            PushRect(out, offset, x, y, half, half, scale, off);
        }
    }
}

/*
 * Boss 血条: 屏幕上方一条宽底槽加一层前景, 上方一行名称.
 * 只在视野内存在 Boss 时绘制, 与普通战斗框互不影响.
 */
void DrawBossBar(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                 float scale) {
    if (!frame.bossVisible) {
        return;
    }
    constexpr float kCenterYNorm = 0.905f;
    constexpr float kHalfWidthNorm = 0.30f;
    constexpr float kHalfHeightNorm = 0.012f;
    const float x = layout.DesignX(0.0f);
    const float y = layout.DesignY(kCenterYNorm);
    const float halfWidth = layout.DesignLength(kHalfWidthNorm);
    const float halfHeight = layout.DesignLength(kHalfHeightNorm);

    const float track[4] = {0.0f, 0.0f, 0.0f, 0.55f};
    PushRect(out, offset, x, y, halfWidth, halfHeight, scale, track);

    float ratio = frame.bossHealth;
    if (ratio < 0.0f) {
        ratio = 0.0f;
    }
    if (ratio > 1.0f) {
        ratio = 1.0f;
    }
    if (ratio > 0.0f) {
        /* 血量越低越偏红, 与状态条同一套配色思路. */
        const float fill[4] = {0.82f, 0.22f + 0.5f * ratio, 0.24f, 0.95f};
        PushRect(out, offset, x - halfWidth + halfWidth * ratio, y, halfWidth * ratio, halfHeight,
                 scale, fill);
    }
    if (frame.bossName != nullptr) {
        const float label[4] = {1.0f, 0.96f, 0.9f, 0.95f};
        PushCenteredText(out, offset, x, y + layout.DesignLength(0.028f), layout.DesignLength(0.0016f),
                         frame.bossName, scale, label);
    }
}

/*
 * 充能球: 准星下方一个小圆环, 用扇形填充表示蓄力进度.
 * 只用矩形近似圆环, 与其它组件同一套纯色图元.
 */
void DrawChargeOrb(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                   float scale) {
    if (!frame.chargeVisible) {
        return;
    }
    constexpr float kCenterYNorm = -0.045f;
    constexpr float kHalfNorm = 0.026f;
    const float x = layout.DesignX(0.0f);
    const float y = layout.DesignY(kCenterYNorm);
    const float half = layout.DesignLength(kHalfNorm);

    const float track[4] = {0.0f, 0.0f, 0.0f, 0.45f};
    PushRect(out, offset, x, y, half, half, scale, track);
    float ratio = frame.chargeProgress;
    if (ratio < 0.0f) {
        ratio = 0.0f;
    }
    if (ratio > 1.0f) {
        ratio = 1.0f;
    }
    if (ratio > 0.0f) {
        const float fill[4] = {0.30f, 0.78f, 0.92f, 0.92f};
        /* 自下而上填充: 底边固定在中心下方, 高度按进度. */
        const float height = half * 2.0f * ratio;
        PushRect(out, offset, x, y - half + height * 0.5f, half, height * 0.5f, scale, fill);
    }
}

/*
 * 死亡遮罩: 整屏压一层暗红, 中间两行字与一条倒计时条.
 * 点阵字只有 ASCII, 因此用两行英文标签.
 */
void DrawDeathOverlay(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                      float scale) {
    if (!frame.deathVisible || !(scale > 0.0f)) {
        return;
    }
    const float veil[4] = {0.28f, 0.04f, 0.04f, 0.55f};
    PushRect(out, offset, 0.0f, 0.0f, 1.0f / scale, 1.0f, scale, veil);

    const float title[4] = {1.0f, 0.90f, 0.86f, 0.98f};
    PushCenteredText(out, offset, 0.0f, 0.10f, 0.030f, "YOU DIED", scale, title);
    const float hint[4] = {1.0f, 0.86f, 0.80f, 0.85f};
    PushCenteredText(out, offset, 0.0f, -0.02f, 0.014f, "RESPAWNING", scale, hint);

    /* 倒计时条: 底槽铺满, 前景按进度回缩, 表示复活还需多久. */
    const float trackHalf = 0.24f;
    const float trackHalfH = 0.012f;
    const float trackY = -0.10f;
    const float track[4] = {0.0f, 0.0f, 0.0f, 0.45f};
    PushRect(out, offset, 0.0f, trackY, trackHalf, trackHalfH, scale, track);
    const float progress = frame.deathProgress < 0.0f
                               ? 0.0f
                               : (frame.deathProgress > 1.0f ? 1.0f : frame.deathProgress);
    const float fill = 1.0f - progress;
    if (fill > 0.0f) {
        const float fillColor[4] = {0.85f, 0.30f, 0.26f, 0.95f};
        PushRect(out, offset, -trackHalf + trackHalf * fill, trackY, trackHalf * fill, trackHalfH,
                 scale, fillColor);
    }
}

/*
 * 状态效果图标.
 * 左上角血条下方一排小方块: 底色按效果编号折算固定色调, 同一效果每次同色;
 * 图标底部一条剩余时间进度条, 6 秒内线性缩短, 超过 6 秒按满格显示.
 * 只由纯色矩形组成, 不依赖界面图集.
 */
void DrawBuffIcons(float* out, int32_t& offset, const HudLayout& layout, const HudFrame& frame,
                   float scale) {
    constexpr float kStartXNorm = 0.055f;
    constexpr float kStepXNorm = 0.042f;
    constexpr float kCenterYNorm = 0.165f;
    constexpr float kHalfNorm = 0.016f;
    for (int32_t index = 0; index < frame.buffCount && index < HudFrame::kMaxBuffIcons; ++index) {
        const float x = layout.DesignX(kStartXNorm + kStepXNorm * static_cast<float>(index));
        const float y = layout.DesignY(kCenterYNorm);
        const float half = layout.DesignLength(kHalfNorm);
        const uint32_t id = frame.buffIds[index];
        const float tint[4] = {
            0.35f + 0.12f * static_cast<float>(id % 3u),
            0.45f + 0.10f * static_cast<float>((id / 3u) % 3u),
            0.55f + 0.08f * static_cast<float>((id / 9u) % 3u),
            0.92f,
        };
        PushRect(out, offset, x, y, half, half, scale, tint);
        /* 剩余时间条: 底部一条, 6 秒封顶. */
        const float remaining = frame.buffRemaining[index];
        const float ratio = remaining <= 0.0f ? 0.0f : (remaining >= 6.0f ? 1.0f : remaining / 6.0f);
        const float barHalfHeight = half * 0.16f;
        const float barY = y - half + barHalfHeight;
        const float track[4] = {0.0f, 0.0f, 0.0f, 0.45f};
        PushRect(out, offset, x, barY, half, barHalfHeight, scale, track);
        if (ratio > 0.0f) {
            const float fill[4] = {0.95f, 0.92f, 0.55f, 0.95f};
            PushRect(out, offset, x - half + half * ratio, barY, half * ratio, barHalfHeight, scale,
                     fill);
        }
    }
}

void HudRenderer::SubmitLayoutCommands(const std::vector<LayoutDrawCommand>& commands) {
    if (!ready_ || !layout_.Valid() || commands.empty()) {
        return;
    }
    static float vertices[kVertexCapacity * kFloatsPerVertex];
    static std::vector<LayoutDrawCommand> adjusted;
    adjusted = commands;
    /*
     * 图集不在线时把贴图命令降级为纯色, 避免采样到未初始化的通道;
     * 在线时给 u 加通道偏移, 与 PushUiQuad 的处理一致.
     */
    for (LayoutDrawCommand& command : adjusted) {
        if (command.texture == nullptr) {
            continue;
        }
        if (!uiAtlasAvailable) {
            command.texture = nullptr;
        }
    }
    LayoutRenderStats stats;
    const int32_t vertexCount = EmitLayoutCommands(
        adjusted, static_cast<float>(layout_.Width()), static_cast<float>(layout_.Height()), vertices,
        kVertexCapacity, &stats);
    if (vertexCount == 0) {
        return;
    }
    if (uiAtlasAvailable) {
        /* 只对真正带 uv 的顶点加通道偏移, 纯色顶点的 uv 保持 -1. */
        for (int32_t index = 0; index < vertexCount; ++index) {
            float* vertex = vertices + static_cast<size_t>(index) * kFloatsPerVertex;
            if (vertex[2] >= 0.0f) {
                vertex[2] += kUiChannel;
            }
        }
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    if (uniformAtlasLocation_ >= 0 && atlasTexture_ != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_);
        glUniform1i(uniformAtlasLocation_, 0);
    }
    if (uniformSpriteLocation_ >= 0 && uiAtlasTexture_ != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uiAtlasTexture_);
        glUniform1i(uniformSpriteLocation_, 1);
    }
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(vertexCount) * kFloatsPerVertex * sizeof(float),
                    vertices);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void HudRenderer::Render(const HudFrame& frame) {
    if (!ready_ || !layout_.Valid()) {
        return;
    }
    const float scale = layout_.Scale();
    static float vertices[kVertexCapacity * kFloatsPerVertex];
    int32_t offset = 0;

    /*
     * 准星: operate 页的 sight 精灵, 以屏幕中心等比摆放;
     * 界面图集不在线时回落到两条细矩形组成的十字.
     */
    const float white[4] = {1.0f, 1.0f, 1.0f, 0.75f};
    if (uiAtlasAvailable) {
        const gen::UiSpriteInfo& sight = gen::UiSpriteOf(gen::UiSprite::Crosshair);
        const float half = layout_.DesignLength(0.0135f);
        PushUiSprite(vertices, offset, 0.0f, 0.0f, half, half * sight.height / sight.width, scale,
                     sight, white);
    } else {
        const float armHalf = layout_.DesignLength(0.006f);
        const float armThickness = layout_.DesignLength(0.0012f);
        PushRect(vertices, offset, 0.0f, 0.0f, armHalf, armThickness, scale, white);
        PushRect(vertices, offset, 0.0f, 0.0f, armThickness, armHalf, scale, white);
    }

    /*
     * 破坏进度: 准星外围的方框, 按周长填充, 进度归零时整圈不绘制,
     * 因此没有在挖掘时准星保持原样.
     */
    if (frame.mineProgress > 0.0f) {
        const float ringHalf = layout_.DesignLength(kMineRingHalfSize);
        const float ringThickness = layout_.DesignLength(kMineRingThickness);
        const float ringTrack[4] = {0.0f, 0.0f, 0.0f, 0.35f};
        const float ringFill[4] = {1.0f, 0.92f, 0.55f, 0.95f};
        PushSquareProgress(vertices, offset, 0.0f, 0.0f, ringHalf, ringThickness, frame.mineProgress,
                           scale, ringTrack, ringFill);
    }

    /* 顶部罗盘与状态图标: 只在没有打开任何面板时绘制, 面板会盖住它们. */
    if (frame.inventory == nullptr && frame.furnace == nullptr && frame.chest == nullptr) {
        DrawCompass(vertices, offset, layout_, frame.yawRadians, scale);
        DrawBattleFrame(vertices, offset, layout_, frame, scale);
        DrawDamagePopup(vertices, offset, layout_, frame, scale);
        DrawBuffIcons(vertices, offset, layout_, frame, scale);
    }

    /* 一个素材像素的设计长度, 九宫边界与内缩量都按它换算. */
    const float artUnit = layout_.DesignLength(kHudArtPixel);
    /* 界面九宫的边界换算同一标尺, 刷新后界面精灵绘制函数直接读取. */
    artPixelUnit = artUnit;

    /*
     * 血条: 左上角横条. 底槽取img_blood_strip_board 九宫, 实心段取
     * img_blood_strip_red 按血量比例截取, 左端固定, 右端随血量收缩;
     * 界面图集不在线时回落到规范底板色与规范血条色的纯色绘制.
     */
    const float healthHalfWidth = layout_.DesignLength(kHealthHalfWidth);
    const float healthHalfHeight = layout_.DesignLength(kHealthHalfHeight);
    const float healthCenterX = layout_.DesignX(kHealthCenterX);
    const float healthCenterY = layout_.DesignY(kHealthCenterY);
    if (uiAtlasAvailable) {
        PushUiNineSlice(vertices, offset, healthCenterX, healthCenterY, healthHalfWidth,
                        healthHalfHeight, scale, gen::UiSpriteOf(gen::UiSprite::HpTrack),
                        kHudBoardColor);
    } else {
        PushNineSlice(vertices, offset, healthCenterX, healthCenterY, healthHalfWidth,
                      healthHalfHeight, scale, gen::UiAssetOf(gen::UiWidget::MiscBarHpTrack),
                      artUnit, kSolidUv, kHudBoardColor);
    }
    const float health = frame.health < 0.0f ? 0.0f : (frame.health > 1.0f ? 1.0f : frame.health);
    if (health > 0.0f) {
        const float fillHalfWidth = healthHalfWidth * health;
        const float fillHalfHeight = std::fmax(healthHalfHeight - artUnit * kHealthFillInset, 0.0f);
        if (uiAtlasAvailable) {
            const gen::UiSpriteInfo& fill = gen::UiSpriteOf(gen::UiSprite::HpFillRed);
            /* 精灵宽度按血量比例截取, uv 同步截取左段, 左端固定右端收缩. */
            const float fillLeft = healthCenterX - healthHalfWidth;
            const float u0 = fill.u0;
            const float u1 = fill.u0 + (fill.u1 - fill.u0) * health;
            PushUiQuad(vertices, offset, fillLeft, healthCenterY - fillHalfHeight,
                       fillLeft + fillHalfWidth * 2.0f, healthCenterY + fillHalfHeight, u0, fill.v0,
                       u1, fill.v1, scale, kSpriteTint);
        } else {
            PushRect(vertices, offset, healthCenterX - healthHalfWidth + fillHalfWidth,
                     healthCenterY, fillHalfWidth, fillHalfHeight, scale, kHudBloodColor);
        }
    }

    /*
     * 饥饿条与体力条: 与血条共用底槽画法, 不额外引入图集精灵.
     * 两行条同宽 (300 设计像素), 饥饿在血条下一行, 体力在快捷栏上方.
     * 值只画实心段, 空值保留底槽, 与血条语义一致.
     */
    auto drawVitalBar = [&](float centerX, float centerY, float halfWidth, float halfHeight,
                            float ratio, const float fillColor[4]) {
        const float clamped = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
        /* 先铺一层略大的深色外沿, 底槽压在其上, 形成一像素级别的分离描边. */
        const float edge = artUnit * kHealthFillInset;
        PushRect(vertices, offset, centerX, centerY, halfWidth + edge, halfHeight + edge,
                 scale, kVitalStrokeColor);
        if (uiAtlasAvailable) {
            PushUiNineSlice(vertices, offset, centerX, centerY, halfWidth, halfHeight, scale,
                            gen::UiSpriteOf(gen::UiSprite::HpTrack), kHudBoardColor);
        } else {
            PushNineSlice(vertices, offset, centerX, centerY, halfWidth, halfHeight, scale,
                          gen::UiAssetOf(gen::UiWidget::MiscBarHpTrack), artUnit,
                          kSolidUv, kHudBoardColor);
        }
        if (clamped <= 0.0f) {
            return;
        }
        const float fillHalfWidth = halfWidth * clamped;
        const float fillHalfHeight = std::fmax(halfHeight - edge, 0.0f);
        PushRect(vertices, offset, centerX - halfWidth + fillHalfWidth, centerY,
                 fillHalfWidth, fillHalfHeight, scale, fillColor);
    };
    /*
     * 图标与行距全部按条高换算成设计坐标, 与血条主体同一标尺.
     * 纵向设计坐标越小越靠下, 因此饥饿条在血条下方要用减号.
     */
    /*
     * 行距按PlayerFoodLevelBar 锚偏移 30 设计像素折算,
     * 标尺与 kHudArtPixel 同源 (一个素材像素对应一个设计像素).
     */
    const float iconHalf = healthHalfHeight * 1.1f;
    const float iconGap = healthHalfHeight * 0.6f;
    const float rowGap = layout_.DesignLength(30.0f * kHudArtPixel);
    const float iconX = healthCenterX - healthHalfWidth - iconGap - iconHalf;
    /* 血条左端心形近似: 两瓣方块加下方三角, 与填充色同族. */
    PushRect(vertices, offset, iconX - iconHalf * 0.5f,
             healthCenterY - iconHalf * 0.35f, iconHalf * 0.55f,
             iconHalf * 0.45f, scale, kVitalHeartColor);
    PushRect(vertices, offset, iconX + iconHalf * 0.5f,
             healthCenterY - iconHalf * 0.35f, iconHalf * 0.55f,
             iconHalf * 0.45f, scale, kVitalHeartColor);
    PushTriangle(vertices, offset, iconX, healthCenterY + iconHalf * 0.55f,
                 iconHalf, iconHalf * 0.9f, scale, kVitalHeartColor);
    const float hungerY = healthCenterY - rowGap;
    drawVitalBar(healthCenterX, hungerY, healthHalfWidth,
                 healthHalfHeight, frame.hunger, kHungerFillColor);
    /* 饥饿条左端鸡腿近似: 圆头方块加斜向柄, 与填充色同族. */
    PushRect(vertices, offset, iconX, hungerY - iconHalf * 0.2f,
             iconHalf * 0.8f, iconHalf * 0.65f, scale, kVitalFoodColor);
    PushRect(vertices, offset, iconX + iconHalf * 0.75f,
             hungerY + iconHalf * 0.55f, iconHalf * 0.7f,
             iconHalf * 0.22f, scale, kVitalFoodColor);
    /* 与血条严格共用设计坐标系: 横纵都走 DesignX/DesignLength/DesignY,
     * 避免两套标尺混用把条推离预期位置. */
    drawVitalBar(layout_.DesignX(kStaminaCenterX), layout_.DesignY(kStaminaCenterY),
                 layout_.DesignLength(kStaminaHalfWidth),
                 layout_.DesignLength(kStaminaHalfHeight), frame.stamina, kStaminaFillColor);

    /*
     * 浮动摇杆: 环画在控制器回报的锚点上, 也就是这一路的按下点;
     * 滑块按摇杆量沿环半径偏移, 环半径直接取控制器使用的满量程半径,
     * 因此界面看到的幅度与真正参与移动的量一致.
     */
    if (frame.stick.active && frame.stick.radius > 0.0f) {
        const float ringRadius = layout_.DesignLength(frame.stick.radius);
        const float centerX = layout_.DesignX(frame.stick.anchorX);
        const float centerY = layout_.DesignY(frame.stick.anchorY);
        const float baseColor[4] = {1.0f, 1.0f, 1.0f, 0.20f};
        const float knobColor[4] = {1.0f, 1.0f, 1.0f, 0.45f};
        PushRing(vertices, offset, centerX, centerY, ringRadius,
                 layout_.DesignLength(kStickRingThickness), scale, baseColor, 40);
        PushRing(vertices, offset, centerX + frame.stick.valueX * ringRadius,
                 centerY + frame.stick.valueY * ringRadius, ringRadius * kKnobRadiusRatio,
                 ringRadius * kKnobRadiusRatio, scale, knobColor, 24);
    }

    /*
     * 跳跃按钮: 右下角取icon_jump 精灵, 等比放入布局的按钮框;
     * 按下时收缩并压暗作为反馈, 界面图集不在线时回落到方块加箭头的纯色画法.
     */
    const float jumpHalf = layout_.DesignLength(layout_.JumpHalfSize());
    const float jumpCenterX = layout_.DesignX(layout_.JumpCenterX());
    const float jumpCenterY = layout_.DesignY(layout_.JumpCenterY());
    const float jumpDrawHalf = frame.jumpPressed ? jumpHalf * kJumpPressedScale : jumpHalf;
    if (uiAtlasAvailable) {
        const gen::UiSpriteInfo& jump = gen::UiSpriteOf(gen::UiSprite::Jump);
        const float tint[4] = {1.0f, 1.0f, 1.0f, frame.jumpPressed ? 0.55f : 1.0f};
        PushUiSprite(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf,
                     jumpDrawHalf * jump.height / jump.width, scale, jump, tint);
    } else {
        const float jumpBackdrop[4] = {0.0f, 0.0f, 0.0f, 0.32f};
        const float jumpBody[4] = {1.0f, 1.0f, 1.0f, frame.jumpPressed ? 0.52f : 0.22f};
        const float jumpArrow[4] = {1.0f, 1.0f, 1.0f, frame.jumpPressed ? 0.95f : 0.62f};
        PushRect(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf, jumpDrawHalf, scale,
                 jumpBackdrop);
        PushRect(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf * 0.86f,
                 jumpDrawHalf * 0.86f, scale, jumpBody);
        PushTriangle(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf * 0.42f,
                     jumpDrawHalf * 0.62f, scale, jumpArrow);
    }

    /* 方块栏: 每格图集图标取自共享槽位表, 选中格加亮并描边. */
    const float slotHalf = layout_.DesignLength(layout_.SlotHalfSize());
    const float slotCenterY = layout_.DesignY(layout_.SlotCenterY());
    const float slotTint[4] = {1.0f, 1.0f, 1.0f, 0.92f};
    const float selectedTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float slotBorder[4] = {1.0f, 0.92f, 0.55f, 1.0f};
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const float centerX = layout_.DesignX(layout_.SlotCenterX(slot));
        const bool selected = slot == frame.selectedSlot;
        const int32_t count = frame.slotCounts[slot] > 0 ? frame.slotCounts[slot] : 0;
        /* 数量为 0 的格整体压暗, 提示这一格已经耗尽, 不能再用作放置材料. */
        const bool spent = count == 0;
        /*
         * 底板取img_item_box 九宫, 选中格换成 img_item_box_selected;
         * 界面图集不在线时回落到纯色九宫, 保持与背包面板同一套底板来源.
         */
        if (uiAtlasAvailable) {
            PushUiNineSlice(vertices, offset, centerX, slotCenterY, slotHalf * 1.05f,
                            slotHalf * 1.05f, scale,
                            gen::UiSpriteOf(selected ? gen::UiSprite::SlotSelected
                                                     : gen::UiSprite::SlotNormal),
                            spent ? kSlotEmptyBoard : kSpriteTint);
        } else {
            PushNineSlice(vertices, offset, centerX, slotCenterY, slotHalf * 1.05f,
                          slotHalf * 1.05f, scale, gen::UiAssetOf(gen::UiWidget::SlotItemFrame),
                          artUnit, kSolidUv, spent ? kSlotEmptyBoard : kHudBoardColor);
        }
        const bool hasItem = frame.slotHasItem[slot];
        const int32_t iconCell = hasItem ? frame.slotAtlasCell[slot] : -1;
        const char initial = hasItem ? frame.slotInitial[slot] : '\0';
        if (!hasItem) {
            /* 空格只保留底板: 具体是哪种物品已经不在格子里, 不再画图标. */
        } else if (iconCell >= 0) {
            PushSprite(vertices, offset, centerX, slotCenterY, slotHalf, scale, iconCell,
                       spent ? kSlotEmptyTint : (selected ? selectedTint : slotTint));
        } else if (initial != '\0') {
            /* 工具与材料没有方块图集格, 用类别色块加首字母占位. */
            const uint8_t tone = frame.slotTone[slot];
            PushRect(vertices, offset, centerX, slotCenterY, slotHalf * 0.72f, slotHalf * 0.72f,
                     scale, kToneColors[tone < 6 ? tone : 0]);
            const char label[2] = {initial, '\0'};
            PushCenteredText(vertices, offset, centerX, slotCenterY, slotHalf * 0.18f, label, scale,
                             spent ? kCountEmptyColor : kPanelTextColor);
        } else {
            PushSprite(vertices, offset, centerX, slotCenterY, slotHalf, scale,
                       BlockSlotAtlasCell(slot),
                       spent ? kSlotEmptyTint : (selected ? selectedTint : slotTint));
        }
        if (selected) {
            const float edge = slotHalf * 0.07f;
            PushRect(vertices, offset, centerX, slotCenterY + slotHalf, slotHalf, edge, scale,
                     slotBorder);
            PushRect(vertices, offset, centerX, slotCenterY - slotHalf, slotHalf, edge, scale,
                     slotBorder);
            PushRect(vertices, offset, centerX - slotHalf, slotCenterY, edge, slotHalf, scale,
                     slotBorder);
            PushRect(vertices, offset, centerX + slotHalf, slotCenterY, edge, slotHalf, scale,
                     slotBorder);
        }

        /* 附魔标记: 与背包面板同一套右上角亮点. */
        if (frame.slotEnchanted[slot]) {
            const float markHalf = slotHalf * 0.16f;
            const float markColor[4] = {0.55f, 0.90f, 0.98f, 0.95f};
            PushRect(vertices, offset, centerX + slotHalf * 0.70f, slotCenterY + slotHalf * 0.70f,
                     markHalf, markHalf, scale, markColor);
        }

        /*
         * 耐久条画在图标与选中框之后: 图标会盖住格子中间, 条必须最后画才看得见.
         * 长度按剩余比例, 颜色随剩余量由绿转黄再转红.
         */
        const int32_t maxDurability = frame.slotMaxDurability[slot];
        if (maxDurability > 0 && frame.slotDurability[slot] >= 0) {
            const float barHalf = slotHalf * 0.78f;
            const float barY = slotCenterY - slotHalf * 0.78f;
            const float barThickness = slotHalf * 0.10f;
            const float ratio = static_cast<float>(frame.slotDurability[slot]) /
                                static_cast<float>(maxDurability);
            const float clamped = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
            PushRect(vertices, offset, centerX, barY, barHalf, barThickness, scale,
                     kDurabilityTrack);
            if (clamped > 0.0f) {
                const float filledHalf = barHalf * clamped;
                const float filledX = centerX - barHalf + filledHalf;
                const float* color = clamped > 0.5f
                                         ? kDurabilityGood
                                         : (clamped > 0.25f ? kDurabilityWarn : kDurabilityLow);
                PushRect(vertices, offset, filledX, barY, filledHalf, barThickness, scale, color);
            }
        }

        /* 数量贴齐槽位右下角并右对齐, 0 也照常绘出, 与压暗的图标一起说明该格已耗尽. */
        char countText[8];
        std::snprintf(countText, sizeof(countText), "%d", count);
        const float countPixel = layout_.DesignLength(kCountPixelSize);
        const float countWidth = MeasureText(countText, countPixel);
        const float countRight = centerX + slotHalf * kCountInsetRatio;
        const float countBottom = slotCenterY - slotHalf * kCountInsetRatio;
        const float countTop = countBottom + static_cast<float>(kGlyphHeight) * countPixel;
        /* 底板比文字各外扩半个点阵像素, 亮色图标上的白色数字也有足够对比. */
        const float backdropPad = countPixel * 0.5f;
        PushRect(vertices, offset, countRight - countWidth * 0.5f,
                 (countBottom + countTop) * 0.5f, countWidth * 0.5f + backdropPad,
                 (countTop - countBottom) * 0.5f + backdropPad, scale, kCountBackdrop);
        PushText(vertices, offset, countRight - countWidth, countTop, countPixel, countText, scale,
                 spent ? kCountEmptyColor : kCountColor);
    }

    /* 长按方块栏时在栏位上方显示方块名, 选中状态不因此改变. */
    if (frame.nameSlot >= 0 && frame.nameSlot < kSlotCount) {
        char text[32];
        const char* source = frame.nameText != nullptr ? frame.nameText : BlockSlotName(frame.nameSlot);
        ToUpper(source, text, sizeof(text));
        const float pixel = layout_.DesignLength(kNamePixelSize);
        const float textWidth = MeasureText(text, pixel);
        const float padding = layout_.DesignLength(kNamePadding);
        const float plateHalfWidth = textWidth * 0.5f + padding;
        const float plateHalfHeight = static_cast<float>(kGlyphHeight) * pixel * 0.5f + padding * 0.5f;
        const float plateCenterX = layout_.DesignX(layout_.SlotCenterX(frame.nameSlot));
        const float plateCenterY = slotCenterY + layout_.DesignLength(layout_.SlotHalfSize()) +
                                   layout_.DesignLength(kNamePlateGap) + plateHalfHeight;
        /*
         * 方块名标签底板取board_tips_icon 九宫, 图集不在线时回落到
         * 深棕黑纯色底板, 两种画法的文字排版一致.
         */
        if (uiAtlasAvailable) {
            PushUiNineSlice(vertices, offset, plateCenterX, plateCenterY, plateHalfWidth,
                            plateHalfHeight, scale, gen::UiSpriteOf(gen::UiSprite::TipsBoard),
                            kSpriteTint);
        } else {
            PushNineSlice(vertices, offset, plateCenterX, plateCenterY, plateHalfWidth,
                          plateHalfHeight, scale, gen::UiAssetOf(gen::UiWidget::DialogTipsBody),
                          artUnit, kSolidUv, kHudTipsColor);
        }
        PushText(vertices, offset, plateCenterX - textWidth * 0.5f,
                 plateCenterY + static_cast<float>(kGlyphHeight) * pixel * 0.5f, pixel, text,
                 scale, white);
    }

    /*
     * 背包按钮: 右上角常驻, 取icon_backpack 精灵居中等比摆放,
     * 底板沿用img_item_box 九宫, 按下时压暗底板作为反馈;
     * 界面图集不在线时回落到九宫底板加 BAG 点阵字的旧画法.
     */
    const InventoryPanelGeometry bagGeometry = MakeInventoryPanelGeometry(layout_);
    if (uiAtlasAvailable) {
        const float tint[4] = {1.0f, 1.0f, 1.0f, frame.bagPressed ? 0.60f : 1.0f};
        PushUiNineSlice(vertices, offset, bagGeometry.bagX, bagGeometry.bagY, bagGeometry.bagHalf,
                        bagGeometry.bagHalf, scale, gen::UiSpriteOf(gen::UiSprite::SlotNormal),
                        tint);
        const gen::UiSpriteInfo& bag = gen::UiSpriteOf(gen::UiSprite::Backpack);
        const float iconHalf = bagGeometry.bagHalf * 0.72f;
        PushUiSprite(vertices, offset, bagGeometry.bagX, bagGeometry.bagY, iconHalf,
                     iconHalf * bag.height / bag.width, scale, bag, kSpriteTint);
    } else {
        PushNineSlice(vertices, offset, bagGeometry.bagX, bagGeometry.bagY, bagGeometry.bagHalf,
                      bagGeometry.bagHalf, scale, gen::UiAssetOf(gen::UiWidget::SlotItemFrame),
                      artUnit, kSolidUv, frame.bagPressed ? kHudTipsColor : kHudBoardColor);
        PushCenteredText(vertices, offset, bagGeometry.bagX, bagGeometry.bagY,
                         bagGeometry.bagHalf * 0.58f, "BAG", scale, kPanelTextColor);
    }

    /*
     * 操作提示: 数量耗尽这类反馈画在方块栏正上方, 高度比方块名标签再抬一层,
     * 两者同时出现时不会互相压字.
     */
    if (frame.tipText != nullptr && frame.tipText[0] != '\0') {
        char text[32];
        ToUpper(frame.tipText, text, sizeof(text));
        const float pixel = layout_.DesignLength(kTipPixelSize);
        const float textWidth = MeasureText(text, pixel);
        const float padding = layout_.DesignLength(kNamePadding);
        const float plateHalfWidth = textWidth * 0.5f + padding;
        const float plateHalfHeight =
            static_cast<float>(kGlyphHeight) * pixel * 0.5f + padding * 0.5f;
        const float plateCenterY = slotCenterY + layout_.DesignLength(layout_.SlotHalfSize()) +
                                   layout_.DesignLength(kTipGap) + plateHalfHeight;
        /* 操作提示条与方块名标签共用同一张board_tips_icon 九宫. */
        if (uiAtlasAvailable) {
            PushUiNineSlice(vertices, offset, 0.0f, plateCenterY, plateHalfWidth, plateHalfHeight,
                            scale, gen::UiSpriteOf(gen::UiSprite::TipsBoard), kSpriteTint);
        } else {
            PushNineSlice(vertices, offset, 0.0f, plateCenterY, plateHalfWidth, plateHalfHeight,
                          scale, gen::UiAssetOf(gen::UiWidget::DialogTipsBody), artUnit, kSolidUv,
                          kHudTipsColor);
        }
        PushText(vertices, offset, -textWidth * 0.5f,
                 plateCenterY + static_cast<float>(kGlyphHeight) * pixel * 0.5f, pixel, text,
                 scale, white);
    }

    /* 背包面板画在最后, 面板打开时覆盖其它 HUD 元素. */
    if (frame.inventory != nullptr) {
        DrawInventoryPanel(vertices, offset, layout_, *frame.inventory, scale);
    }
    /* 容器面板与背包面板互斥打开, 画在其后保证压在最上层. */
    if (frame.furnace != nullptr) {
        DrawFurnacePanel(vertices, offset, layout_, *frame.furnace, scale);
    }
    if (frame.chest != nullptr) {
        DrawChestPanel(vertices, offset, layout_, *frame.chest, scale);
    }

    if (offset == 0) {
        return;
    }

    /* 2D 覆盖层: 关闭深度与剔除, 开启混合. */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program_);
    /* 0 号单元挂方块图集, 1 号单元挂界面图集, 两个采样器各占一路. */
    if (uniformAtlasLocation_ >= 0 && atlasTexture_ != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_);
        glUniform1i(uniformAtlasLocation_, 0);
    }
    if (uniformSpriteLocation_ >= 0 && uiAtlasTexture_ != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uiAtlasTexture_);
        glUniform1i(uniformSpriteLocation_, 1);
    }
    /* 护甲指示与状态条同区, 先于其它常规组件绘制. */
    DrawArmorPips(vertices, offset, layout_, frame, scale);

    /* Boss 血条与充能球属于常规 HUD, 先画在遮罩之下. */
    DrawBossBar(vertices, offset, layout_, frame, scale);
    DrawChargeOrb(vertices, offset, layout_, frame, scale);

    /* 死亡遮罩压在常规 HUD 之上, 暂停面板再压在它之上. */
    DrawDeathOverlay(vertices, offset, layout_, frame, scale);

    /* 暂停面板最后画: 压在所有 HUD 之上, 与命中判定共用同一份按钮矩形. */
    if (frame.pauseMenuVisible) {
        DrawPauseMenu(vertices, offset, layout_, scale);
    }

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(offset) * kFloatsPerVertex * sizeof(float), vertices);
    glDrawArrays(GL_TRIANGLES, 0, offset);
    glBindVertexArray(0);
    /* 把激活单元还原到 0 号, 后续渲染代码默认在 0 号单元上取纹理. */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

/*
 * 远端玩家占位标记: 世界坐标竖条.
 * 复用 HUD 的共享着色程序与顶点格式, 独立顶点缓冲, 不触碰 HudRenderer 的实例状态;
 * 投影换算与编辑器覆盖层同一套约定: 列主序矩阵, 设计坐标长度取屏高的一半.
 */
void RenderRemoteMarkers(const float* xs, const float* ys, const float* zs, const float* hps,
                         int32_t count, const float* viewProjection, int32_t width,
                         int32_t height) {
    if (viewProjection == nullptr || width <= 0 || height <= 0) {
        return;
    }
    const HudLayout layout(width, height);
    const float scale = layout.Scale();
    if (count == 0) {
        return;
    }
    if (xs == nullptr || ys == nullptr || zs == nullptr) {
        return;
    }

    /* 竖条几何: 底在脚底, 顶到约两人高, 线宽取屏宽的千分之六, 远近都认得出. */
    const float halfWidth = layout.DesignLength(0.003f);
    const float bottomLift = 0.05f;   /* 底端略抬, 避免插进地面方块纹理里 */
    const float topHeight = 1.9f;     /* 与玩家碰撞盒高度同量级 */

    static float vertices[kVertexCapacity * kFloatsPerVertex];
    int32_t offset = 0;

    for (int32_t index = 0; index < count && offset + 6 <= kVertexCapacity; ++index) {
        const float wx = xs[index];
        const float wy = ys[index];
        const float wz = zs[index];
        const float bx = wx;
        const float by = wy + bottomLift;
        const float bz = wz;
        const float tx = wx;
        const float ty = wy + topHeight;
        const float tz = wz;

        /* 列主序投影, 相机背后的点剔除; 与 PushBoxOutline 同一套换算. */
        const float cx0 = viewProjection[0] * bx + viewProjection[4] * by +
                          viewProjection[8] * bz + viewProjection[12];
        const float cy0 = viewProjection[1] * bx + viewProjection[5] * by +
                          viewProjection[9] * bz + viewProjection[13];
        const float cx1 = viewProjection[0] * tx + viewProjection[4] * ty +
                          viewProjection[8] * tz + viewProjection[12];
        const float cy1 = viewProjection[1] * tx + viewProjection[5] * ty +
                          viewProjection[9] * tz + viewProjection[13];
        const float cw = viewProjection[3] * bx + viewProjection[7] * by +
                         viewProjection[11] * bz + viewProjection[15];
        if (cw <= 0.0001f) {
            continue;
        }
        const float invW = 1.0f / cw;
        const float x0 = (cx0 * invW) / (scale > 0.0001f ? scale : 1.0f);
        const float y0 = cy0 * invW;
        const float x1 = (cx1 * invW) / (scale > 0.0001f ? scale : 1.0f);
        const float y1 = cy1 * invW;

        /* 屏外裁剪: 两个端点都越出同一侧才丢弃, 单侧越出交给硬件裁剪. */
        if ((x0 < -1.2f && x1 < -1.2f) || (x0 > 1.2f && x1 > 1.2f) ||
            (y0 < -1.2f && y1 < -1.2f) || (y0 > 1.2f && y1 > 1.2f)) {
            continue;
        }

        /* 屏内简化: 以两端的中点做方形端帽, 端帽宽度随距离投影自然变化. */
        const float midX = (x0 + x1) * 0.5f;
        const float midY = (y0 + y1) * 0.5f;
        const float capHalf = halfWidth * 1.6f;

        PushLine(vertices, offset, x0, y0, x1, y1, halfWidth * 2.0f, scale, kRemoteMarkerColor);
        PushRect(vertices, offset, midX, midY, capHalf, capHalf, scale, kRemoteMarkerColor);

        /*
         * 血条: 画在标记顶端一小段横条上, 底槽常驻, 前景按血量比例回缩.
         * hps 为空时不画, 与旧版对端兼容.
         */
        if (hps != nullptr) {
            float ratio = hps[index];
            ratio = ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
            const float barHalf = halfWidth * 6.0f;
            const float barHalfHeight = halfWidth * 0.8f;
            const float barY = y1 + barHalfHeight * 3.0f;
            const float trackColor[4] = {0.0f, 0.0f, 0.0f, 0.55f};
            PushRect(vertices, offset, x1, barY, barHalf, barHalfHeight, scale, trackColor);
            if (ratio > 0.0f) {
                const float fillColor[4] = {0.35f + 0.55f * ratio, 0.75f * ratio + 0.1f, 0.25f, 0.95f};
                PushRect(vertices, offset, x1 - barHalf + barHalf * ratio, barY, barHalf * ratio,
                         barHalfHeight, scale, fillColor);
            }
        }
    }

    if (offset == 0) {
        return;
    }

    /* 与 HUD 相同的覆盖层状态: 关深度与剔除, 开混合, 画完恢复. */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program_shared);
    const GLint atlasLocation = glGetUniformLocation(program_shared, "uAtlas");
    if (atlasLocation >= 0 && atlasTexture_shared != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_shared);
        glUniform1i(atlasLocation, 0);
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(offset) * kFloatsPerVertex * sizeof(float), vertices,
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(4 * sizeof(float)));
    glDrawArrays(GL_TRIANGLES, 0, offset);
    glBindVertexArray(0);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

bool HudRenderer::HitTestBag(float normalizedX, float normalizedY) const {
    if (!layout_.Valid()) {
        return false;
    }
    const InventoryPanelGeometry geometry = MakeInventoryPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);
    return InsideBox(x, y, geometry.bagX, geometry.bagY, geometry.bagHalf, geometry.bagHalf);
}

int32_t HudRenderer::HitTestPauseMenu(float normalizedX, float normalizedY) const {
    if (!layout_.Valid()) {
        return 0;
    }
    const float x = normalizedX * 2.0f - 1.0f;
    const float y = 1.0f - normalizedY * 2.0f;
    /* 按钮矩形是设计坐标: 归一化坐标先乘 2 减 1 再按缩放系数折算横向. */
    const float designX = layout_.Scale() > 0.0001f ? x / layout_.Scale() : x;
    const float designY = y;
    const auto inside = [designX, designY](const PauseButtonRect& rect) {
        return designX >= rect.centerX - rect.halfWidth &&
               designX <= rect.centerX + rect.halfWidth &&
               designY >= rect.centerY - rect.halfHeight &&
               designY <= rect.centerY + rect.halfHeight;
    };
    if (inside(kPauseResumeButton)) {
        return 1;
    }
    if (inside(kPauseQuitButton)) {
        return 2;
    }
    return 0;
}

bool HudRenderer::HitTestInventoryPanel(float normalizedX, float normalizedY) const {
    if (!layout_.Valid()) {
        return false;
    }
    const InventoryPanelGeometry geometry = MakeInventoryPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);
    return InsideBox(x, y, geometry.centerX, geometry.centerY, geometry.halfWidth,
                     geometry.halfHeight);
}

InventoryHit HudRenderer::HitTestInventory(float normalizedX, float normalizedY) const {
    InventoryHit hit;
    if (!layout_.Valid()) {
        return hit;
    }
    const InventoryPanelGeometry geometry = MakeInventoryPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);

    /* 关闭按钮与两个配方按钮先判, 它们压在底板之上, 与绘制顺序一致. */
    if (InsideBox(x, y, geometry.closeX, geometry.closeY, geometry.closeHalf,
                  geometry.closeHalf)) {
        hit.region = InventoryRegion::kClose;
        return hit;
    }
    if (InsideBox(x, y, geometry.nextX, geometry.nextY, geometry.buttonHalfW,
                  geometry.buttonHalfH)) {
        hit.region = InventoryRegion::kNextRecipe;
        return hit;
    }
    if (InsideBox(x, y, geometry.bookX, geometry.bookY, geometry.buttonHalfW,
                  geometry.buttonHalfH)) {
        hit.region = InventoryRegion::kBookRecipe;
        return hit;
    }
    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 3; ++column) {
            if (InsideBox(x, y, geometry.craftX[column], geometry.craftY[row], geometry.slotHalf,
                          geometry.slotHalf)) {
                hit.region = InventoryRegion::kCrafting;
                hit.index = row * 3 + column;
                return hit;
            }
        }
    }
    if (InsideBox(x, y, geometry.resultX, geometry.resultY, geometry.resultHalf,
                  geometry.resultHalf)) {
        hit.region = InventoryRegion::kResult;
        hit.index = 0;
        return hit;
    }
    for (int32_t row = 0; row < 4; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            if (InsideBox(x, y, geometry.backpackX[column], geometry.backpackY[row],
                          geometry.slotHalf, geometry.slotHalf)) {
                hit.region = InventoryRegion::kBackpack;
                hit.index = row * 9 + column;
                return hit;
            }
        }
    }
    for (int32_t column = 0; column < 9; ++column) {
        if (InsideBox(x, y, geometry.hotbarX[column], geometry.hotbarY, geometry.slotHalf,
                      geometry.slotHalf)) {
            hit.region = InventoryRegion::kHotbar;
            hit.index = column;
            return hit;
        }
    }
    if (InsideBox(x, y, geometry.centerX, geometry.centerY, geometry.halfWidth,
                  geometry.halfHeight)) {
        hit.region = InventoryRegion::kPanel;
    }
    return hit;
}

/*
 * 容器面板命中判定组. 三个方法与背包面板的命中同一套返回结构,
 * 命中顺序与绘制顺序一致: 关闭按钮最上, 其次三格或 27 格, 再次背包与快捷栏,
 * 底板兜底吃掉点击避免穿透到世界.
 */
bool HudRenderer::HitTestContainerPanel(float normalizedX, float normalizedY) const {
    if (!layout_.Valid()) {
        return false;
    }
    const ContainerPanelGeometry geometry = MakeContainerPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);
    return InsideBox(x, y, geometry.centerX, geometry.centerY, geometry.halfWidth,
                     geometry.halfHeight);
}

InventoryHit HudRenderer::HitTestFurnace(float normalizedX, float normalizedY) const {
    InventoryHit hit;
    if (!layout_.Valid()) {
        return hit;
    }
    const ContainerPanelGeometry geometry = MakeContainerPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);
    if (InsideBox(x, y, geometry.closeX, geometry.closeY, geometry.closeHalf,
                  geometry.closeHalf)) {
        hit.region = InventoryRegion::kClose;
        return hit;
    }
    if (InsideBox(x, y, geometry.inputX, geometry.inputY, geometry.slotHalf,
                  geometry.slotHalf)) {
        hit.region = InventoryRegion::kFurnaceInput;
        hit.index = 0;
        return hit;
    }
    if (InsideBox(x, y, geometry.fuelX, geometry.fuelY, geometry.slotHalf,
                  geometry.slotHalf)) {
        hit.region = InventoryRegion::kFurnaceFuel;
        hit.index = 0;
        return hit;
    }
    if (InsideBox(x, y, geometry.outputX, geometry.outputY, geometry.outputHalf,
                  geometry.outputHalf)) {
        hit.region = InventoryRegion::kFurnaceOutput;
        hit.index = 0;
        return hit;
    }
    for (int32_t row = 0; row < 4; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            if (InsideBox(x, y, geometry.backpackX[column], geometry.backpackY[row],
                          geometry.slotHalf, geometry.slotHalf)) {
                hit.region = InventoryRegion::kBackpack;
                hit.index = row * 9 + column;
                return hit;
            }
        }
    }
    for (int32_t column = 0; column < 9; ++column) {
        if (InsideBox(x, y, geometry.hotbarX[column], geometry.hotbarY, geometry.slotHalf,
                      geometry.slotHalf)) {
            hit.region = InventoryRegion::kHotbar;
            hit.index = column;
            return hit;
        }
    }
    if (InsideBox(x, y, geometry.centerX, geometry.centerY, geometry.halfWidth,
                  geometry.halfHeight)) {
        hit.region = InventoryRegion::kPanel;
    }
    return hit;
}

InventoryHit HudRenderer::HitTestChest(float normalizedX, float normalizedY) const {
    InventoryHit hit;
    if (!layout_.Valid()) {
        return hit;
    }
    const ContainerPanelGeometry geometry = MakeContainerPanelGeometry(layout_);
    const float x = layout_.DesignX(normalizedX);
    const float y = layout_.DesignY(normalizedY);
    if (InsideBox(x, y, geometry.closeX, geometry.closeY, geometry.closeHalf,
                  geometry.closeHalf)) {
        hit.region = InventoryRegion::kClose;
        return hit;
    }
    for (int32_t row = 0; row < 3; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            if (InsideBox(x, y, geometry.chestX[column], geometry.chestY[row],
                          geometry.slotHalf, geometry.slotHalf)) {
                hit.region = InventoryRegion::kChest;
                hit.index = row * 9 + column;
                return hit;
            }
        }
    }
    for (int32_t row = 0; row < 4; ++row) {
        for (int32_t column = 0; column < 9; ++column) {
            if (InsideBox(x, y, geometry.backpackX[column], geometry.backpackY[row],
                          geometry.slotHalf, geometry.slotHalf)) {
                hit.region = InventoryRegion::kBackpack;
                hit.index = row * 9 + column;
                return hit;
            }
        }
    }
    for (int32_t column = 0; column < 9; ++column) {
        if (InsideBox(x, y, geometry.hotbarX[column], geometry.hotbarY, geometry.slotHalf,
                      geometry.slotHalf)) {
            hit.region = InventoryRegion::kHotbar;
            hit.index = column;
            return hit;
        }
    }
    if (InsideBox(x, y, geometry.centerX, geometry.centerY, geometry.halfWidth,
                  geometry.halfHeight)) {
        hit.region = InventoryRegion::kPanel;
    }
    return hit;
}

/*
 * 界面图集随包路径, 由 gen_atlas_ui.py 生成, pack_assets.py 复制进 APK.
 * 裸 RGBA8 数据, 无文件头, 行序与生成器画布一致.
 */
constexpr const char* kUiAtlasAssetPath = "ui/generated/atlas_ui.rgba";
constexpr int32_t kUiAtlasPixels = 2048;
constexpr size_t kUiAtlasBytes =
    static_cast<size_t>(kUiAtlasPixels) * static_cast<size_t>(kUiAtlasPixels) * 4u;

uint32_t LoadUiAtlasFromAssets(AAssetManager* assets) {
    if (assets == nullptr || !HasCurrentContext()) {
        IM_LOGW(kTag, "ui atlas skipped, no asset manager or no graphics context");
        return 0;
    }
    AAsset* asset = AAssetManager_open(assets, kUiAtlasAssetPath, AASSET_MODE_STREAMING);
    if (asset == nullptr) {
        IM_LOGW(kTag, "ui atlas '%s' missing, hud sprites fall back to solid color",
                kUiAtlasAssetPath);
        return 0;
    }
    const off64_t length = AAsset_getLength64(asset);
    if (length != static_cast<off64_t>(kUiAtlasBytes)) {
        IM_LOGW(kTag, "ui atlas '%s' size mismatch, got %lld expected %zu", kUiAtlasAssetPath,
                static_cast<long long>(length), kUiAtlasBytes);
        AAsset_close(asset);
        return 0;
    }
    std::vector<uint8_t> pixels(kUiAtlasBytes);
    size_t total = 0;
    while (total < kUiAtlasBytes) {
        const size_t request = kUiAtlasBytes - total > 1u << 20 ? 1u << 20 : kUiAtlasBytes - total;
        const int32_t read = AAsset_read(asset, pixels.data() + total, request);
        if (read <= 0) {
            break;
        }
        total += static_cast<size_t>(read);
    }
    AAsset_close(asset);
    if (total != kUiAtlasBytes) {
        IM_LOGW(kTag, "ui atlas '%s' read %zu of %zu bytes", kUiAtlasAssetPath, total,
                kUiAtlasBytes);
        return 0;
    }
    /*
     * 生成器画布的首行是精灵图案的上边缘, 而 glTexImage2D 把数据首行放到 v=0,
     * 采样约定 v 向上为正; 这里按整行顺序反转, 让图案在界面上正立,
     * 生成的 uv 已经按翻转后的坐标给出, 无需在顶点侧换算.
     */
    std::vector<uint8_t> flipped(kUiAtlasBytes);
    constexpr size_t kRowBytes = static_cast<size_t>(kUiAtlasPixels) * 4u;
    for (int32_t row = 0; row < kUiAtlasPixels; ++row) {
        std::memcpy(flipped.data() + static_cast<size_t>(row) * kRowBytes,
                    pixels.data() + static_cast<size_t>(kUiAtlasPixels - 1 - row) * kRowBytes,
                    kRowBytes);
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    /* 线性采样让精灵在缩放时平滑, 边缘夹紧避免 uv 越界取到邻格. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kUiAtlasPixels, kUiAtlasPixels, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, flipped.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    if (DrainGlError("ui atlas")) {
        glDeleteTextures(1, &texture);
        return 0;
    }
    IM_LOGI(kTag, "ui atlas ready, %dx%d", kUiAtlasPixels, kUiAtlasPixels);
    return texture;
}



}  /* namespace immirror::ui */
