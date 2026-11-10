# -*- coding: utf-8 -*-
"""原版两级加密容器的解码与再编码。

文本族 FF D9 FF D7: 头部 8 字节 (魔数加解压后长度), 其后是 zlib 流与 16 字节
周期密钥异或的结果。二进制族 FF D9 FF D8: 头部记录的加密字节数 (实测恒为 64),
只对明文前若干字节做同样的异或, 其余原样。

两族的再编码都能与原始文件逐字节相同: 文本族固定用 zlib 默认级别 (6),
实测 36 个样本全部命中。
"""

from __future__ import annotations

import zlib

from .base import Codec, CodecError, FIDELITY_EDIT, FIDELITY_PASSTHROUGH, Payload
from ..util.textio import decode_text, detect_eol

KEY = bytes.fromhex("d6020800f4feff3f01000000d0ca0100")

MAGIC_TEXT = b"\xff\xd9\xff\xd7"
MAGIC_BINARY = b"\xff\xd9\xff\xd8"

"""再编码文本族时的压缩级别, 由实测反推, 改动会破坏逐字节一致。"""
TEXT_ZLIB_LEVEL = 6

"""二进制族默认加密前缀长度, 实际以文件头记录为准。"""
BINARY_PREFIX_DEFAULT = 64

TEXT_EXTS = (".lua", ".csv", ".xml", ".txt")


def xor_stream(data: bytes, key: bytes = KEY) -> bytes:
    """按密钥周期逐字节异或, 密钥从 data 起点对齐。"""
    n = len(key)
    stream = (key * (len(data) // n + 1))[: len(data)]
    return bytes(a ^ b for a, b in zip(data, stream))


class TextFamilyCodec(Codec):
    name = "container-text-xor16-zlib"
    fidelity = FIDELITY_EDIT
    readability = "text"
    extensions = TEXT_EXTS
    magics = (MAGIC_TEXT,)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == MAGIC_TEXT

    def decode(self, data: bytes) -> Payload:
        if len(data) <= 8:
            raise CodecError("文本族容器过短: %d 字节" % len(data))
        declared = int.from_bytes(data[4:8], "big")
        try:
            plain = zlib.decompress(xor_stream(data[8:]))
        except zlib.error as exc:
            raise CodecError("文本族容器的 zlib 解压失败: %s" % exc) from exc
        if len(plain) != declared:
            raise CodecError(
                "文本族容器长度不符: 头部声明 %d, 解压得到 %d" % (declared, len(plain))
            )
        text, encoding, bom = decode_text(plain)
        return Payload(
            kind="text",
            text=text,
            meta={
                "container": "text-family",
                "encoding": encoding,
                "bom": bom,
                "eol": detect_eol(plain),
                "plain_size": len(plain),
            },
        )

    def encode(self, payload: Payload) -> bytes:
        plain = payload.raw_bytes()
        body = xor_stream(zlib.compress(plain, TEXT_ZLIB_LEVEL))
        return MAGIC_TEXT + len(plain).to_bytes(4, "big") + body


class BinaryFamilyCodec(Codec):
    name = "container-binary-xor16-head"
    fidelity = FIDELITY_PASSTHROUGH
    extensions = (".ktx", ".ogg", ".omod")
    magics = (MAGIC_BINARY,)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == MAGIC_BINARY

    def decode(self, data: bytes) -> Payload:
        if len(data) <= 8:
            raise CodecError("二进制族容器过短: %d 字节" % len(data))
        prefix = int.from_bytes(data[4:8], "big")
        if prefix > len(data) - 8:
            raise CodecError("二进制族容器声明的前缀长度越界: %d" % prefix)
        plain = xor_stream(data[8 : 8 + prefix]) + data[8 + prefix :]
        return Payload(
            kind="binary",
            blob=plain,
            meta={
                "container": "binary-family",
                "enc_prefix": prefix,
                "plain_magic": plain[:4].hex(),
                "plain_size": len(plain),
            },
        )

    def encode(self, payload: Payload) -> bytes:
        plain = payload.raw_bytes()
        prefix = int(payload.meta.get("enc_prefix", BINARY_PREFIX_DEFAULT))
        if prefix > len(plain):
            raise CodecError("载荷不足 %d 字节, 无法套用二进制族容器" % prefix)
        return MAGIC_BINARY + prefix.to_bytes(4, "big") + xor_stream(plain[:prefix]) + plain[prefix:]
