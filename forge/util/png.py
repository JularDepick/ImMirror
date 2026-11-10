# -*- coding: utf-8 -*-
"""PNG 8 位真彩读写, 只覆盖管线需要的子集。

写只产出非隔行 8 位 PNG (RGB 或 RGBA), 过滤器固定为 0, 便于逐像素比对;
读支持颜色类型 0/2/3/4/6 且位深为 8 的非隔行图, 16 位与隔行图直接报错,
由上层决定回退, 不做静默降级。
"""

from __future__ import annotations

import struct
import zlib

MAGIC = b"\x89PNG\r\n\x1a\n"

COLOR_GRAY = 0
COLOR_RGB = 2
COLOR_PALETTE = 3
COLOR_GRAY_ALPHA = 4
COLOR_RGBA = 6

"""每种颜色类型的通道数。"""
CHANNELS = {COLOR_GRAY: 1, COLOR_RGB: 3, COLOR_PALETTE: 3, COLOR_GRAY_ALPHA: 2, COLOR_RGBA: 4}


class PngError(Exception):
    """PNG 读写失败, 消息里带可定位的上下文。"""


def _chunk(tag: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + tag
        + payload
        + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)
    )


def write_rgba(width: int, height: int, pixels: bytes, level: int = 9) -> bytes:
    """把 RGBA8 像素写成 PNG 字节。"""
    if len(pixels) != width * height * 4:
        raise PngError(
            "RGBA 像素长度不符: 期望 %d, 实得 %d" % (width * height * 4, len(pixels))
        )
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * stride : (y + 1) * stride])
    return (
        MAGIC
        + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, COLOR_RGBA, 0, 0, 0))
        + _chunk(b"IDAT", zlib.compress(bytes(raw), level))
        + _chunk(b"IEND", b"")
    )


def write_rgb(width: int, height: int, pixels: bytes, level: int = 9) -> bytes:
    """把 RGB8 像素写成 PNG 字节。"""
    if len(pixels) != width * height * 3:
        raise PngError(
            "RGB 像素长度不符: 期望 %d, 实得 %d" % (width * height * 3, len(pixels))
        )
    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)
        raw.extend(pixels[y * stride : (y + 1) * stride])
    return (
        MAGIC
        + _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, COLOR_RGB, 0, 0, 0))
        + _chunk(b"IDAT", zlib.compress(bytes(raw), level))
        + _chunk(b"IEND", b"")
    )


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _unfilter(raw: bytes, width: int, height: int, bpp: int) -> bytearray:
    stride = width * bpp
    if len(raw) < (stride + 1) * height:
        raise PngError("IDAT 数据不足: 期望 %d, 实得 %d" % ((stride + 1) * height, len(raw)))
    out = bytearray(stride * height)
    prev = bytearray(stride)
    pos = 0
    for y in range(height):
        method = raw[pos]
        pos += 1
        line = bytearray(raw[pos : pos + stride])
        pos += stride
        if method == 0:
            pass
        elif method == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 0xFF
        elif method == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif method == 3:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif method == 4:
            for i in range(stride):
                left = line[i - bpp] if i >= bpp else 0
                upleft = prev[i - bpp] if i >= bpp else 0
                line[i] = (line[i] + _paeth(left, prev[i], upleft)) & 0xFF
        else:
            raise PngError("未知的 PNG 行过滤器: %d (行 %d)" % (method, y))
        out[y * stride : (y + 1) * stride] = line
        prev = line
    return out


def read_png(data: bytes):
    """解析 PNG, 返回 (宽, 高, RGBA 像素)。

    位深非 8 或隔行扫描的图不做支持, 抛 PngError。
    """
    if data[:8] != MAGIC:
        raise PngError("PNG 魔数不符")
    pos = 8
    width = height = 0
    color = -1
    palette = b""
    trns = b""
    idat = bytearray()
    while pos + 8 <= len(data):
        length = struct.unpack_from(">I", data, pos)[0]
        tag = data[pos + 4 : pos + 8]
        payload = data[pos + 8 : pos + 8 + length]
        pos += 12 + length
        if tag == b"IHDR":
            width, height, depth, color, _comp, _filt, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if depth != 8:
                raise PngError("只支持 8 位 PNG, 实得位深 %d" % depth)
            if interlace != 0:
                raise PngError("不支持隔行 PNG")
        elif tag == b"PLTE":
            palette = payload
        elif tag == b"tRNS":
            trns = payload
        elif tag == b"IDAT":
            idat.extend(payload)
        elif tag == b"IEND":
            break
    if color < 0:
        raise PngError("缺少 IHDR")
    if color not in CHANNELS:
        raise PngError("不支持的颜色类型: %d" % color)
    bpp = CHANNELS[color]
    raw = zlib.decompress(bytes(idat))
    plain = _unfilter(raw, width, height, bpp)
    if color == COLOR_RGBA:
        return width, height, bytes(plain)
    out = bytearray(width * height * 4)
    if color == COLOR_RGB:
        for i in range(width * height):
            out[i * 4 : i * 4 + 3] = plain[i * 3 : i * 3 + 3]
            out[i * 4 + 3] = 255
        return width, height, bytes(out)
    if color == COLOR_GRAY:
        for i in range(width * height):
            v = plain[i]
            out[i * 4 : i * 4 + 4] = bytes((v, v, v, 255))
        return width, height, bytes(out)
    if color == COLOR_GRAY_ALPHA:
        for i in range(width * height):
            v = plain[i * 2]
            out[i * 4 : i * 4 + 4] = bytes((v, v, v, plain[i * 2 + 1]))
        return width, height, bytes(out)
    for i in range(width * height):
        idx = plain[i]
        if idx * 3 + 3 > len(palette):
            raise PngError("调色板索引越界: %d" % idx)
        out[i * 4 : i * 4 + 3] = palette[idx * 3 : idx * 3 + 3]
        out[i * 4 + 3] = trns[idx] if idx < len(trns) else 255
    return width, height, bytes(out)
