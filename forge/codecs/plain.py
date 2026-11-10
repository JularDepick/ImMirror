# -*- coding: utf-8 -*-
"""明文文本素材 (未经加密的 csv / xml / lua / shader / obj 等)。

解码只做编码探测, 不改写换行与 BOM, 因此编码回去逐字节相同;
工程文件统一以 UTF-8 落盘, 索引里记录原始编码, 便于阅读与 diff。
"""

from __future__ import annotations

from .base import Codec, FIDELITY_EDIT, Payload
from ..util.textio import decode_text, detect_eol

TEXT_EXTENSIONS = (
    ".lua", ".csv", ".xml", ".essl", ".hlsl", ".glsl", ".obj", ".mtl",
    ".json", ".svg", ".txt", ".md", ".ini", ".cfg", ".dat",
)


class PlainTextCodec(Codec):
    name = "plain-text"
    fidelity = FIDELITY_EDIT
    readability = "text"
    extensions = TEXT_EXTENSIONS
    magics = ()

    def encode(self, payload: Payload) -> bytes:
        return payload.raw_bytes()

    def decode(self, data: bytes) -> Payload:
        text, encoding, bom = decode_text(data)
        return Payload(
            kind="text",
            text=text,
            meta={
                "container": "plain",
                "encoding": encoding,
                "bom": bom,
                "eol": detect_eol(data),
                "plain_size": len(data),
            },
        )
