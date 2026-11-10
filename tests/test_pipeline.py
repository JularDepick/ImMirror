# -*- coding: utf-8 -*-
"""管线级测试: 源树定位, 工程树校验, 分层抽样与往返验证, 以及新增公开格式的编解码。"""

from __future__ import annotations

import json
import os
import struct
import tempfile
import unittest

from forge.codecs.axml import AxmlCodec
from forge.codecs.webp import WebpCodec
from forge.project import TARGET_ORIGINAL, TARGET_PLAIN, pack_file, pack_tree, write_index
from forge.registry import select
from forge.roundtrip import category_of, plan_samples, run_sample
from forge.source import SourceTree
from forge.verify import verify_tree


def _varint8(value: int) -> bytes:
    """字符串池的 UTF-8 变长长度前缀。"""
    if value < 0x80:
        return bytes((value,))
    return bytes(((value >> 8) | 0x80, value & 0xFF))


def _pool(strings) -> bytes:
    """构造一个最小的 UTF-8 RES_STRING_POOL 块。"""
    offsets = []
    blob = b""
    for text in strings:
        raw = text.encode("utf-8")
        offsets.append(len(blob))
        blob += _varint8(len(text)) + _varint8(len(raw)) + raw + b"\x00"
    header = 28
    strings_start = header + 4 * len(strings)
    size = strings_start + len(blob)
    head = struct.pack("<HHI", 0x0001, header, size)
    body = struct.pack("<IIIII", len(strings), 0, 0x100, strings_start, 0)
    table = b"".join(struct.pack("<I", off) for off in offsets)
    return head + body + table + blob


def _start_element(name_index: int, line: int = 1) -> bytes:
    """一个不带属性的开始元素块。"""
    body = (struct.pack("<II", line, 0xFFFFFFFF)
            + struct.pack("<IIHHHHHH", 0xFFFFFFFF, name_index, 0x14, 0x14, 0, 0, 0, 0))
    return struct.pack("<HHI", 0x0102, 0x10, 8 + len(body)) + body


def _start_element_with_attr(name_index: int, attr_name: int, value: int) -> bytes:
    """一个带整数属性的开始元素块, 属性个数写在块头里。"""
    body = (struct.pack("<II", 1, 0xFFFFFFFF)
            + struct.pack("<IIHHHHHH", 0xFFFFFFFF, name_index, 0x14, 0x14, 1, 0, 0, 0))
    body += struct.pack("<IIIHBBI", 0xFFFFFFFF, attr_name, 0xFFFFFFFF, 8, 0, 0x10, value)
    return struct.pack("<HHI", 0x0102, 0x10, 8 + len(body)) + body


def _end_element(name_index: int, line: int = 1) -> bytes:
    body = struct.pack("<II", line, 0xFFFFFFFF) + struct.pack("<II", 0xFFFFFFFF, name_index)
    return struct.pack("<HHI", 0x0103, 0x10, 8 + len(body)) + body


def _axml() -> bytes:
    """拼一份最小的 RES_XML: 字符串池加一层元素。"""
    strings = ["Root", "count"]
    pool = _pool(strings)
    nodes = _start_element_with_attr(0, 1, 7) + _start_element(0) + _end_element(0) + _end_element(0)
    total = 8 + len(pool) + len(nodes)
    return struct.pack("<HHI", 0x0003, 0x0008, total) + pool + nodes


class TestAxml(unittest.TestCase):
    def test_sniff_and_view(self):
        data = _axml()
        codec = AxmlCodec()
        self.assertTrue(codec.sniff(data))
        self.assertEqual(select("res/layout/main.xml", data).name, "axml")
        payload = codec.decode(data)
        self.assertEqual(payload.kind, "binary")
        view = codec.view(payload)
        self.assertEqual(view["elements"], 2)
        self.assertIn('count="7"', view["xml"])
        self.assertIn("</Root>", view["xml"])
        self.assertEqual(codec.encode(payload), data)

    def test_broken_payload_degrades(self):
        payload = AxmlCodec().decode(b"\x03\x00\x08\x00" + b"\x00" * 8)
        view = AxmlCodec().view(payload)
        self.assertEqual(view["xml"], "")
        self.assertEqual(view["elements"], 0)


class TestWebp(unittest.TestCase):
    def test_sniff_and_view_lossless(self):
        payload = b"\x2f" + (9 | (4 << 14)).to_bytes(4, "little") + b"\x00" * 8
        body = b"WEBPVP8L" + struct.pack("<I", len(payload)) + payload
        data = b"RIFF" + struct.pack("<I", len(body)) + body
        codec = WebpCodec()
        self.assertTrue(codec.sniff(data))
        self.assertEqual(select("res/drawable/a.webp", data).name, "webp")
        view = codec.view(codec.decode(data))
        self.assertEqual((view["variant"], view["width"], view["height"]), ("lossless", 10, 5))

    def test_png_disguised_as_webp(self):
        png = b"\x89PNG\r\n\x1a\n" + b"\x00" * 32
        self.assertEqual(select("res/drawable/a.webp", png).name, "png")


class TestSourceTree(unittest.TestCase):
    def test_locate_across_roots(self):
        with tempfile.TemporaryDirectory() as tmp:
            first = os.path.join(tmp, "a")
            second = os.path.join(tmp, "b")
            os.makedirs(os.path.join(second, "blocks"))
            with open(os.path.join(second, "blocks", "stone.png"), "wb") as fh:
                fh.write(b"x")
            tree = SourceTree([first, second])
            self.assertEqual(tree.locate("blocks/stone.png"),
                             os.path.join(second, "blocks", "stone.png"))
            self.assertIsNone(tree.locate("blocks/air.png"))
            coverage = tree.coverage(["blocks/stone.png", "blocks/air.png"])
            self.assertEqual(coverage["missing"], 1)
            self.assertRaises(Exception, tree.resolve, "blocks/air.png")


class TestRoundtripSelection(unittest.TestCase):
    def test_category_rules(self):
        self.assertEqual(category_of("csvdef/a.csv", "plain-text", ".csv"), "数据表 csv")
        self.assertEqual(category_of("particles/1.emo", "named-block", ".emo"), "命名区块容器")
        self.assertEqual(category_of("worlds/1/x0z0.r", "opaque", ".r"), "体素与世界存档")
        self.assertEqual(category_of("ui/mobile/main.xml", "plain-text", ".xml"), "图标集与界面")

    def test_plan_spreads_across_categories(self):
        records = []
        for index in range(50):
            records.append({"path": "csvdef/%03d.csv" % index, "codec": "plain-text"})
        for index in range(50):
            records.append({"path": "particles/%03d.emo" % index, "codec": "named-block"})
        for index in range(5):
            records.append({"path": "voxel/%03d.vmo" % index, "codec": "opaque"})
        picked = plan_samples(records, 12, seed=1)
        self.assertEqual(len(picked), 12)
        categories = {category_of(r["path"], r["codec"], os.path.splitext(r["path"])[1]) for r in picked}
        self.assertEqual(categories, {"数据表 csv", "命名区块容器", "体素与世界存档"})

    def test_sample_roundtrip_and_verify(self):
        with tempfile.TemporaryDirectory() as tmp:
            source_root = os.path.join(tmp, "src")
            os.makedirs(os.path.join(source_root, "csvdef"))
            plain = "编号,名称\r\n1,草方块\r\n".encode("gbk")
            import zlib

            from forge.codecs.container import MAGIC_TEXT, xor_stream

            raw = MAGIC_TEXT + len(plain).to_bytes(4, "big") + xor_stream(zlib.compress(plain, 6))
            with open(os.path.join(source_root, "csvdef", "item.csv"), "wb") as fh:
                fh.write(raw)
            row = run_sample({"path": "csvdef/item.csv", "codec": "plain-text"},
                             SourceTree([source_root]), os.path.join(tmp, "work"))
            self.assertEqual(row["status"], "ok", row["detail"])
            self.assertEqual(row["codec"], "container-text-xor16-zlib")
            self.assertEqual(row["encoding"], "gbk")

    def test_verify_detects_missing_and_deep_mismatch(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = os.path.join(tmp, "project")
            os.makedirs(os.path.join(project, "csvdef"))
            payload = b"a,b\n1,2\n"
            with open(os.path.join(project, "csvdef", "t.csv"), "wb") as fh:
                fh.write(payload)
            record = {
                "path": "csvdef/t.csv",
                "codec": "plain-text",
                "fidelity": "lossless-edit",
                "source_size": len(payload),
                "source_sha256": "0" * 64,
                "payload_kind": "text",
                "meta": {"container": "plain", "encoding": "utf-8", "bom": False,
                         "eol": "lf", "plain_size": len(payload)},
            }
            index = os.path.join(tmp, "index.jsonl")
            write_index(index, [record])
            report = verify_tree(project, index)
            self.assertTrue(report["ok"])
            deep = verify_tree(project, index, deep=True)
            self.assertEqual(deep["deep"]["mismatch_total"], 1)
            self.assertFalse(deep["ok"])
            os.remove(os.path.join(project, "csvdef", "t.csv"))
            missing = verify_tree(project, index)
            self.assertEqual(missing["missing_total"], 1)

    def test_pack_text_policies(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = os.path.join(tmp, "project")
            os.makedirs(os.path.join(project, "csvdef"))
            with open(os.path.join(project, "csvdef", "t.csv"), "wb") as fh:
                fh.write("a,b\r\n1,2\r\n".encode("utf-8"))
            record = {
                "path": "csvdef/t.csv",
                "codec": "plain-text",
                "fidelity": "lossless-edit",
                "source_size": 12,
                "source_sha256": "0" * 64,
                "payload_kind": "text",
                "meta": {"container": "plain", "encoding": "utf-8", "bom": False,
                         "eol": "crlf", "plain_size": 12},
            }
            verbatim = pack_file(project, record, TARGET_PLAIN)
            self.assertEqual(verbatim, b"a,b\r\n1,2\r\n")
            normalized = pack_file(project, record, TARGET_PLAIN, text_policy="utf8-lf")
            self.assertEqual(normalized, b"a,b\n1,2\n")
            original = pack_file(project, record, TARGET_ORIGINAL)
            self.assertEqual(original, b"a,b\r\n1,2\r\n")

    def test_pack_tree_index_roundtrip(self):
        with tempfile.TemporaryDirectory() as tmp:
            project = os.path.join(tmp, "project")
            os.makedirs(os.path.join(project, "csvdef"))
            with open(os.path.join(project, "csvdef", "t.csv"), "wb") as fh:
                fh.write(b"x")
            index = os.path.join(tmp, "index.jsonl")
            write_index(index, [{
                "path": "csvdef/t.csv", "codec": "plain-text", "fidelity": "lossless-edit",
                "source_size": 1, "source_sha256": "0" * 64, "payload_kind": "text",
                "meta": {"container": "plain", "encoding": "utf-8", "bom": False,
                         "eol": "lf", "plain_size": 1},
            }])
            out = os.path.join(tmp, "build")
            results = pack_tree(project, index, out, TARGET_PLAIN)
            self.assertEqual(len(results), 1)
            self.assertTrue(os.path.isfile(os.path.join(out, "csvdef", "t.csv")))


if __name__ == "__main__":
    unittest.main()
