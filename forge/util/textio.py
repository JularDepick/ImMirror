# -*- coding: utf-8 -*-
"""文本字节流的无损编解码。

解码后必须能原样编码回完全相同的字节, 因此不改变换行符与 BOM,
只把探测到的编码与 BOM 状态记录到索引里。
"""

from __future__ import annotations

BOM_UTF8 = b"\xef\xbb\xbf"

"""候选编码按优先级排列: 先严格 UTF-8, 再中文常用编码, 最后 latin-1 兜底。"""
ENCODING_CANDIDATES = ("utf-8", "gbk", "gb18030", "big5")

"""索引里归一化后的编码名。"""
ENCODING_ALIASES = {
    "utf-8-sig": "utf-8",
    "gb2312": "gbk",
    "gb18030": "gbk",
}


def detect_eol(raw: bytes) -> str:
    """换行风格只做记录, 不参与改写。"""
    crlf = raw.count(b"\r\n")
    lf = raw.count(b"\n") - crlf
    if crlf and not lf:
        return "crlf"
    if lf and not crlf:
        return "lf"
    if crlf and lf:
        return "mixed"
    return "none"


def decode_text(raw: bytes):
    """返回 (文本, 编码名, 是否带 UTF-8 BOM)。失败抛 UnicodeDecodeError。"""
    has_bom = raw.startswith(BOM_UTF8)
    if has_bom:
        return raw.decode("utf-8-sig"), "utf-8", True
    for enc in ENCODING_CANDIDATES:
        try:
            return raw.decode(enc), ENCODING_ALIASES.get(enc, enc), False
        except UnicodeDecodeError:
            continue
    """兜底: latin-1 对任意字节都能解码, 保证不丢数据, 但内容不可读。"""
    return raw.decode("latin-1"), "latin-1", False


def encode_text(text: str, encoding: str = "utf-8", bom: bool = False) -> bytes:
    """按记录的编码把文本编码回字节; bom 为真时补回 UTF-8 BOM。"""
    enc = "utf-8" if encoding == "utf-8" else encoding
    data = text.encode(enc)
    if bom and not data.startswith(BOM_UTF8):
        data = BOM_UTF8 + data
    return data


def is_probably_text(raw: bytes) -> bool:
    """粗判字节流是否为文本: 不允许出现 0 字节, 且可被任一候选编码严格解码。"""
    if b"\x00" in raw[:4096]:
        return False
    try:
        decode_text(raw[:8192])
    except UnicodeDecodeError:
        return False
    return True
