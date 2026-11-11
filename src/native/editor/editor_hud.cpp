#include "editor/editor_hud.h"

#include <GLES3/gl3.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "ui/hud_font.h"
#include "voxel/block_registry.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditorHud";

/* 顶点布局与游戏 HUD 一致: 位置 2, 图集坐标 2, 颜色 4. */
constexpr int32_t kFloatsPerVertex = 8;

/* 顶点缓冲容量, 编辑器覆盖层的常态用量比游戏 HUD 更大, 预留到一万六. */
constexpr int32_t kVertexCapacity = 16384;

/* 图集为 16x16 格. */
constexpr float kAtlasGrid = 16.0f;

/* 界面配色, 与游戏 HUD 的底板色同源, 编辑器侧再压暗一档以便区分模式. */
constexpr float kPanelColor[4] = {0x1B / 255.0f, 0x20 / 255.0f, 0x2E / 255.0f, 0.90f};
constexpr float kPanelBorder[4] = {0x46 / 255.0f, 0x56 / 255.0f, 0x77 / 255.0f, 1.0f};
constexpr float kTextColor[4] = {0.95f, 0.97f, 1.0f, 1.0f};
constexpr float kTextMuted[4] = {0.72f, 0.78f, 0.90f, 1.0f};
constexpr float kTextAccent[4] = {1.0f, 0.85f, 0.35f, 1.0f};
constexpr float kButtonColor[4] = {0.16f, 0.20f, 0.30f, 0.92f};
constexpr float kButtonActive[4] = {0.26f, 0.45f, 0.78f, 0.96f};
constexpr float kRowHighlight[4] = {0.24f, 0.32f, 0.48f, 0.85f};
constexpr float kCrosshairColor[4] = {1.0f, 1.0f, 1.0f, 0.85f};
constexpr float kTargetOutline[4] = {1.0f, 0.86f, 0.25f, 0.95f};
constexpr float kFillOutline[4] = {0.30f, 0.92f, 1.0f, 0.95f};
constexpr float kSpriteTint[4] = {1.0f, 1.0f, 1.0f, 0.95f};
constexpr float kSpriteSelected[4] = {1.0f, 1.0f, 1.0f, 1.0f};

/* 点阵像素边长, 以屏宽为 1. */
constexpr float kStatusPixel = 0.0034f;
constexpr float kPanelPixel = 0.0027f;
constexpr float kButtonPixel = 0.0029f;
constexpr float kHintPixel = 0.0036f;
constexpr float kTitlePixel = 0.0038f;

/* 世界空间框线的粗细, 以屏宽为 1. */
constexpr float kOutlineThickness = 0.0022f;

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

/* 纯色矩形: 中心与半宽半高都是设计坐标, 横向按 scale 等比拉伸. */
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

/* 带图集纹理的方块图标, cell 为图集格序号. */
void PushSprite(float* out, int32_t& offset, float centerX, float centerY, float halfSize,
                float scale, int32_t cell, const float color[4]) {
    if (!HasRoom(offset, 6)) {
        return;
    }
    const float column = static_cast<float>(cell % 16);
    const float row = static_cast<float>(cell / 16);
    const float u0 = column / kAtlasGrid;
    const float u1 = (column + 1.0f) / kAtlasGrid;
    const float v1 = 1.0f - row / kAtlasGrid;
    const float v0 = 1.0f - (row + 1.0f) / kAtlasGrid;
    const float left = centerX - halfSize;
    const float bottom = centerY - halfSize;
    const float right = centerX + halfSize;
    const float top = centerY + halfSize;
    const float positions[4][2] = {
        {left, bottom}, {right, bottom}, {right, top}, {left, top}};
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

/* 点阵文字, 左上角对齐, 逐字符按 5x7 点阵铺小方块. */
void PushText(float* out, int32_t& offset, float left, float top, float pixelSize,
              const char* text, float scale, const float color[4]) {
    if (text == nullptr) {
        return;
    }
    float cursorX = left;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        const uint8_t* rows = ui::GlyphRows(*cursor);
        if (rows != nullptr) {
            for (int32_t row = 0; row < ui::kGlyphHeight; ++row) {
                for (int32_t column = 0; column < ui::kGlyphWidth; ++column) {
                    const bool lit = ((rows[row] >> (ui::kGlyphWidth - 1 - column)) & 1u) != 0u;
                    if (!lit) {
                        continue;
                    }
                    const float centerX =
                        cursorX + (static_cast<float>(column) + 0.5f) * pixelSize;
                    const float centerY = top - (static_cast<float>(row) + 0.5f) * pixelSize;
                    PushRect(out, offset, centerX, centerY, pixelSize * 0.5f, pixelSize * 0.5f,
                             scale, color);
                }
            }
        }
        cursorX += static_cast<float>(ui::kGlyphWidth + ui::kGlyphSpacing) * pixelSize;
    }
}

/* 设计坐标下的一条线段, 用细长四边形近似, 用于世界空间框选. */
void PushLine(float* out, int32_t& offset, float x0, float y0, float x1, float y1,
              float thickness, float scale, const float color[4]) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001f) {
        return;
    }
    /* 法线方向取半粗细, 四角围成一个矩形. */
    const float nx = -dy / length * thickness * 0.5f;
    const float ny = dx / length * thickness * 0.5f;
    const float positions[4][2] = {
        {x0 + nx, y0 + ny}, {x1 + nx, y1 + ny}, {x1 - nx, y1 - ny}, {x0 - nx, y0 - ny}};
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    if (!HasRoom(offset, 6)) {
        return;
    }
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

/*
 * 世界坐标到设计坐标的投影, 与渲染管线使用同一个视投影矩阵.
 * 相机背后的点返回 false, 对应的框线直接跳过.
 */
bool ProjectToDesign(const float* vp, float wx, float wy, float wz, float scale, float& outX,
                     float& outY) {
    if (vp == nullptr) {
        return false;
    }
    const float cx = vp[0] * wx + vp[4] * wy + vp[8] * wz + vp[12];
    const float cy = vp[1] * wx + vp[5] * wy + vp[9] * wz + vp[13];
    const float cw = vp[3] * wx + vp[7] * wy + vp[11] * wz + vp[15];
    if (cw <= 0.0001f) {
        return false;
    }
    const float invW = 1.0f / cw;
    outX = (cx * invW) / (scale > 0.0001f ? scale : 1.0f);
    outY = cy * invW;
    return true;
}

/* 画一个方块范围的十二条棱, 范围为闭区间, 实际角点向外各扩一格. */
void PushBoxOutline(float* out, int32_t& offset, const float* vp, float scale,
                    const voxel::BlockPos& minPos, const voxel::BlockPos& maxPos,
                    const float color[4]) {
    const float x0 = static_cast<float>(minPos.x);
    const float y0 = static_cast<float>(minPos.y);
    const float z0 = static_cast<float>(minPos.z);
    const float x1 = static_cast<float>(maxPos.x + 1);
    const float y1 = static_cast<float>(maxPos.y + 1);
    const float z1 = static_cast<float>(maxPos.z + 1);
    const float corners[8][3] = {
        {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1},
        {x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}, {x0, y1, z1},
    };
    float projected[8][2] = {};
    bool valid[8] = {};
    for (int32_t index = 0; index < 8; ++index) {
        valid[index] = ProjectToDesign(vp, corners[index][0], corners[index][1], corners[index][2],
                                       scale, projected[index][0], projected[index][1]);
    }
    const int32_t edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (const auto& edge : edges) {
        if (!valid[edge[0]] || !valid[edge[1]]) {
            continue;
        }
        PushLine(out, offset, projected[edge[0]][0], projected[edge[0]][1],
                 projected[edge[1]][0], projected[edge[1]][1], kOutlineThickness, scale, color);
    }
}

/* 方块图标使用的图集格: 优先顶面, 未指定时回落到默认格. */
int32_t AtlasCellOf(voxel::BlockId id) {
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(id);
    return info.atlasTop != 0xFF ? static_cast<int32_t>(info.atlasTop)
                                 : static_cast<int32_t>(info.atlasIndex);
}

}  /* namespace */

EditorHud::EditorHud() = default;

EditorHud::~EditorHud() = default;

bool EditorHud::Initialize(uint32_t atlasTexture) {
    if (ready_) {
        return true;
    }
    atlasTexture_ = atlasTexture;
    vertices_.assign(static_cast<size_t>(kVertexCapacity) * kFloatsPerVertex, 0.0f);
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
    IM_LOGI(kTag, "editor hud ready, atlas=%u", atlasTexture_);
    return true;
}

void EditorHud::Shutdown() {
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

void EditorHud::SetViewport(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
    layout_.scaleX = width_ > 0 ? static_cast<float>(height_) / static_cast<float>(width_) : 1.0f;
}

void EditorHud::RowBounds(const Layout& layout, int32_t index, float& top, float& bottom) const {
    top = layout.panelTop + layout.panelTitleHeight + static_cast<float>(index) * layout.rowHeight;
    bottom = top + layout.rowHeight;
}

void EditorHud::ButtonBounds(const Layout& layout, int32_t index, float& top, float& bottom) const {
    top = layout.buttonTop + static_cast<float>(index) * (layout.buttonHeight + layout.buttonGap);
    bottom = top + layout.buttonHeight;
}

void EditorHud::PaletteBounds(const Layout& layout, int32_t index, float& left, float& right) const {
    const int32_t count = PaletteCount();
    const float originX = 0.5f - layout.palettePitch * static_cast<float>(count - 1) * 0.5f;
    left = originX + static_cast<float>(index) * layout.palettePitch - layout.paletteHalf;
    right = left + layout.paletteHalf * 2.0f;
}

void EditorHud::Render(const EditorFrame& frame) {
    if (!ready_) {
        return;
    }
    const float scale = layout_.scaleX > 0.0001f ? layout_.scaleX : 1.0f;
    /* 设计坐标换算: 归一化横坐标到以屏心为原点的设计横坐标. */
    const auto designX = [scale](float normalized) {
        return (normalized * 2.0f - 1.0f) / (scale > 0.0001f ? scale : 1.0f);
    };
    const auto designY = [](float normalized) { return 1.0f - normalized * 2.0f; };
    const auto designLength = [scale](float normalized) {
        return normalized * 2.0f / (scale > 0.0001f ? scale : 1.0f);
    };

    if (static_cast<int32_t>(vertices_.size()) < kVertexCapacity * kFloatsPerVertex) {
        vertices_.assign(static_cast<size_t>(kVertexCapacity) * kFloatsPerVertex, 0.0f);
    }
    float* vertices = vertices_.data();
    int32_t offset = 0;

    /* 状态区底板与逐行文本. */
    if (frame.statusLineCount > 0) {
        const float lineHeight = 0.0185f;
        const float top = 0.028f;
        const float bottom = top + 0.020f + lineHeight * static_cast<float>(frame.statusLineCount);
        const float left = designX(0.012f);
        const float right = designX(0.62f);
        PushRect(vertices, offset, (left + right) * 0.5f, (designY(top) + designY(bottom)) * 0.5f,
                 (right - left) * 0.5f, (designY(top) - designY(bottom)) * 0.5f, scale,
                 kPanelColor);
        PushRect(vertices, offset, (left + right) * 0.5f, designY(top), (right - left) * 0.5f,
                 designLength(0.0009f), scale, kPanelBorder);
        if (frame.title != nullptr) {
            PushText(vertices, offset, left + designLength(0.008f), designY(top + 0.004f),
                     designLength(kTitlePixel), frame.title, scale, kTextAccent);
        }
        for (int32_t index = 0; index < frame.statusLineCount; ++index) {
            if (frame.statusLines[index] == nullptr) {
                continue;
            }
            const float lineTop = top + 0.020f + lineHeight * static_cast<float>(index);
            PushText(vertices, offset, left + designLength(0.008f), designY(lineTop),
                     designLength(kStatusPixel), frame.statusLines[index], scale, kTextColor);
        }
    }

    /* 右侧工具条. */
    for (int32_t index = 0; index < frame.buttonCount; ++index) {
        float top = 0.0f;
        float bottom = 0.0f;
        ButtonBounds(layout_, index, top, bottom);
        const float left = designX(layout_.buttonX0);
        const float right = designX(layout_.buttonX1);
        PushRect(vertices, offset, (left + right) * 0.5f, (designY(top) + designY(bottom)) * 0.5f,
                 (right - left) * 0.5f, (designY(top) - designY(bottom)) * 0.5f, scale,
                 frame.buttons[index].active ? kButtonActive : kButtonColor);
        PushRect(vertices, offset, (left + right) * 0.5f, designY(top), (right - left) * 0.5f,
                 designLength(0.0007f), scale, kPanelBorder);
        const float textWidth =
            ui::MeasureText(frame.buttons[index].label, designLength(kButtonPixel));
        PushText(vertices, offset, (left + right) * 0.5f - textWidth * 0.5f,
                 (designY(top) + designY(bottom)) * 0.5f +
                     static_cast<float>(ui::kGlyphHeight) * designLength(kButtonPixel) * 0.5f,
                 designLength(kButtonPixel), frame.buttons[index].label, scale, kTextColor);
    }

    /* 底部方块调色板. */
    if (frame.selectedPalette >= 0) {
        const float centerY = designY(layout_.paletteCenterY);
        const float half = designLength(layout_.paletteHalf);
        for (int32_t index = 0; index < PaletteCount(); ++index) {
            float left = 0.0f;
            float right = 0.0f;
            PaletteBounds(layout_, index, left, right);
            const float centerX = (designX(left) + designX(right)) * 0.5f;
            const bool selected = index == frame.selectedPalette;
            PushRect(vertices, offset, centerX, centerY, half * 1.08f, half * 1.08f, scale,
                     selected ? kButtonActive : kPanelColor);
            PushSprite(vertices, offset, centerX, centerY, half, scale,
                       AtlasCellOf(PaletteBlock(index)),
                       selected ? kSpriteSelected : kSpriteTint);
        }
    }

    /* 属性面板. */
    if (frame.panelVisible && frame.rowCount > 0) {
        float firstTop = 0.0f;
        float firstBottom = 0.0f;
        RowBounds(layout_, 0, firstTop, firstBottom);
        float lastTop = 0.0f;
        float lastBottom = 0.0f;
        RowBounds(layout_, frame.rowCount - 1, lastTop, lastBottom);
        const float left = designX(layout_.panelX0);
        const float right = designX(layout_.panelX1);
        const float panelTop = layout_.panelTop;
        const float panelBottom = lastBottom + 0.008f;
        PushRect(vertices, offset, (left + right) * 0.5f,
                 (designY(panelTop) + designY(panelBottom)) * 0.5f, (right - left) * 0.5f,
                 (designY(panelTop) - designY(panelBottom)) * 0.5f, scale, kPanelColor);
        PushRect(vertices, offset, (left + right) * 0.5f, designY(panelTop), (right - left) * 0.5f,
                 designLength(0.0009f), scale, kPanelBorder);
        if (frame.panelTitle != nullptr) {
            PushText(vertices, offset, left + designLength(0.008f),
                     designY(layout_.panelTop + 0.006f), designLength(kTitlePixel),
                     frame.panelTitle, scale, kTextAccent);
        }
        for (int32_t index = 0; index < frame.rowCount; ++index) {
            float top = 0.0f;
            float bottom = 0.0f;
            RowBounds(layout_, index, top, bottom);
            const EditorRowState& row = frame.rows[index];
            if (index == frame.highlightedRow) {
                PushRect(vertices, offset, (left + right) * 0.5f,
                         (designY(top) + designY(bottom)) * 0.5f, (right - left) * 0.5f,
                         (designY(top) - designY(bottom)) * 0.5f * 0.92f, scale,
                         kRowHighlight);
            }
            const float textY = (designY(top) + designY(bottom)) * 0.5f +
                                static_cast<float>(ui::kGlyphHeight) * designLength(kPanelPixel) *
                                    0.5f;
            PushText(vertices, offset, left + designLength(0.008f), textY,
                     designLength(kPanelPixel), row.label, scale, kTextMuted);
            const float valueWidth = ui::MeasureText(row.value, designLength(kPanelPixel));
            PushText(vertices, offset, right - designLength(0.115f) - valueWidth, textY,
                     designLength(kPanelPixel), row.value, scale, kTextColor);
            if (row.steppable) {
                /* 加减号用纯色小方块拼出, 不依赖字形表. */
                const float minusX = right - designLength(0.078f);
                const float plusX = right - designLength(0.030f);
                const float boxHalf = designLength(0.014f);
                const float barHalf = designLength(0.008f);
                const float barThick = designLength(0.0025f);
                PushRect(vertices, offset, minusX, textY - designLength(0.0035f), boxHalf, boxHalf,
                         scale, kButtonColor);
                PushRect(vertices, offset, minusX, textY - designLength(0.0035f), barHalf,
                         barThick, scale, kTextColor);
                PushRect(vertices, offset, plusX, textY - designLength(0.0035f), boxHalf, boxHalf,
                         scale, kButtonColor);
                PushRect(vertices, offset, plusX, textY - designLength(0.0035f), barHalf, barThick,
                         scale, kTextColor);
                PushRect(vertices, offset, plusX, textY - designLength(0.0035f), barThick, barHalf,
                         scale, kTextColor);
            }
        }
    }

    /* 世界空间框选: 目标方块与框选范围. */
    if (frame.drawTarget) {
        PushBoxOutline(vertices, offset, frame.viewProjection, scale, frame.targetMin,
                       frame.targetMax, kTargetOutline);
    }
    if (frame.drawFill) {
        PushBoxOutline(vertices, offset, frame.viewProjection, scale, frame.fillMin, frame.fillMax,
                       kFillOutline);
    }

    /* 准星: 只在编辑与试玩都显示, 便于确认拾取方向. */
    const float crossHalf = designLength(0.012f);
    const float crossThick = designLength(0.0016f);
    PushRect(vertices, offset, 0.0f, 0.0f, crossHalf, crossThick, scale, kCrosshairColor);
    PushRect(vertices, offset, 0.0f, 0.0f, crossThick, crossHalf, scale, kCrosshairColor);

    /* 提示文本. */
    if (frame.hint != nullptr && frame.hint[0] != '\0') {
        const float pixel = designLength(kHintPixel);
        const float textWidth = ui::MeasureText(frame.hint, pixel);
        const float plateHalfWidth = textWidth * 0.5f + designLength(0.02f);
        const float plateHalfHeight =
            static_cast<float>(ui::kGlyphHeight) * pixel * 0.5f + designLength(0.012f);
        const float centerY = designY(0.80f);
        PushRect(vertices, offset, 0.0f, centerY, plateHalfWidth, plateHalfHeight, scale,
                 kPanelColor);
        PushText(vertices, offset, -textWidth * 0.5f, centerY + plateHalfHeight - designLength(0.008f),
                 pixel, frame.hint, scale, kTextAccent);
    }

    if (offset == 0) {
        return;
    }

    /* 覆盖层: 关闭深度与剔除, 开启混合. */
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

EditorHit EditorHud::HitTest(const EditorFrame& frame, float normalizedX,
                             float normalizedY) const {
    EditorHit hit;
    if (width_ <= 0 || height_ <= 0) {
        return hit;
    }
    const auto withinX = [](float value, float left, float right) {
        return value >= left && value <= right;
    };
    const auto withinY = [](float value, float top, float bottom) {
        return value >= top && value <= bottom;
    };

    /* 工具条优先: 它压在面板与调色板之上. */
    for (int32_t index = 0; index < frame.buttonCount; ++index) {
        float top = 0.0f;
        float bottom = 0.0f;
        ButtonBounds(layout_, index, top, bottom);
        if (withinX(normalizedX, layout_.buttonX0, layout_.buttonX1) &&
            withinY(normalizedY, top, bottom)) {
            hit.button = frame.buttons[index].id;
            return hit;
        }
    }

    /* 属性面板: 左半是减号, 右半是加号, 中间选中整行. */
    if (frame.panelVisible && frame.rowCount > 0) {
        for (int32_t index = 0; index < frame.rowCount; ++index) {
            float top = 0.0f;
            float bottom = 0.0f;
            RowBounds(layout_, index, top, bottom);
            if (!withinX(normalizedX, layout_.panelX0, layout_.panelX1) ||
                !withinY(normalizedY, top, bottom)) {
                continue;
            }
            hit.row = index;
            if (frame.rows[index].steppable) {
                if (normalizedX > layout_.panelX1 - 0.095f &&
                    normalizedX <= layout_.panelX1 - 0.052f) {
                    hit.rowDelta = -1;
                } else if (normalizedX > layout_.panelX1 - 0.052f) {
                    hit.rowDelta = 1;
                }
            }
            return hit;
        }
    }

    /* 方块调色板. */
    if (frame.selectedPalette >= 0) {
        for (int32_t index = 0; index < PaletteCount(); ++index) {
            float left = 0.0f;
            float right = 0.0f;
            PaletteBounds(layout_, index, left, right);
            if (withinX(normalizedX, left, right) &&
                withinY(normalizedY, layout_.paletteCenterY - layout_.paletteHalf,
                        layout_.paletteCenterY + layout_.paletteHalf)) {
                hit.paletteIndex = index;
                return hit;
            }
        }
    }
    return hit;
}

}  /* namespace immirror::editor */
