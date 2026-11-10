# -*- coding: utf-8 -*-
"""标准公开格式 (PNG / GIF / JPEG / WebP / ZIP / TTF / Ogg / MP3) 的识别与视图。

这些格式不需要逆向, 外部工具即可直接读取; 管线只负责识别、给出关键头字段视图,
并原样保留字节, 避免工程树里出现只剩二进制而没有说明的素材。
"""

from __future__ import annotations

import io
import struct
import zipfile

from .axml import AxmlCodec
from .base import Codec, FIDELITY_PASSTHROUGH, Payload
from .webp import WebpCodec

READABILITY = "binary-standard"


class PngCodec(Codec):
    name = "png"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".png", ".png_", ".thumb")
    magics = (b"\x89PNG\r\n\x1a\n",)

    COLOR_TYPES = {0: "gray", 2: "rgb", 3: "palette", 4: "gray+alpha", 6: "rgba"}

    def sniff(self, data: bytes) -> bool:
        return data[:8] == b"\x89PNG\r\n\x1a\n"

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "png", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < 33 or data[12:16] != b"IHDR":
            return None
        width, height, depth, color, _comp, _filt, interlace = struct.unpack_from(">IIBBBBB", data, 16)
        return {
            "container": "png",
            "width": width,
            "height": height,
            "bitDepth": depth,
            "colorType": self.COLOR_TYPES.get(color, color),
            "interlace": interlace,
            "bytes": len(data),
        }


class GifCodec(Codec):
    name = "gif"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".gif",)
    magics = (b"GIF87a", b"GIF89a")

    def sniff(self, data: bytes) -> bool:
        return data[:6] in (b"GIF87a", b"GIF89a")

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "gif", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < 13:
            return None
        width, height = struct.unpack_from("<HH", data, 6)
        return {"container": "gif", "version": data[:6].decode("ascii", "replace"),
                "width": width, "height": height, "bytes": len(data)}


class JpegCodec(Codec):
    name = "jpeg"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".jpg", ".jpeg")
    magics = (b"\xff\xd8\xff",)

    def sniff(self, data: bytes) -> bool:
        return data[:3] == b"\xff\xd8\xff"

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "jpeg", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        pos = 2
        while pos + 4 <= len(data):
            if data[pos] != 0xFF:
                pos += 1
                continue
            marker = data[pos + 1]
            if marker in (0xD8, 0xD9) or 0xD0 <= marker <= 0xD7:
                pos += 2
                continue
            seg_len = struct.unpack_from(">H", data, pos + 2)[0]
            if marker in (0xC0, 0xC1, 0xC2, 0xC3):
                height, width = struct.unpack_from(">HH", data, pos + 5)
                return {"container": "jpeg", "width": width, "height": height,
                        "bytes": len(data)}
            pos += 2 + seg_len
        return {"container": "jpeg", "bytes": len(data)}


class ZipCodec(Codec):
    name = "zip"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".zip", ".jar")
    magics = (b"PK\x03\x04",)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == b"PK\x03\x04"

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "zip", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        try:
            with zipfile.ZipFile(io.BytesIO(data)) as archive:
                entries = [{"name": info.filename, "size": info.file_size}
                           for info in archive.infolist()]
        except (zipfile.BadZipFile, OSError) as exc:
            return {"container": "zip", "error": str(exc)}
        return {"container": "zip", "entryCount": len(entries), "entries": entries[:200]}


class TtfCodec(Codec):
    name = "ttf"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".ttf", ".otf")
    magics = (b"\x00\x01\x00\x00", b"OTTO")

    def sniff(self, data: bytes) -> bool:
        return data[:4] in (b"\x00\x01\x00\x00", b"OTTO", b"true")

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "ttf", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < 12:
            return None
        num_tables = struct.unpack_from(">H", data, 4)[0]
        tags = []
        for i in range(min(num_tables, 64)):
            off = 12 + i * 16
            if off + 4 > len(data):
                break
            tags.append(data[off : off + 4].decode("ascii", "replace"))
        return {"container": "ttf", "sfntVersion": data[:4].hex(),
                "numTables": num_tables, "tables": tags, "bytes": len(data)}


class OggCodec(Codec):
    name = "ogg"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".ogg",)
    magics = (b"OggS",)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == b"OggS"

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "ogg", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        if len(data) < 28:
            return None
        info = {"container": "ogg", "bytes": len(data)}
        first = data.find(b"\x01vorbis")
        if first >= 0 and first + 16 <= len(data):
            channels = data[first + 11]
            rate = struct.unpack_from("<I", data, first + 12)[0]
            info.update({"codec": "vorbis", "channels": channels, "sampleRate": rate})
        elif data.find(b"OpusHead") >= 0:
            info.update({"codec": "opus"})
        else:
            info.update({"codec": "unknown"})
        return info


class Mp3Codec(Codec):
    name = "mp3"
    fidelity = FIDELITY_PASSTHROUGH
    readability = READABILITY
    extensions = (".mp3",)
    magics = ()

    def sniff(self, data: bytes) -> bool:
        return len(data) >= 2 and data[0] == 0xFF and (data[1] & 0xE0) == 0xE0

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data, meta={"container": "mp3", "plain_size": len(data)})


STANDARD_CODECS = (PngCodec(), GifCodec(), JpegCodec(), ZipCodec(), TtfCodec(), OggCodec(),
                   Mp3Codec(), WebpCodec(), AxmlCodec())
