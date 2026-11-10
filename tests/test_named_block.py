# -*- coding: utf-8 -*-
"""命名区块容器的切块闭合性与字段视图测试。

合成的两三条用例保证契约本身可独立验证; 真实样本用例在客户端资源树或原版逆向
明文树上按扩展名配额取样, 断言块序列刚好铺满到文件末尾、视图可 JSON 序列化、
逐字节往返成立, 并统计闭式覆盖率。
"""

from __future__ import annotations

import json
import os
import unittest

from forge.codecs.named_block import (
    HEADER_SIZE,
    MAGIC_NAMED_BLOCK,
    CodecError,
    NamedBlockCodec,
    container_of,
    describe,
    scan_headers,
    split_blocks,
)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLIENT_ASSETS = os.path.abspath(os.path.join(ROOT, "..", "client", "src", "assets"))
ORIGIN_ROOT = os.path.abspath(os.path.join(
    ROOT, "..", "client", "temp", "original", "fake-mini-BUT-true-moments", "src"))
ORIGIN_BLOCKMAP = os.path.abspath(os.path.join(
    ROOT, "..", "client", "temp", "original", "fake-mini-BUT-true-moments",
    "src", "decoded", "blocks", "blockmap.csv"))

"""二进制族外壳, 客户端资源树里有一部分 .omod 还套着它。"""
MAGIC_BINARY = b"\xff\xd9\xff\xd8"

"""每类样本的取样配额, 覆盖 .emo .ent .omod .otex .oanim 五类。"""
SAMPLE_QUOTA = ((".emo", 8), (".ent", 6), (".omod", 6), (".otex", 2), (".oanim", 2))

"""真实样本数下限, 低于该值整组用例跳过。"""
MIN_SAMPLES = 20

"""块名到字段视图必需键的对照, 覆盖任务要求的五个块族及其余主要块族。"""
REQUIRED_FIELDS = {
    "ModelData": ("child_block_count",),
    "SkeletonData": ("bone_count",),
    "MeshData": ("mesh_name", "child_block_count"),
    "BoneData": ("bone_name", "parent_index", "matrix"),
    "BoneTrack": ("bone_name", "position_key_count", "channels"),
    "Material": ("material_name", "type_name", "param_count", "params"),
    "SubMeshData": ("index_count", "vertex_count", "declaration", "vertex_stride"),
    "SkinPatch": ("vertex_count", "index_count", "patch_count"),
    "EntityData": ("form",),
    "EntityMotionData": ("scale", "name"),
    "AnimationData": ("form",),
    "TextureData": ("width", "height"),
    "SurfaceData": ("width", "height", "bytes_per_pixel"),
}


def _named_block_head(path):
    """读前 8 字节判断是否命名区块族 (含二进制族外壳)。"""
    try:
        with open(path, "rb") as fh:
            head = fh.read(8)
    except OSError:
        return None
    if head[:4] == MAGIC_NAMED_BLOCK:
        return "plain"
    if head[:4] == MAGIC_BINARY:
        return "wrapped"
    return None


def collect_samples(root):
    """按扩展名配额取样, 返回按扩展名分组的路径列表。"""
    quota = dict(SAMPLE_QUOTA)
    found = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            ext = os.path.splitext(name)[1].lower()
            limit = quota.get(ext)
            if not limit:
                continue
            bucket = found.setdefault(ext, [])
            if len(bucket) >= limit:
                continue
            path = os.path.join(dirpath, name)
            if _named_block_head(path) is None:
                continue
            bucket.append(path)
    return found


def sample_paths():
    """优先用客户端资源树取样, 数量不足时退回原版逆向明文树。"""
    for root in (CLIENT_ASSETS, ORIGIN_ROOT):
        if not os.path.isdir(root):
            continue
        found = collect_samples(root)
        paths = [path for _ext, _limit in SAMPLE_QUOTA for path in found.get(_ext, [])]
        if len(paths) >= MIN_SAMPLES:
            return paths
    return []


SAMPLES = sample_paths()


def load_container(path):
    """读文件并剥掉二进制族外壳, 返回明文容器字节。"""
    with open(path, "rb") as fh:
        raw = fh.read()
    return container_of(raw)


def read_bytes(path):
    """整体读取一个文件。"""
    with open(path, "rb") as fh:
        return fh.read()


class TestSyntheticContainer(unittest.TestCase):
    """合成容器: 不依赖任何素材即可验证切块与视图契约。"""

    @staticmethod
    def build(blocks):
        head = MAGIC_NAMED_BLOCK + (100).to_bytes(4, "little") + (1).to_bytes(4, "little")
        body = b""
        for type_id, name, payload in blocks:
            body += type_id.to_bytes(2, "little") + bytes((len(name),)) + name.encode("ascii")
            body += payload
        return head + body

    def test_single_modeldata(self):
        data = self.build([(101, "ModelData", (3).to_bytes(4, "little"))])
        blocks = split_blocks(data)
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0].name, "ModelData")
        self.assertEqual(blocks[0].offset, HEADER_SIZE)
        self.assertEqual(blocks[0].end, len(data))
        self.assertEqual(blocks[0].mode, "closed")

    def test_two_blocks_tile_to_eof(self):
        bone_payload = (
            (4).to_bytes(2, "little") + b"Root" + (0).to_bytes(2, "little")
            + (-1).to_bytes(4, "little", signed=True) + b"\x00" * 64
        )
        data = self.build([
            (100, "SkeletonData", (1).to_bytes(4, "little")),
            (100, "BoneData", bone_payload),
        ])
        blocks = split_blocks(data)
        self.assertEqual([b.name for b in blocks], ["SkeletonData", "BoneData"])
        self.assertEqual(sum(b.length for b in blocks) + HEADER_SIZE, len(data))
        self.assertEqual(blocks[1].mode, "closed")
        fields = describe(data)["blocks"][1]["fields"]
        self.assertEqual(fields["bone_name"], "Root")
        self.assertEqual(fields["parent_index"], -1)

    def test_magic_mismatch_raises(self):
        with self.assertRaises(CodecError):
            split_blocks(b"nope" + bytes(64))

    def test_too_short_raises(self):
        with self.assertRaises(CodecError):
            split_blocks(MAGIC_NAMED_BLOCK + bytes(4))

    def test_roundtrip(self):
        data = self.build([(101, "ModelData", (0).to_bytes(4, "little"))])
        codec = NamedBlockCodec()
        payload = codec.decode(data)
        self.assertEqual(payload.meta["version"], 100)
        self.assertEqual(payload.meta["header_flag"], 1)
        self.assertEqual(codec.encode(payload), data)

    def test_scan_headers_finds_all(self):
        data = self.build([
            (101, "ModelData", (0).to_bytes(4, "little")),
            (100, "SkeletonData", (1).to_bytes(4, "little")),
        ])
        offsets = [offset for offset, _type_id, _name in scan_headers(data)]
        self.assertEqual(offsets, [12, 28])

    def test_view_is_json_serializable(self):
        data = self.build([(101, "ModelData", (0).to_bytes(4, "little"))])
        view = NamedBlockCodec().view(NamedBlockCodec().decode(data))
        text = json.dumps(view, ensure_ascii=False)
        self.assertEqual(json.loads(text)["block_count"], 1)


@unittest.skipUnless(len(SAMPLES) >= MIN_SAMPLES, "没有足够的命名区块真实样本, 跳过")
class TestRealSamples(unittest.TestCase):
    """在真实素材上验证块序列闭合、闭式覆盖率与字段视图。"""

    def test_split_tiles_to_eof(self):
        for path in SAMPLES:
            data = load_container(path)
            blocks = split_blocks(data)
            self.assertTrue(blocks, "没有切出块: %s" % path)
            cursor = HEADER_SIZE
            for block in blocks:
                self.assertEqual(block.offset, cursor, "块不连续: %s" % path)
                self.assertGreaterEqual(block.payload_length, 0, "载荷长为负: %s" % path)
                self.assertEqual(block.payload_offset, block.offset + block.header_length)
                self.assertEqual(block.length, block.header_length + block.payload_length)
                cursor = block.end
            self.assertEqual(cursor, len(data), "块序列没有铺到文件末尾: %s" % path)

    def test_block_header_matches_name(self):
        for path in SAMPLES:
            data = load_container(path)
            for block in split_blocks(data):
                if block.type_id < 0:
                    continue
                self.assertIn(block.type_id, (100, 101, 102, 104, 105, 106))
                raw = data[block.offset:block.payload_offset]
                self.assertEqual(raw[2], len(block.name))
                self.assertEqual(raw[3:], block.name.encode("ascii"))

    def test_view_json_serializable(self):
        for path in SAMPLES:
            data = load_container(path)
            view = NamedBlockCodec().view(NamedBlockCodec().decode(data))
            text = json.dumps(view, ensure_ascii=False)
            restored = json.loads(text)
            self.assertEqual(restored["container"], "named-block")
            self.assertEqual(restored["block_count"], len(restored["blocks"]))
            self.assertTrue(restored["coverage"]["tiles_to_eof"], "未铺满: %s" % path)
            for block in restored["blocks"]:
                self.assertIsInstance(block["fields"], dict)
                self.assertIn(block["mode"], ("closed", "predicted-mismatch", "no-closed-form", "unparsed"))

    def test_roundtrip_bytes(self):
        codec = NamedBlockCodec()
        for path in SAMPLES:
            data = load_container(path)
            self.assertEqual(codec.encode(codec.decode(data)), data, "往返失败: %s" % path)

    def test_closed_form_ratio(self):
        total = 0
        closed = 0
        for path in SAMPLES:
            for block in split_blocks(load_container(path)):
                total += 1
                if block.mode == "closed":
                    closed += 1
        self.assertGreater(total, 100, "样本块数过少: %d" % total)
        self.assertGreaterEqual(closed / total, 0.99,
                                "闭式覆盖率不足: %d/%d" % (closed, total))

    def test_required_field_views(self):
        seen = {}
        for path in SAMPLES:
            data = load_container(path)
            for entry in describe(data)["blocks"]:
                name = entry["name"]
                if name in REQUIRED_FIELDS and name not in seen:
                    seen[name] = entry["fields"]
            if len(seen) == len(REQUIRED_FIELDS):
                break
        missing = [name for name in REQUIRED_FIELDS if name not in seen]
        self.assertFalse(missing, "样本里没有出现这些块族: %s" % ", ".join(missing))
        for name, keys in REQUIRED_FIELDS.items():
            for key in keys:
                self.assertIn(key, seen[name], "%s 视图缺少字段 %s" % (name, key))

    def test_material_params_typed(self):
        for path in SAMPLES:
            data = load_container(path)
            for entry in describe(data)["blocks"]:
                if entry["name"] != "Material":
                    continue
                fields = entry["fields"]
                self.assertEqual(len(fields["params"]), fields["param_count"])
                for param in fields["params"]:
                    self.assertIn("type", param)
                    self.assertIn("key", param)
                    self.assertIn("value", param)
                return
        self.skipTest("样本里没有 Material 块")


@unittest.skipUnless(os.path.isfile(ORIGIN_BLOCKMAP), "没有原版逆向的块级清单, 跳过")
class TestAgainstOriginBlockmap(unittest.TestCase):
    """与独立实现产出的块级清单逐块比对边界 (只比前三块, 保证用例轻量)。"""

    ORIGIN_ASSETS = os.path.abspath(os.path.join(
        ROOT, "..", "client", "temp", "original", "fake-mini-BUT-true-moments", "src"))

    def test_boundaries(self):
        import csv

        wanted = 0
        checked = 0
        with open(ORIGIN_BLOCKMAP, encoding="utf-8") as fh:
            rows = list(csv.DictReader(fh))
        by_path = {}
        for row in rows:
            path = row["path"].replace("\\", "/")
            if not path.startswith("src/assets/"):
                continue
            by_path.setdefault(path[len("src/"):], []).append(
                (int(row["offset"]), int(row["size"]), row["name"]))
        for rel in sorted(by_path)[:40]:
            full = os.path.join(self.ORIGIN_ASSETS, rel)
            if not os.path.isfile(full):
                continue
            wanted += 1
            data = container_of(read_bytes(full))
            actual = [(b.offset, b.length, b.name) for b in split_blocks(data)]
            expect = sorted(by_path[rel])
            self.assertEqual(actual, expect, "块边界与原版清单不符: %s" % rel)
            checked += 1
        self.assertGreater(checked, 0, "没有可比对的文件")


if __name__ == "__main__":
    unittest.main()
