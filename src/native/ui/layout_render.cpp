#include "ui/layout_render.h"

#include "ui/hud_font.h"

#include <cstring>
#include <string>

#include "ui/generated/ui_atlas_gen.h"
#include "ui/generated/ui_sprite_atlas_gen.h"
#include "ui/generated/ui_sprite_index_gen.h"

namespace immirror::ui {
namespace {

/* 取贴图路径的文件名主干: 去掉目录与扩展名. */
std::string StemOf(const char* path) {
    if (path == nullptr) {
        return std::string();
    }
    const char* slash = std::strrchr(path, '/');
    const char* start = slash == nullptr ? path : slash + 1;
    const char* dot = std::strrchr(start, '.');
    const size_t length = dot == nullptr ? std::strlen(start) : static_cast<size_t>(dot - start);
    return std::string(start, length);
}

}  /* namespace */

bool UiSpriteRectUv(const char* spriteName, float* u0, float* v0, float* u1, float* v1) {
    if (spriteName == nullptr || spriteName[0] == '\0') {
        return false;
    }
    /*
     * 子图集索引按名字排序, 用二分查找.
     * 名字带扩展名时去掉再比对, 索引里存的是不带扩展名的精灵名.
     */
    std::string key(spriteName);
    if (key.size() > 4 && key.compare(key.size() - 4, 4, ".png") == 0) {
        key.resize(key.size() - 4);
    }
    /*
     * 合图索引: 精灵已经重排进一张 4096 图, uv 直接是合图坐标.
     * 旧的按原版图集页记录的 uv 与运行时纹理不匹配, 已废弃.
     */
    int32_t low = 0;
    int32_t high = gen::kUiAtlasSpriteCount - 1;
    while (low <= high) {
        const int32_t middle = low + (high - low) / 2;
        const int32_t order = key.compare(gen::kUiAtlasSprites[middle].name);
        if (order == 0) {
            const gen::UiAtlasSprite& rect = gen::kUiAtlasSprites[middle];
            if (u0 != nullptr) {
                *u0 = rect.u0;
            }
            if (v0 != nullptr) {
                *v0 = rect.v0;
            }
            if (u1 != nullptr) {
                *u1 = rect.u1;
            }
            if (v1 != nullptr) {
                *v1 = rect.v1;
            }
            return true;
        }
        if (order < 0) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
    }
    return false;
}

bool UiPageSize(const char* texturePath, float* width, float* height) {
    if (texturePath == nullptr || texturePath[0] == '\0') {
        return false;
    }
    std::string key(texturePath);
    /* 布局里常写 .png, 实际图集是同名 .ktx, 找不到时按容器名再试一次. */
    std::string alternative;
    if (key.size() > 4 && key.compare(key.size() - 4, 4, ".png") == 0) {
        alternative = key.substr(0, key.size() - 4) + ".ktx";
    }
    for (int32_t index = 0; index < gen::kUiPageSizeCount; ++index) {
        const gen::UiPageSize& page = gen::kUiPageSizes[index];
        if (page.path == nullptr || (key != page.path && alternative != page.path)) {
            continue;
        }
        if (width != nullptr) {
            *width = page.width;
        }
        if (height != nullptr) {
            *height = page.height;
        }
        return page.width > 0.0f && page.height > 0.0f;
    }
    return false;
}

void LayoutTextOrigin(const LayoutDrawCommand& command, float pixelSize, float* x, float* y,
                      float* width, float* height) {
    const float textWidth = command.text == nullptr ? 0.0f : MeasureText(command.text, pixelSize);
    const float textHeight = static_cast<float>(kGlyphHeight) * pixelSize;
    float originX = command.rect.x;
    float originY = command.rect.y;
    if (command.justifyH == 1) {
        originX += (command.rect.width - textWidth) * 0.5f;
    } else if (command.justifyH == 2) {
        originX += command.rect.width - textWidth;
    }
    if (command.justifyV == 1) {
        originY += (command.rect.height - textHeight) * 0.5f;
    } else if (command.justifyV == 2) {
        originY += command.rect.height - textHeight;
    }
    if (x != nullptr) {
        *x = originX;
    }
    if (y != nullptr) {
        *y = originY;
    }
    if (width != nullptr) {
        *width = textWidth;
    }
    if (height != nullptr) {
        *height = textHeight;
    }
}

bool UiSpriteSize(const char* spriteName, float* width, float* height) {
    if (spriteName == nullptr || spriteName[0] == '\0') {
        return false;
    }
    std::string key(spriteName);
    if (key.size() > 4 && key.compare(key.size() - 4, 4, ".png") == 0) {
        key.resize(key.size() - 4);
    }
    int32_t low = 0;
    int32_t high = gen::kUiSpriteRectCount - 1;
    while (low <= high) {
        const int32_t middle = low + (high - low) / 2;
        const int32_t order = key.compare(gen::kUiSpriteRects[middle].name);
        if (order == 0) {
            const gen::UiSpriteRect& rect = gen::kUiSpriteRects[middle];
            if (width != nullptr) {
                *width = rect.w;
            }
            if (height != nullptr) {
                *height = rect.h;
            }
            return rect.w > 0.0f && rect.h > 0.0f;
        }
        if (order < 0) {
            high = middle - 1;
        } else {
            low = middle + 1;
        }
    }
    return false;
}

bool UiSpriteUv(const char* texturePath, float* u0, float* v0, float* u1, float* v1) {
    const std::string stem = StemOf(texturePath);
    if (stem.empty()) {
        return false;
    }
    for (const gen::UiSpriteInfo& sprite : gen::kUiSprites) {
        if (sprite.name == nullptr || stem != sprite.name) {
            continue;
        }
        if (u0 != nullptr) {
            *u0 = sprite.u0;
        }
        if (v0 != nullptr) {
            *v0 = sprite.v0;
        }
        if (u1 != nullptr) {
            *u1 = sprite.u1;
        }
        if (v1 != nullptr) {
            *v1 = sprite.v1;
        }
        return true;
    }
    return false;
}

int32_t EmitLayoutCommands(const std::vector<LayoutDrawCommand>& commands, float screenWidth,
                           float screenHeight, float* out, int32_t capacityVertices,
                           LayoutRenderStats* stats) {
    if (out == nullptr || capacityVertices <= 0 || !(screenWidth > 0.0f) || !(screenHeight > 0.0f)) {
        return 0;
    }
    int32_t offset = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (offset + 6 > capacityVertices) {
            if (stats != nullptr) {
                stats->dropped += 1;
            }
            continue;
        }
        /*
         * 贴图通道: 素材名能在界面图集里查到就写真实 uv, 查不到保持纯色约定.
         * uv 顺序与四角对应: 左下, 右下, 右上, 左上.
         */
        float u0 = kLayoutSolidUv;
        float v0 = kLayoutSolidUv;
        float u1 = kLayoutSolidUv;
        float v1 = kLayoutSolidUv;
        bool textured = false;
        if (command.texture != nullptr) {
            /* 带精灵名时按名字取子图集矩形, 否则按整图名去查. */
            textured = command.sprite != nullptr && command.sprite[0] != '\0'
                           ? UiSpriteRectUv(command.sprite, &u0, &v0, &u1, &v1)
                           : UiSpriteUv(command.texture, &u0, &v0, &u1, &v1);
        }

        /* 颜色可覆盖: 文字描边用深色, 正文用命令自身颜色. */
        auto emitQuad = [&](float x0, float y0, float x1, float y1, float su0, float sv0, float su1,
                            float sv1, const float* colorOverride = nullptr) {
            if (offset + 6 > capacityVertices) {
                if (stats != nullptr) {
                    stats->dropped += 1;
                }
                return;
            }
            /* 像素矩形转裁剪空间: 左上角 (-1, 1), 右下角 (1, -1). */
            const float left = x0 / screenWidth * 2.0f - 1.0f;
            const float right = x1 / screenWidth * 2.0f - 1.0f;
            const float top = 1.0f - y0 / screenHeight * 2.0f;
            const float bottom = 1.0f - y1 / screenHeight * 2.0f;
            const float positions[4][2] = {{left, bottom}, {right, bottom}, {right, top},
                                           {left, top}};
            /* 贴图翻转: 交换 uv 两端, 复用同一张贴图的镜像朝向. */
            const float fu0 = command.flipU ? su1 : su0;
            const float fu1 = command.flipU ? su0 : su1;
            const float fv0 = command.flipV ? sv1 : sv0;
            const float fv1 = command.flipV ? sv0 : sv1;
            /*
             * 直角旋转: 把 uv 的两个轴对调.
             * 顶点顺序固定为左下/右下/右上/左上, 旋转后逐个角重新取 uv.
             */
            float uvs[4][2] = {{fu0, fv0}, {fu1, fv0}, {fu1, fv1}, {fu0, fv1}};
            if (command.rotateQuarter == 1) {
                const float rotated[4][2] = {{fu0, fv1}, {fu0, fv0}, {fu1, fv0}, {fu1, fv1}};
                for (int32_t index = 0; index < 4; ++index) {
                    uvs[index][0] = rotated[index][0];
                    uvs[index][1] = rotated[index][1];
                }
            } else if (command.rotateQuarter == 2) {
                /* 180 度: 四个角全部对角交换. */
                const float rotated[4][2] = {{fu1, fv1}, {fu0, fv1}, {fu0, fv0}, {fu1, fv0}};
                for (int32_t index = 0; index < 4; ++index) {
                    uvs[index][0] = rotated[index][0];
                    uvs[index][1] = rotated[index][1];
                }
            } else if (command.rotateQuarter == 3) {
                const float rotated[4][2] = {{fu1, fv0}, {fu1, fv1}, {fu0, fv1}, {fu0, fv0}};
                for (int32_t index = 0; index < 4; ++index) {
                    uvs[index][0] = rotated[index][0];
                    uvs[index][1] = rotated[index][1];
                }
            }
            const int32_t order[6] = {0, 1, 2, 0, 2, 3};
            for (int32_t index = 0; index < 6; ++index) {
                float* vertex = out + static_cast<size_t>(offset) * kLayoutFloatsPerVertex;
                vertex[0] = positions[order[index]][0];
                vertex[1] = positions[order[index]][1];
                vertex[2] = uvs[order[index]][0];
                vertex[3] = uvs[order[index]][1];
                vertex[4] = colorOverride != nullptr ? colorOverride[0] : command.red;
                vertex[5] = colorOverride != nullptr ? colorOverride[1] : command.green;
                vertex[6] = colorOverride != nullptr ? colorOverride[2] : command.blue;
                vertex[7] = colorOverride != nullptr ? colorOverride[3] : command.alpha;
                offset += 1;
            }
            if (stats != nullptr) {
                stats->quads += 1;
                if (textured) {
                    stats->textured += 1;
                }
            }
        };

        /*
         * 平铺: 九宫参数里边角为 0 时, 按 tile 尺寸重复贴图块填满区域.
         * 布局里大量底板用这种写法(tilew/tileh 从 1 到 100 不等), 整体拉伸会明显变形.
         */
        const bool tiled = textured && command.insetLeft <= 0.0f && command.insetRight <= 0.0f &&
                           command.insetTop <= 0.0f && command.insetBottom <= 0.0f &&
                           command.tileWidth > 0.0f && command.tileHeight > 0.0f &&
                           (command.rect.width > command.tileWidth ||
                            command.rect.height > command.tileHeight);
        if (tiled) {
            const float right = command.rect.x + command.rect.width;
            const float bottom = command.rect.y + command.rect.height;
            /* 平铺起点按模板声明的偏移错开, 决定图案与容器边界的对齐. */
            for (float y = command.rect.y - command.tileOffsetY; y < bottom;
                 y += command.tileHeight) {
                for (float x = command.rect.x - command.tileOffsetX; x < right;
                     x += command.tileWidth) {
                    if (y + command.tileHeight <= command.rect.y ||
                        x + command.tileWidth <= command.rect.x) {
                        continue;
                    }
                    const float x1 = x + command.tileWidth > right ? right : x + command.tileWidth;
                    const float y1 = y + command.tileHeight > bottom ? bottom : y + command.tileHeight;
                    /* 不满一块时按比例截取 uv, 避免整块压缩. */
                    const float u1Clipped =
                        u0 + (u1 - u0) * (x1 - x) / command.tileWidth;
                    const float v1Clipped =
                        v0 + (v1 - v0) * (y1 - y) / command.tileHeight;
                    emitQuad(x, y, x1, y1, u0, v0, u1Clipped, v1Clipped);
                }
            }
        }

        /*
         * 九宫与三片: 边角按模板内边距固定, 中间拉伸.
         * 需要精灵的像素尺寸把内边距换算成 uv 比例; 取不到尺寸时退回整体拉伸.
         */
        float sourceWidth = 0.0f;
        float sourceHeight = 0.0f;
        const bool hasInsets = !tiled && (command.insetLeft > 0.0f || command.insetRight > 0.0f ||
                                          command.insetTop > 0.0f || command.insetBottom > 0.0f);
        const bool canSlice = textured && hasInsets && command.sprite != nullptr &&
                              UiSpriteSize(command.sprite, &sourceWidth, &sourceHeight) &&
                              sourceWidth > 0.0f && sourceHeight > 0.0f &&
                              command.rect.width > command.insetLeft + command.insetRight &&
                              command.rect.height > command.insetTop + command.insetBottom;
        /*
         * 有内边距却切不了: 只统计"贴图取到了、但内边距之和超过元素尺寸"的情况.
         * 贴图本身缺失(无精灵名或图集里没有)属于素材问题, 不该记成切片退化.
         */
        if (hasInsets && textured && !canSlice && stats != nullptr) {
            stats->sliceFallback += 1;
        }
        const float left = command.rect.x;
        const float top = command.rect.y;
        const float right = left + command.rect.width;
        const float bottom = top + command.rect.height;
        /* 纯文本命令不画底色块: 底色会把文字盖成一片实心矩形. */
        const bool drawBackground = command.texture != nullptr || command.text == nullptr;
        if (!drawBackground) {
            /* 只画字形, 下面统一展开. */
        } else if (!canSlice) {
            /*
             * 平铺已经画满整块区域, 这里不能再补一张整体拉伸 ——
             * 那会把平铺图案整个盖住, 看起来又变回拉伸.
             */
            if (!tiled) {
                emitQuad(left, top, right, bottom, u0, v0, u1, v1);
            }
        } else {
            const float xs[4] = {left, left + command.insetLeft, right - command.insetRight, right};
            const float ys[4] = {top, top + command.insetTop, bottom - command.insetBottom, bottom};
            const float us[4] = {u0, u0 + (u1 - u0) * command.insetLeft / sourceWidth,
                                 u1 - (u1 - u0) * command.insetRight / sourceWidth, u1};
            const float vs[4] = {v0, v0 + (v1 - v0) * command.insetTop / sourceHeight,
                                 v1 - (v1 - v0) * command.insetBottom / sourceHeight, v1};
            for (int32_t row = 0; row < 3; ++row) {
                for (int32_t col = 0; col < 3; ++col) {
                    emitQuad(xs[col], ys[row], xs[col + 1], ys[row + 1], us[col], vs[row],
                             us[col + 1], vs[row + 1]);
                }
            }
        }

        /*
         * 文本: 5x7 点阵逐位展开成纯色小方块.
         * 字号决定单个点阵像素的边长, 定位走 LayoutTextOrigin 的对齐规则.
         */
        if (command.text != nullptr && command.text[0] != '\0') {
            const float fontSize = command.fontSize > 0.0f ? command.fontSize : 20.0f;
            const float pixelSize = fontSize / static_cast<float>(kGlyphHeight);
            float textX = 0.0f;
            float textY = 0.0f;
            LayoutTextOrigin(command, pixelSize, &textX, &textY, nullptr, nullptr);
            /*
             * 画一遍文字: 偏移与颜色由调用方给, 描边与正文共用这段.
             * 单行文字超出帧宽的部分不绘制, 与原版单行控件的裁剪行为一致.
             */
            const float clipRight =
                command.rect.width > 0.0f ? command.rect.x + command.rect.width : 0.0f;
            const auto drawText = [&](float shiftX, float shiftY, const float* color) {
                float cursorX = textX + shiftX;
                const float baseY = textY + shiftY;
                for (const char* p = command.text; *p != '\0'; ++p) {
                    if (clipRight > 0.0f && cursorX >= clipRight) {
                        if (stats != nullptr && shiftX == 0.0f && shiftY == 0.0f) {
                            /* 只统计正文那一遍, 描边四遍不重复计. */
                            for (const char* rest = p; *rest != '\0'; ++rest) {
                                stats->clippedChars += 1;
                            }
                        }
                        break;
                    }
                    const uint8_t* rows = GlyphRows(*p);
                    if (rows != nullptr) {
                        for (int32_t row = 0; row < kGlyphHeight; ++row) {
                            for (int32_t col = 0; col < kGlyphWidth; ++col) {
                                if ((rows[row] &
                                     static_cast<uint8_t>(1u << (kGlyphWidth - 1 - col))) == 0) {
                                    continue;
                                }
                                const float x0 = cursorX + static_cast<float>(col) * pixelSize;
                                const float x1 = cursorX + static_cast<float>(col + 1) * pixelSize;
                                if (clipRight > 0.0f && x0 >= clipRight) {
                                    continue;
                                }
                                emitQuad(x0, baseY + static_cast<float>(row) * pixelSize,
                                         x1 > clipRight && clipRight > 0.0f ? clipRight : x1,
                                         baseY + static_cast<float>(row + 1) * pixelSize,
                                         kLayoutSolidUv, kLayoutSolidUv, kLayoutSolidUv,
                                         kLayoutSolidUv, color);
                            }
                        }
                    }
                    cursorX += static_cast<float>(kGlyphWidth + kGlyphSpacing) * pixelSize;
                }
            };
            if (command.bold) {
                /* 加粗: 同色右移一个点阵像素再画一遍, 笔画看起来粗一倍. */
                drawText(pixelSize, 0.0f, nullptr);
            }
            if (command.bold) {
                /* 加粗: 同色右移一个点阵像素再画一遍. */
                drawText(pixelSize, 0.0f, nullptr);
            }
            if (command.outline) {
                /* 描边: 上右下左各偏移一个点阵像素, 用深色先铺一遍. */
                const float edge[4] = {command.shadowRed, command.shadowGreen,
                                       command.shadowBlue, command.alpha};
                const float shift = pixelSize;
                drawText(-shift, 0.0f, edge);
                drawText(shift, 0.0f, edge);
                drawText(0.0f, -shift, edge);
                drawText(0.0f, shift, edge);
            }
            drawText(0.0f, 0.0f, nullptr);
        }
    }
    if (stats != nullptr) {
        stats->vertices = offset;
    }
    return offset;
}

}  /* namespace immirror::ui */
