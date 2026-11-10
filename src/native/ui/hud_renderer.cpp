#include "ui/hud_renderer.h"

#include <GLES3/gl3.h>

#include <cmath>
#include <cstring>

#include "core/log.h"

namespace immirror::ui {
namespace {

constexpr const char* kTag = "ImMirrorHud";

/* 顶点只带屏幕归一化坐标与颜色. */
constexpr int32_t kFloatsPerVertex = 6;

const char* const kVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
void main() {
    gl_Position = vec4(aPosition, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* const kFragmentShader = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
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

/* 以屏幕比例描述矩形, 原点在屏幕中心, x 向右 y 向上. */
struct Rect {
    float centerX;
    float centerY;
    float halfWidth;
    float halfHeight;
    float aspect;
};

void PushRect(float* out, int32_t& vertexOffset, const Rect& rect, const float color[4]) {
    const float left = (rect.centerX - rect.halfWidth) * rect.aspect;
    const float right = (rect.centerX + rect.halfWidth) * rect.aspect;
    const float bottom = rect.centerY - rect.halfHeight;
    const float top = rect.centerY + rect.halfHeight;
    const float positions[4][2] = {{left, bottom}, {right, bottom}, {right, top}, {left, top}};
    const int32_t indices[6] = {0, 1, 2, 0, 2, 3};
    for (int32_t index = 0; index < 6; ++index) {
        const float* position = positions[indices[index]];
        float* target = out + static_cast<size_t>(vertexOffset) * kFloatsPerVertex;
        target[0] = position[0];
        target[1] = position[1];
        target[2] = color[0];
        target[3] = color[1];
        target[4] = color[2];
        target[5] = color[3];
        vertexOffset += 1;
    }
}

void PushRing(float* out, int32_t& vertexOffset, float centerX, float centerY, float radius,
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
            float* target = out + static_cast<size_t>(vertexOffset) * kFloatsPerVertex;
            target[0] = quad[order[index]][0] * aspect;
            target[1] = quad[order[index]][1];
            target[2] = color[0];
            target[3] = color[1];
            target[4] = color[2];
            target[5] = color[3];
            vertexOffset += 1;
        }
    }
}

}  /* namespace */

HudRenderer::HudRenderer() = default;

HudRenderer::~HudRenderer() = default;

bool HudRenderer::Initialize() {
    if (ready_) {
        return true;
    }
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
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, 64 * 1024, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glBindVertexArray(0);
    ready_ = true;
    IM_LOGI(kTag, "hud renderer ready");
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

void HudRenderer::Render(float stickX, float stickY, bool stickActive) {
    if (!ready_) {
        return;
    }
    const float aspect = viewportHeight_ > 0
                             ? static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_)
                             : 1.0f;
    float vertices[64 * 1024 / sizeof(float)];
    int32_t offset = 0;

    /* 准星: 两条细矩形组成十字. */
    const float white[4] = {1.0f, 1.0f, 1.0f, 0.75f};
    PushRect(vertices, offset, Rect{0.0f, 0.0f, 0.006f, 0.0012f, aspect}, white);
    PushRect(vertices, offset, Rect{0.0f, 0.0f, 0.0012f, 0.006f, aspect}, white);

    /* 摇杆: 底圈固定在左下角, 摇杆头随输入偏移. */
    const float stickCenterX = -0.52f;
    const float stickCenterY = -0.42f;
    const float baseColor[4] = {1.0f, 1.0f, 1.0f, 0.16f};
    const float knobColor[4] = {1.0f, 1.0f, 1.0f, 0.38f};
    PushRing(vertices, offset, stickCenterX, stickCenterY, 0.14f, 0.012f, aspect, baseColor, 24);
    const float knobX = stickCenterX + stickX * 0.1f * aspect;
    const float knobY = stickCenterY + stickY * 0.1f;
    PushRing(vertices, offset, knobX, knobY, 0.052f, 0.046f, aspect,
             stickActive ? knobColor : baseColor, 16);

    /* 跳跃按钮: 右下角方框. */
    const float jumpColor[4] = {1.0f, 1.0f, 1.0f, 0.22f};
    PushRect(vertices, offset, Rect{0.62f, -0.42f, 0.075f, 0.075f, aspect}, jumpColor);

    if (offset == 0) {
        return;
    }

    /* 2D 覆盖层: 关闭深度与剔除, 开启混合. */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(program_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(offset) * kFloatsPerVertex * sizeof(float), vertices);
    glDrawArrays(GL_TRIANGLES, 0, offset);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

}  /* namespace immirror::ui */
