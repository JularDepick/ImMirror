#include "ui/hud_renderer.h"

#include <GLES3/gl3.h>

#include <cmath>
#include <cstddef>
#include <cstring>

#include "core/log.h"
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

/* 方块名标签: 点阵像素边长, 内边距与离方块栏的间距, 均以屏宽为 1. */
constexpr float kNamePixelSize = 0.0035f;
constexpr float kNamePadding = 0.012f;
constexpr float kNamePlateGap = 0.012f;

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
    const float slotBackdrop[4] = {0.0f, 0.0f, 0.0f, 0.35f};
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const float centerX = layout_.DesignX(layout_.SlotCenterX(slot));
        const bool selected = slot == frame.selectedSlot;
        PushRect(vertices, offset, centerX, slotCenterY, slotHalf * 1.05f, slotHalf * 1.05f,
                 scale, slotBackdrop);
        PushSprite(vertices, offset, centerX, slotCenterY, slotHalf, scale,
                   BlockSlotAtlasCell(slot), selected ? selectedTint : slotTint);
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
        const float plateColor[4] = {0.0f, 0.0f, 0.0f, 0.62f};
        PushRect(vertices, offset, plateCenterX, plateCenterY, plateHalfWidth, plateHalfHeight,
                 scale, plateColor);
        PushText(vertices, offset, plateCenterX - textWidth * 0.5f,
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
