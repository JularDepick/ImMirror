# -*- coding: utf-8 -*-
"""ETC 系列 4x4 块压缩纹理的软件解码与简单再编码。

覆盖两种像素格式:

- etc1       ETC1 RGB8 (glInternalFormat 0x8D64)
- etc2-rgba  ETC2 RGBA8 (glInternalFormat 0x9278), ETC2 RGB 块加 EAC alpha 块

位布局、调制表与像素遍历顺序对齐 Khronos 规范与 Ericsson ETCPACK 参考解码器,
ETC1 的遍历顺序在真实素材上做过像素级验证, 结论见 docs/notes/ktx.md。

块内约定:
- 每块 4x4 像素; 首字 (块内 bits 63..32) 放颜色与控制位, 次字 (bits 31..0) 放索引;
- 索引分高低两个位平面, 低位对应子块内遍历序的第一个像素;
- 子块内遍历为列优先 (x 外层, y 内层), 步长随 flip 位切换。
"""

from __future__ import annotations

from .base import CodecError

"""ETC1/ETC2 的 8 档调制表, 每档给出小/大两个幅度。"""
MODIFIER_STEPS = ((2, 8), (5, 17), (9, 29), (13, 42), (18, 60), (24, 80), (33, 106), (47, 183))

"""规范里使用幅度 0..3 档的顺序: 大幅负、小幅负、小幅正、大幅正。"""
MODIFIER_ORDER = tuple((-large, -small, small, large) for small, large in MODIFIER_STEPS)

"""索引到调制档的置乱表, 规范固定为 2/3/1/0。"""
INDEX_SCRAMBLE = (2, 3, 1, 0)

"""置乱表的逆, 编码时由调制档反查原始两位码。"""
INDEX_SCRAMBLE_INVERSE = tuple(INDEX_SCRAMBLE.index(index) for index in range(4))

"""ETC2 T/H 模式的 3 位距离表。"""
DISTANCE_TABLE = (3, 6, 11, 16, 23, 32, 41, 64)

"""EAC alpha 的 16 张基表, 每张 4 个负向幅度, 正向幅度由取反减一得到。"""
ALPHA_BASE = (
    (-15, -9, -6, -3),
    (-13, -10, -7, -3),
    (-13, -8, -5, -2),
    (-13, -6, -4, -2),
    (-12, -8, -6, -3),
    (-11, -9, -7, -3),
    (-11, -8, -7, -4),
    (-11, -8, -5, -3),
    (-10, -8, -6, -2),
    (-10, -8, -5, -2),
    (-10, -8, -4, -2),
    (-10, -7, -5, -2),
    (-10, -7, -4, -3),
    (-10, -3, -2, -1),
    (-9, -8, -6, -4),
    (-9, -7, -5, -3),
)


def _build_alpha_table():
    """把 16 张基表展开成 256 行 x 8 列, 高 4 位是倍率, 低 4 位是表号。"""
    rows = []
    for index in range(256):
        table = index & 0x0F
        multiple = index >> 4
        base = ALPHA_BASE[table]
        values = []
        for column in range(8):
            if column < 4:
                value = base[3 - column]
            else:
                value = -base[3 - (column - 4)] - 1
            values.append(value * multiple)
        rows.append(tuple(values))
    return tuple(rows)


ALPHA_TABLE = _build_alpha_table()


def _mask(size):
    return (1 << size) - 1


def _get_high(word, size, start):
    """取高位字 (块内 bits 63..32) 中的字段。"""
    return (word >> (start - 32 - size + 1)) & _mask(size)


def _get_low(word, size, start):
    """取低位字 (块内 bits 31..0) 中的字段。"""
    return (word >> (start - size + 1)) & _mask(size)


def _put_high(word, value, size, start):
    shift = start - 32 - size + 1
    mask = _mask(size) << shift
    return (word & ~mask & 0xFFFFFFFF) | ((value & _mask(size)) << shift)


def _put_low(word, value, size, start):
    shift = start - size + 1
    mask = _mask(size) << shift
    return (word & ~mask & 0xFFFFFFFF) | ((value & _mask(size)) << shift)


def _expand(value, bits):
    """把 bits 位基色按规范扩展到 8 位。"""
    return (value << (8 - bits)) | (value >> (2 * bits - 8))


def _sign3(value):
    return value - 8 if value & 4 else value


def _clamp8(value):
    if value < 0:
        return 0
    if value > 255:
        return 255
    return value


def _clamp(value, low, high):
    if value < low:
        return low
    if value > high:
        return high
    return value


def _subblock_palette(color, table):
    """把一个子块的基色与调制档展开成 4 个实际颜色。"""
    steps = MODIFIER_ORDER[table]
    return (
        (_clamp8(color[0] + steps[0]), _clamp8(color[1] + steps[0]), _clamp8(color[2] + steps[0])),
        (_clamp8(color[0] + steps[1]), _clamp8(color[1] + steps[1]), _clamp8(color[2] + steps[1])),
        (_clamp8(color[0] + steps[2]), _clamp8(color[1] + steps[2]), _clamp8(color[2] + steps[2])),
        (_clamp8(color[0] + steps[3]), _clamp8(color[1] + steps[3]), _clamp8(color[2] + steps[3])),
    )


def _subblock_colors(word, differential):
    """解析两个子块的基色。

    differential 为真时按 5:5:5 基色加 3 位差分, 同时回传未裁剪的 5 位差值,
    供 ETC2 判定 T/H/Planar 扩展模式。
    """
    if differential:
        r1 = _get_high(word, 5, 63)
        g1 = _get_high(word, 5, 55)
        b1 = _get_high(word, 5, 47)
        dr = _sign3(_get_high(word, 3, 58))
        dg = _sign3(_get_high(word, 3, 50))
        db = _sign3(_get_high(word, 3, 42))
        return (
            (_expand(r1, 5), _expand(g1, 5), _expand(b1, 5)),
            (
                _expand(_clamp(r1 + dr, 0, 31), 5),
                _expand(_clamp(g1 + dg, 0, 31), 5),
                _expand(_clamp(b1 + db, 0, 31), 5),
            ),
            (r1 + dr, g1 + dg, b1 + db),
        )
    r1 = _expand(_get_high(word, 4, 63), 4)
    g1 = _expand(_get_high(word, 4, 55), 4)
    b1 = _expand(_get_high(word, 4, 47), 4)
    r2 = _expand(_get_high(word, 4, 59), 4)
    g2 = _expand(_get_high(word, 4, 51), 4)
    b2 = _expand(_get_high(word, 4, 43), 4)
    return (r1, g1, b1), (r2, g2, b2), None


def _pixel_slots(flip):
    """给出 16 个像素的 (x, y, 子块号, 位序号), 与解码遍历顺序一致。"""
    slots = []
    if flip == 0:
        shift = 0
        for x in range(2):
            for y in range(4):
                slots.append((x, y, 0, shift))
                shift += 1
        shift = 8
        for x in range(2, 4):
            for y in range(4):
                slots.append((x, y, 1, shift))
                shift += 1
        return slots
    shift = 0
    for x in range(4):
        for y in range(2):
            slots.append((x, y, 0, shift))
            shift += 1
        shift += 2
    shift = 2
    for x in range(4):
        for y in range(2, 4):
            slots.append((x, y, 1, shift))
            shift += 1
        shift += 2
    return slots


def _fill_pixels(out, palette1, palette2, msb, lsb, flip):
    """按索引位平面把两个子块的调色板铺到 16 个像素上。"""
    for x, y, subblock, shift in _pixel_slots(flip):
        index = (((msb >> shift) & 1) << 1) | ((lsb >> shift) & 1)
        index = INDEX_SCRAMBLE[index]
        out[y * 4 + x] = palette1[index] if subblock == 0 else palette2[index]


def _unstuff59(word1, word2):
    """把 T 模式的散装位重排成 59 位标准布局。"""
    stuffed = word1
    out = (stuffed >> 1) & 0xFFFFFFFF
    out = _put_low(out, stuffed & 1, 1, 32)
    r0a = _get_high(stuffed, 2, 60)
    out = _put_high(out, r0a, 2, 58)
    out = _put_high(out, 0, 5, 63)
    return out, word2


def _unstuff58(word1, word2):
    """把 H 模式的散装位重排成 58 位标准布局。"""
    part0 = _get_high(word1, 7, 62)
    part1 = _get_high(word1, 2, 52)
    part2 = _get_high(word1, 16, 49)
    part3 = _get_high(word1, 1, 32)
    out = 0
    out = _put_high(out, part0, 7, 57)
    out = _put_high(out, part1, 2, 50)
    out = _put_high(out, part2, 16, 48)
    out = _put_high(out, part3, 1, 32)
    return out, word2


def _unstuff57(word1, word2):
    """把 Planar 模式的散装位重排成 57 位标准布局。"""
    ro = _get_high(word1, 6, 62)
    go1 = _get_high(word1, 1, 56)
    go2 = _get_high(word1, 6, 54)
    bo1 = _get_high(word1, 1, 48)
    bo2 = _get_high(word1, 2, 44)
    bo3 = _get_high(word1, 3, 41)
    rh1 = _get_high(word1, 5, 38)
    rh2 = _get_high(word1, 1, 32)
    gh = _get_low(word2, 7, 31)
    bh = _get_low(word2, 6, 24)
    rv = _get_low(word2, 6, 18)
    gv = _get_low(word2, 7, 12)
    bv = _get_low(word2, 6, 5)
    out1 = 0
    out1 = _put_high(out1, ro, 6, 63)
    out1 = _put_high(out1, go1, 1, 57)
    out1 = _put_high(out1, go2, 6, 56)
    out1 = _put_high(out1, bo1, 1, 50)
    out1 = _put_high(out1, bo2, 2, 49)
    out1 = _put_high(out1, bo3, 3, 47)
    out1 = _put_high(out1, rh1, 5, 44)
    out1 = _put_high(out1, rh2, 1, 39)
    out1 = _put_high(out1, gh, 7, 38)
    out2 = 0
    out2 = _put_low(out2, bh, 6, 31)
    out2 = _put_low(out2, rv, 6, 25)
    out2 = _put_low(out2, gv, 7, 19)
    out2 = _put_low(out2, bv, 6, 12)
    return out1, out2


def _decode_t_block(word1, word2, out):
    word1, word2 = _unstuff59(word1, word2)
    c0 = (
        _expand(_get_high(word1, 4, 58), 4),
        _expand(_get_high(word1, 4, 54), 4),
        _expand(_get_high(word1, 4, 50), 4),
    )
    c1 = (
        _expand(_get_high(word1, 4, 46), 4),
        _expand(_get_high(word1, 4, 42), 4),
        _expand(_get_high(word1, 4, 38), 4),
    )
    distance = DISTANCE_TABLE[_get_high(word1, 3, 34)]
    palette = (
        c0,
        (_clamp8(c1[0] + distance), _clamp8(c1[1] + distance), _clamp8(c1[2] + distance)),
        c1,
        (_clamp8(c1[0] - distance), _clamp8(c1[1] - distance), _clamp8(c1[2] - distance)),
    )
    for x in range(4):
        for y in range(4):
            index = (_get_low(word2, 1, y + x * 4 + 16) << 1) | _get_low(word2, 1, y + x * 4)
            out[y * 4 + x] = palette[index]


def _decode_h_block(word1, word2, out):
    word1, word2 = _unstuff58(word1, word2)
    c0 = (
        _expand(_get_high(word1, 4, 57), 4),
        _expand(_get_high(word1, 4, 53), 4),
        _expand(_get_high(word1, 4, 49), 4),
    )
    c1 = (
        _expand(_get_high(word1, 4, 45), 4),
        _expand(_get_high(word1, 4, 41), 4),
        _expand(_get_high(word1, 4, 37), 4),
    )
    distance = _get_high(word1, 2, 33) << 1
    if _get_high(word1, 12, 57) >= _get_high(word1, 12, 45):
        distance |= 1
    spread = DISTANCE_TABLE[distance]
    palette = (
        (_clamp8(c0[0] + spread), _clamp8(c0[1] + spread), _clamp8(c0[2] + spread)),
        (_clamp8(c0[0] - spread), _clamp8(c0[1] - spread), _clamp8(c0[2] - spread)),
        (_clamp8(c1[0] + spread), _clamp8(c1[1] + spread), _clamp8(c1[2] + spread)),
        (_clamp8(c1[0] - spread), _clamp8(c1[1] - spread), _clamp8(c1[2] - spread)),
    )
    for x in range(4):
        for y in range(4):
            index = (_get_low(word2, 1, y + x * 4 + 16) << 1) | _get_low(word2, 1, y + x * 4)
            out[y * 4 + x] = palette[index]


def _extend_planar(value, bits):
    if bits == 6:
        return (value << 2) | (value >> 4)
    return (value << 1) | (value >> 6)


def _decode_planar_block(word1, word2, out):
    word1, word2 = _unstuff57(word1, word2)
    origin = (
        _extend_planar(_get_high(word1, 6, 63), 6),
        _extend_planar(_get_high(word1, 7, 57), 7),
        _extend_planar(_get_high(word1, 6, 50), 6),
    )
    horizontal = (
        _extend_planar(_get_high(word1, 6, 44), 6),
        _extend_planar(_get_high(word1, 7, 38), 7),
        _extend_planar(_get_low(word2, 6, 31), 6),
    )
    vertical = (
        _extend_planar(_get_low(word2, 6, 25), 6),
        _extend_planar(_get_low(word2, 7, 19), 7),
        _extend_planar(_get_low(word2, 6, 12), 6),
    )
    for y in range(4):
        for x in range(4):
            red = (x * (horizontal[0] - origin[0]) + y * (vertical[0] - origin[0]) + 4 * origin[0] + 2) >> 2
            green = (x * (horizontal[1] - origin[1]) + y * (vertical[1] - origin[1]) + 4 * origin[1] + 2) >> 2
            blue = (x * (horizontal[2] - origin[2]) + y * (vertical[2] - origin[2]) + 4 * origin[2] + 2) >> 2
            out[y * 4 + x] = (_clamp8(red), _clamp8(green), _clamp8(blue))


def decode_etc1_block(block: bytes):
    """解一个 ETC1 8 字节块, 返回 16 个 (r, g, b) 元组, 行优先。"""
    if len(block) < 8:
        raise CodecError("ETC1 块不足 8 字节: %d" % len(block))
    word1 = int.from_bytes(block[0:4], "big")
    word2 = int.from_bytes(block[4:8], "big")
    differential = bool(_get_high(word1, 1, 33))
    flip = _get_high(word1, 1, 32)
    color1, color2, _spread = _subblock_colors(word1, differential)
    palette1 = _subblock_palette(color1, _get_high(word1, 3, 39))
    palette2 = _subblock_palette(color2, _get_high(word1, 3, 36))
    out = [None] * 16
    _fill_pixels(out, palette1, palette2, _get_low(word2, 16, 31), _get_low(word2, 16, 15), flip)
    return out


def decode_etc2_rgb_block(block: bytes):
    """解一个 ETC2 RGB 8 字节块, 覆盖个体/差分/ T / H / Planar 五种模式。"""
    if len(block) < 8:
        raise CodecError("ETC2 块不足 8 字节: %d" % len(block))
    word1 = int.from_bytes(block[0:4], "big")
    word2 = int.from_bytes(block[4:8], "big")
    out = [None] * 16
    differential = bool(_get_high(word1, 1, 33))
    if differential:
        _color1, _color2, spread = _subblock_colors(word1, True)
        red, green, blue = spread
        if red < 0 or red > 31:
            _decode_t_block(word1, word2, out)
            return out
        if green < 0 or green > 31:
            _decode_h_block(word1, word2, out)
            return out
        if blue < 0 or blue > 31:
            _decode_planar_block(word1, word2, out)
            return out
    flip = _get_high(word1, 1, 32)
    color1, color2, _spread = _subblock_colors(word1, differential)
    palette1 = _subblock_palette(color1, _get_high(word1, 3, 39))
    palette2 = _subblock_palette(color2, _get_high(word1, 3, 36))
    _fill_pixels(out, palette1, palette2, _get_low(word2, 16, 31), _get_low(word2, 16, 15), flip)
    return out


def decode_etc2_alpha_block(block: bytes):
    """解一个 EAC alpha 8 字节块, 返回 16 个 alpha 值, 行优先。"""
    if len(block) < 8:
        raise CodecError("EAC alpha 块不足 8 字节: %d" % len(block))
    base = block[0]
    table = ALPHA_TABLE[block[1]]
    out = [0] * 16
    bit = 0
    byte = 2
    for x in range(4):
        for y in range(4):
            index = 0
            for _pos in range(3):
                index = (index << 1) | ((block[byte] >> (7 - bit)) & 1)
                bit += 1
                if bit > 7:
                    bit = 0
                    byte += 1
            out[y * 4 + x] = _clamp8(base + table[index])
    return out


def decode_etc2_rgba_block(block: bytes):
    """解一个 ETC2 RGBA8 16 字节块, 返回 16 个 (r, g, b, a) 元组。"""
    if len(block) < 16:
        raise CodecError("ETC2 RGBA 块不足 16 字节: %d" % len(block))
    rgb = decode_etc2_rgb_block(block[0:8])
    alpha = decode_etc2_alpha_block(block[8:16])
    return [(rgb[i][0], rgb[i][1], rgb[i][2], alpha[i]) for i in range(16)]


def _half_order(flip, subblock):
    """给出某个子块内 8 个像素的遍历顺序, 与 _pixel_slots 一致。"""
    return [(x, y) for x, y, part, _shift in _pixel_slots(flip) if part == subblock]


def _nearest_step(color, palette):
    best = 0
    best_error = None
    for index in range(4):
        candidate = palette[index]
        error = (
            abs(candidate[0] - color[0])
            + abs(candidate[1] - color[1])
            + abs(candidate[2] - color[2])
        )
        if best_error is None or error < best_error:
            best_error = error
            best = index
    return best


def _fit_subblock(pixels, differential):
    """给 8 个像素找一组 (量化基色, 调制档), 返回 (基色, 量化值, 档号, 档内序号, 误差)。"""
    count = len(pixels)
    means = [
        sum(pixel[channel] for pixel in pixels) / count for channel in range(3)
    ]
    if differential:
        quantized = tuple(_clamp(int(round(value * 31.0 / 255.0)), 0, 31) for value in means)
        base = tuple(_expand(value, 5) for value in quantized)
    else:
        quantized = tuple(_clamp(int(round(value * 15.0 / 255.0)), 0, 15) for value in means)
        base = tuple(value * 17 for value in quantized)
    best = None
    for table in range(8):
        palette = _subblock_palette(base, table)
        error = 0
        chosen = []
        for pixel in pixels:
            index = _nearest_step(pixel, palette)
            chosen.append(index)
            candidate = palette[index]
            error += (
                abs(candidate[0] - pixel[0])
                + abs(candidate[1] - pixel[1])
                + abs(candidate[2] - pixel[2])
            )
        if best is None or error < best[4]:
            best = (base, quantized, table, chosen, error)
    return best


def _assemble_block(flip, candidate):
    """把子块拟合结果拼成 8 字节块并回解, 用来算真实误差。"""
    first, second, differential = candidate
    base1, quant1, table1, index1, _e1 = first
    base2, quant2, table2, index2, _e2 = second
    word1 = 0
    if differential:
        r1, g1, b1 = quant1
        r2, g2, b2 = quant2
        word1 = _put_high(word1, r1, 5, 63)
        word1 = _put_high(word1, g1, 5, 55)
        word1 = _put_high(word1, b1, 5, 47)
        word1 = _put_high(word1, (r2 - r1) & 0x7, 3, 58)
        word1 = _put_high(word1, (g2 - g1) & 0x7, 3, 50)
        word1 = _put_high(word1, (b2 - b1) & 0x7, 3, 42)
    else:
        word1 = _put_high(word1, quant1[0], 4, 63)
        word1 = _put_high(word1, quant1[1], 4, 55)
        word1 = _put_high(word1, quant1[2], 4, 47)
        word1 = _put_high(word1, quant2[0], 4, 59)
        word1 = _put_high(word1, quant2[1], 4, 51)
        word1 = _put_high(word1, quant2[2], 4, 43)
    word1 = _put_high(word1, table1, 3, 39)
    word1 = _put_high(word1, table2, 3, 36)
    word1 = _put_high(word1, 1 if differential else 0, 1, 33)
    word1 = _put_high(word1, flip, 1, 32)
    # 按遍历顺序把两个子块的档内序号写到两个位平面上
    word2 = 0
    order_first = _half_order(flip, 0)
    order_second = _half_order(flip, 1)
    for x, y, subblock, shift in _pixel_slots(flip):
        table = order_first if subblock == 0 else order_second
        position = table.index((x, y))
        index = (index1 if subblock == 0 else index2)[position]
        raw = INDEX_SCRAMBLE_INVERSE[index]
        word2 = _put_low(word2, (raw >> 1) & 1, 1, shift + 16)
        word2 = _put_low(word2, raw & 1, 1, shift)
    return word1.to_bytes(4, "big") + word2.to_bytes(4, "big")


def _block_error(decoded, pixels):
    error = 0
    for index in range(16):
        pixel = pixels[index]
        candidate = decoded[index]
        error += (
            abs(candidate[0] - pixel[0])
            + abs(candidate[1] - pixel[1])
            + abs(candidate[2] - pixel[2])
        )
    return error


def encode_etc1_block(pixels):
    """把 16 个 (r, g, b) 像素压成 ETC1 8 字节块。

    左右切分与上下切分各试差分和独立两种模式, 取回解误差最小的一组。
    """
    if len(pixels) != 16:
        raise CodecError("ETC1 编码需要 16 个像素, 实得 %d" % len(pixels))
    best = None
    for flip in (0, 1):
        slots = _pixel_slots(flip)
        first = []
        second = []
        for x, y, subblock, _shift in slots:
            pixel = pixels[y * 4 + x]
            if subblock == 0:
                first.append(pixel)
            else:
                second.append(pixel)
        for differential in (True, False):
            fit1 = _fit_subblock(first, differential)
            fit2 = _fit_subblock(second, differential)
            if differential:
                r1, g1, b1 = fit1[1]
                r2, g2, b2 = fit2[1]
                if not all(-4 <= value <= 3 for value in (r2 - r1, g2 - g1, b2 - b1)):
                    continue
            block = _assemble_block(flip, (fit1, fit2, differential))
            decoded = decode_etc1_block(block)
            error = _block_error(decoded, pixels)
            if best is None or error < best[0]:
                best = (error, block)
    if best is None:
        raise CodecError("ETC1 编码没有可用模式")
    return best[1]


def encode_etc2_rgba_block(pixels):
    """把 16 个 (r, g, b, a) 像素压成 ETC2 RGBA8 16 字节块。"""
    if len(pixels) != 16:
        raise CodecError("ETC2 编码需要 16 个像素, 实得 %d" % len(pixels))
    rgb_block = encode_etc1_block([(p[0], p[1], p[2]) for p in pixels])
    ordered = [pixels[(index // 4) * 4 + (index % 4)][3] for index in range(16)]
    alpha_slots = []
    for x in range(4):
        for y in range(4):
            alpha_slots.append(pixels[y * 4 + x][3])
    base = _clamp(int(round(sum(alpha_slots) / 16.0)), 0, 255)
    best = None
    for table in range(16):
        for multiple in (1, 2, 3, 4, 6, 8, 12, 15):
            values = ALPHA_TABLE[(multiple << 4) | table]
            error = 0
            chosen = []
            for value in alpha_slots:
                best_index = 0
                best_error = None
                for index in range(8):
                    difference = abs(_clamp8(base + values[index]) - value)
                    if best_error is None or difference < best_error:
                        best_error = difference
                        best_index = index
                error += best_error
                chosen.append(best_index)
            if best is None or error < best[0]:
                best = (error, table, multiple, chosen)
    _error, table, multiple, chosen = best
    packed = 0
    for index in chosen:
        packed = (packed << 3) | index
    alpha_block = bytearray(8)
    alpha_block[0] = base
    alpha_block[1] = (multiple << 4) | table
    alpha_block[2:8] = packed.to_bytes(6, "big")
    return rgb_block + bytes(alpha_block)


class Etc1Codec:
    """ETC1 RGB8, 每块 8 字节。"""

    name = "etc1"
    format_ids = (0x8D64,)
    block_size = 8
    has_alpha = False

    def decode_block(self, block, offset=0):
        return decode_etc1_block(block[offset : offset + 8])

    def encode_block(self, pixels):
        return encode_etc1_block([(p[0], p[1], p[2]) for p in pixels])


class Etc2RgbaCodec:
    """ETC2 RGBA8, 每块 16 字节, 前半 RGB 块后半 alpha 块。"""

    name = "etc2-rgba"
    format_ids = (0x9278,)
    block_size = 16
    has_alpha = True

    def decode_block(self, block, offset=0):
        return decode_etc2_rgba_block(block[offset : offset + 16])

    def encode_block(self, pixels):
        return encode_etc2_rgba_block(pixels)


PIXEL_CODECS = (Etc1Codec(), Etc2RgbaCodec())

PIXEL_CODEC_BY_FORMAT = {}
for _codec in PIXEL_CODECS:
    for _format in _codec.format_ids:
        PIXEL_CODEC_BY_FORMAT[_format] = _codec


def codec_for_format(format_id):
    """按 glInternalFormat 取像素编解码器, 未支持返回 None。"""
    return PIXEL_CODEC_BY_FORMAT.get(format_id)


def decode_image(codec, data: bytes, width: int, height: int) -> bytes:
    """把一整张压缩像素解成 RGBA8 字节串。"""
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    size = codec.block_size
    need = blocks_x * blocks_y * size
    if len(data) < need:
        raise CodecError("像素数据不足: 需要 %d 字节, 实得 %d" % (need, len(data)))
    out = bytearray(width * height * 4)
    pos = 0
    for by in range(blocks_y):
        for bx in range(blocks_x):
            block = codec.decode_block(data, pos)
            pos += size
            for index in range(16):
                x = bx * 4 + (index % 4)
                y = by * 4 + (index // 4)
                if x >= width or y >= height:
                    continue
                pixel = block[index]
                offset = (y * width + x) * 4
                out[offset] = pixel[0]
                out[offset + 1] = pixel[1]
                out[offset + 2] = pixel[2]
                out[offset + 3] = pixel[3] if len(pixel) > 3 else 255
    return bytes(out)


def encode_image(codec, rgba: bytes, width: int, height: int) -> bytes:
    """把 RGBA8 字节串压成一整张压缩像素。"""
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    out = bytearray()
    for by in range(blocks_y):
        for bx in range(blocks_x):
            pixels = []
            for index in range(16):
                x = bx * 4 + (index % 4)
                y = by * 4 + (index // 4)
                if x >= width or y >= height:
                    pixels.append((0, 0, 0, 255))
                    continue
                offset = (y * width + x) * 4
                pixels.append((rgba[offset], rgba[offset + 1], rgba[offset + 2], rgba[offset + 3]))
            out.extend(codec.encode_block(pixels))
    return bytes(out)
