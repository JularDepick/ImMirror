#include "ui/hud_renderer.h"

#include <GLES3/gl3.h>

#include <cmath>
#include <cstring>

#include "core/log.h"

namespace immirror::ui {
namespace {

constexpr const char* kTag = "ImMirrorHud";

/* 顶点: 屏幕归一化坐标 (2) + 纹理坐标 (2) + 颜色 (4), uv 为负表示纯色矩形. */
constexpr int32_t kFloatsPerVertex = 8;

/* 图集为 16x16 格, 每格 64 像素. */
constexpr float kAtlasGrid = 16.0f;

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

/* 直线段, 用于准星与选中框. */
void PushRect(float* out, int32_t& offset, float centerX, float centerY, float halfWidth,
              float halfHeight, float aspect, const float color[4]) {
    const float positions[4][2] = {
        {(centerX - halfWidth) * aspect, centerY - halfHeight},
        {(centerX + halfWidth) * aspect, centerY - halfHeight},
        {(centerX + halfWidth) * aspect, centerY + halfHeight},
        {(centerX - halfWidth) * aspect, centerY + halfHeight},
    };
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0];
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

/* 带图集纹理的矩形, cell 为图集格序号. */
void PushSprite(float* out, int32_t& offset, float left, float bottom, float right, float top,
                float aspect, int32_t cell, const float color[4]) {
    const float column = static_cast<float>(cell % 16);
    const float row = static_cast<float>(cell / 16);
    const float u0 = column / kAtlasGrid;
    const float u1 = (column + 1.0f) / kAtlasGrid;
    /* 图集上传时已按格垂直翻转, 数据首行即格内底边, 因此 v 随行号增大. */
    const float v0 = row / kAtlasGrid;
    const float v1 = (row + 1.0f) / kAtlasGrid;
    const float positions[4][2] = {
        {left * aspect, bottom},
        {right * aspect, bottom},
        {right * aspect, top},
        {left * aspect, top},
    };
    const float uvs[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
        target[0] = positions[order[index]][0];
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

void PushRing(float* out, int32_t& offset, float centerX, float centerY, float radius,
              float thickness, float aspect, const float color[4], int32_t segments) {
    for (int32_t segment = 0; segment < segments; ++segment) {
        const float a0 = 6.28318530718f * static_cast<float>(segment) / static_cast<float>(segments);
        const float a1 = 6.28318530718f * static_cast<float>(segment + 1) / static_cast<float>(segments);
        const float innerR = radius - thickness;
        const float quad[4][2] = {
            {centerX + std::cos(a0) * innerR, centerY + std::sin(a0) * innerR},
            {centerX + std::cos(a1) * innerR, centerY + std::sin(a1) * innerR},
            {centerX + std::cos(a1) * radius, centerY + std::sin(a1) * radius},
            {centerX + std::cos(a0) * radius, centerY + std::sin(a0) * radius},
        };
        const int32_t order[6] = {0, 1, 2, 0, 2, 3};
        for (int32_t index = 0; index < 6; ++index) {
            float* target = out + static_cast<size_t>(offset) * kFloatsPerVertex;
            target[0] = quad[order[index]][0] * aspect;
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

/* 方块栏几何: 九格居中排列在屏幕底部. */
constexpr float kSlotSize = 0.052f;
constexpr float kSlotGap = 0.004f;
constexpr float kSlotCenterY = -0.86f;

/* 相邻格中心间距, 中间格落在屏幕水平中心. */
constexpr float kSlotPitch = kSlotSize * 2.0f + kSlotGap;
constexpr float kSlotOriginX = -kSlotPitch * 4.0f;

float SlotCenterX(int32_t slot) {
    return kSlotOriginX + static_cast<float>(slot) * kSlotPitch;
}

/* 方块栏中依次展示的图集格. */
const int32_t kSlotCells[HudRenderer::kSlotCount] = {0, 2, 3, 4, 6, 8, 10, 11, 12};

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
    glBufferData(GL_ARRAY_BUFFER, 96 * 1024, nullptr, GL_DYNAMIC_DRAW);
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
    viewportWidth_ = width;
    viewportHeight_ = height;
}

int32_t HudRenderer::HitTestSlot(float normalizedX, float normalizedY) const {
    /* 归一化坐标原点在左上角, 转换为以屏幕中心为原点的比例坐标. */
    const float aspect = viewportHeight_ > 0
                             ? static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_)
                             : 1.0f;
    const float x = (normalizedX * 2.0f - 1.0f) * aspect;
    const float y = 1.0f - normalizedY * 2.0f;
    const float half = kSlotSize;
    if (y < kSlotCenterY - half || y > kSlotCenterY + half) {
        return -1;
    }
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const float centerX = SlotCenterX(slot) * aspect;
        if (x >= centerX - half * aspect && x <= centerX + half * aspect) {
            return slot;
        }
    }
    return -1;
}

void HudRenderer::Render(float stickX, float stickY, bool stickActive, int32_t selectedSlot) {
    if (!ready_) {
        return;
    }
    const float aspect = viewportHeight_ > 0
                             ? static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_)
                             : 1.0f;
    static float vertices[96 * 1024 / sizeof(float)];
    int32_t offset = 0;

    /* 准星: 两条细矩形组成十字. */
    const float white[4] = {1.0f, 1.0f, 1.0f, 0.75f};
    PushRect(vertices, offset, 0.0f, 0.0f, 0.006f, 0.0012f, aspect, white);
    PushRect(vertices, offset, 0.0f, 0.0f, 0.0012f, 0.006f, aspect, white);

    /* 摇杆: 底圈固定在左下角, 摇杆头随输入偏移. */
    const float stickCenterX = -0.52f;
    const float stickCenterY = -0.42f;
    const float baseColor[4] = {1.0f, 1.0f, 1.0f, 0.16f};
    const float knobColor[4] = {1.0f, 1.0f, 1.0f, 0.38f};
    PushRing(vertices, offset, stickCenterX, stickCenterY, 0.14f, 0.012f, aspect, baseColor, 24);
    const float knobX = stickCenterX + stickX * 0.1f;
    const float knobY = stickCenterY + stickY * 0.1f;
    PushRing(vertices, offset, knobX, knobY, 0.052f, 0.046f, aspect,
             stickActive ? knobColor : baseColor, 16);

    /* 跳跃按钮: 右下角方框. */
    const float jumpColor[4] = {1.0f, 1.0f, 1.0f, 0.22f};
    PushRect(vertices, offset, 0.62f, -0.42f, 0.075f, 0.075f, aspect, jumpColor);

    /* 方块栏: 九格图集图标, 选中格加亮边. */
    const float slotTint[4] = {1.0f, 1.0f, 1.0f, 0.92f};
    const float selectedTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float borderColor[4] = {1.0f, 0.92f, 0.55f, 1.0f};
    const float backdrop[4] = {0.0f, 0.0f, 0.0f, 0.35f};
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const float centerX = SlotCenterX(slot);
        const bool selected = slot == selectedSlot;
        PushRect(vertices, offset, centerX, kSlotCenterY, kSlotSize * 1.12f, kSlotSize * 1.12f,
                 aspect, backdrop);
        PushSprite(vertices, offset, centerX - kSlotSize, kSlotCenterY - kSlotSize,
                   centerX + kSlotSize, kSlotCenterY + kSlotSize, aspect, kSlotCells[slot],
                   selected ? selectedTint : slotTint);
        if (selected) {
            PushRect(vertices, offset, centerX, kSlotCenterY + kSlotSize, kSlotSize, 0.0035f, aspect, borderColor);
            PushRect(vertices, offset, centerX, kSlotCenterY - kSlotSize, kSlotSize, 0.0035f, aspect, borderColor);
            PushRect(vertices, offset, centerX - kSlotSize, kSlotCenterY, 0.0035f, kSlotSize, aspect, borderColor);
            PushRect(vertices, offset, centerX + kSlotSize, kSlotCenterY, 0.0035f, kSlotSize, aspect, borderColor);
        }
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
