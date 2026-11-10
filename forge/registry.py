# -*- coding: utf-8 -*-
"""编解码器注册表。

选择顺序: 先按魔数匹配容器族 (加密族, KTX, 命名区块, 标准格式),
再按扩展名匹配明文文本, 最后落到兜底编解码器。新增格式只改这里一处。
"""

from __future__ import annotations

import os

from .codecs.base import Codec
from .codecs.container import BinaryFamilyCodec, TextFamilyCodec
from .codecs.ktx import Ktx11Codec
from .codecs.named_block import NamedBlockCodec
from .codecs.opaque import OpaqueCodec
from .codecs.plain import PlainTextCodec
from .codecs.standard import STANDARD_CODECS
from .util.textio import is_probably_text

"""按魔数识别的容器族, 顺序即优先级: 加密族先于其解码后的标准格式。"""
MAGIC_CODECS = (
    TextFamilyCodec(),
    BinaryFamilyCodec(),
    Ktx11Codec(),
    NamedBlockCodec(),
) + STANDARD_CODECS

"""按扩展名识别的明文格式。"""
EXT_CODECS = (PlainTextCodec(),)

"""兜底编解码器。"""
FALLBACK_CODEC = OpaqueCodec()


def all_codecs():
    return list(MAGIC_CODECS) + list(EXT_CODECS) + [FALLBACK_CODEC]


def select(rel_path: str, data: bytes) -> Codec:
    """为一份素材选出编解码器。rel_path 只用于取扩展名。"""
    for codec in MAGIC_CODECS:
        if codec.sniff(data):
            return codec
    ext = os.path.splitext(rel_path)[1].lower()
    for codec in EXT_CODECS:
        if ext in codec.extensions and is_probably_text(data[:8192]):
            return codec
    return FALLBACK_CODEC
