# -*- coding: utf-8 -*-
"""WebP (RIFF) 容器的识别与关键字段视图。

WebP 是公开格式, 不需要逆向; 管线只做识别、取画布尺寸与块清单, 字节原样保留。
客户端资源树里有 19 个真实 WebP 与 2 个扩展名不符 (实为 PNG) 的文件,
后者由魔数路由到 PNG 编解码器, 因此这里只按魔数判定。
"""

from __future__ import annotations

import struct

from .base import Codec, Payload

MAGIC_RIFF = b"RIFF"
MAGIC_WEBP = b"WEBP"

"""单块清单在视图里保留的条数上限。"""
MAX_CHUNKS = 32


def _chunks(data: bytes):
    """枚举 RIFF 子块, 返回 (四字标识, 载荷偏移, 载荷长度)。"""
    pos = 12
    out = []
    while pos + 8 <= len(data) and len(out) < MAX_CHUNKS:
        fourcc = data[pos:pos + 4].decode("ascii", "replace")
        size = struct.unpack_from("<I", data, pos + 4)[0]
        start = pos + 8
        if start + size > len(data):
            break
        out.append((fourcc, start, size))
        pos = start + size + (size & 1)
    return out


def _vp8x_size(payload: bytes):
    """VP8X 画布尺寸: 两个 24 位小端, 实际值减一。"""
    if len(payload) < 10:
        return None
    width = int.from_bytes(payload[4:7], "little") + 1
    height = int.from_bytes(payload[7:10], "little") + 1
    return width, height


def _vp8_size(payload: bytes):
    """有损 VP8 关键帧里的尺寸, 14 位有效。"""
    if len(payload) < 10 or payload[3:6] != b"\x9d\x01\x2a":
        return None
    width = struct.unpack_from("<H", payload, 6)[0] & 0x3FFF
    height = struct.unpack_from("<H", payload, 8)[0] & 0x3FFF
    return width, height


def _vp8l_size(payload: bytes):
    """无损 VP8L 位流里的尺寸, 14 位各减一。"""
    if len(payload) < 5 or payload[0] != 0x2F:
        return None
    bits = int.from_bytes(payload[1:5], "little")
    width = (bits & 0x3FFF) + 1
    height = ((bits >> 14) & 0x3FFF) + 1
    return width, height


class WebpCodec(Codec):
    name = "webp"
    fidelity = "lossless-passthrough"
    readability = "binary-standard"
    extensions = (".webp",)
    magics = (MAGIC_RIFF,)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == MAGIC_RIFF and data[8:12] == MAGIC_WEBP

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data,
                       meta={"container": "webp", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < 12:
            return None
        chunks = _chunks(data)
        info = {
            "container": "webp",
            "riffSize": struct.unpack_from("<I", data, 4)[0],
            "bytes": len(data),
            "chunks": [{"fourcc": fourcc, "offset": start, "size": size}
                       for fourcc, start, size in chunks],
        }
        for fourcc, start, size in chunks:
            block = data[start:start + size]
            if fourcc == "VP8X":
                pair = _vp8x_size(block)
                if pair:
                    info.update({"variant": "extended", "width": pair[0], "height": pair[1]})
                break
            if fourcc == "VP8 ":
                pair = _vp8_size(block)
                if pair:
                    info.update({"variant": "lossy", "width": pair[0], "height": pair[1]})
                break
            if fourcc == "VP8L":
                pair = _vp8l_size(block)
                if pair:
                    info.update({"variant": "lossless", "width": pair[0], "height": pair[1]})
                break
        return info
