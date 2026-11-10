# -*- coding: utf-8 -*-
"""Ogre 命名区块容器 (魔数 89 67 45 23) 的块边界切分与字段级视图。

容器结构: 12 字节头 (4 字节魔数, u32le 版本, u32le 恒为 1 的字段), 其后是具名
区块序列。区块头为 u16le 类型标识, u8 名长, 名长字节的 ASCII 名; 区块头不带
长度字段, 块边界只能由该块名对应的字段序 (长度闭式) 算出。

本模块做三件事:
1. 把原版逆向解出的 20 个块名的长度闭式落成块 schema 表, 一个块族一个类;
2. 切块: 以块头判据枚举出的位置为边界来源 (同一文件内的区块首尾相接), 产出刚好
   铺满到文件末尾的块序列 [(name, offset, length, payload)], 并对每块用闭式独立
   预测载荷长, 预测命中记为 closed, 两者之比即闭式覆盖率;
3. 对可解析的块输出字段级 JSON 视图, 块内未解部分保留 hex 摘要。

逐字节往返不受影响: decode 原样保留容器字节, encode 由基类原样回写。
"""

from __future__ import annotations

import hashlib
import re
import struct
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

from .base import Codec, CodecError, FIDELITY_PASSTHROUGH, Payload
from .container import MAGIC_BINARY, xor_stream

MAGIC_NAMED_BLOCK = bytes.fromhex("89674523")

"""容器头长度: 4 字节魔数加版本号与常量为 1 的 u32。"""
HEADER_SIZE = 12

"""区块头允许的类型标识, 全族实测只出现这六个。"""
TYPE_IDS = (100, 101, 102, 104, 105, 106)

"""全族封闭的 21 个块名, 块头谓词的词表部分。"""
BLOCK_NAMES = (
    "AnimationData",
    "BillboardData",
    "BoneData",
    "BoneTrack",
    "DummyNodeData",
    "EntityData",
    "EntityMotionData",
    "Material",
    "MaterialParamTrack",
    "MeshData",
    "ModelData",
    "MotionAnimElementData",
    "MotionBindElementData",
    "ParamShapeData",
    "ParticleEmitterData",
    "RibbonEmitterData",
    "SkinPatch",
    "SkeletonData",
    "SubMeshData",
    "SurfaceData",
    "TextureData",
)

"""Material 条目值区宽度, 按条目类型号索引, 镜像 0x1AE6DD4 的九项常量。"""
MATERIAL_VALUE_SIZE = (4, 8, 12, 16, 0, 0, 16, 64, 4)

"""纹理路径型条目, 值区为 u16 串长加串而不是定宽。"""
MATERIAL_TYPE_STRING = 5

"""Material 条目之后的残留长度, 实测只有这三个取值。"""
MATERIAL_RESIDUALS = (4, 37, 74)

"""顶点步长表, 键为顶点声明号; 声明号 4 二义, 按描述符取 36 或 40。"""
VERTEX_STRIDE = {2: 24, 3: 32, 5: 40, 6: 48}
VERTEX_STRIDE_ALT = {4: (36, 40)}

"""节点包装头: 6 字节零加 3 个 1.0f, 其后是 u16 名长与 GBK 名。"""
WRAPPER_HEAD = b"\x00" * 6 + b"\x00\x00\x80\x3f" * 3

"""节点尾区头的三形态, 元素为 (尾区头长, 记录条数字段偏移)。"""
WRAPPER_TAILS = ((115, 23), (119, 23), (40, 0))

"""节点尾区单条记录的长度。"""
WRAPPER_RECORD = 72

"""BoneTrack 文件级尾区: 定长 35 字节加 8 字节一组挂点表项。"""
BONE_TRACK_TAIL_HEAD = 35
BONE_TRACK_TAIL_STEP = 8

"""单块内真实要遍历的条目数上限, 防止字段被误解时出现无界循环。

只用于确实要 for 循环的条目 (材质条目与实体路径表); 仅作乘数的计数不设限,
以免把合法的十万元素索引数组误判成异常。
"""
MAX_LOOP_ITEMS = 1 << 20

"""单块包装头候选的上限, 防止异常数据把候选表撑爆。"""
MAX_WRAPPER_CANDIDATES = 4096

"""字段视图里采样数组的条数上限, 避免工程树里的视图文件膨胀。"""
MAX_SAMPLE_ITEMS = 6

"""hex 摘要里保留的原始字节数。"""
HEX_HEAD_BYTES = 32


class Truncated(Exception):
    """载荷读到边界之外; 候选生成与字段解析都按它降级, 不向上抛。"""


class Cursor:
    """载荷只读游标, 起点与边界由调用方给定。"""

    __slots__ = ("data", "pos", "end")

    def __init__(self, data: bytes, pos: int, end: int):
        self.data = data
        self.pos = pos
        self.end = end

    def need(self, n: int) -> None:
        if n < 0 or self.pos + n > self.end:
            raise Truncated("载荷剩余 %d 字节, 需要 %d" % (self.end - self.pos, n))

    def u8(self) -> int:
        self.need(1)
        value = self.data[self.pos]
        self.pos += 1
        return value

    def u16(self) -> int:
        self.need(2)
        value = struct.unpack_from("<H", self.data, self.pos)[0]
        self.pos += 2
        return value

    def u32(self) -> int:
        self.need(4)
        value = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4
        return value

    def i32(self) -> int:
        self.need(4)
        value = struct.unpack_from("<i", self.data, self.pos)[0]
        self.pos += 4
        return value

    def f32(self):
        self.need(4)
        value = struct.unpack_from("<f", self.data, self.pos)[0]
        self.pos += 4
        return _finite(value)

    def raw(self, n: int) -> bytes:
        self.need(n)
        value = self.data[self.pos:self.pos + n]
        self.pos += n
        return value

    def skip(self, n: int) -> None:
        self.need(n)
        self.pos += n

    def string16(self) -> str:
        return _decode_name(self.raw(self.u16()))


def _at(data: bytes, off: int, end: int) -> Cursor:
    """在绝对偏移处开一个游标, 供候选生成本地读取字段。"""
    return Cursor(data, off, end)


def _decode_name(raw: bytes) -> str:
    """块内名字串的编码是 GBK 与 ASCII 混合, 先试 UTF-8 再试 GBK。"""
    for encoding in ("utf-8", "gbk"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", "replace")


def _finite(value: float):
    """非有限浮点转成字符串, 避免视图 JSON 里出现 NaN 与 Infinity 字面量。"""
    if value != value:
        return "nan"
    if value == float("inf"):
        return "inf"
    if value == float("-inf"):
        return "-inf"
    return value


def _decode_slot(raw: bytes) -> str:
    """定长名字槽按零截断后解码。"""
    return _decode_name(raw.rstrip(b"\x00"))


def _digest(data: bytes, start: int, end: int, prefix: str = "raw") -> Dict:
    """未解区域的摘要: 长度, 头部 hex 与 SHA256 前缀。"""
    chunk = data[start:end]
    return {
        prefix + "_size": len(chunk),
        prefix + "_hex_head": chunk[:HEX_HEAD_BYTES].hex(),
        prefix + "_sha256": hashlib.sha256(chunk).hexdigest()[:16],
    }


@dataclass(frozen=True)
class BlockSpan:
    """一个区块的边界与来源。

    length 为块头加载荷的总字节数, 与逆向产物 blockmap.csv 的 size 列同口径。
    mode 只有三种取值, 描述该块的闭式校验结果:
    closed 为闭式预测等于实测载荷长, predicted-mismatch 为有闭式但预测落空,
    no-closed-form 为没有可用闭式 (含块名未登记与块头之前的字节)。
    """

    name: str
    type_id: int
    offset: int
    header_length: int
    payload_length: int
    form: str
    mode: str

    @property
    def payload_offset(self) -> int:
        return self.offset + self.header_length

    @property
    def length(self) -> int:
        return self.header_length + self.payload_length

    @property
    def end(self) -> int:
        return self.offset + self.length

    def as_dict(self) -> Dict:
        return {
            "name": self.name,
            "type_id": self.type_id,
            "offset": self.offset,
            "header_length": self.header_length,
            "payload_offset": self.payload_offset,
            "payload_length": self.payload_length,
            "length": self.length,
            "mode": self.mode,
            "form": self.form,
        }


class BlockSchema:
    """一个块族的长度闭式与字段视图。

    candidates 只读载荷自身的字段, 给出候选 (载荷长, 形态标签) 列表; 块边界由
    驱动层按「落点必须等于下一个合法块头或文件末尾」判决, 因此闭式不读取任何
    后继块的信息, 落点闭合才算非平凡命中。
    """

    name = ""

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        return []

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        return {}


class FixedPayloadBlock(BlockSchema):
    """载荷恒为定长的块族。"""

    size = 0

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        return [(self.size, "载荷恒为 %d 字节" % self.size)]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        return self.fields(r)

    def fields(self, r: Cursor) -> Dict:
        return {}


class ModelDataBlock(FixedPayloadBlock):
    """ModelData 载荷为 4 字节的顶层子块数。"""

    name = "ModelData"
    size = 4

    def fields(self, r: Cursor) -> Dict:
        return {"child_block_count": r.u32()}


class SkeletonDataBlock(FixedPayloadBlock):
    """SkeletonData 载荷为 4 字节的骨骼数。"""

    name = "SkeletonData"
    size = 4

    def fields(self, r: Cursor) -> Dict:
        return {"bone_count": r.u32()}


class TextureDataBlock(FixedPayloadBlock):
    """TextureData 载荷恒为 32 字节的 8 个 u32。"""

    name = "TextureData"
    size = 32

    def fields(self, r: Cursor) -> Dict:
        values = [r.u32() for _ in range(8)]
        return {"width": values[1], "height": values[2], "u32_fields": values}


class MotionAnimElementDataBlock(FixedPayloadBlock):
    """MotionAnimElementData 载荷恒为 34 字节, 与 EntityMotionData 同构但无名字段。"""

    name = "MotionAnimElementData"
    size = 34

    def fields(self, r: Cursor) -> Dict:
        out = {
            "u32_0": r.u32(),
            "u32_4": r.u32(),
            "u32_8": r.u32(),
            "f32_12": r.f32(),
            "u32_16": r.u32(),
            "i32_20": r.i32(),
        }
        out.update(_digest(r.data, r.pos, r.end, "rest"))
        return out


class SurfaceDataBlock(BlockSchema):
    """SurfaceData 载荷为 32 字节头加宽乘高乘每像素字节数的像素区。"""

    name = "SurfaceData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        width = _at(data, p + 4, end).u32()
        height = _at(data, p + 8, end).u32()
        bpp = _at(data, p + 16, end).u32()
        total = width * height * bpp
        if total < 0 or p + 32 + total > end:
            return []
        return [(32 + total, "32+宽*高*每像素字节")]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        out = {
            "reserved0": r.u32(),
            "width": r.u32(),
            "height": r.u32(),
            "flag12": r.u32(),
            "bytes_per_pixel": r.u32(),
            "stride": r.u32(),
            "data_size": r.u32(),
            "data_size_dup": r.u32(),
        }
        out.update(_digest(r.data, r.pos, r.end, "pixels"))
        return out


class MeshDataBlock(BlockSchema):
    """MeshData 载荷为 u16 名长加网格名加子节点计数, 计数后可另有 33 字节扩展区。"""

    name = "MeshData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        name_len = _at(data, p, end).u16()
        base = 2 + name_len
        return [
            (base + 4, "2+名长+4"),
            (base + 37, "2+名长+37 带 33 字节扩展区"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        name_len = r.u16()
        out = {"mesh_name": _decode_name(r.raw(name_len))}
        base = 2 + name_len
        out["child_block_count"] = r.u32()
        if length > base + 4:
            out.update(_digest(data, p + base + 4, end, "extension"))
        return out


class BoneDataBlock(BlockSchema):
    """BoneData 载荷为两段名加父索引加 4x4 局部变换, 末骨骼另有 4 字节尾部。"""

    name = "BoneData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        l1 = _at(data, p, end).u16()
        l2 = _at(data, p + 2 + l1, end).u16()
        base = 4 + l1 + l2
        return [
            (base + 68, "4+两段名长+68"),
            (base + 72, "4+两段名长+72 带尾部 u32"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        l1 = r.u16()
        bone_name = _decode_name(r.raw(l1))
        l2 = r.u16()
        tag = _decode_name(r.raw(l2)) if l2 else ""
        out = {
            "bone_name": bone_name,
            "tag": tag,
            "parent_index": r.i32(),
            "matrix": [r.f32() for _ in range(16)],
        }
        if length >= 4 + l1 + l2 + 72:
            out["tail_u32"] = r.u32()
        return out


class EntityMotionDataBlock(BlockSchema):
    """EntityMotionData 载荷为缩放系数加名字与三个 u32。"""

    name = "EntityMotionData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        name_len = _at(data, p + 4, end).u16()
        return [(18 + name_len, "18+名长")]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        out = {"scale": r.f32()}
        name_len = r.u16()
        out["name"] = _decode_name(r.raw(name_len))
        out["u32_after_name"] = r.u32()
        out["u32_default_frames"] = r.u32()
        out["bind_emitter_pair_count"] = r.u32()
        return out


class AnimationDataBlock(BlockSchema):
    """AnimationData 载荷为 12 加 16 乘记录数, 另有 63 字节的定长变体。"""

    name = "AnimationData"

    """63 字节变体: u32 加 u32 加 16 字节名字槽加 35 字节键帧区加 u32。"""
    VARIANT_SIZE = 63

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        count = _at(data, p + 4, end).u32()
        return [
            (12 + 16 * count, "12+16*记录数"),
            (self.VARIANT_SIZE, "63 字节变体"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        if length == self.VARIANT_SIZE:
            out = {
                "form": "variant-63",
                "u32_0": r.u32(),
                "u32_4": r.u32(),
                "name_slot": _decode_slot(r.raw(16)),
            }
            """变体布局为 8 加 16 字节名字槽加 35 字节键帧区加 u32 尾。"""
            out.update(_digest(data, r.pos, min(r.pos + 35, end), "keyframe"))
            if r.pos + 35 + 4 <= end:
                out["tail_u32"] = _at(data, r.pos + 35, end).u32()
            return out
        head = r.u32()
        count = r.u32()
        records = []
        for index in range(min(count, MAX_SAMPLE_ITEMS)):
            records.append([r.u32() for _ in range(4)])
        tail = None
        if count * 16 + 12 == length:
            r.pos = p + 8 + 16 * count
            tail = r.u32()
        return {
            "form": "record-array",
            "u32_0": head,
            "record_count": count,
            "sample_records": records,
            "tail_u32": tail,
        }


class MotionBindElementDataBlock(BlockSchema):
    """MotionBindElementData 载荷定长 72 字节, 变体另有 43 字节路径扩展区。"""

    name = "MotionBindElementData"
    BASE_SIZE = 72
    VARIANT_SIZE = 302

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        return [
            (self.BASE_SIZE, "载荷恒为 72 字节"),
            (self.VARIANT_SIZE, "72+43+72+43+72 带扩展路径"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        out = {
            "u32_0": r.u32(),
            "f32_4": r.f32(),
            "f32_8": r.f32(),
            "f32_12": r.f32(),
            "u32_16": r.u32(),
            "i32_20": r.i32(),
            "u32_24": r.u32(),
            "u32_28": r.u32(),
            "u32_32": r.u32(),
        }
        if length == self.VARIANT_SIZE:
            out["form"] = "variant-302"
            out.update(_digest(data, p + 36, p + self.BASE_SIZE, "record_tail"))
            out["extension_paths"] = _motion_bind_paths(data, p + self.BASE_SIZE, end)
        else:
            out["form"] = "base-72"
            out.update(_digest(data, p + 36, end, "record_tail"))
        return out


def _motion_bind_paths(data: bytes, start: int, end: int) -> List[str]:
    """302 字节变体的扩展区: 6 字节零, 3 个 1.0f, u16 路长与路径, 重复。"""
    paths = []
    pos = start
    while pos + 26 <= end and len(paths) < MAX_SAMPLE_ITEMS:
        try:
            cursor = Cursor(data, pos, end)
            cursor.skip(18)
            path_len = cursor.u16()
            paths.append(_decode_name(cursor.raw(path_len)))
            pos = cursor.pos
        except Truncated:
            break
    return paths


class EntityDataBlock(BlockSchema):
    """EntityData 有三形态: 6 字节计数, 多路径表, 单路径加 u32。"""

    name = "EntityData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        first = _at(data, p, end).u16()
        out: List[Tuple[int, str]] = []
        if first != 0:
            out.append((2 + first + 4, "形态 c: u16 路长加路径加 u32"))
            return out
        try:
            code = _at(data, p + 2, end).u32()
        except Truncated:
            return out
        if code == 1:
            out.append((6, "形态 a: u16 0 加 u32 1"))
        # 形态 b 的路径表可能超出候选窗口, 单独兜住, 不影响形态 a。
        try:
            count = _at(data, p + 6, end).u32()
            if count > MAX_LOOP_ITEMS:
                return out
            pos = p + 10
            for _ in range(count):
                pos += 2 + _at(data, pos, end).u16()
            out.append((pos - p, "形态 b: 10 加 %d 组路径" % count))
        except Truncated:
            pass
        return out

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        if length == 6:
            return {"form": "a", "u16_0": r.u16(), "u32_flag": r.u32()}
        first = _at(data, p, end).u16()
        if first != 0:
            path_len = r.u16()
            return {
                "form": "c",
                "path": _decode_name(r.raw(path_len)),
                "u32_tail": r.u32(),
            }
        count = _at(data, p + 6, end).u32()
        cursor = Cursor(data, p + 10, end)
        paths = []
        for _ in range(min(count, MAX_SAMPLE_ITEMS)):
            path_len = cursor.u16()
            paths.append(_decode_name(cursor.raw(path_len)))
        return {
            "form": "b",
            "u16_0": first,
            "u32_2": _at(data, p + 2, end).u32(),
            "path_count": count,
            "sample_paths": paths,
        }


class BoneTrackBlock(BlockSchema):
    """BoneTrack 载荷为骨骼名加位置关键帧加三段同构通道。"""

    name = "BoneTrack"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        name_len = _at(data, p, end).u16()
        key_count = _at(data, p + 10 + name_len, end).u32()
        pos = p + 14 + name_len + 16 * key_count
        for stage in (0, 1, 2):
            keys = _at(data, pos + 4, end).u32()
            head = 8 * keys + (8 if stage == 2 else 16)
            if stage == 0:
                records = _at(data, pos + head - 4, end).u32()
                pos += head + 20 * records
            elif stage == 1:
                records = _at(data, pos + head - 4, end).u32()
                pos += head + 16 * records
            else:
                pos += head
        base = pos - p
        out: List[Tuple[int, str]] = [(base, "14+名长+16C+24N+40+20X1+16X2")]
        # 没有骨骼与网格的 .oanim 里, 文件级尾区退化为 4 字节 u32。
        if p + base + 4 <= end:
            out.append((base + 4, "…+4 字节退化尾区"))
        tail = p + base
        # 末块另带文件级尾区 35 加 8K, 判定条件是尾区内偏移 30 处的 u8 恒为 1。
        if tail + BONE_TRACK_TAIL_HEAD <= end and data[tail + 30] == 1:
            anchors = struct.unpack_from("<I", data, tail + 31)[0]
            out.append((
                base + BONE_TRACK_TAIL_HEAD + BONE_TRACK_TAIL_STEP * anchors,
                "…+文件级尾区 35+8K",
            ))
        return out

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        name_len = r.u16()
        out = {
            "bone_name": _decode_name(r.raw(name_len)),
            "bone_index": r.u32(),
            "constant_one": r.u32(),
        }
        key_count = r.u32()
        out["position_key_count"] = key_count
        if key_count:
            first = _at(data, r.pos, end)
            last = _at(data, r.pos + 16 * (key_count - 1), end)
            out["position_first_time"] = first.u32()
            out["position_last_time"] = last.u32()
        r.skip(16 * key_count)
        channels = []
        for stage in (0, 1, 2):
            keys = _at(data, r.pos + 4, end).u32()
            head = 8 * keys + (8 if stage == 2 else 16)
            start = r.pos
            records = 0
            record_bytes = 20 if stage == 0 else (16 if stage == 1 else 0)
            if record_bytes:
                records = _at(data, start + head - 4, end).u32()
            r.skip(head + records * record_bytes)
            channels.append({
                "channel": stage + 1,
                "key_count": keys,
                "record_count": records,
                "record_bytes": record_bytes,
                "head_bytes": head,
            })
        out["channels"] = channels
        rest = end - r.pos
        if rest > 0:
            out.update(_digest(data, r.pos, end, "tail_region"))
            file_tail = _bone_track_file_tail(data, r.pos, end)
            if file_tail is not None:
                out["file_tail"] = file_tail
        return out


def _bone_track_file_tail(data: bytes, start: int, end: int) -> Optional[Dict]:
    """BoneTrack 末块之后的文件级尾区, 形态为 35 加 8K。"""
    if end - start < BONE_TRACK_TAIL_HEAD or data[start + 30] != 1:
        return None
    anchors = struct.unpack_from("<I", data, start + 31)[0]
    if start + BONE_TRACK_TAIL_HEAD + BONE_TRACK_TAIL_STEP * anchors != end:
        return None
    box = [_finite(v) for v in struct.unpack_from("<6f", data, start + 6)]
    table = [
        list(struct.unpack_from("<II", data, start + BONE_TRACK_TAIL_HEAD + 8 * i))
        for i in range(min(anchors, MAX_SAMPLE_ITEMS))
    ]
    return {
        "size": end - start,
        "material_param_track_count": struct.unpack_from("<I", data, start)[0],
        "bbox_min": list(box[:3]),
        "bbox_max": list(box[3:]),
        "flag": data[start + 30],
        "anchor_count": anchors,
        "sample_anchors": table,
    }


class SubMeshDataBlock(BlockSchema):
    """SubMeshData 载荷为索引数组加顶点声明加顶点缓冲加 28 字节定长尾区。"""

    name = "SubMeshData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        index_count = _at(data, p + 8, end).u32()
        vertex_count = _at(data, p + 16 + 2 * index_count, end).u32()
        decl = _at(data, p + 20 + 2 * index_count, end).u32()
        base = 16 + 2 * index_count + 8 + 4 * decl + 28
        strides = []
        if decl in VERTEX_STRIDE:
            strides.append(VERTEX_STRIDE[decl])
        else:
            strides.extend(VERTEX_STRIDE_ALT.get(decl, ()))
        out = []
        for stride in strides:
            out.append((base + vertex_count * stride,
                        "16+2I+8+4D+V*%d+28" % stride))
        return out

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        out = {
            "constant_four": r.u32(),
            "submesh_index": r.u32(),
            "index_count": r.u32(),
            "u16_12": r.u16(),
            "u16_14": r.u16(),
        }
        index_count = out["index_count"]
        sample = [_at(data, p + 16 + 2 * i, end).u16()
                  for i in range(min(index_count, MAX_SAMPLE_ITEMS))]
        out["sample_indices"] = sample
        out["vertex_count"] = _at(data, p + 16 + 2 * index_count, end).u32()
        out["declaration"] = _at(data, p + 20 + 2 * index_count, end).u32()
        decl = out["declaration"]
        desc_start = p + 24 + 2 * index_count
        out["declaration_hex"] = data[desc_start:desc_start + 4 * decl].hex()
        buffer_start = desc_start + 4 * decl
        stride = self.stride_of(decl, length - (buffer_start - p) - 28,
                                out["vertex_count"])
        out["vertex_stride"] = stride
        out.update(_digest(data, buffer_start, buffer_start + out["vertex_count"] * stride,
                           "vertex_buffer"))
        out.update(_digest(data, end - 28, end, "fixed_tail"))
        return out

    @staticmethod
    def stride_of(decl: int, remaining: int, vertex_count: int) -> Optional[int]:
        """按声明号与剩余字节反解步长, 用于视图里标注实际取值。"""
        candidates = [VERTEX_STRIDE[decl]] if decl in VERTEX_STRIDE else list(
            VERTEX_STRIDE_ALT.get(decl, ()))
        for stride in candidates:
            if remaining == vertex_count * stride:
                return stride
        return candidates[0] if candidates else None


class SkinPatchBlock(BlockSchema):
    """SkinPatch 载荷为补丁索引加每补丁一个 4x4 矩阵, 有全形与缺尾形两形态。"""

    name = "SkinPatch"
    TAIL_BYTES = 33

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        patch_count = _at(data, p + 16, end).u32()
        return [
            (66 * patch_count + 57, "66*pc+57 全形"),
            (66 * patch_count + 24, "66*pc+24 缺尾形"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        out = {
            "u32_0": r.u32(),
            "vertex_count": r.u32(),
            "u32_8": r.u32(),
            "index_count": r.u32(),
        }
        patch_count = r.u32()
        out["patch_count"] = patch_count
        out["sample_patch_indices"] = [
            _at(data, r.pos + 2 * i, end).u16() for i in range(min(patch_count, MAX_SAMPLE_ITEMS))
        ]
        matrix_start = p + 24 + 2 * patch_count
        out["matrix_equal_patch_count"] = _at(data, matrix_start - 4, end).u32()
        if patch_count:
            out["patch_matrix_0"] = [
                _at(data, matrix_start + 4 * i, end).f32() for i in range(16)
            ]
        out.update(_digest(data, matrix_start + 64 * patch_count, end, "patch_tail"))
        return out


class MaterialBlock(BlockSchema):
    """Material 载荷为两段名加键数加条目区加残留区。"""

    name = "Material"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        l1 = _at(data, p, end).u16()
        l2 = _at(data, p + 2 + l1, end).u16()
        key_count = _at(data, p + 4 + l1 + l2, end).u32()
        if key_count > MAX_LOOP_ITEMS:
            return []
        pos = p + 8 + l1 + l2
        for _ in range(key_count):
            entry_type = _at(data, pos, end).u32()
            key_len = _at(data, pos + 4, end).u16()
            pos += 6 + key_len
            if entry_type == MATERIAL_TYPE_STRING:
                pos += 2 + _at(data, pos, end).u16()
            elif entry_type < len(MATERIAL_VALUE_SIZE):
                pos += MATERIAL_VALUE_SIZE[entry_type]
            else:
                return []
        base = pos - p
        return [(base + residual, "8+l1+l2+%d 条条目+残长 %d" % (key_count, residual))
                for residual in MATERIAL_RESIDUALS]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        l1 = r.u16()
        out = {"material_name": _decode_name(r.raw(l1)) if l1 else ""}
        l2 = r.u16()
        out["type_name"] = _decode_name(r.raw(l2)) if l2 else ""
        key_count = r.u32()
        out["param_count"] = key_count
        params = []
        for index in range(key_count):
            entry_type = r.u32()
            key_len = r.u16()
            key_name = _decode_name(r.raw(key_len))
            params.append({
                "index": index,
                "type": entry_type,
                "key": key_name,
                "value": _material_value(r, entry_type),
            })
        out["params"] = params
        out.update(_digest(data, r.pos, end, "residual"))
        return out


def _material_value(r: Cursor, entry_type: int):
    """按类型号读 Material 条目的值区。"""
    if entry_type == MATERIAL_TYPE_STRING:
        return r.string16()
    if entry_type >= len(MATERIAL_VALUE_SIZE):
        raise Truncated("未知的材质值类型 %d" % entry_type)
    width = MATERIAL_VALUE_SIZE[entry_type]
    if width == 0:
        return None
    raw = r.raw(width)
    if width == 4:
        return {"u32": struct.unpack("<I", raw)[0], "f32": _finite(struct.unpack("<f", raw)[0])}
    if width in (8, 12, 16):
        return {
            "u32": list(struct.unpack("<%dI" % (width // 4), raw)),
            "f32": [_finite(v) for v in struct.unpack("<%df" % (width // 4), raw)],
        }
    return {"hex": raw.hex()}


class DummyNodeDataBlock(BlockSchema):
    """DummyNodeData 载荷为两个自有字段加节点包装头加尾区头加 72 乘计数字节记录。"""

    name = "DummyNodeData"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        name_len = _at(data, p + 23, end).u16()
        tail_start = p + 25 + name_len
        out = []
        for tail_head, count_offset in WRAPPER_TAILS:
            count = _at(data, tail_start + count_offset, end).u32()
            out.append((
                25 + name_len + tail_head + WRAPPER_RECORD * count,
                "140+名长+72*%d (尾区头 %d)" % (count, tail_head),
            ))
        return out

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        out = {"node_serial": r.u32(), "node_flag": r.u8()}
        out.update(_digest(data, r.pos, p + 11, "wrap_zero"))
        r.skip(6)
        out["scale"] = [r.f32() for _ in range(3)]
        name_len = r.u16()
        out["node_name"] = _decode_name(r.raw(name_len))
        tail_start = r.pos
        matched = None
        for tail_head, count_offset in WRAPPER_TAILS:
            if tail_start + tail_head <= end:
                count = _at(data, tail_start + count_offset, end).u32()
                if tail_start + tail_head + WRAPPER_RECORD * count == end:
                    matched = (tail_head, count)
                    break
        if matched is None:
            out.update(_digest(data, tail_start, end, "wrapper"))
            hint = _wrapper_head_hint(data, p, end)
            if hint is not None:
                out["wrapper_hint"] = hint
            return out
        tail_head, count = matched
        out["wrapper"] = {
            "tail_head_bytes": tail_head,
            "record_count": count,
            "record_bytes": WRAPPER_RECORD * count,
        }
        out.update(_digest(data, tail_start + tail_head, end, "records"))
        return out


def _wrapper_spans(data: bytes, payload_start: int,
                   end: int) -> List[Tuple[int, int, int, int, int]]:
    """在载荷内枚举「包装头加尾区头」的自洽落点。

    返回 (载荷长, 包装头相对偏移, 尾区头长, 名长, 记录条数) 五元组列表; 该模型
    与逆向文档的整块校验脚本同源: 包装头字节形态在载荷内可能多次出现, 只有
    「尾区头加 72 乘条数恰好收在块末」的那一处自洽。
    """
    out: List[Tuple[int, int, int, int, int]] = []
    found = data.find(WRAPPER_HEAD, payload_start, end)
    while found >= 0:
        if found + 20 <= end:
            name_len = struct.unpack_from("<H", data, found + 18)[0]
            tail = found + 20 + name_len
            for tail_head, count_offset in WRAPPER_TAILS:
                if tail + count_offset + 4 > end:
                    continue
                count = struct.unpack_from("<I", data, tail + count_offset)[0]
                length = tail + tail_head + WRAPPER_RECORD * count - payload_start
                if 0 < length <= end - payload_start:
                    out.append((length, found - payload_start, tail_head, name_len, count))
        if len(out) >= MAX_WRAPPER_CANDIDATES:
            break
        found = data.find(WRAPPER_HEAD, found + 1, end)
    """按载荷长升序排列: 包装头越靠后越可能是后继块的字节形态, 短的优先。"""
    out.sort()
    return out


def _wrapper_head_hint(data: bytes, payload_start: int, end: int) -> Optional[Dict]:
    """未闭合时给出载荷内第一个包装头的相对偏移与名字, 便于人工核对残差块。"""
    found = data.find(WRAPPER_HEAD, payload_start, end)
    if found < 0 or found + 20 > end:
        return None
    name_len = struct.unpack_from("<H", data, found + 18)[0]
    if found + 20 + name_len > end:
        return {"head_offset": found - payload_start}
    return {
        "head_offset": found - payload_start,
        "name": _decode_name(data[found + 20:found + 20 + name_len]),
    }


class WrapperTailBlock(BlockSchema):
    """载荷尾部带节点包装尾区的块族。

    闭式形式为「类自有字段与路径列表的终点」加包装头 (20 加名长) 加尾区头
    (115 或 119 或 40) 加 72 乘记录条数。由于自有字段区的终点要先逐数组解析
    关键帧, 这里改用包装头定位加尾区自洽, 与逆向文档的整块校验脚本同一模型。
    """

    """自有字段区的定长前缀长度; 版本差异由 type_id 或版本号决定。"""
    head_bytes = 0

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        return [
            (length, "包装头@%d 尾区头 %d 记录 %d" % (head, tail_head, count))
            for length, head, tail_head, _name_len, count in _wrapper_spans(r.data, r.pos, r.end)
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        head = self.fixed_head(type_id)
        out = {"fixed_head_bytes": head}
        try:
            out["head_fields"] = self.head_fields(data, p, min(p + head, end), type_id)
        except (Truncated, struct.error) as exc:
            out["head_fields"] = {"error": str(exc)}
        match = None
        for candidate in _wrapper_spans(data, p, end):
            if candidate[0] == length:
                match = candidate
                break
        head_end = p + head
        if match is None:
            out.update(_digest(data, head_end, end, "body"))
            hint = _wrapper_head_hint(data, p, end)
            if hint is not None:
                out["wrapper_hint"] = hint
            return out
        _length, head_offset, tail_head, name_len, count = match
        wrapper_start = p + head_offset
        out["wrapper"] = {
            "head_offset": head_offset,
            "name": _decode_name(data[wrapper_start + 20:wrapper_start + 20 + name_len]),
            "tail_head_bytes": tail_head,
            "record_count": count,
            "record_bytes": WRAPPER_RECORD * count,
        }
        out.update(_digest(data, head_end, wrapper_start, "body"))
        out.update(_digest(data, wrapper_start + 20 + name_len + tail_head, end, "records"))
        return out

    def fixed_head(self, type_id: int) -> int:
        return self.head_bytes

    def head_fields(self, data: bytes, p: int, head_end: int, type_id: int) -> Dict:
        return _digest(data, p, head_end, "head")


class ParticleEmitterDataBlock(WrapperTailBlock):
    """ParticleEmitterData: 定长前段加 12 个加 9 个关键帧数组加原始数组加路径列表加包装尾区。"""

    name = "ParticleEmitterData"

    """类型 106 的定长前段比 105 多 4 字节, 这是两个变体的唯一差别。"""
    head_bytes = 221
    head_bytes_105 = 217

    def fixed_head(self, type_id: int) -> int:
        return self.head_bytes if type_id == 106 else self.head_bytes_105

    def head_fields(self, data: bytes, p: int, head_end: int, type_id: int) -> Dict:
        out = {
            "fixed_param_block_1": _digest(data, p, p + 132, "a")["a_hex_head"],
            "fixed_param_block_2": _digest(data, p + 132, p + 212, "b")["b_hex_head"],
        }
        if p + 213 <= head_end:
            out["flag_212"] = data[p + 212]
            out["u32_213"] = struct.unpack_from("<I", data, p + 213)[0]
        if type_id == 106 and p + 221 <= head_end:
            out["u32_217"] = struct.unpack_from("<I", data, p + 217)[0]
        return out


class BillboardDataBlock(WrapperTailBlock):
    """BillboardData: 定长头加颜色曲线加 14 个浮点曲线加两条纹理串加包装尾区。"""

    name = "BillboardData"
    head_bytes = 188
    head_bytes_104 = 186

    def fixed_head(self, type_id: int) -> int:
        return self.head_bytes_104 if type_id == 104 else self.head_bytes

    def head_fields(self, data: bytes, p: int, head_end: int, type_id: int) -> Dict:
        out = {"param_area_120": _digest(data, p, p + 120, "p1")["p1_hex_head"]}
        if p + 184 <= head_end:
            out["param_area_64"] = _digest(data, p + 120, p + 184, "p2")["p2_hex_head"]
        if type_id == 104 and p + 186 <= head_end:
            out["version_branch"] = [data[p + 184], data[p + 185]]
        elif p + 188 <= head_end:
            out["version_branch"] = struct.unpack_from("<I", data, p + 184)[0]
        return out


class RibbonEmitterDataBlock(WrapperTailBlock):
    """RibbonEmitterData: 两段定长结构加原始数组加可变曲线序列加两条纹理串加包装尾区。"""

    name = "RibbonEmitterData"
    head_bytes = 44

    def head_fields(self, data: bytes, p: int, head_end: int, type_id: int) -> Dict:
        out = {"constant_8620589": struct.unpack_from("<I", data, p)[0]}
        out.update(_digest(data, p + 4, head_end, "head_rest"))
        return out


class ParamShapeDataBlock(WrapperTailBlock):
    """ParamShapeData: 54 字节头加两条纹理串加 23 个曲线段加包装尾区。"""

    name = "ParamShapeData"
    head_bytes = 54

    def head_fields(self, data: bytes, p: int, head_end: int, type_id: int) -> Dict:
        out = {"u8_0": data[p]}
        if p + 33 <= head_end:
            out["u32_1_to_32"] = list(struct.unpack_from("<8I", data, p + 1))
            out["u8_33"] = data[p + 33]
        if p + 54 <= head_end:
            out["u32_34_to_53"] = list(struct.unpack_from("<5I", data, p + 34))
        return out


class MaterialParamTrackBlock(BlockSchema):
    """MaterialParamTrack 的长度闭式未解, 这里只给出候选式。

    载荷为三条 u16 前缀串加 u32 类型号加一个关键帧曲线子对象; 子对象之后第 3 块
    另有 63 字节附加区, 因此候选式只对 2/3 块成立, 落点不闭合时由驱动层回退扫描。
    """

    name = "MaterialParamTrack"

    def candidates(self, r: Cursor, type_id: int) -> List[Tuple[int, str]]:
        data, p, end = r.data, r.pos, r.end
        pos = p
        for _ in range(3):
            string_len = _at(data, pos, end).u16()
            pos += 2 + string_len
        pos += 4
        first = _at(data, pos, end).u32()
        second = _at(data, pos + 4, end).u32()
        third = _at(data, pos + 8, end).u32()
        if max(first, second, third) > MAX_LOOP_ITEMS:
            return []
        after_keys = pos + 12 + 16 * second
        third_keys = _at(data, after_keys, end).u32()
        # 三种候选: 标准子对象式, 文档候选式, 以及第 3 块多出的 63 字节附加区。
        standard = after_keys + 4 + 16 * third + 8 * third_keys - p
        candidate = pos + 12 + 16 * second + 16 * third + 8 * third_keys - p
        return [
            (standard, "16+16*c1+16*c2+8*r 子对象"),
            (candidate, "12+16*c1+16*c2+8*r 候选式"),
            (standard + 63, "子对象式加 63 字节附加区"),
            (candidate + 63, "候选式加 63 字节附加区"),
        ]

    def parse(self, r: Cursor, length: int, type_id: int) -> Dict:
        data, p, end = r.data, r.pos, r.end
        names = [r.string16() for _ in range(3)]
        out = {
            "target_name": names[0],
            "material_name": names[1],
            "param_name": names[2],
            "param_type": r.u32(),
        }
        out.update(_digest(data, r.pos, end, "curve_object"))
        return out


"""块名到块族的表: 未知块名即没有闭式, 由驱动层回退扫描并保留 hex 摘要。"""
SCHEMAS: Dict[str, BlockSchema] = {
    "AnimationData": AnimationDataBlock(),
    "BillboardData": BillboardDataBlock(),
    "BoneData": BoneDataBlock(),
    "BoneTrack": BoneTrackBlock(),
    "DummyNodeData": DummyNodeDataBlock(),
    "EntityData": EntityDataBlock(),
    "EntityMotionData": EntityMotionDataBlock(),
    "Material": MaterialBlock(),
    "MaterialParamTrack": MaterialParamTrackBlock(),
    "MeshData": MeshDataBlock(),
    "ModelData": ModelDataBlock(),
    "MotionAnimElementData": MotionAnimElementDataBlock(),
    "MotionBindElementData": MotionBindElementDataBlock(),
    "ParamShapeData": ParamShapeDataBlock(),
    "ParticleEmitterData": ParticleEmitterDataBlock(),
    "RibbonEmitterData": RibbonEmitterDataBlock(),
    "SkinPatch": SkinPatchBlock(),
    "SkeletonData": SkeletonDataBlock(),
    "SubMeshData": SubMeshDataBlock(),
    "SurfaceData": SurfaceDataBlock(),
    "TextureData": TextureDataBlock(),
}

"""块头正则: 类型标识两字节加名长字节加精确块名, 用零宽前瞻取全部落点。"""
HEADER_RE = re.compile(
    rb"(?=[\x64\x65\x66\x68\x69\x6a]\x00(?:"
    + b"|".join(re.escape(bytes((len(n),)) + n.encode("ascii")) for n in BLOCK_NAMES)
    + rb"))"
)


def container_of(data: bytes) -> bytes:
    """接受明文容器或二进制族加密外壳, 返回明文容器字节。

    客户端资源树里 535 个 .omod 还套着 FF D9 FF D8 外壳, 明文容器被异或前缀
    遮住; 该函数让调用方不必先分辨两种形态。
    """
    if data[:4] == MAGIC_NAMED_BLOCK:
        return data
    if data[:4] == MAGIC_BINARY:
        prefix = int.from_bytes(data[4:8], "big")
        return xor_stream(data[8:8 + prefix]) + data[8 + prefix:]
    return data


def scan_headers(data: bytes) -> List[Tuple[int, int, str]]:
    """枚举全部合法块头位置, 判据与逆向文档的块头谓词一致。"""
    out = []
    for match in HEADER_RE.finditer(data, HEADER_SIZE):
        offset = match.start()
        name_len = data[offset + 2]
        out.append((
            offset,
            struct.unpack_from("<H", data, offset)[0],
            data[offset + 3:offset + 3 + name_len].decode("ascii"),
        ))
    return out


def _payload_bounds(headers: List[Tuple[int, int, str]], index: int, end: int) -> int:
    """块末偏移等于下一个合法块头的偏移, 末块取文件末尾。"""
    if index + 1 < len(headers):
        return headers[index + 1][0]
    return end


def _classify(data: bytes, offset: int, type_id: int, name: str,
              payload_start: int, payload_length: int, block_end: int) -> Tuple[str, str]:
    """用块名闭式独立预测载荷长, 并与实际边界比对, 返回 (形态说明, 来源标签)。

    预测只读本块载荷内的字段, 因此「预测等于实际」是一次非平凡检验; 来源标签为
    closed (预测命中), predicted-mismatch (预测落空) 或 no-closed-form (无闭式)。
    """
    schema = SCHEMAS.get(name)
    if schema is None:
        return "块名未登记闭式", "no-closed-form"
    try:
        candidates = schema.candidates(Cursor(data, payload_start, block_end), type_id)
    except (Truncated, struct.error):
        candidates = ()
    if not candidates:
        return "闭式未给出候选", "no-closed-form"
    for length, form in candidates:
        if length == payload_length:
            return form, "closed"
    predicted = ", ".join(str(length) for length, _form in candidates)
    return "闭式预测 %s, 实测载荷 %d" % (predicted, payload_length), "predicted-mismatch"


def split_blocks(data: bytes) -> List[BlockSpan]:
    """把容器字节切成块序列, 块序列刚好铺满到文件末尾。

    切块以块头枚举为边界来源: 区块头判据 (类型标识, 名长, 封闭块名) 在全族上只有
    极低随机命中率, 且同一文件内的区块首尾相接, 因此「下一个合法块头」就是本块的
    块末, 末块延伸到文件末尾。每个块再单独用块名闭式预测载荷长, 预测与实际相等时
    记为 closed, 否则记 predicted-mismatch, 两者之比即闭式覆盖率。
    """
    if len(data) < HEADER_SIZE:
        raise CodecError("命名区块容器过短: %d 字节" % len(data))
    if data[:4] != MAGIC_NAMED_BLOCK:
        raise CodecError("命名区块容器魔数不符: %s" % data[:4].hex())
    end = len(data)
    headers = scan_headers(data)
    blocks: List[BlockSpan] = []
    if not headers:
        if end > HEADER_SIZE:
            blocks.append(BlockSpan(
                name="?unknown",
                type_id=-1,
                offset=HEADER_SIZE,
                header_length=0,
                payload_length=end - HEADER_SIZE,
                form="没有合法块头, 整段保留",
                mode="no-closed-form",
            ))
        return blocks
    if headers[0][0] > HEADER_SIZE:
        blocks.append(BlockSpan(
            name="?unknown",
            type_id=-1,
            offset=HEADER_SIZE,
            header_length=0,
            payload_length=headers[0][0] - HEADER_SIZE,
            form="首个块头之前的字节, 整段保留",
            mode="no-closed-form",
        ))
    for position, (offset, type_id, name) in enumerate(headers):
        header_length = 3 + len(name)
        payload_start = offset + header_length
        block_end = _payload_bounds(headers, position, end)
        payload_length = max(0, block_end - payload_start)
        form, mode = _classify(data, offset, type_id, name, payload_start,
                              payload_length, block_end)
        blocks.append(BlockSpan(
            name=name,
            type_id=type_id,
            offset=offset,
            header_length=header_length,
            payload_length=payload_length,
            form=form,
            mode=mode,
        ))
    return blocks


def block_fields(data: bytes, span: BlockSpan) -> Dict:
    """单个块的字段视图; 解析失败时退回 hex 摘要, 绝不抛给调用方。"""
    schema = SCHEMAS.get(span.name)
    if schema is None:
        out: Dict = {"note": "块名未登记闭式"}
        out.update(_digest(data, span.payload_offset, span.end))
        return out
    try:
        return schema.parse(
            Cursor(data, span.payload_offset, span.end),
            span.payload_length,
            span.type_id,
        )
    except (Truncated, struct.error) as exc:
        out = {"parse_error": str(exc)}
        out.update(_digest(data, span.payload_offset, span.end))
        return out


def describe(data: bytes) -> Dict:
    """容器字节 -> 块序列加字段视图, 返回值可直接 json 序列化。"""
    if len(data) < HEADER_SIZE:
        raise CodecError("命名区块容器过短: %d 字节" % len(data))
    if data[:4] != MAGIC_NAMED_BLOCK:
        raise CodecError("命名区块容器魔数不符: %s" % data[:4].hex())
    version, header_flag = struct.unpack_from("<II", data, 4)
    blocks = split_blocks(data)
    entries = []
    modes: Dict[str, int] = {}
    for position, span in enumerate(blocks):
        entry = {"index": position}
        entry.update(span.as_dict())
        entry["fields"] = block_fields(data, span)
        entries.append(entry)
        modes[span.mode] = modes.get(span.mode, 0) + 1
    covered = sum(span.length for span in blocks)
    closed = modes.get("closed", 0)
    return {
        "container": "named-block",
        "version": version,
        "header_flag": header_flag,
        "size": len(data),
        "block_count": len(blocks),
        "coverage": {
            "header_bytes": HEADER_SIZE,
            "block_bytes": covered,
            "tail_bytes": len(data) - HEADER_SIZE - covered,
            "tiles_to_eof": HEADER_SIZE + covered == len(data),
            "closed_blocks": closed,
            "predicted_mismatch_blocks": modes.get("predicted-mismatch", 0),
            "no_closed_form_blocks": modes.get("no-closed-form", 0),
            "closed_ratio": round(closed / len(blocks), 6) if blocks else 0.0,
        },
        "blocks": entries,
    }


class NamedBlockCodec(Codec):
    name = "named-block"
    fidelity = FIDELITY_PASSTHROUGH
    readability = "binary-structured"
    extensions = (".emo", ".ent", ".omod", ".otex", ".oanim", ".r119085", ".r119179")
    magics = (MAGIC_NAMED_BLOCK,)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == MAGIC_NAMED_BLOCK

    def decode(self, data: bytes) -> Payload:
        if len(data) < HEADER_SIZE:
            raise CodecError("命名区块容器过短: %d 字节" % len(data))
        version, flag = struct.unpack_from("<II", data, 4)
        return Payload(
            kind="binary",
            blob=data,
            meta={
                "container": "named-block",
                "version": version,
                "header_flag": flag,
                "plain_size": len(data),
            },
        )

    def view(self, payload: Payload):
        data = payload.blob or b""
        try:
            return describe(data)
        except CodecError as exc:
            return {"container": "named-block", "error": str(exc)}

