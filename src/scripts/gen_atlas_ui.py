#!/usr/bin/env python3
"""把 HUD 用的原版界面精灵合成为第二张图集.

输入: 素材管线明文工程树 (single-branch/assets/project/ui) 的精灵页 PNG 与
      本脚本内置的挑选表 (来源 texture0/operate 等页的 63 个精灵, 已对照
      assets/docs/attribution-blocklist.txt 署名排除清单逐页核对).
输出:
  src/assets/ui/generated/atlas_ui.rgba   2048x2048 裸 RGBA8, 无文件头
  src/assets/ui/generated/atlas_ui.json   格位表, 供核对与调试
  src/native/ui/generated/ui_atlas_gen.h  精灵名到 uv 与尺寸的 C++ 常量表
  src/android/.../generated/UiAtlas.java  Java 侧同源 uv 常量

与方块图集 generated/atlas.rgba 完全独立: HUD 精灵不走 16x16 网格,
按精灵实际尺寸排布, uv 记录在 ui_atlas_gen.h 里, 由 hud_renderer 直接采样.
生成器可重复运行, 输入不变时输出内容不变且不写盘.
"""

import json
import os
import struct
import sys
import zlib

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
CLIENT_ROOT = os.path.dirname(SRC_ROOT)
PROJECT_UI = os.path.join(CLIENT_ROOT, "assets", "project", "ui")
# 精灵页解码 PNG: 由素材管线的 forge.ktx_png 从 KTX 解出, ui_extract 暂存在 temp 下
DECODED_PNG = os.path.join(CLIENT_ROOT, "temp", "ui_extract", "build", "png", "_png", "mobile")

OUT_DIR = os.path.join(SRC_ROOT, "assets", "ui", "generated")
NATIVE_OUT = os.path.join(SRC_ROOT, "native", "ui", "generated")
JAVA_OUT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "java",
                        "com", "julardepick", "immirror", "generated", "UiAtlas.java")

ATLAS_PIXELS = 2048
ATLAS_BYTES = ATLAS_PIXELS * ATLAS_PIXELS * 4
# 精灵之间的空隙, 避免线性采样蹭到邻格边缘
PAD = 2

# 挑选表: HUD 全部控件与通用面板, 一行一个精灵.
# 字段: 名称, 所在页, 精灵名, 页内矩形, 裁剪画布 (含 trim 偏移), 拉伸方式与四边边界.
# 边界取自原版 uitemplate 的模板声明, 与 assets/ui/ui_manifest.json 同源同值.
SPRITES = [
    ("hp_track", "texture0/operate.xml", "img_blood_strip_board", 308, 530, 15, 15, 0, 0, 15, 15, "nine_slice", 5, 5, 5, 5),
    ("hp_fill_red", "texture0/operate.xml", "img_blood_strip_red", 308, 477, 15, 16, 0, 0, 15, 16, "stretch", 0, 0, 0, 0),
    ("hp_fill_blue", "texture0/operate.xml", "img_blood_strip_blue", 308, 495, 15, 16, 0, 0, 15, 16, "stretch", 0, 0, 0, 0),
    ("hp_fill_yellow", "texture0/operate.xml", "img_blood_strip_yellow", 308, 459, 15, 16, 0, 0, 15, 16, "stretch", 0, 0, 0, 0),
    ("slot_normal", "texture0/operate.xml", "img_item_box", 655, 395, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("slot_selected", "texture0/operate.xml", "img_item_box_selected", 655, 363, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("slot_circle", "texture0/operate.xml", "img_item_box_circle", 102, 478, 76, 76, 0, 0, 76, 76, "stretch", 0, 0, 0, 0),
    ("hotbar_board", "texture0/operate.xml", "img_down_bar", 480, 554, 60, 70, 0, 0, 60, 70, "stretch", 0, 0, 0, 0),
    ("board_res", "texture0/operate.xml", "img_board_res", 423, 154, 109, 106, 0, 0, 109, 106, "three_slice", 50, 0, 57, 0),
    ("board_scoring", "texture0/operate.xml", "img_board_scoring", 416, 554, 62, 55, 0, 0, 62, 55, "three_slice", 31, 0, 31, 0),
    ("board_scoring_s", "texture0/operate.xml", "img_board_scoring_s", 366, 611, 60, 40, 0, 0, 60, 40, "three_slice", 28, 0, 28, 0),
    ("board_scoring_sel", "texture0/operate.xml", "img_board_scoring_sel_s", 200, 627, 60, 40, 0, 0, 60, 40, "three_slice", 28, 0, 28, 0),
    ("progress_track", "texture0/operate.xml", "img_progress_bar", 514, 418, 9, 8, 0, 0, 9, 8, "three_slice", 3, 0, 3, 0),
    ("progress_orange", "texture0/operate.xml", "img_progress_bar_orange", 514, 428, 9, 6, 0, 0, 9, 6, "three_slice", 3, 0, 3, 0),
    ("progress_green", "texture0/operate.xml", "img_progress_bar_green", 514, 436, 9, 6, 0, 0, 9, 6, "three_slice", 3, 0, 3, 0),
    ("progress_yellow", "texture0/operate.xml", "img_progress_bar_yellow", 1016, 170, 5, 6, 0, 0, 5, 6, "stretch", 0, 0, 0, 0),
    ("item_slot_line", "texture0/operate.xml", "img_item", 262, 631, 58, 12, 0, 0, 58, 12, "stretch", 0, 0, 0, 0),
    ("jump", "texture0/operate.xml", "icon_jump", 816, 123, 117, 119, 0, 0, 117, 119, "stretch", 0, 0, 0, 0),
    ("backpack", "texture0/operate.xml", "icon_backpack", 879, 300, 54, 51, 0, 0, 54, 51, "stretch", 0, 0, 0, 0),
    ("heart", "texture0/operate.xml", "icon_heart", 760, 431, 20, 16, 0, 0, 20, 16, "stretch", 0, 0, 0, 0),
    ("joystick_base", "texture0/operate.xml", "icon_rocker", 694, 244, 92, 92, 0, 0, 92, 92, "stretch", 0, 0, 0, 0),
    ("joystick_knob", "texture0/operate.xml", "icon_rocker_circle", 93, 632, 74, 74, 0, 0, 74, 74, "stretch", 0, 0, 0, 0),
    ("crosshair", "texture0/operate.xml", "sight", 879, 353, 52, 52, 0, 0, 52, 52, "stretch", 0, 0, 0, 0),
    ("operating_bar", "texture0/operate.xml", "img_operating_bar_r", 423, 262, 104, 104, 0, 0, 104, 104, "stretch", 0, 0, 0, 0),
    ("operating_board_r", "texture0/operate.xml", "img_operating_board_r", 937, 88, 80, 80, 0, 0, 80, 80, "stretch", 0, 0, 0, 0),
    ("operating_board_s", "texture0/operate.xml", "img_operating_board_s", 935, 255, 74, 74, 0, 0, 74, 74, "stretch", 0, 0, 0, 0),
    ("operating_board_square", "texture0/operate.xml", "operating_board_square", 783, 430, 88, 88, 0, 0, 88, 88, "stretch", 0, 0, 0, 0),
    ("steering_wheel", "texture0/operate.xml", "operating_steering_wheel", 228, 2, 193, 193, 0, 0, 193, 193, "stretch", 0, 0, 0, 0),
    ("health_mode", "texture0/operate.xml", "img_board_healthmode_1", 2, 2, 224, 34, 0, 0, 224, 34, "stretch", 0, 0, 0, 0),
    ("map_board", "texture0/operate.xml", "img_board_map", 2, 220, 180, 180, 0, 0, 180, 180, "stretch", 0, 0, 0, 0),
    ("compass", "texture0/operate.xml", "img_north", 2, 38, 180, 180, 0, 0, 180, 180, "stretch", 0, 0, 0, 0),
    ("white", "texture0/operate.xml", "white", 308, 441, 16, 16, 0, 0, 16, 16, "stretch", 0, 0, 0, 0),
    ("sneak", "texture0/operate.xml", "icon_sneak", 217, 441, 89, 91, 0, 0, 89, 91, "stretch", 0, 0, 0, 0),
    ("sprint", "texture0/operate.xml", "icon_sprint", 818, 2, 117, 119, 0, 0, 117, 119, "stretch", 0, 0, 0, 0),
    ("shoot", "texture0/operate.xml", "icon_shoot", 2, 593, 89, 91, 0, 0, 89, 91, "stretch", 0, 0, 0, 0),
    ("arrow_down", "texture0/operate.xml", "icon_down", 2, 686, 89, 91, 0, 0, 89, 91, "stretch", 0, 0, 0, 0),
    ("arrow_up", "texture0/operate.xml", "icon_up", 574, 245, 89, 91, 0, 0, 89, 91, "stretch", 0, 0, 0, 0),
    ("chat", "texture0/operate.xml", "icon_chat", 184, 304, 37, 37, 0, 0, 37, 37, "stretch", 0, 0, 0, 0),
    ("expression", "texture0/operate.xml", "icon_expression", 509, 478, 51, 49, 0, 0, 51, 49, "stretch", 0, 0, 0, 0),
    ("horn", "texture0/operate.xml", "icon_horn", 180, 526, 31, 33, 0, 0, 31, 33, "stretch", 0, 0, 0, 0),
    ("more", "texture0/operate.xml", "icon_more", 184, 343, 36, 10, 0, 0, 36, 10, "stretch", 0, 0, 0, 0),
    ("settings", "texture0/operate.xml", "icon_set", 184, 392, 35, 34, 0, 0, 35, 34, "stretch", 0, 0, 0, 0),
    ("voice", "texture0/operate.xml", "icon_voice", 665, 300, 25, 34, 0, 0, 25, 34, "stretch", 0, 0, 0, 0),
    ("switch", "texture0/operate.xml", "icon_switch", 514, 444, 45, 32, 0, 0, 45, 32, "stretch", 0, 0, 0, 0),
    ("arrow_last", "texture0/operate.xml", "icon_last", 873, 494, 45, 32, 0, 0, 45, 32, "stretch", 0, 0, 0, 0),
    ("arrow_next", "texture0/operate.xml", "icon_next", 873, 460, 45, 32, 0, 0, 45, 32, "stretch", 0, 0, 0, 0),
    ("direction", "texture0/operate.xml", "icon_direction", 847, 520, 24, 22, 0, 0, 24, 22, "stretch", 0, 0, 0, 0),
    ("use", "texture0/operate.xml", "icon_use", 332, 290, 89, 91, 0, 0, 89, 91, "stretch", 0, 0, 0, 0),
    ("blocking", "texture0/operate.xml", "icon_blocking", 575, 124, 117, 119, 0, 0, 117, 119, "stretch", 0, 0, 0, 0),
    ("panel_blue", "texture2/common.xml", "board_bottom_blue_big", 355, 832, 60, 60, 0, 0, 60, 60, "nine_slice", 20, 20, 20, 20),
    ("panel_white", "texture2/common.xml", "board_white", 579, 875, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("panel_round", "texture2/common.xml", "board_blue_r", 739, 835, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("panel_scrim", "texture0/common.xml", "img_bd_black50", 629, 834, 15, 15, 0, 0, 15, 15, "nine_slice", 5, 5, 5, 5),
    ("panel_gray", "texture0/common.xml", "board_more", 986, 897, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("btn_primary", "texture2/common.xml", "btn_big_blue", 66, 65, 96, 61, 0, 0, 96, 61, "three_slice", 35, 0, 35, 0),
    ("btn_secondary", "texture2/common.xml", "btn_big_yellow", 144, 511, 182, 61, 0, 0, 182, 61, "three_slice", 45, 0, 46, 0),
    ("btn_danger", "texture2/common.xml", "btn_big_red", 195, 392, 182, 61, 0, 0, 182, 61, "three_slice", 45, 0, 46, 0),
    ("btn_alert", "texture2/common.xml", "btn_big_orange", 183, 574, 158, 61, 0, 0, 158, 61, "three_slice", 45, 0, 46, 0),
    ("btn_compact_orange", "texture2/common.xml", "btn_mini_orange", 176, 92, 40, 43, 0, 0, 40, 43, "three_slice", 15, 0, 15, 0),
    ("btn_compact_blue", "texture2/common.xml", "btn_mini_blue", 466, 785, 40, 43, 0, 0, 40, 43, "three_slice", 15, 0, 15, 0),
    ("close", "texture2/common.xml", "btn_close", 667, 771, 36, 46, 0, 0, 36, 46, "stretch", 0, 0, 0, 0),
    ("tips_board", "texture2/messagebox.xml", "board_tips_icon", 334, 2, 30, 30, 0, 0, 30, 30, "nine_slice", 10, 10, 10, 10),
    ("dialog_body", "texture2/messagebox.xml", "board_clip", 2, 71, 300, 90, 0, 0, 300, 90, "nine_slice", 100, 30, 100, 30),
]

# 署名排除清单的页名集合, 命中即拒绝打包; 与 single-branch/assets/docs 排除清单同步.
BLOCKLIST_PAGES = frozenset(["minilobby"])


def decode_png(path):
    """读一页 PNG, 返回 (宽, 高, RGBA 字节), 只支持 8 位非交错格式."""
    with open(path, "rb") as handle:
        data = handle.read()
    pos = 8
    idat = b""
    palette = None
    trns = None
    width = height = colortype = 0
    while pos < len(data):
        length, ctype = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            width, height, _bitdepth, colortype = struct.unpack(">IIBB", body[:10])
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"PLTE":
            palette = body
        elif ctype == b"tRNS":
            trns = body
        pos += 12 + length
        if ctype == b"IEND":
            break
    if colortype not in (0, 2, 3, 4, 6):
        raise SystemExit("不支持的 PNG 颜色类型 %d: %s" % (colortype, path))
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[colortype]
    raw = zlib.decompress(idat)
    stride = width * channels
    out = bytearray(width * height * 4)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        scan_filter = raw[pos]
        pos += 1
        line = bytearray(raw[pos:pos + stride])
        pos += stride
        if scan_filter == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 255
        elif scan_filter == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 255
        elif scan_filter == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 255
        elif scan_filter == 4:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                up = prev[i]
                upper_left = prev[i - channels] if i >= channels else 0
                predictor = left + up - upper_left
                distance_left = abs(predictor - left)
                distance_up = abs(predictor - up)
                distance_corner = abs(predictor - upper_left)
                if distance_left <= distance_up and distance_left <= distance_corner:
                    nearest = left
                elif distance_up <= distance_corner:
                    nearest = up
                else:
                    nearest = upper_left
                line[i] = (line[i] + nearest) & 255
        prev = line
        for x in range(width):
            offset = (y * width + x) * 4
            if colortype == 6:
                out[offset:offset + 4] = line[x * 4:x * 4 + 4]
            elif colortype == 2:
                out[offset] = line[x * 3]
                out[offset + 1] = line[x * 3 + 1]
                out[offset + 2] = line[x * 3 + 2]
                out[offset + 3] = 255
            elif colortype == 3:
                index = line[x]
                out[offset] = palette[index * 3]
                out[offset + 1] = palette[index * 3 + 1]
                out[offset + 2] = palette[index * 3 + 2]
                out[offset + 3] = trns[index] if trns and index < len(trns) else 255
            elif colortype == 0:
                gray = line[x]
                out[offset] = out[offset + 1] = out[offset + 2] = gray
                out[offset + 3] = 255
            elif colortype == 4:
                gray = line[x * 2]
                out[offset] = out[offset + 1] = out[offset + 2] = gray
                out[offset + 3] = line[x * 2 + 1]
    return width, height, bytes(out)


def blit(canvas, page_pixels, page_width, entry, cell_x, cell_y):
    """把一个精灵画进图集画布, 裁剪页内矩形并补回 trim 偏移."""
    for row in range(entry["h"]):
        src_row = ((entry["y"] + row) * page_width + entry["x"]) * 4
        for col in range(entry["w"]):
            dst_col = entry["oX"] + col
            dst_row = entry["oY"] + row
            if dst_col < 0 or dst_col >= entry["oW"] or dst_row < 0 or dst_row >= entry["oH"]:
                continue
            src = src_row + col * 4
            dst = ((cell_y + dst_row) * ATLAS_PIXELS + cell_x + dst_col) * 4
            canvas[dst:dst + 4] = page_pixels[src:src + 4]


def pack_entries(entries):
    """按高度降序的货架排布, 返回 {名称: (x, y, 宽, 高)} 与已占用像素数."""
    placements = {}
    shelf_x = PAD
    shelf_y = PAD
    shelf_h = 0
    used = 0
    for entry in sorted(entries, key=lambda item: (-item["oH"], -item["oW"], item["name"])):
        w, h = entry["oW"], entry["oH"]
        if w > ATLAS_PIXELS - PAD * 2 or h > ATLAS_PIXELS - PAD * 2:
            raise SystemExit("精灵超出图集尺寸: %s %dx%d" % (entry["name"], w, h))
        if shelf_x + w + PAD > ATLAS_PIXELS:
            shelf_y += shelf_h + PAD
            shelf_x = PAD
            shelf_h = 0
        if shelf_y + h + PAD > ATLAS_PIXELS:
            raise SystemExit("图集放不下全部精灵, 当前卡在: %s" % entry["name"])
        placements[entry["name"]] = (shelf_x, shelf_y, w, h)
        shelf_x += w + PAD
        shelf_h = max(shelf_h, h)
        used += w * h
    return placements, used


def enum_name(name):
    """精灵名转枚举名, 例如 btn_compact_orange 得 BtnCompactOrange."""
    return "".join(chunk.capitalize() for chunk in name.split("_") if chunk)


def write_if_changed(path, payload):
    """内容不变不写盘, 保证重复运行不刷新构建输入."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        with open(path, "rb") as handle:
            if handle.read() == payload:
                return False
    with open(path, "wb") as handle:
        handle.write(payload)
    return True


def render_header(entries, placements):
    """生成 C++ 头: 枚举, 常量表与越界回落函数."""
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("/*")
    lines.append(" * 界面图集格位表, 由 scripts/gen_atlas_ui.py 生成, 请勿手改.")
    lines.append(" * 图集为 %dx%d 裸 RGBA8, 随包路径 ui/generated/atlas_ui.rgba." % (ATLAS_PIXELS, ATLAS_PIXELS))
    lines.append(" * 上传时按整图垂直翻转 (数据首行落到 v=1), 因此这里的 v 已经换算为")
    lines.append(" * 翻转后的取值: v0 是精灵下边缘, v1 是精灵上边缘, 与 PushQuad 一致.")
    lines.append(" */")
    lines.append("namespace immirror::ui::gen {")
    lines.append("")
    lines.append("/* 图集边长, 与生成器 ATLAS_PIXELS 一致. */")
    lines.append("inline constexpr int32_t kUiAtlasPixels = %d;" % ATLAS_PIXELS)
    lines.append("")
    lines.append("/* 界面精灵枚举, 顺序与 kUiSprites 逐项对应. */")
    lines.append("enum class UiSprite : uint16_t {")
    for entry in entries:
        lines.append("    %s," % enum_name(entry["name"]))
    lines.append("    Count,")
    lines.append("};")
    lines.append("")
    lines.append("/* 一个界面精灵: 图集内归一化 uv, 素材尺寸与拉伸边界 (素材像素). */")
    lines.append("struct UiSpriteInfo {")
    lines.append("    const char* name;")
    lines.append("    float u0;")
    lines.append("    float v0;")
    lines.append("    float u1;")
    lines.append("    float v1;")
    lines.append("    float width;")
    lines.append("    float height;")
    lines.append("    float left;")
    lines.append("    float top;")
    lines.append("    float right;")
    lines.append("    float bottom;")
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr UiSpriteInfo kUiSprites[] = {")
    for entry in entries:
        x, y, w, h = placements[entry["name"]]
        u0 = x / float(ATLAS_PIXELS)
        u1 = (x + w) / float(ATLAS_PIXELS)
        # 整图垂直翻转后的 v: 原始 v 范围 [y, y+h] 映射为 [1 - (y+h), 1 - y]
        v0 = 1.0 - (y + h) / float(ATLAS_PIXELS)
        v1 = 1.0 - y / float(ATLAS_PIXELS)
        lines.append('    {"%s", %.6ff, %.6ff, %.6ff, %.6ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff},' % (
            entry["name"], u0, v0, u1, v1, float(w), float(h),
            float(entry["l"]), float(entry["t"]), float(entry["r"]), float(entry["b"])))
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr size_t kUiSpriteCount = sizeof(kUiSprites) / sizeof(kUiSprites[0]);")
    lines.append("")
    lines.append('static_assert(kUiSpriteCount == static_cast<size_t>(UiSprite::Count),')
    lines.append('              "界面精灵枚举与格位表条目数不一致, 请重新运行 gen_atlas_ui.py");')
    lines.append("")
    lines.append("/* 按枚举取精灵参数, 越界时回落到首项, 避免绘制路径读到脏数据. */")
    lines.append("inline const UiSpriteInfo& UiSpriteOf(UiSprite sprite) {")
    lines.append("    const size_t index = static_cast<size_t>(sprite);")
    lines.append("    return index < kUiSpriteCount ? kUiSprites[index] : kUiSprites[0];")
    lines.append("}")
    lines.append("")
    lines.append("}  /* namespace immirror::ui::gen */")
    lines.append("")
    return ("\n".join(lines)).encode("utf-8")


def render_java(entries, placements):
    """生成 Java 图集常量: 精灵枚举与 uv 表, 与 UiResources.Widget 并存."""
    lines = []
    lines.append("package com.julardepick.immirror.generated;")
    lines.append("")
    lines.append("/*")
    lines.append(" * 界面图集格位表, 由 scripts/gen_atlas_ui.py 生成, 请勿手改.")
    lines.append(" * 与 UiResources.Widget 并存: 这里是图集内 uv, 那里是单文件素材参数.")
    lines.append(" */")
    lines.append("public final class UiAtlas {")
    lines.append("")
    lines.append("    /* 图集边长, 与生成器 ATLAS_PIXELS 一致. */")
    lines.append("    public static final int ATLAS_PIXELS = %d;" % ATLAS_PIXELS)
    lines.append("")
    lines.append("    /* 精灵枚举, 顺序与生成器挑选表逐项对应. */")
    lines.append("    public enum Sprite {")
    for index, entry in enumerate(entries):
        tail = ";" if index == len(entries) - 1 else ","
        x, y, w, h = placements[entry["name"]]
        lines.append('        %s(%.6ff, %.6ff, %.6ff, %.6ff, %d, %d)%s' % (
            enum_name(entry["name"]),
            x / float(ATLAS_PIXELS), y / float(ATLAS_PIXELS),
            (x + w) / float(ATLAS_PIXELS), (y + h) / float(ATLAS_PIXELS),
            w, h, tail))
    lines.append("")
    lines.append("        public final float u0;")
    lines.append("        public final float v0;")
    lines.append("        public final float u1;")
    lines.append("        public final float v1;")
    lines.append("        public final int width;")
    lines.append("        public final int height;")
    lines.append("")
    lines.append("        Sprite(float u0, float v0, float u1, float v1, int width, int height) {")
    lines.append("            this.u0 = u0;")
    lines.append("            this.v0 = v0;")
    lines.append("            this.u1 = u1;")
    lines.append("            this.v1 = v1;")
    lines.append("            this.width = width;")
    lines.append("            this.height = height;")
    lines.append("        }")
    lines.append("    }")
    lines.append("")
    lines.append("    private UiAtlas() {")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    return ("\n".join(lines)).encode("utf-8")


def main() -> int:
    # 精灵登记它实际所在页; 页名去 .xml 后缀直接定位明文工程树里的 PNG
    entries = [
        {
            "name": item[0], "page": item[1], "sprite": item[2],
            "x": item[3], "y": item[4], "w": item[5], "h": item[6],
            "oX": item[7], "oY": item[8], "oW": item[9], "oH": item[10],
            "draw": item[11], "l": item[12], "t": item[13], "r": item[14], "b": item[15],
        }
        for item in SPRITES
    ]
    for entry in entries:
        page_stem = entry["page"][:-len(".xml")]
        page_name = page_stem.split("/", 1)[1]
        if page_name in BLOCKLIST_PAGES:
            raise SystemExit("精灵 %s 命中署名排除清单: %s" % (entry["name"], entry["page"]))
    placements, used = pack_entries(entries)
    canvas = bytearray(ATLAS_BYTES)
    page_cache = {}
    for entry in entries:
        page_stem = entry["page"][:-len(".xml")]
        if page_stem not in page_cache:
            png_path = os.path.join(DECODED_PNG, page_stem + ".png")
            if not os.path.exists(png_path):
                raise SystemExit("精灵页解码 PNG 缺失: %s, 请先运行 temp/ui_extract/ui_extract.py 解码" % png_path)
            page_cache[page_stem] = decode_png(png_path)
        page_width, _page_height, pixels = page_cache[page_stem]
        cell_x, cell_y, _w, _h = placements[entry["name"]]
        blit(canvas, pixels, page_width, entry, cell_x, cell_y)
    atlas_changed = write_if_changed(os.path.join(OUT_DIR, "atlas_ui.rgba"), bytes(canvas))
    meta = {
        "pixels": ATLAS_PIXELS,
        "pad": PAD,
        "sprites": [
            {
                "name": entry["name"], "page": entry["page"], "sprite": entry["sprite"],
                "x": placements[entry["name"]][0], "y": placements[entry["name"]][1],
                "w": placements[entry["name"]][2], "h": placements[entry["name"]][3],
                "draw": entry["draw"],
                "insets": {"left": entry["l"], "top": entry["t"], "right": entry["r"], "bottom": entry["b"]},
            }
            for entry in entries
        ],
    }
    json_changed = write_if_changed(os.path.join(OUT_DIR, "atlas_ui.json"),
                                    (json.dumps(meta, ensure_ascii=False, indent=1) + "\n").encode("utf-8"))
    header_changed = write_if_changed(os.path.join(NATIVE_OUT, "ui_atlas_gen.h"), render_header(entries, placements))
    java_changed = write_if_changed(JAVA_OUT, render_java(entries, placements))
    print("界面图集: %d 个精灵, 占用 %d/%d 像素 (%.1f%%)" % (
        len(entries), used, ATLAS_PIXELS * ATLAS_PIXELS, 100.0 * used / (ATLAS_PIXELS * ATLAS_PIXELS)))
    print("  图集  %s (%s)" % (os.path.relpath(os.path.join(OUT_DIR, "atlas_ui.rgba"), SRC_ROOT),
                              "已更新" if atlas_changed else "未变化"))
    print("  格位  %s (%s)" % (os.path.relpath(os.path.join(OUT_DIR, "atlas_ui.json"), SRC_ROOT),
                              "已更新" if json_changed else "未变化"))
    print("  C++   %s (%s)" % (os.path.relpath(os.path.join(NATIVE_OUT, "ui_atlas_gen.h"), SRC_ROOT),
                              "已更新" if header_changed else "未变化"))
    print("  Java  %s (%s)" % (os.path.relpath(JAVA_OUT, SRC_ROOT),
                              "已更新" if java_changed else "未变化"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
