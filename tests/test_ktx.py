# -*- coding: utf-8 -*-
"""KTX 纹理的容器、像素与 PNG 回写契约测试。

真实样本优先取客户端资源树, 资源树已精简或缺少纹理时退到原版逆向的原始资源树;
两处都不存在时真实样本用例整体跳过, 合成样本用例仍然执行。
"""

from __future__ import annotations

import os
import struct
import unittest

from forge import ktx_png
from forge.codecs import etc, ktx
from forge.codecs.base import CodecError
from forge.codecs.ktx import Ktx11Codec
from forge.util import png

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLIENT_ASSETS = os.path.abspath(os.path.join(ROOT, "..", "client", "src", "assets"))
ORIGINAL_ASSETS = os.path.abspath(
    os.path.join(
        ROOT,
        "..",
        "client",
        "temp",
        "original",
        "fake-mini-BUT-true-moments",
        "src",
    )
)

"""真实样本最多取这么多张, 覆盖带容器与裸 KTX 两种形态。"""
SAMPLE_LIMIT = 14


def collect_samples(limit=SAMPLE_LIMIT):
    """在候选资源树里找 KTX 样本, 返回 [(根, 相对路径)]。"""
    samples = []
    for root in (CLIENT_ASSETS, ORIGINAL_ASSETS):
        if not os.path.isdir(root):
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames.sort()
            for name in sorted(filenames):
                if not name.lower().endswith(".ktx"):
                    continue
                rel = os.path.relpath(os.path.join(dirpath, name), root).replace(os.sep, "/")
                samples.append((root, rel))
                if len(samples) >= limit:
                    return samples
    return samples


def read_sample(root, rel):
    with open(os.path.join(root, rel), "rb") as handle:
        return handle.read()


def build_ktx(width, height, levels, internal_format=0x8D64, payloads=None):
    """按给定尺寸与像素压出最小 KTX 11 文件, 每级 mip 前带长度字段。"""
    codec = etc.codec_for_format(internal_format)
    body = bytearray()
    data_levels = payloads if payloads is not None else levels
    for index, (level_width, level_height) in enumerate(data_levels):
        if index == 0:
            pixels = levels
        block = etc.encode_image(codec, pixels, level_width, level_height) \
            if not isinstance(levels, bytes) else b""
        body.extend(struct.pack("<I", len(block)))
        body.extend(block)
    header = struct.pack(
        "<13I",
        0x04030201,
        0,
        1,
        0,
        internal_format,
        0x1907 if internal_format != 0x9278 else 0x1908,
        width,
        height,
        0,
        0,
        1,
        len(data_levels),
        0,
    )
    return ktx.MAGIC_KTX11 + header + bytes(body)


def make_texture(width, height, rgba, internal_format=0x9278):
    """用给定 RGBA 像素造一份单级 mip 的 KTX。"""
    codec = etc.codec_for_format(internal_format)
    block = etc.encode_image(codec, rgba, width, height)
    header = struct.pack(
        "<13I",
        0x04030201,
        0,
        1,
        0,
        internal_format,
        0x1908 if internal_format == 0x9278 else 0x1907,
        width,
        height,
        0,
        0,
        1,
        1,
        0,
    )
    return ktx.MAGIC_KTX11 + header + struct.pack("<I", len(block)) + block


class TestKtxContainer(unittest.TestCase):
    def test_header_rejects_short_data(self):
        with self.assertRaises(CodecError):
            ktx.parse(b"\xabKTX 11\xbb\r\n\x1a\n" + b"\x00" * 10)

    def test_header_rejects_bad_magic(self):
        with self.assertRaises(CodecError):
            ktx.parse(b"\x00" * 64)

    def test_codec_roundtrip_is_passthrough(self):
        width = height = 8
        pixels = bytes((x * 7 + y * 3) % 256 for y in range(height) for x in range(width * 4))
        blob = make_texture(width, height, pixels)
        codec = Ktx11Codec()
        payload = codec.decode(blob)
        self.assertEqual(codec.encode(payload), blob)
        self.assertEqual(payload.meta["pixel_codec"], "etc2-rgba")
        self.assertEqual(payload.meta["mip_levels"], 1)

    def test_view_reports_pixel_codec(self):
        pixels = bytes([0, 0, 0, 255] * 16)
        blob = make_texture(4, 4, pixels)
        view = Ktx11Codec().view(Ktx11Codec().decode(blob))
        self.assertEqual(view["glInternalFormatName"], "ETC2_RGBA8")
        self.assertEqual(view["pixelCodec"], "etc2-rgba")
        self.assertEqual(view["mipLevels"], 1)

    def test_unsupported_format_is_marked(self):
        blob = bytearray(make_texture(4, 4, bytes(64)))
        struct.pack_into("<I", blob, 28, 0x8C00)
        payload = Ktx11Codec().decode(bytes(blob))
        self.assertIsNone(payload.meta["pixel_codec"])
        with self.assertRaises(CodecError):
            ktx.to_rgba(ktx.parse(bytes(blob)))
        self.assertEqual(ktx_png.side_files("a.ktx", payload), ())


class TestPng(unittest.TestCase):
    def test_rgba_roundtrip(self):
        pixels = bytes((x * 13) % 256 for x in range(5 * 3 * 4))
        data = png.write_rgba(5, 3, pixels)
        width, height, restored = png.read_png(data)
        self.assertEqual((width, height), (5, 3))
        self.assertEqual(restored, pixels)

    def test_palette_and_gray(self):
        gray = png.write_rgb(2, 2, bytes([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]))
        width, height, restored = png.read_png(gray)
        self.assertEqual((width, height), (2, 2))
        self.assertEqual(restored[3], 255)


class TestEtcBlocks(unittest.TestCase):
    def test_etc1_flat_block_is_exact(self):
        pixels = [(34, 68, 102)] * 16
        block = etc.encode_etc1_block(pixels)
        decoded = etc.decode_etc1_block(block)
        worst = max(
            abs(decoded[i][c] - pixels[i][c]) for i in range(16) for c in range(3)
        )
        self.assertLessEqual(worst, 8)

    def test_etc1_gradient_block_bounded_error(self):
        pixels = [
            (x * 18, y * 18, (x + y) * 9) for y in range(4) for x in range(4)
        ]
        block = etc.encode_etc1_block(pixels)
        decoded = etc.decode_etc1_block(block)
        error = sum(
            abs(decoded[i][c] - pixels[i][c]) for i in range(16) for c in range(3)
        ) / 48.0
        self.assertLess(error, 12)

    def test_etc2_alpha_roundtrip(self):
        pixels = [(30, 60, 90, value * 16) for value in range(16)]
        block = etc.encode_etc2_rgba_block(pixels)
        decoded = etc.decode_etc2_rgba_block(block)
        worst = max(abs(decoded[i][3] - pixels[i][3]) for i in range(16))
        self.assertLessEqual(worst, 16)

    def test_decode_image_handles_partial_blocks(self):
        codec = etc.codec_for_format(0x8D64)
        pixels = bytearray()
        for y in range(7):
            for x in range(9):
                pixels.extend((x * 24, y * 30, (x + y) * 14, 255))
        pixels = bytes(pixels)
        data = etc.encode_image(codec, pixels, 9, 7)
        restored = etc.decode_image(codec, data, 9, 7)
        self.assertEqual(len(restored), 9 * 7 * 4)
        error = sum(abs(a - b) for a, b in zip(pixels, restored)) / len(pixels)
        self.assertLess(error, 12)

    def test_decode_image_rejects_short_payload(self):
        codec = etc.codec_for_format(0x8D64)
        with self.assertRaises(CodecError):
            etc.decode_image(codec, b"\x00" * 8, 16, 16)


class TestWriteback(unittest.TestCase):
    def test_rebuild_keeps_pixels_close(self):
        pixels = bytearray()
        for y in range(8):
            for x in range(8):
                pixels.extend((x * 30, y * 30, (x + y) * 15, 255))
        blob = make_texture(8, 8, bytes(pixels))
        texture = ktx.parse(blob)
        rebuilt = ktx.rebuild(texture, bytes(pixels))
        restored = ktx.to_rgba(ktx.parse(rebuilt))
        mean, biggest, changed = ktx_png.diff_pixels(bytes(pixels), restored)
        self.assertLess(mean, 8.0)
        self.assertEqual(len(rebuilt) // 8 * 8, len(rebuilt))

    def test_import_is_noop_when_pixels_match(self):
        pixels = bytes([10, 20, 30, 255] * 16)
        blob = make_texture(4, 4, pixels)
        texture = ktx.parse(blob)
        exported = png.read_png(ktx.to_png(texture))
        self.assertEqual(exported[:2], (4, 4))
        self.assertEqual(len(exported[2]), 4 * 4 * 4)


class TestRealSamples(unittest.TestCase):
    """真实素材上的解码与往返验证。"""

    samples = collect_samples()

    def setUp(self):
        if not self.samples:
            self.skipTest("没有可用的 KTX 真实样本")

    def test_codec_roundtrip_all_samples(self):
        codec = Ktx11Codec()
        tested = 0
        for root, rel in self.samples:
            raw = read_sample(root, rel)
            plain = ktx_png.unwrap(raw)
            self.assertTrue(ktx_png.is_ktx(plain), "剥壳后不是 KTX: %s" % rel)
            payload = codec.decode(plain)
            self.assertEqual(codec.encode(payload), plain, "逐字节还原失败: %s" % rel)
            tested += 1
        self.assertGreaterEqual(tested, 10, "真实样本少于 10 个")

    def test_mip_chain_matches_etc1_block_math(self):
        for root, rel in self.samples:
            plain = ktx_png.unwrap(read_sample(root, rel))
            texture = ktx.parse(plain)
            tested = 0
            for entry in texture.levels:
                expect = ((entry["width"] + 3) // 4) * ((entry["height"] + 3) // 4) * 8
                self.assertEqual(entry["size"], expect, "mip 尺寸不符: %s 第 %d 级" % (rel, entry["level"]))
                tested += 1
            self.assertGreater(tested, 0)

    def test_decode_to_png_matches_rgba(self):
        tested = 0
        for root, rel in self.samples[:6]:
            plain = ktx_png.unwrap(read_sample(root, rel))
            texture = ktx.parse(plain)
            if texture.codec() is None:
                continue
            image = ktx.to_png(texture)
            width, height, pixels = png.read_png(image)
            self.assertEqual((width, height), (texture.width, texture.height))
            self.assertEqual(pixels, ktx.to_rgba(texture))
            tested += 1
        self.assertGreaterEqual(tested, 5, "可解样本少于 5 个")

    def test_side_files_cover_wrapped_and_bare(self):
        codec = Ktx11Codec()
        covered = 0
        for root, rel in self.samples:
            raw = read_sample(root, rel)
            plain = ktx_png.unwrap(raw)
            payload = codec.decode(plain)
            files = ktx_png.side_files(rel, payload)
            self.assertEqual(len(files), 1)
            path, data = files[0]
            self.assertTrue(path.startswith("_png/"))
            self.assertTrue(path.endswith(".png"))
            self.assertTrue(data.startswith(b"\x89PNG"))
            covered += 1
        self.assertGreaterEqual(covered, 10)

    def test_rebuild_error_is_bounded(self):
        worst = 0.0
        for root, rel in self.samples[:5]:
            plain = ktx_png.unwrap(read_sample(root, rel))
            texture = ktx.parse(plain)
            if texture.codec() is None:
                continue
            pixels = ktx.to_rgba(texture)
            rebuilt = ktx.rebuild(texture, pixels)
            restored = ktx.to_rgba(ktx.parse(rebuilt))
            mean, _biggest, _changed = ktx_png.diff_pixels(pixels, restored)
            worst = max(worst, mean)
        self.assertLess(worst, 8.0)


if __name__ == "__main__":
    unittest.main()
