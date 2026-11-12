#pragma once

#include <cstdint>

namespace immirror::ui {

/*
 * 5x7 点阵字形. 每个字形固定 7 行, 每行低 5 位有效, bit4 为最左列.
 * 界面层目前没有字体纹理, 文字用纯色矩形拼出, 用于方块名这类短标签;
 * 接入字体资源后可以直接替换这一层, 调用方不需要改动.
 */
constexpr int32_t kGlyphWidth = 5;
constexpr int32_t kGlyphHeight = 7;
/* 字形之间的空白列数. */
constexpr int32_t kGlyphSpacing = 1;

/* 取字形点阵, 未收录的字符返回空指针, 渲染时按空白处理. */
const uint8_t* GlyphRows(char character);

/* 文本在设计坐标下占用的宽度, pixelSize 为单个点阵像素的边长. */
float MeasureText(const char* text, float pixelSize);

}  /* namespace immirror::ui */
