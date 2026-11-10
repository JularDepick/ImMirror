# -*- coding: utf-8 -*-
"""KTX 11 纹理容器, 含像素层解码与回写。

容器层保持 lossless-passthrough: 工程文件存放原始 KTX 字节, 再编码逐字节相同。
像素层在旁挂的 PNG 视图里落地 (路径见 docs/notes/ktx.md), 只做数据派生:

- 解码时按 glInternalFormat 选 ETC1 或 ETC2 RGBA 解出 RGBA, 写成 PNG;
- 回写时由 PNG 重新压块, 头部沿用原文件字段, mip 链按新的 0 级重算。

回写结果不保证与源文件逐字节相同 (块编码存在多解), 只保证像素级接近,
差异幅度由 forge.ktx_png 的自检报告给出。
"""

from __future__ import annotations

import struct

from . import etc
from .base import Codec, CodecError, FIDELITY_PASSTHROUGH, Payload
from ..util import png

MAGIC_KTX11 = b"\xabKTX 11\xbb\r\n\x1a\n"

"""KTX 11 头长度为 64 字节, 字段顺序见 Khronos 规范。"""
HEADER_SIZE = 64

"""旁挂 PNG 视图的目录, 与工程树 _view 并列。"""
PNG_DIR = "_png"

FORMAT_NAMES = {
    0x8D64: "ETC1_RGB8",
    0x8D66: "ETC1_RGB8_A8",
    0x9278: "ETC2_RGBA8",
    0x8C92: "ATC_RGB",
    0x8C93: "ATC_RGBA",
    0x8C00: "PVRTC1_2",
    0x8C02: "PVRTC1_4",
    0x8C01: "PVRTC1_2_A8",
    0x8C03: "PVRTC1_4_A8",
    0x83F0: "DXT1",
    0x83F1: "DXT3",
}


class KtxTexture:
    """一份 KTX 11 纹理的结构化视图, 带各级 mip 的像素区间。"""

    def __init__(self, header, key_values, levels, layout):
        self.header = header
        self.key_values = key_values
        self.levels = levels
        self.layout = layout

    @property
    def width(self):
        return self.header["pixelWidth"]

    @property
    def height(self):
        return self.header["pixelHeight"]

    @property
    def internal_format(self):
        return self.header["glInternalFormat"]

    @property
    def format_name(self):
        return FORMAT_NAMES.get(self.internal_format, "unknown")

    def codec(self):
        return etc.codec_for_format(self.internal_format)


def parse(data: bytes) -> KtxTexture:
    """解析 KTX 11 容器, 失败抛 CodecError。"""
    if len(data) < HEADER_SIZE:
        raise CodecError("KTX 头不足 64 字节: %d" % len(data))
    if data[:12] != MAGIC_KTX11:
        raise CodecError("KTX 魔数不符: %s" % data[:12].hex())
    endianness = struct.unpack_from("<I", data, 12)[0]
    if endianness == 0x04030201:
        layout = "<"
    elif struct.unpack_from(">I", data, 12)[0] == 0x04030201:
        layout = ">"
    else:
        raise CodecError("KTX 端序字段非法: 0x%08x" % endianness)
    fields = struct.unpack_from(layout + "IIIIIIIIIIIII", data, 12)
    header = {
        "endianness": 0x04030201,
        "glType": fields[1],
        "glTypeSize": fields[2],
        "glFormat": fields[3],
        "glInternalFormat": fields[4],
        "glBaseInternalFormat": fields[5],
        "pixelWidth": fields[6],
        "pixelHeight": fields[7],
        "pixelDepth": fields[8],
        "numberOfArrayElements": fields[9],
        "numberOfFaces": fields[10],
        "numberOfMipmapLevels": fields[11],
        "bytesOfKeyValueData": fields[12],
    }
    offset = HEADER_SIZE + header["bytesOfKeyValueData"]
    key_values = data[HEADER_SIZE:offset]
    mip_levels = header["numberOfMipmapLevels"] or 1
    faces = header["numberOfFaces"] or 1
    array = header["numberOfArrayElements"] or 0
    levels = []
    for level in range(mip_levels):
        if offset + 4 > len(data):
            raise CodecError("KTX 第 %d 级 mip 缺少长度字段" % level)
        size = struct.unpack_from(layout + "I", data, offset)[0]
        offset += 4
        if offset + size * max(1, faces) * max(1, array) > len(data):
            raise CodecError(
                "KTX 第 %d 级 mip 数据越界: 声明 %d 字节" % (level, size * faces * max(1, array))
            )
        payload = data[offset : offset + size]
        offset += size * max(1, faces) * max(1, array)
        levels.append(
            {
                "level": level,
                "width": max(1, header["pixelWidth"] >> level),
                "height": max(1, header["pixelHeight"] >> level),
                "size": size,
                "data": payload,
            }
        )
    return KtxTexture(header, key_values, levels, layout)


def to_rgba(texture: KtxTexture, level: int = 0) -> bytes:
    """把指定 mip 级解成 RGBA8 像素。"""
    codec = texture.codec()
    if codec is None:
        raise CodecError("KTX 像素格式未支持: %s (%s)" % (texture.format_name, hex(texture.internal_format)))
    if level >= len(texture.levels):
        raise CodecError("KTX 只有 %d 级 mip, 取不到第 %d 级" % (len(texture.levels), level))
    entry = texture.levels[level]
    return etc.decode_image(codec, entry["data"], entry["width"], entry["height"])


def to_png(texture: KtxTexture, level: int = 0) -> bytes:
    """把指定 mip 级写成 PNG 字节。"""
    pixels = to_rgba(texture, level)
    entry = texture.levels[level]
    return png.write_rgba(entry["width"], entry["height"], pixels)


def _downsample(width: int, height: int, pixels: bytes):
    """按 2x2 盒式滤波降采样, 用于重算 mip 链。"""
    new_width = max(1, width // 2)
    new_height = max(1, height // 2)
    out = bytearray(new_width * new_height * 4)
    for y in range(new_height):
        for x in range(new_width):
            acc = [0, 0, 0, 0]
            count = 0
            for dy in range(2):
                sy = min(height - 1, y * 2 + dy)
                for dx in range(2):
                    sx = min(width - 1, x * 2 + dx)
                    offset = (sy * width + sx) * 4
                    for channel in range(4):
                        acc[channel] += pixels[offset + channel]
                    count += 1
            offset = (y * new_width + x) * 4
            for channel in range(4):
                out[offset + channel] = acc[channel] // count
    return new_width, new_height, bytes(out)


def rebuild(texture: KtxTexture, rgba: bytes) -> bytes:
    """用新的 0 级 RGBA 像素重建整份 KTX 字节。

    头部除 mip 级数对应的长度字段外全部沿用; 低层 mip 由 0 级逐级盒式降采样重算,
    因此改写后低层 mip 与原始文件不再一致, 这一点在文档里明确标注。
    """
    codec = texture.codec()
    if codec is None:
        raise CodecError("KTX 像素格式未支持, 无法回写: %s" % texture.format_name)
    width = texture.width
    height = texture.height
    if len(rgba) != width * height * 4:
        raise CodecError("RGBA 长度不符: 期望 %d, 实得 %d" % (width * height * 4, len(rgba)))
    header = texture.header
    body = bytearray()
    pixels = rgba
    current_width = width
    current_height = height
    for level in range(len(texture.levels)):
        if level:
            current_width, current_height, pixels = _downsample(
                current_width, current_height, pixels
            )
        block = etc.encode_image(codec, pixels, current_width, current_height)
        body.extend(struct.pack(texture.layout + "I", len(block)))
        body.extend(block)
    fields = (
        0x04030201,
        header["glType"],
        header["glTypeSize"],
        header["glFormat"],
        header["glInternalFormat"],
        header["glBaseInternalFormat"],
        header["pixelWidth"],
        header["pixelHeight"],
        header["pixelDepth"],
        header["numberOfArrayElements"],
        header["numberOfFaces"],
        header["numberOfMipmapLevels"],
        header["bytesOfKeyValueData"],
    )
    return (
        MAGIC_KTX11
        + struct.pack("<13I", *fields)
        + texture.key_values
        + bytes(body)
    )


class Ktx11Codec(Codec):
    name = "ktx11"
    fidelity = FIDELITY_PASSTHROUGH
    readability = "binary-structured"
    extensions = (".ktx",)
    magics = (MAGIC_KTX11,)

    def sniff(self, data: bytes) -> bool:
        return data[:12] == MAGIC_KTX11

    def decode(self, data: bytes) -> Payload:
        if len(data) < HEADER_SIZE:
            raise CodecError("KTX 头不足 64 字节: %d" % len(data))
        texture = parse(data)
        codec = texture.codec()
        return Payload(
            kind="binary",
            blob=data,
            meta={
                "container": "ktx11",
                "endianness": "little" if texture.layout == "<" else "big",
                "plain_size": len(data),
                "glInternalFormat": texture.internal_format,
                "pixel_format": texture.format_name,
                "pixel_codec": codec.name if codec else None,
                "mip_levels": len(texture.levels),
                "width": texture.width,
                "height": texture.height,
            },
        )

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < HEADER_SIZE:
            return None
        try:
            texture = parse(data)
        except CodecError as exc:
            return {"error": str(exc)}
        header = texture.header
        codec = texture.codec()
        return {
            "container": "ktx11",
            "endianness": payload.meta.get("endianness"),
            "glType": header["glType"],
            "glTypeSize": header["glTypeSize"],
            "glFormat": header["glFormat"],
            "glInternalFormat": header["glInternalFormat"],
            "glInternalFormatName": texture.format_name,
            "glBaseInternalFormat": header["glBaseInternalFormat"],
            "pixelWidth": header["pixelWidth"],
            "pixelHeight": header["pixelHeight"],
            "pixelDepth": header["pixelDepth"],
            "arrayElements": header["numberOfArrayElements"],
            "faces": header["numberOfFaces"],
            "mipLevels": header["numberOfMipmapLevels"],
            "firstImageSize": texture.levels[0]["size"] if texture.levels else 0,
            "pixelCodec": codec.name if codec else None,
            "mipSizes": [entry["size"] for entry in texture.levels],
        }


