#!/usr/bin/env python3
"""生成界面精灵的合图.

输入: src/native/ui/generated/ui_sprite_index_gen.h
      single-branch/client/temp/ui_extract/build/png/_png/<page>.png
输出: src/assets/ui/generated/atlas_ui_full.rgba  (4096x4096 裸 RGBA8)
      src/native/ui/generated/ui_sprite_atlas_gen.h
"""

import io
import os
import re
import struct
import sys
import zlib

PAD = 2
PIXELS = 4096
BYTES = PIXELS * PIXELS * 4

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CLIENT = os.path.dirname(ROOT)
INDEX = os.path.join(ROOT, "native", "ui", "generated", "ui_sprite_index_gen.h")
PNG_ROOT = os.path.join(CLIENT, "temp", "ui_extract", "build", "png", "_png")
OUT_RGBA = os.path.join(ROOT, "assets", "ui", "generated", "atlas_ui_full.rgba")
OUT_HEAD = os.path.join(ROOT, "native", "ui", "generated", "ui_sprite_atlas_gen.h")

ROW = re.compile(r'\{"([^"]+)",\s*"([^"]+)",\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f\}')


def decode_png(path):
    data = io.open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not png")
    off = 8
    w = h = depth = color = 0
    idat = b""
    while off + 8 <= len(data):
        length = struct.unpack(">I", data[off:off + 4])[0]
        kind = data[off + 4:off + 8]
        body = data[off + 8:off + 8 + length]
        off += 12 + length
        if kind == b"IHDR":
            w, h, depth, color = struct.unpack(">IIBB", body[:10])
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    if depth != 8 or color not in (2, 6):
        raise ValueError("bad png")
    ch = 4 if color == 6 else 3
    raw = zlib.decompress(idat)
    stride = w * ch
    out = bytearray(w * h * 4)
    prev = bytearray(stride)
    pos = 0
    for row in range(h):
        ft = raw[pos]
        pos += 1
        line = bytearray(raw[pos:pos + stride])
        pos += stride
        if ft == 1:
            for i in range(ch, stride):
                line[i] = (line[i] + line[i - ch]) & 0xFF
        elif ft == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ft == 3:
            for i in range(stride):
                left = line[i - ch] if i >= ch else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ft == 4:
            for i in range(stride):
                a = line[i - ch] if i >= ch else 0
                b = prev[i]
                c = prev[i - ch] if i >= ch else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 0xFF
        prev = line
        base = row * w * 4
        for col in range(w):
            s = col * ch
            d = base + col * 4
            out[d] = line[s]
            out[d + 1] = line[s + 1]
            out[d + 2] = line[s + 2]
            out[d + 3] = line[s + 3] if ch == 4 else 255
    return w, h, bytes(out)


def main():
    body = io.open(INDEX, encoding="utf-8").read()
    rows = ROW.findall(body)
    print("索引精灵: %d" % len(rows))
    items = []
    for name, page, u0, v0, u1, v1, w, h in rows:
        items.append((name, page, float(u0), float(v0), float(w), float(h)))
    items.sort(key=lambda it: (-it[5], -it[4], it[0]))
    place = {}
    canvas = bytearray(BYTES)
    shelf_x = PAD
    shelf_y = PAD
    shelf_h = 0
    for it in items:
        name = it[0]
        iw, ih = int(round(it[4])), int(round(it[5]))
        if iw <= 0 or ih <= 0 or iw > PIXELS - PAD * 2 or ih > PIXELS - PAD * 2:
            continue
        if shelf_x + iw + PAD > PIXELS:
            shelf_y += shelf_h + PAD
            shelf_x = PAD
            shelf_h = 0
        if shelf_y + ih + PAD > PIXELS:
            print("警告: 单页放不下, 停在 %s (已排 %d)" % (name, len(place)))
            break
        place[name] = (shelf_x, shelf_y, iw, ih, it[1], it[2], it[3])
        shelf_x += iw + PAD
        if ih > shelf_h:
            shelf_h = ih
    print("已排布: %d" % len(place))
    cache = {}
    pasted = 0
    missing = 0
    for name, (cx, cy, iw, ih, page, u0, v0) in place.items():
        stem = page[:-4] if page.endswith(".xml") else page
        stem = stem.replace("ui/mobile/", "").replace("/", os.sep)
        png = os.path.join(PNG_ROOT, stem + ".png")
        if not os.path.exists(png):
            missing += 1
            continue
        if png not in cache:
            cache[png] = decode_png(png)
        pw, ph, pixels = cache[png]
        ox = int(round(u0 * pw))
        oy = int(round(v0 * ph))
        for row in range(ih):
            srow = oy + row
            if srow < 0 or srow >= ph:
                break
            s = (srow * pw + ox) * 4
            d = ((cy + row) * PIXELS + cx) * 4
            canvas[d:d + iw * 4] = pixels[s:s + iw * 4]
        pasted += 1
    print("已贴 %d, 缺页 %d" % (pasted, missing))
    os.makedirs(os.path.dirname(OUT_RGBA), exist_ok=True)
    old = io.open(OUT_RGBA, "rb").read() if os.path.exists(OUT_RGBA) else b""
    if old != bytes(canvas):
        io.open(OUT_RGBA, "wb").write(bytes(canvas))
        print("写出 %s (%.1f MB)" % (OUT_RGBA, len(canvas) / 1048576.0))
    lines = ["#pragma once", "", "#include <cstddef>", "#include <cstdint>", "",
             "/* 界面精灵合图格位表, 由 scripts/gen_atlas_ui_full.py 生成, 请勿手改. */",
             "namespace immirror::ui::gen {", "",
             "inline constexpr int32_t kUiAtlasFullPixels = %d;" % PIXELS, "",
             "struct UiAtlasSprite {", "    const char* name;",
             "    float u0;", "    float v0;", "    float u1;", "    float v1;",
             "    float w;", "    float h;", "};", "",
             "inline constexpr UiAtlasSprite kUiAtlasSprites[] = {"]
    for name, (cx, cy, iw, ih, page, u0, v0) in sorted(place.items()):
        # 上传时按整图垂直翻转, 因此 v0 取精灵下边缘, v1 取上边缘
        lines.append('    {"%s", %.6ff, %.6ff, %.6ff, %.6ff, %.1ff, %.1ff},' % (
            name, cx / float(PIXELS), (PIXELS - cy - ih) / float(PIXELS),
            (cx + iw) / float(PIXELS), (PIXELS - cy) / float(PIXELS), float(iw), float(ih)))
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr int32_t kUiAtlasSpriteCount =")
    lines.append("    static_cast<int32_t>(sizeof(kUiAtlasSprites) / sizeof(kUiAtlasSprites[0]));")
    lines.append("")
    lines.append("}  /* namespace immirror::ui::gen */")
    lines.append("")
    payload = "\n".join(lines)
    old = io.open(OUT_HEAD, encoding="utf-8").read() if os.path.exists(OUT_HEAD) else ""
    if old != payload:
        io.open(OUT_HEAD, "w", encoding="utf-8", newline="\n").write(payload)
        print("写出 %s (%d 条)" % (OUT_HEAD, len(place)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
