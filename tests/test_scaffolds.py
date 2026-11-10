# -*- coding: utf-8 -*-
"""脚手架契约测试: 各分类的工程目录导出与回写。

覆盖三层:

1. 样例工程: 每个分类的 sample/ 回写后与 source.sha256 一致, 且不依赖源二进制;
2. 合成样本: 各格式用最小合成二进制走 export -> build, 并覆盖若干编辑路径;
3. 真实样本: 原版逆向树存在时逐字节比对, 缺失时跳过。
"""

from __future__ import annotations

import contextlib
import io
import os
import shutil
import struct
import tempfile
import unittest

from scaffolds import CATEGORY_NAMES, list_categories, load_category
from scaffolds import util
from scaffolds.__main__ import main as cli_main

from forge.codecs import etc
from forge.codecs.container import MAGIC_TEXT, xor_stream
from forge.source import DEFAULT_SOURCE_ROOTS, SourceTree, normalize_roots
from forge.util import png as pngutil

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = SourceTree(normalize_roots(DEFAULT_SOURCE_ROOTS, ROOT))
MAGIC_KTX11 = b"\xabKTX 11\xbb\r\n\x1a\n"
TEMP_ROOT = os.path.join(ROOT, "temp", "tests-scaffolds")


def _work_dir() -> str:
    os.makedirs(TEMP_ROOT, exist_ok=True)
    return tempfile.mkdtemp(dir=TEMP_ROOT)


def _read(path: str) -> bytes:
    with open(path, "rb") as handle:
        return handle.read()


def _write(path: str, data: bytes) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)


def _ktx_bytes(width: int = 8, height: int = 8) -> bytes:
    """合成一份最小 KTX 11: ETC1_RGB8, 单级 mip。"""
    rgba = bytes((200, 120, 60, 255)) * (width * height)
    codec = etc.codec_for_format(0x8D64)
    block = etc.encode_image(codec, rgba, width, height)
    header = struct.pack(
        "<13I", 0x04030201, 0, 1, 0, 0x8D64, 0x1907, width, height, 0, 0, 1, 1, 0
    )
    return MAGIC_KTX11 + header + struct.pack("<I", len(block)) + block


def _named_block(blocks) -> bytes:
    """合成命名区块容器; blocks 为 (块名, 类型标识, 载荷) 序列。"""
    body = bytearray(b"\x89gE#" + struct.pack("<II", 100, 1))
    for name, type_id, payload in blocks:
        raw = name.encode("ascii")
        body += struct.pack("<H", type_id) + bytes((len(raw),)) + raw + payload
    return bytes(body)


def _otex_bytes(width: int = 8, height: int = 8) -> bytes:
    """合成 RGB8 otex: TextureData 32 字节加 SurfaceData 32 字节头与像素。"""
    texture_data = struct.pack("<8I", 0, width, height, 1, 1, 10, 0, 1)
    pixels = bytes((30, 60, 90)) * (width * height)
    surface = struct.pack(
        "<8I", 10, width, height, 1, 3, width * 3, len(pixels), len(pixels)
    ) + pixels
    return _named_block([("TextureData", 100, texture_data), ("SurfaceData", 100, surface)])


def _ogg_bytes() -> bytes:
    """合成单个 Ogg 页: 27 字节页头, 1 段, 4 字节正文。"""
    body = b"data"
    header = b"OggS" + bytes((0, 2)) + struct.pack("<q", 0) + struct.pack("<II", 7, 0)
    header += struct.pack("<I", 0) + bytes((1,))
    return header + bytes((len(body),)) + body


def _flatbuffers_min() -> bytes:
    """合成最小 FlatBuffers: 根表偏移 8, vtable 长度 4, 对象长度 4, 无字段。"""
    return struct.pack("<I", 8) + struct.pack("<HH", 4, 4) + struct.pack("<i", 4)


def _world_chunk_bytes(x: int = -1, z: int = -4) -> bytes:
    """合成世界区块: 密钥 33, 坐标区按单字节异或。"""
    key = 33
    plain = bytes((1, 0, 0)) + struct.pack("<ii", x, z)
    cipher = bytes(value ^ key for value in plain)
    header = b"\x0c\x1c\x07\x06" + cipher + struct.pack("<I", 16)
    header += b"\x00\x00\x00" + bytes.fromhex("205d004000000012003083c001")
    return header + b"\x01\x02\x03\x04" + b"\x00" * 12


def _cust_bytes() -> bytes:
    """合成 CUST: 明文首 4 字节 a91306ed 加最小 FlatBuffers。"""
    from scaffolds.world import CUST_KEY

    plain = bytes.fromhex("a91306ed") + _flatbuffers_min()
    cipher = bytes(value ^ CUST_KEY[index % 16] for index, value in enumerate(plain))
    return b"CUST" + cipher


class TestRegistry(unittest.TestCase):
    def test_categories_load(self):
        items = list_categories()
        self.assertEqual([item["name"] for item in items], list(CATEGORY_NAMES))
        for item in items:
            module = load_category(item["name"])
            self.assertTrue(module.TITLE)
            self.assertTrue(module.EXTENSIONS)
            self.assertTrue(module.FORMATS)
            self.assertTrue(module.PROJECT_FILE)
            self.assertTrue(callable(module.export))
            self.assertTrue(callable(module.build))

    def test_unknown_category(self):
        with self.assertRaises(ValueError):
            load_category("nope")


class TestSampleProjects(unittest.TestCase):
    """每个分类的 sample/ 回写后必须与 source.sha256 一致。"""

    def test_samples_roundtrip(self):
        work = _work_dir()
        try:
            for name in CATEGORY_NAMES:
                with self.subTest(category=name):
                    module = load_category(name)
                    sample = os.path.join(ROOT, "scaffolds", name, "sample")
                    self.assertTrue(os.path.isdir(sample), sample)
                    files = sorted(os.listdir(sample))
                    self.assertGreaterEqual(len(files), 1)
                    self.assertLessEqual(len(files), 2, "样例目录只放 1 到 2 个文件")
                    out = os.path.join(work, name + ".bin")
                    result = module.build(sample, out)
                    doc = util.read_json(
                        util.find_project_json(sample, module.PROJECT_FILE)
                    )
                    self.assertEqual(result["sha256"], doc["source"]["sha256"])
                    self.assertTrue(result["byte_identical"])
        finally:
            shutil.rmtree(work, ignore_errors=True)


class TestSyntheticRoundtrip(unittest.TestCase):
    """合成样本走 export -> build, 并验证若干编辑路径。"""

    def setUp(self):
        self.work = _work_dir()

    def tearDown(self):
        shutil.rmtree(self.work, ignore_errors=True)

    def _roundtrip(self, category: str, name: str, data: bytes):
        module = load_category(category)
        src = os.path.join(self.work, name)
        _write(src, data)
        project = os.path.join(self.work, name + ".project")
        summary = module.export(src, project)
        out = os.path.join(self.work, name + ".out")
        result = module.build(project, out)
        self.assertEqual(_read(out), data, "%s 回写字节不一致" % name)
        self.assertTrue(result["byte_identical"])
        return module, project, summary, out

    def test_models(self):
        data = _named_block(
            [
                ("ModelData", 101, struct.pack("<I", 2)),
                ("MeshData", 100, struct.pack("<HB", 1, 0) + b"mesh\x00" + struct.pack("<I", 7)),
            ]
        )
        module, project, summary, out = self._roundtrip("models", "synthetic.emo", data)
        self.assertEqual(summary["format"], "named-block")
        doc = util.read_json(os.path.join(project, "model.json"))
        self.assertEqual(len(doc["blocks"]), 2)
        self.assertTrue(doc["coverage"]["tiles_to_eof"])

        block = doc["blocks"][0]
        payload = util.blob_field_bytes(block["payload"])
        block["payload"] = util.make_blob_field(bytes([payload[0] ^ 0xFF]) + payload[1:])
        util.write_json(os.path.join(project, "model.json"), doc)
        edited = os.path.join(self.work, "synthetic-edited.emo")
        result = module.build(project, edited)
        self.assertFalse(result["byte_identical"])
        self.assertEqual(result["size"], len(data))

    def test_textures_ktx(self):
        data = _ktx_bytes()
        module, project, summary, _out = self._roundtrip("textures", "synthetic.ktx", data)
        self.assertEqual(summary["format"], "ktx11")
        png_path = os.path.join(project, "mip0.png")
        self.assertTrue(os.path.isfile(png_path))

        width, height, rgba = pngutil.read_png(_read(png_path))
        """整张图换成纯蓝, 保证重新压块后必然与原块不同 (单像素改动可能被量化吸收)。"""
        pixels = bytearray()
        for _ in range(width * height):
            pixels += b"\x00\x00\xff\xff"
        _write(png_path, pngutil.write_rgba(width, height, bytes(pixels)))
        out = os.path.join(self.work, "synthetic-edited.ktx")
        result = module.build(project, out)
        self.assertTrue(result["reencoded"])
        self.assertFalse(result["byte_identical"])
        self.assertEqual(len(_read(out)), len(data))
        self.assertIn("pixel_diff", result)

    def test_textures_otex(self):
        data = _otex_bytes()
        module, project, summary, _out = self._roundtrip("textures", "synthetic.otex", data)
        self.assertEqual(summary["format"], "otex")
        png_path = os.path.join(project, "mip0.png")
        width, height, rgba = pngutil.read_png(_read(png_path))
        pixels = bytearray(rgba)
        pixels[0:3] = b"\x01\x02\x03"
        _write(png_path, pngutil.write_rgba(width, height, bytes(pixels)))
        out = os.path.join(self.work, "synthetic-edited.otex")
        result = module.build(project, out)
        self.assertTrue(result["reencoded"])
        self.assertEqual(result["size"], len(data))

    def test_ui_axml_rejects_xml_edit(self):
        data = os.path.join(ROOT, "scaffolds", "ui", "sample", "layout.json")
        self.assertTrue(os.path.isfile(data))
        sample = os.path.join(ROOT, "scaffolds", "ui", "sample")
        module = load_category("ui")
        project = os.path.join(self.work, "ui-sample")
        shutil.copytree(sample, project)
        xml_path = os.path.join(project, "layout.xml")
        with open(xml_path, "a", encoding="utf-8") as handle:
            handle.write("\n")
        with self.assertRaises(util.ScaffoldError):
            module.build(project, os.path.join(self.work, "ui-sample.xml"))

    def test_ui_text_xml(self):
        text = '<LinearLayout width="100">\n  <Button text="确定" />\n</LinearLayout>\n'
        data = text.encode("utf-8")
        module, project, summary, _out = self._roundtrip("ui", "layout.xml", data)
        self.assertEqual(summary["format"], "ui-xml-text")
        text_path = os.path.join(project, "layout.xml")
        with open(text_path, "w", encoding="utf-8", newline="") as handle:
            handle.write(text.replace("100", "120"))
        out = os.path.join(self.work, "layout-edited.xml")
        result = module.build(project, out)
        self.assertFalse(result["byte_identical"])
        self.assertIn(b"120", _read(out))

    def test_data_text_family(self):
        plain = "id,name\r\n1,草方块\r\n".encode("gbk")
        import zlib

        data = MAGIC_TEXT + len(plain).to_bytes(4, "big") + xor_stream(zlib.compress(plain, 6))
        module, project, summary, _out = self._roundtrip("data", "table.csv", data)
        self.assertEqual(summary["encoding"], "gbk")
        text_path = os.path.join(project, "table.csv")
        with open(text_path, "w", encoding="utf-8", newline="") as handle:
            handle.write("id,name\r\n2,石头\r\n")
        out = os.path.join(self.work, "table-edited.csv")
        module.build(project, out)
        """回写按原编码转回 GBK; 直接剥容器解码, 避免文本编码探测把 GBK 误判成 UTF-8。"""
        import zlib

        raw = _read(out)
        self.assertEqual(raw[:4], MAGIC_TEXT)
        plain_out = zlib.decompress(xor_stream(raw[8:]))
        self.assertIn("石头".encode("gbk"), plain_out)
        self.assertTrue(plain_out.startswith(b"id,name\r\n2,"))

    def test_audio_ogg(self):
        data = _ogg_bytes()
        module, project, summary, _out = self._roundtrip("audio", "tone.ogg", data)
        self.assertEqual(summary["format"], "ogg")
        doc = util.read_json(os.path.join(project, "audio.json"))
        self.assertEqual(doc["pages"]["pages"], 1)

    def test_world_vmof(self):
        size = [2, 2, 2]
        voxels = struct.pack("<8H", 0, 1, 2, 3, 4, 5, 6, 7)
        data = b"VMOF" + struct.pack("<4I", 100, *size) + voxels
        module, project, summary, _out = self._roundtrip("world", "cube.vmo", data)
        self.assertEqual(summary["format"], "vmof")
        doc = util.read_json(os.path.join(project, "world.json"))
        self.assertEqual(doc["header"]["size"], size)
        self.assertEqual(doc["voxels"]["nonzero"], 7)

    def test_world_vox(self):
        size = struct.pack("<3I", 4, 3, 2)
        voxels = struct.pack("<I", 2) + bytes((0, 0, 0, 1)) + bytes((1, 1, 1, 2))
        palette = b"".join(bytes((index, index, index, 255)) for index in range(256))
        main = (
            b"MAIN" + struct.pack("<II", 0, 12 + len(size) + 12 + len(voxels) + 12 + len(palette))
        )
        data = b"VOX " + struct.pack("<I", 150) + main
        data += b"SIZE" + struct.pack("<II", len(size), 0) + size
        data += b"XYZI" + struct.pack("<II", len(voxels), 0) + voxels
        data += b"RGBA" + struct.pack("<II", len(palette), 0) + palette
        module, project, summary, _out = self._roundtrip("world", "cube.vox", data)
        self.assertEqual(summary["format"], "vox")
        doc = util.read_json(os.path.join(project, "world.json"))
        self.assertEqual(doc["chunks"][0]["children"][0]["size"], [4, 3, 2])

    def test_world_chunk(self):
        data = _world_chunk_bytes()
        module, project, summary, _out = self._roundtrip("world", "x-1z-4.r", data)
        self.assertEqual(summary["format"], "world-chunk")
        doc = util.read_json(os.path.join(project, "world.json"))
        self.assertEqual(doc["coordinate"]["x"], -1)
        self.assertEqual(doc["coordinate"]["z"], -4)
        self.assertEqual(doc["coordinate"]["key"], 33)

    def test_world_cust(self):
        data = _cust_bytes()
        module, project, summary, _out = self._roundtrip("world", "model.cm", data)
        self.assertEqual(summary["format"], "cust")
        doc = util.read_json(os.path.join(project, "world.json"))
        self.assertEqual(doc["shell"]["magic"], "CUST")
        self.assertEqual(doc["flatbuffer"]["root_offset"], 8)

    def test_world_flatbuffers(self):
        data = _flatbuffers_min()
        module, project, summary, _out = self._roundtrip("world", "model.fcm", data)
        self.assertEqual(summary["format"], "flatbuffers")
        doc = util.read_json(os.path.join(project, "world.json"))
        self.assertEqual(doc["root_table"]["slots"], [])


class TestRealSamples(unittest.TestCase):
    """真实样本逐字节比对; 原版逆向树缺失时跳过。"""

    SAMPLES = (
        ("models", "entity/100035/hand.omod"),
        ("models", "particles/1001.emo"),
        ("textures", "ui/dotted.ktx"),
        ("textures", "itemmods/coord/$1.otex"),
        ("ui", "res/anim/accelerate_cubic.xml"),
        ("ui", "assets/blockgeom.xml"),
        ("data", "csvdef/ARstylesetting.csv"),
        ("data", "checkTask.json"),
        ("audio", "sounds/buff/buff_pop.ogg"),
        ("world", "voxel/baozishuding.vmo"),
        ("world", "voxel/longdan.vox"),
        ("world", "worlds/homegarden_customize_map/10/1/x-1z-4.r"),
        ("world", "custommodel_equip/7345778771590029785.cm"),
        ("world", "custommodel_equip/fully/7345778771590031430.fcm"),
    )

    def test_real_samples(self):
        work = _work_dir()
        try:
            for category, rel in self.SAMPLES:
                with self.subTest(sample=rel):
                    path = SOURCE.locate(rel)
                    if path is None:
                        self.skipTest("真实样本缺失: %s" % rel)
                    module = load_category(category)
                    project = os.path.join(work, rel.replace("/", "_"))
                    module.export(path, project)
                    out = os.path.join(work, rel.replace("/", "_") + ".bin")
                    result = module.build(project, out)
                    self.assertEqual(_read(out), _read(path), rel)
                    self.assertTrue(result["byte_identical"])
        finally:
            shutil.rmtree(work, ignore_errors=True)


def _run_cli(argv) -> int:
    """跑命令行入口并吞掉它打印的内容, 避免污染测试输出。"""
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        return cli_main(argv)


class TestCli(unittest.TestCase):
    def test_list(self):
        self.assertEqual(_run_cli(["list"]), 0)

    def test_build_sample(self):
        work = _work_dir()
        try:
            out = os.path.join(work, "sample.ent")
            code = _run_cli(
                ["models", "build", os.path.join(ROOT, "scaffolds", "models", "sample"),
                 "-o", out]
            )
            self.assertEqual(code, 0)
            self.assertTrue(os.path.isfile(out))
            self.assertGreater(os.path.getsize(out), 0)
        finally:
            shutil.rmtree(work, ignore_errors=True)

    def test_unknown_extension_fails(self):
        work = _work_dir()
        try:
            path = os.path.join(work, "unknown.bin")
            _write(path, b"\x00\x01\x02\x03" + b"\x00" * 16)
            self.assertEqual(_run_cli(["models", "export", path]), 1)
        finally:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    unittest.main()
