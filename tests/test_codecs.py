# -*- coding: utf-8 -*-
"""编解码器契约测试: 解码后重新编码必须还原成原始字节。"""

from __future__ import annotations

import os
import unittest

from forge.codecs.container import BinaryFamilyCodec, TextFamilyCodec
from forge.codecs.ktx import Ktx11Codec
from forge.codecs.named_block import NamedBlockCodec
from forge.codecs.plain import PlainTextCodec
from forge.registry import select

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

"""真实样本可能留在客户端资源树, 也可能只在原版逆向工程树, 两处都查。"""
from forge.source import DEFAULT_SOURCE_ROOTS, SourceTree, normalize_roots  # noqa: E402

SOURCE = SourceTree(normalize_roots(DEFAULT_SOURCE_ROOTS, ROOT))


class TestTextFamily(unittest.TestCase):
    def test_roundtrip_utf8(self):
        codec = TextFamilyCodec()
        plain = "id,name\r\n1,方块\r\n".encode("utf-8")
        import zlib

        from forge.codecs.container import MAGIC_TEXT, xor_stream

        raw = MAGIC_TEXT + len(plain).to_bytes(4, "big") + xor_stream(zlib.compress(plain, 6))
        payload = codec.decode(raw)
        self.assertEqual(payload.text, plain.decode("utf-8"))
        self.assertEqual(codec.encode(payload), raw)

    def test_roundtrip_gbk(self):
        codec = TextFamilyCodec()
        plain = "编号,名称\n1,草方块\n".encode("gbk")
        import zlib

        from forge.codecs.container import MAGIC_TEXT, xor_stream

        raw = MAGIC_TEXT + len(plain).to_bytes(4, "big") + xor_stream(zlib.compress(plain, 6))
        payload = codec.decode(raw)
        self.assertEqual(payload.meta["encoding"], "gbk")
        self.assertEqual(codec.encode(payload), raw)


class TestBinaryFamily(unittest.TestCase):
    def test_roundtrip(self):
        codec = BinaryFamilyCodec()
        plain = bytes(range(256)) * 4
        from forge.codecs.container import MAGIC_BINARY, xor_stream

        raw = MAGIC_BINARY + (64).to_bytes(4, "big") + xor_stream(plain[:64]) + plain[64:]
        payload = codec.decode(raw)
        self.assertEqual(payload.blob, plain)
        self.assertEqual(codec.encode(payload), raw)


class TestPlainText(unittest.TestCase):
    def test_roundtrip_with_bom_and_crlf(self):
        codec = PlainTextCodec()
        plain = b"\xef\xbb\xbfname,value\r\n1,2\r\n"
        payload = codec.decode(plain)
        self.assertTrue(payload.meta["bom"])
        self.assertEqual(payload.meta["eol"], "crlf")
        self.assertEqual(codec.encode(payload), plain)


class TestSniffOrder(unittest.TestCase):
    def test_container_wins_over_extension(self):
        data = b"\xff\xd9\xff\xd7" + (4).to_bytes(4, "big") + b"\xae\x9e\x00\x00"
        self.assertEqual(select("assets/csvdef/x.csv", data).name, "container-text-xor16-zlib")

    def test_ktx_by_magic(self):
        data = b"\xabKTX 11\xbb\r\n\x1a\n" + b"\x00" * 52
        self.assertEqual(select("media/ktx/a.ktx", data).name, "ktx11")

    def test_named_block_by_magic(self):
        data = bytes.fromhex("89674523") + (100).to_bytes(4, "little") + (1).to_bytes(4, "little")
        self.assertEqual(select("particles/1.emo", data).name, "named-block")


@unittest.skipUnless(any(os.path.isdir(root) for root in SOURCE.roots),
                     "候选源树均不存在, 跳过真实样本测试")
class TestRealSamples(unittest.TestCase):
    """在真实素材上验证逐字节往返, 覆盖加密族与明文族。"""

    SAMPLES = (
        "csvdef/animact.csv",
        "ui/mobile/main_lobby.xml",
        "ui/mobile/main_lobby.lua",
    )

    def test_text_family_real(self):
        from forge.codecs.container import TextFamilyCodec

        codec = TextFamilyCodec()
        tested = 0
        for rel in self.SAMPLES:
            path = SOURCE.locate(rel)
            if path is None:
                continue
            with open(path, "rb") as fh:
                raw = fh.read()
            if raw[:4] != b"\xff\xd9\xff\xd7":
                continue
            payload = codec.decode(raw)
            self.assertEqual(codec.encode(payload), raw, "逐字节还原失败: %s" % rel)
            tested += 1
        self.assertGreater(tested, 0, "没有找到可用的加密文本样本")


if __name__ == "__main__":
    unittest.main()
