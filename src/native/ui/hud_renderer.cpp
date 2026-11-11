#include "ui/hud_renderer.h"

#include <GLES3/gl3.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "ui/generated/ui_resources_gen.h"
#include "ui/hud_font.h"
#include "ui/hud_slots.h"

namespace immirror::ui {
namespace {

constexpr const char* kTag = "ImMirrorHud";

/* 顶点: 屏幕归一化坐标 (2) + 纹理坐标 (2) + 颜色 (4), uv 为负表示纯色矩形. */
constexpr int32_t kFloatsPerVertex = 8;

/* 顶点缓冲容量, 按顶点数计. HUD 常态用量不足一千, 预留到八千以容纳方块名文字. */
constexpr int32_t kVertexCapacity = 8192;

/* 图集为 16x16 格, 每格 64 像素. */
constexpr float kAtlasGrid = 16.0f;

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
in vec2 vTexCoord;
in vec4 vColor;
out vec4 fragColor;
void main() {
    if (vTexCoord.x < 0.0) {
        fragColor = vColor;
        return;
    }
    fragColor = texture(uAtlas, vTexCoord) * vColor;
}
)";

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

}  /* namespace */

HudRenderer::HudRenderer() = default;

HudRenderer::~HudRenderer() = default;

bool HudRenderer::Initialize(uint32_t atlasTexture) {
    if (ready_) {
        return true;
    }
    atlasTexture_ = atlasTexture;
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

    /* 准星: 两条细矩形组成十字, 固定在屏幕中心. */
    const float white[4] = {1.0f, 1.0f, 1.0f, 0.75f};
    const float armHalf = layout_.DesignLength(0.006f);
    const float armThickness = layout_.DesignLength(0.0012f);
    PushRect(vertices, offset, 0.0f, 0.0f, armHalf, armThickness, scale, white);
    PushRect(vertices, offset, 0.0f, 0.0f, armThickness, armHalf, scale, white);

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

    /*
     * 血条: 左上角横条. 底槽走九宫底板并取规范底板色, 实心段取规范血条色,
     * 长度按血量比例截取, 左端固定, 右端随血量收缩.
     */
    const float healthHalfWidth = layout_.DesignLength(kHealthHalfWidth);
    const float healthHalfHeight = layout_.DesignLength(kHealthHalfHeight);
    const float healthCenterX = layout_.DesignX(kHealthCenterX);
    const float healthCenterY = layout_.DesignY(kHealthCenterY);
    PushNineSlice(vertices, offset, healthCenterX, healthCenterY, healthHalfWidth,
                  healthHalfHeight, scale, gen::UiAssetOf(gen::UiWidget::MiscBarHpTrack),
                  artUnit, kSolidUv, kHudBoardColor);
    const float health = frame.health < 0.0f ? 0.0f : (frame.health > 1.0f ? 1.0f : frame.health);
    if (health > 0.0f) {
        const float fillHalfWidth = healthHalfWidth * health;
        const float fillHalfHeight = std::fmax(healthHalfHeight - artUnit * kHealthFillInset, 0.0f);
        PushRect(vertices, offset, healthCenterX - healthHalfWidth + fillHalfWidth, healthCenterY,
                 fillHalfWidth, fillHalfHeight, scale, kHudBloodColor);
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

    /* 跳跃按钮: 右下角方块加向上箭头, 按下时收缩并加亮作为反馈. */
    const float jumpHalf = layout_.DesignLength(layout_.JumpHalfSize());
    const float jumpCenterX = layout_.DesignX(layout_.JumpCenterX());
    const float jumpCenterY = layout_.DesignY(layout_.JumpCenterY());
    const float jumpDrawHalf = frame.jumpPressed ? jumpHalf * kJumpPressedScale : jumpHalf;
    const float jumpBackdrop[4] = {0.0f, 0.0f, 0.0f, 0.32f};
    const float jumpBody[4] = {1.0f, 1.0f, 1.0f, frame.jumpPressed ? 0.52f : 0.22f};
    const float jumpArrow[4] = {1.0f, 1.0f, 1.0f, frame.jumpPressed ? 0.95f : 0.62f};
    PushRect(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf, jumpDrawHalf, scale,
             jumpBackdrop);
    PushRect(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf * 0.86f,
             jumpDrawHalf * 0.86f, scale, jumpBody);
    PushTriangle(vertices, offset, jumpCenterX, jumpCenterY, jumpDrawHalf * 0.42f,
                 jumpDrawHalf * 0.62f, scale, jumpArrow);

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
        PushNineSlice(vertices, offset, centerX, slotCenterY, slotHalf * 1.05f, slotHalf * 1.05f,
                      scale, gen::UiAssetOf(gen::UiWidget::SlotItemFrame), artUnit, kSolidUv,
                      spent ? kSlotEmptyBoard : kHudBoardColor);
        PushSprite(vertices, offset, centerX, slotCenterY, slotHalf, scale,
                   BlockSlotAtlasCell(slot),
                   spent ? kSlotEmptyTint : (selected ? selectedTint : slotTint));
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
        ToUpper(BlockSlotName(frame.nameSlot), text, sizeof(text));
        const float pixel = layout_.DesignLength(kNamePixelSize);
        const float textWidth = MeasureText(text, pixel);
        const float padding = layout_.DesignLength(kNamePadding);
        const float plateHalfWidth = textWidth * 0.5f + padding;
        const float plateHalfHeight = static_cast<float>(kGlyphHeight) * pixel * 0.5f + padding * 0.5f;
        const float plateCenterX = layout_.DesignX(layout_.SlotCenterX(frame.nameSlot));
        const float plateCenterY = slotCenterY + layout_.DesignLength(layout_.SlotHalfSize()) +
                                   layout_.DesignLength(kNamePlateGap) + plateHalfHeight;
        PushNineSlice(vertices, offset, plateCenterX, plateCenterY, plateHalfWidth,
                      plateHalfHeight, scale, gen::UiAssetOf(gen::UiWidget::DialogTipsBody),
                      artUnit, kSolidUv, kHudTipsColor);
        PushText(vertices, offset, plateCenterX - textWidth * 0.5f,
                 plateCenterY + static_cast<float>(kGlyphHeight) * pixel * 0.5f, pixel, text,
                 scale, white);
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
        PushNineSlice(vertices, offset, 0.0f, plateCenterY, plateHalfWidth, plateHalfHeight,
                      scale, gen::UiAssetOf(gen::UiWidget::DialogTipsBody), artUnit, kSolidUv,
                      kHudTipsColor);
        PushText(vertices, offset, -textWidth * 0.5f,
                 plateCenterY + static_cast<float>(kGlyphHeight) * pixel * 0.5f, pixel, text,
                 scale, white);
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
    if (uniformAtlasLocation_ >= 0 && atlasTexture_ != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlasTexture_);
        glUniform1i(uniformAtlasLocation_, 0);
    }
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(offset) * kFloatsPerVertex * sizeof(float), vertices);
    glDrawArrays(GL_TRIANGLES, 0, offset);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

}  /* namespace immirror::ui */
