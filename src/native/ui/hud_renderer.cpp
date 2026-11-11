#include "ui/hud_renderer.h"

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

/* 方块图集为 16x16 格. */
constexpr float kAtlasGrid = 16.0f;

/*
 * 纹理通道编码: 顶点 uv 复用一对 float, 按 uv.x 的取值范围区分三路来源.
 * uv.x < 0                纯色矩形, 不采样
 * 0 <= uv.x < kUiChannel  方块图集, 采样 uAtlas
 * uv.x >= kUiChannel      界面图集, 采样 uSprite, 采样前减回偏移
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
/* #465677: HUD 底板, 原版 img_item 的槽位底色, 与素材一致取不透明 */
constexpr float kHudBoardColor[4] = {0x46 / 255.0f, 0x56 / 255.0f, 0x77 / 255.0f, 1.0f};
/* #20180F: 提示条底, 原版 board_tips_icon 的深棕黑, 同样不透明 */
constexpr float kHudTipsColor[4] = {0x20 / 255.0f, 0x18 / 255.0f, 0x0F / 255.0f, 1.0f};
/* #D61919: 血条实心段, 原版 img_blood_strip_red */
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
     * 底板取原版 img_item_box 九宫, 高亮格换成 img_item_box_selected;
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
     * 面板底板取原版 board_bottom_blue_big 九宫 (texture2/common 的 60x60 大板),
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
     * 关闭按钮: 取原版 btn_mini_orange 三片底板加 X 字, 面板内唯一必然可用的出口;
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
     * 两个配方按钮取原版 btn_mini_blue 三片底板: NEXT 轮换候选常亮,
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

void HudRenderer::Render(const HudFrame& frame) {
    if (!ready_ || !layout_.Valid()) {
        return;
    }
    const float scale = layout_.Scale();
    static float vertices[kVertexCapacity * kFloatsPerVertex];
    int32_t offset = 0;

    /*
     * 准星: 原版 operate 页的 sight 精灵, 以屏幕中心等比摆放;
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

    /* 一个素材像素的设计长度, 九宫边界与内缩量都按它换算. */
    const float artUnit = layout_.DesignLength(kHudArtPixel);
    /* 界面九宫的边界换算同一标尺, 刷新后界面精灵绘制函数直接读取. */
    artPixelUnit = artUnit;

    /*
     * 血条: 左上角横条. 底槽取原版 img_blood_strip_board 九宫, 实心段取
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
     * 跳跃按钮: 右下角取原版 icon_jump 精灵, 等比放入布局的按钮框;
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
         * 底板取原版 img_item_box 九宫, 选中格换成 img_item_box_selected;
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
         * 方块名标签底板取原版 board_tips_icon 九宫, 图集不在线时回落到
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
     * 背包按钮: 右上角常驻, 取原版 icon_backpack 精灵居中等比摆放,
     * 底板沿用原版 img_item_box 九宫, 按下时压暗底板作为反馈;
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
        /* 操作提示条与方块名标签共用同一张原版 board_tips_icon 九宫. */
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
void RenderRemoteMarkers(const float* xs, const float* ys, const float* zs, int32_t count,
                         const float* viewProjection, int32_t width, int32_t height) {
    if (viewProjection == nullptr || width <= 0 || height <= 0) {
        return;
    }
    const HudLayout layout(width, height);
    const float scale = layout.Scale();
    const int32_t total = count > 0 ? count : 0;
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


/* 生物标记: 与远程玩家标记同模式, 绿色区分, 高度略低. */
void RenderCreatureMarkers(const float* xs, const float* ys, const float* zs, int32_t count,
                           const float* viewProjection, int32_t width, int32_t height) {
    if (viewProjection == nullptr || width <= 0 || height <= 0 || count <= 0) {
        return;
    }
    const HudLayout layout(width, height);
    const float halfWidth = layout.DesignLength(0.003f);
    const float bottomLift = 0.05f;
    const float topHeight = 1.4f;
    static float vertices[kVertexCapacity * kFloatsPerVertex];
    int32_t offset = 0;
    for (int32_t i = 0; i < count; ++i) {
        const float wx = xs[i];
        const float wy = ys[i];
        const float wz = zs[i];
        if (wy < -500.0f) {
            continue;
        }
        const float clipW = viewProjection[3] * wx + viewProjection[7] * wy +
                            viewProjection[11] * wz + viewProjection[15];
        if (clipW <= 0.1f) {
            continue;
        }
        const float ndcX = (viewProjection[0] * wx + viewProjection[4] * wy +
                            viewProjection[8] * wz + viewProjection[12]) / clipW;
        const float ndcYB = (viewProjection[1] * wx + viewProjection[5] * (wy + bottomLift) +
                             viewProjection[9] * wz + viewProjection[13]) / clipW;
        const float ndcYT = (viewProjection[1] * wx + viewProjection[5] * (wy + topHeight) +
                             viewProjection[9] * wz + viewProjection[13]) / clipW;
        if (ndcX < -1.2f || ndcX > 1.2f || ndcYB < -1.2f || ndcYT > 1.2f) {
            continue;
        }
        const float sx = (ndcX + 1.0f) * 0.5f * static_cast<float>(width);
        const float sb = (ndcYB + 1.0f) * 0.5f * static_cast<float>(height);
        const float st = (ndcYT + 1.0f) * 0.5f * static_cast<float>(height);
        const float green[4] = {0.3f, 0.85f, 0.3f, 0.6f};
        const float lw = 4.0f;
        const float lft = sx - lw * 0.5f;
        const float rgt = sx + lw * 0.5f;
        const float quads[4][2] = {
            {lft, sb}, {rgt, sb}, {rgt, st}, {lft, st},
        };
        const int32_t order[6] = {0, 1, 2, 0, 2, 3};
        for (int32_t v = 0; v < 6 && offset < kVertexCapacity; ++v) {
            float* target = vertices + static_cast<size_t>(offset) * kFloatsPerVertex;
            target[0] = quads[order[v]][0] / static_cast<float>(width) * 2.0f - 1.0f;
            target[1] = quads[order[v]][1] / static_cast<float>(height) * 2.0f - 1.0f;
            target[2] = -1.0f;
            target[3] = -1.0f;
            target[4] = green[0];
            target[5] = green[1];
            target[6] = green[2];
            target[7] = green[3];
            offset += 1;
        }
    }
    if (offset == 0) {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_shared);
    GLuint tempVao = 0, tempVbo = 0;
    glGenVertexArrays(1, &tempVao);
    glGenBuffers(1, &tempVbo);
    glBindVertexArray(tempVao);
    glBindBuffer(GL_ARRAY_BUFFER, tempVbo);
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
    glDeleteBuffers(1, &tempVbo);
    glDeleteVertexArrays(1, &tempVao);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

}  /* namespace immirror::ui */
