# -*- coding: utf-8 -*-
"""脚手架共用工具。

只提供与分类无关的基础设施: 工程文档的公共字段, 文本与 JSON 读写,
base64 载荷封装, 二进制族外壳的剥离与重套, 以及 FlatBuffers 表的只读解析。
所有分类模块都从这里取用, 避免各自复制一份。

约定:
- 工程文档是 JSON, 顶层固定字段 project / version / category / format / source;
- 二进制兜底一律用 base64, 字段名以 _b64 结尾或形如 {"encoding": "base64", "base64": ...};
- 本模块不改写任何来源文件, 写操作只发生在调用方给出的工程目录与输出路径。
"""

from __future__ import annotations

import base64
import hashlib
import json
import os
import struct
import sys

"""工程文档的标识与版本, 回写时用于拒绝来历不明的 JSON。"""
PROJECT_NAME = "immirror-scaffold"
PROJECT_VERSION = 1

"""二进制族容器魔数, 与 forge.codecs.container 保持同一口径。"""
MAGIC_BINARY = b"\xff\xd9\xff\xd8"

"""常见来源扩展名, 供 CLI 与文档展示。"""
TEXT_ENCODINGS = ("utf-8", "gbk", "gb18030", "big5", "latin-1")


def _ensure_forge_importable() -> None:
    """把 assets 根目录放进 sys.path, 保证无论从哪个工作目录启动都能 import forge。"""
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if root not in sys.path:
        sys.path.insert(0, root)


_ensure_forge_importable()


class ScaffoldError(Exception):
    """脚手架层面的错误, 消息里必须带可定位的路径或字段。"""


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_bytes(path: str) -> bytes:
    try:
        with open(path, "rb") as handle:
            return handle.read()
    except OSError as exc:
        raise ScaffoldError("读取失败: %s (%s)" % (path, exc)) from exc


def write_bytes(path: str, data: bytes) -> str:
    parent = os.path.dirname(os.path.abspath(path))
    os.makedirs(parent, exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(data)
    return path


def read_text(path: str) -> str:
    return read_bytes(path).decode("utf-8")


def write_text(path: str, text: str) -> str:
    return write_bytes(path, text.encode("utf-8"))


def read_json(path: str):
    try:
        return json.loads(read_text(path))
    except json.JSONDecodeError as exc:
        raise ScaffoldError("工程文件不是合法 JSON: %s (%s)" % (path, exc)) from exc


def json_safe(obj):
    """把不便于 JSON 序列化的值收敛成可读形态, 避免 NaN 与 bytes 让导出失败。"""
    if isinstance(obj, float):
        if obj != obj or obj in (float("inf"), float("-inf")):
            return None
        return obj
    if isinstance(obj, (bytes, bytearray)):
        return bytes(obj).hex()
    if isinstance(obj, dict):
        return {str(key): json_safe(value) for key, value in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [json_safe(value) for value in obj]
    if isinstance(obj, (str, int, bool)) or obj is None:
        return obj
    return str(obj)


def write_json(path: str, obj) -> str:
    """稳定输出: 键保持插入顺序, 缩进 2, 结尾换行, 便于人工编辑与 diff。"""
    text = json.dumps(json_safe(obj), ensure_ascii=False, indent=2, allow_nan=False)
    return write_text(path, text + "\n")


def ensure_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def b64encode(data: bytes) -> str:
    return base64.b64encode(data).decode("ascii")


def b64decode(text: str) -> bytes:
    try:
        return base64.b64decode(text.encode("ascii"), validate=True)
    except (ValueError, UnicodeEncodeError) as exc:
        raise ScaffoldError("base64 载荷非法: %s" % exc) from exc


def make_blob_field(data: bytes, note: str = "") -> dict:
    """二进制兜底字段: base64 正文加长度与摘要, 便于人工核对改动。"""
    field = {
        "encoding": "base64",
        "bytes": len(data),
        "sha256": sha256_bytes(data),
        "base64": b64encode(data),
    }
    if note:
        field["note"] = note
    return field


def blob_field_bytes(field, what: str = "二进制字段") -> bytes:
    if not isinstance(field, dict):
        raise ScaffoldError("%s 必须是对象, 实得 %s" % (what, type(field).__name__))
    if field.get("encoding") != "base64":
        raise ScaffoldError("%s 缺少 encoding=base64" % what)
    data = b64decode(field.get("base64", ""))
    declared = field.get("bytes")
    if isinstance(declared, int) and declared != len(data):
        raise ScaffoldError("%s 长度不符: 声明 %d, 实得 %d" % (what, declared, len(data)))
    return data


def make_doc(category: str, fmt: str, src_path: str, data: bytes) -> dict:
    name = os.path.basename(src_path)
    stem, ext = os.path.splitext(name)
    return {
        "project": PROJECT_NAME,
        "version": PROJECT_VERSION,
        "category": category,
        "format": fmt,
        "source": {
            "name": name,
            "stem": stem,
            "extension": ext.lower(),
            "size": len(data),
            "sha256": sha256_bytes(data),
        },
    }


def check_doc(doc, category: str, formats=None) -> dict:
    """校验工程文档的公共字段, 返回 source 段。"""
    if not isinstance(doc, dict):
        raise ScaffoldError("工程文档必须是 JSON 对象")
    if doc.get("project") != PROJECT_NAME:
        raise ScaffoldError("工程文档标识不符: %r" % doc.get("project"))
    if doc.get("version") != PROJECT_VERSION:
        raise ScaffoldError("工程文档版本不符: %r" % doc.get("version"))
    if doc.get("category") != category:
        raise ScaffoldError("工程文档分类不符: 期望 %s, 实得 %r" % (category, doc.get("category")))
    if formats is not None and doc.get("format") not in formats:
        raise ScaffoldError("工程文档格式未支持: %r" % doc.get("format"))
    source = doc.get("source")
    if not isinstance(source, dict) or "sha256" not in source:
        raise ScaffoldError("工程文档缺少 source.sha256")
    return source


def detect_envelope(data: bytes) -> dict:
    """识别二进制族外壳, 没有外壳时返回空表。"""
    if data[:4] != MAGIC_BINARY:
        return {}
    if len(data) < 8:
        raise ScaffoldError("二进制族外壳过短: %d 字节" % len(data))
    prefix = int.from_bytes(data[4:8], "big")
    if prefix > len(data) - 8:
        raise ScaffoldError("二进制族外壳声明的前缀长度越界: %d" % prefix)
    return {"container": "binary-family", "enc_prefix": prefix}


def unwrap_envelope(data: bytes):
    """剥掉二进制族外壳, 返回 (明文载荷, 外壳参数或空表)。"""
    envelope = detect_envelope(data)
    if not envelope:
        return data, {}
    from forge.codecs.container import xor_stream

    prefix = envelope["enc_prefix"]
    plain = xor_stream(data[8 : 8 + prefix]) + data[8 + prefix :]
    return plain, envelope


def wrap_envelope(data: bytes, envelope: dict) -> bytes:
    """按外壳参数把明文载荷重新套回二进制族容器。"""
    if not envelope:
        return data
    if envelope.get("container") != "binary-family":
        raise ScaffoldError("未知外壳类型: %r" % envelope.get("container"))
    from forge.codecs.container import BINARY_PREFIX_DEFAULT, xor_stream

    prefix = int(envelope.get("enc_prefix", BINARY_PREFIX_DEFAULT))
    if prefix > len(data):
        raise ScaffoldError("载荷不足 %d 字节, 无法套用二进制族外壳" % prefix)
    return MAGIC_BINARY + prefix.to_bytes(4, "big") + xor_stream(data[:prefix]) + data[prefix:]


"""命名区块容器: 块序列的拆装由 forge.codecs.named_block 负责切分, 组装在这里。"""


def build_named_block(container: dict, blocks) -> bytes:
    """按容器头与块序列组装 89 67 45 23 容器。

    blocks 的每项是 (块名, 类型标识, 载荷字节); 块名为问号开头或类型标识为负时
    视为未解析区, 不写块头只写载荷, 保证兜底样本也能回写。
    """
    import struct

    magic_hex = container.get("magic", "89674523")
    try:
        magic = bytes.fromhex(magic_hex)
    except (TypeError, ValueError) as exc:
        raise ScaffoldError("container.magic 不是十六进制: %r" % magic_hex) from exc
    if len(magic) != 4:
        raise ScaffoldError("container.magic 必须是 4 字节, 实得 %d" % len(magic))
    body = bytearray(magic)
    body += struct.pack("<II", int(container.get("version", 100)),
                        int(container.get("header_flag", 1)))
    for item in blocks:
        name, type_id, payload = item
        if not isinstance(name, str):
            raise ScaffoldError("区块缺少 name")
        if name.startswith("?") or not isinstance(type_id, int) or type_id < 0:
            body += payload
            continue
        raw_name = name.encode("ascii", "strict")
        if not 1 <= len(raw_name) <= 255:
            raise ScaffoldError("区块名长度非法: %r" % name)
        body += struct.pack("<H", type_id) + bytes((len(raw_name),)) + raw_name + payload
    return bytes(body)


def named_block_units(doc: dict):
    """把工程文档里的区块列表还原成 (名, 类型标识, 载荷) 序列。"""
    units = []
    for block in doc.get("blocks") or []:
        name = block.get("name")
        payload = blob_field_bytes(block.get("payload"), "区块 %s 的 payload" % name)
        units.append((name, block.get("type_id"), payload))
    return units


def project_json_path(project_dir: str, name: str = "project.json") -> str:
    return os.path.join(project_dir, name)


def find_project_json(project_dir: str, preferred: str = "project.json") -> str:
    """在工程目录里找工程文档: 先找约定名, 再找唯一的 .json。"""
    if not os.path.isdir(project_dir):
        raise ScaffoldError("工程目录不存在: %s" % project_dir)
    candidate = os.path.join(project_dir, preferred)
    if os.path.isfile(candidate):
        return candidate
    names = sorted(name for name in os.listdir(project_dir) if name.lower().endswith(".json"))
    if len(names) == 1:
        return os.path.join(project_dir, names[0])
    if not names:
        raise ScaffoldError("工程目录里没有 JSON 工程文档: %s" % project_dir)
    raise ScaffoldError(
        "工程目录里有多个 JSON, 无法判断哪个是工程文档: %s" % ", ".join(names)
    )


def iter_source_files(paths) -> list:
    """把文件与目录参数展开成文件列表, 目录按扩展名过滤并递归。"""
    out = []
    for path in paths:
        full = os.path.abspath(path)
        if os.path.isdir(full):
            for dirpath, dirnames, filenames in os.walk(full):
                dirnames.sort()
                for name in sorted(filenames):
                    out.append(os.path.join(dirpath, name))
        elif os.path.isfile(full):
            out.append(full)
        else:
            raise ScaffoldError("路径不存在: %s" % path)
    return out


def human_size(size: int) -> str:
    value = float(size)
    for unit in ("B", "KiB", "MiB", "GiB"):
        if value < 1024 or unit == "GiB":
            return "%.0f %s" % (value, unit) if unit == "B" else "%.2f %s" % (value, unit)
        value /= 1024.0
    return "%.2f GiB" % value


def pixel_diff(left: bytes, right: bytes) -> dict:
    """比较两份 RGBA 像素, 返回供报告使用的误差统计。"""
    if len(left) != len(right):
        raise ScaffoldError("像素长度不一致: %d 对 %d" % (len(left), len(right)))
    total = 0
    biggest = 0
    changed = 0
    for i in range(0, len(left), 4):
        diff = 0
        for channel in range(4):
            delta = abs(left[i + channel] - right[i + channel])
            total += delta
            diff += delta
            if delta > biggest:
                biggest = delta
        if diff:
            changed += 1
    pixels = max(1, len(left) // 4)
    return {
        "mean": round(total / float(len(left)), 4),
        "max": biggest,
        "changed_pixels": changed,
        "pixels": pixels,
    }


"""FlatBuffers 只读解析: 只读取 vtable 与槽位, 不尝试重排缓冲区。"""


def fb_u16(buf: bytes, off: int) -> int:
    return struct.unpack_from("<H", buf, off)[0]


def fb_u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def fb_i32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<i", buf, off)[0]


def fb_table(buf: bytes, table_off: int) -> dict:
    """读取一张表的 vtable 与槽位偏移。

    返回 {table, vtable, vlen, tlen, slots}; 结构不自洽时抛 ScaffoldError。
    """
    if table_off < 0 or table_off + 4 > len(buf):
        raise ScaffoldError("FlatBuffers 表偏移越界: %d" % table_off)
    soffset = fb_i32(buf, table_off)
    vtable = table_off - soffset
    if vtable < 0 or vtable + 4 > len(buf):
        raise ScaffoldError("FlatBuffers vtable 偏移越界: %d" % vtable)
    vlen, tlen = struct.unpack_from("<HH", buf, vtable)
    if vlen < 4 or vlen % 2 or vtable + vlen > len(buf):
        raise ScaffoldError("FlatBuffers vtable 长度非法: %d" % vlen)
    slots = []
    for index in range((vlen - 4) // 2):
        slots.append(fb_u16(buf, vtable + 4 + index * 2))
    return {"table": table_off, "vtable": vtable, "vlen": vlen, "tlen": tlen,
            "slots": slots}


def fb_string(buf: bytes, off: int):
    """把偏移解释为长度前缀字符串, 失败返回 None。

    优先 UTF-8, 失败退回 GBK: 原版素材里存在 GBK 骨骼名。
    """
    try:
        length = fb_u32(buf, off)
    except struct.error:
        return None
    if length == 0 or length > 4096 or off + 4 + length > len(buf):
        return None
    raw = buf[off + 4 : off + 4 + length]
    if raw[-1:] == b"\x00":
        raw = raw[:-1]
    for encoding in ("utf-8", "gbk"):
        try:
            text = raw.decode(encoding)
        except UnicodeDecodeError:
            continue
        if all(ord(char) >= 32 or char in "\r\n\t" for char in text):
            return text
    return None


def fb_vector(buf: bytes, off: int):
    """把偏移解释为 vector, 返回 {count, data_offset} 或 None。"""
    try:
        count = fb_u32(buf, off)
    except struct.error:
        return None
    if off + 4 + count > len(buf):
        return None
    return {"count": count, "data_offset": off + 4}


def fb_uoffset_target(buf: bytes, at: int):
    """读取 uoffset 并换算成目标位置, 越界时返回 None。"""
    try:
        rel = fb_u32(buf, at)
    except struct.error:
        return None
    if rel == 0:
        return None
    target = at + rel
    if target >= len(buf):
        return None
    return target


def fb_describe_slot(buf: bytes, field_off: int) -> dict:
    """尽力把字段解释成字符串/表/向量或数值, 供只读视图使用。"""
    info = {"offset": field_off, "hex": buf[field_off : field_off + 8].hex()}
    if field_off + 4 > len(buf):
        return info
    target = fb_uoffset_target(buf, field_off)
    if target is not None:
        text = fb_string(buf, target)
        if text is not None:
            info.update({"type": "string", "value": text})
            return info
        try:
            table = fb_table(buf, target)
        except ScaffoldError:
            table = None
        if table is not None and 8 <= table["vlen"] <= 512 and table["tlen"] >= 4:
            info.update({"type": "table", "table": table})
            return info
        vector = fb_vector(buf, target)
        if vector is not None and vector["count"] < 1 << 24:
            info.update({"type": "vector", "count": vector["count"]})
            return info
    raw = buf[field_off : field_off + 8].ljust(8, b"\x00")
    info["numbers"] = {
        "u8": raw[0],
        "i8": struct.unpack_from("<b", raw, 0)[0],
        "u16": struct.unpack_from("<H", raw, 0)[0],
        "i16": struct.unpack_from("<h", raw, 0)[0],
        "u32": struct.unpack_from("<I", raw, 0)[0],
        "i32": struct.unpack_from("<i", raw, 0)[0],
        "u64": struct.unpack_from("<Q", raw, 0)[0],
    }
    return info
