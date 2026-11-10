# -*- coding: utf-8 -*-
"""体素与世界脚手架的模块实现。

受理五类:

- vmof: VMOF 体素模型 (.vmo), 20 字节头加 u16 体素数组, 版本 200 另带 FlatBuffers 尾块;
  体素数组以 base64 保存, 结构字段可读可改。
- vox: MagicaVoxel 体素文件 (.vox), 块序列递归保存, SIZE/XYZI/RGBA 解成可读数组,
  其余块原样 base64; 回写时按数组重新序列化块内容。
- world-chunk: 世界区块 (.r), 头 35 字节拆成魔数/坐标区/载荷长度/常量区,
  坐标区按单字节密钥 K 还原成可读坐标, 数据区以 base64 原样保存。
  数据区编码未定, 不改数据区时逐字节一致。
- cust: CUST 自定义模型 (.cm), 载荷为 16 字节周期异或密文, 解密后是 FlatBuffers;
  根表字段, 字符串与方块记录 (id 加坐标) 出可读视图, 正文以 base64 原样保存。
- flatbuffers: FlatBuffers 容器 (.fcm/.bp), 根表槽位出只读视图, 正文 base64 保存。
"""

from __future__ import annotations

import os
import re
import struct

from .. import util

NAME = "world"
TITLE = "体素与世界"
EXTENSIONS = (".vmo", ".vox", ".r", ".cm", ".fcm", ".bp")
FORMATS = {
    "vmof": "VMOF 体素模型, 头加 u16 体素数组, 版本 200 另带尾块",
    "vox": "MagicaVoxel VOX 块序列",
    "world-chunk": "0C 1C 07 06 世界区块",
    "world-chunk-raw": "其余世界区块的整份兜底形态",
    "cust": "CUST 自定义模型, 载荷为异或密文加 FlatBuffers",
    "flatbuffers": "FlatBuffers 容器, 根表槽位只读, 正文原样",
}

PROJECT_FILE = "world.json"
MAGIC_VMOF = b"VMOF"
MAGIC_VOX = b"VOX "
MAGIC_WORLD = b"\x0c\x1c\x07\x06"
MAGIC_CUST = b"CUST"

"""CUST 载荷的 16 字节周期异或密钥, 按解密后偏移取模。"""
CUST_KEY = bytes.fromhex("7cd7cec633f2b218ced6c7aba0f9c121")

"""CUST 解密正文的首 4 字节, 引擎不校验但可作为解密正确的判据。"""
CUST_PLAIN_PREFIX = bytes.fromhex("a91306ed")

"""世界区块头部长度: 魔数 4 加坐标区 11 加长度 4 加常量区 16。"""
WORLD_HEADER_SIZE = 35

WORLD_CONST_RE = re.compile(r"x(-?\d+)z(-?\d+)\.r$", re.IGNORECASE)


def detect(path: str, data: bytes) -> str:
    if data[:4] == MAGIC_VMOF:
        return "vmof"
    if data[:4] == MAGIC_VOX:
        return "vox"
    if data[:4] == MAGIC_WORLD:
        return "world-chunk"
    if data[:4] == MAGIC_CUST:
        return "cust"
    if os.path.splitext(path)[1].lower() == ".r":
        return "world-chunk-raw"
    try:
        _flatbuffers_root(data)
    except util.ScaffoldError:
        pass
    else:
        return "flatbuffers"
    raise util.ScaffoldError(
        "%s 不属于体素与世界分类 (魔数 %s); 本分类受理 %s"
        % (path, data[:8].hex(), " ".join(EXTENSIONS))
    )


"""VMOF 体素模型。"""


def _export_vmof(data: bytes, doc: dict, project_dir: str) -> dict:
    if len(data) < 20:
        raise util.ScaffoldError("VMOF 头不足 20 字节: %d" % len(data))
    magic, version, size_x, size_y, size_z = struct.unpack_from("<4sIIII", data, 0)
    count = size_x * size_y * size_z
    body_start = 20
    body_end = body_start + count * 2
    if body_end > len(data):
        raise util.ScaffoldError(
            "VMOF 体素数组越界: 需要 %d 字节, 实得 %d" % (body_end, len(data))
        )
    voxels = data[body_start:body_end]
    values = struct.unpack_from("<%dH" % count, voxels, 0) if count else ()
    doc["format"] = "vmof"
    doc["header"] = {"magic": magic.decode("ascii"), "version": version,
                     "size": [size_x, size_y, size_z]}
    doc["voxels"] = util.make_blob_field(
        voxels, note="u16le 体素数组, 行优先 x 变化最快; 改尺寸必须同步改数组长度"
    )
    doc["voxels"]["count"] = count
    doc["voxels"]["nonzero"] = sum(1 for value in values if value)
    doc["voxels"]["distinct_values"] = len(set(values)) if values else 0
    rest = data[body_end:]
    if version == 200:
        if len(rest) < 4:
            raise util.ScaffoldError("VMOF 版本 200 缺少尾块长度")
        tail_length = struct.unpack_from("<I", rest, 0)[0]
        tail = rest[4:]
        if len(tail) != tail_length:
            raise util.ScaffoldError(
                "VMOF 尾块长度不符: 声明 %d, 实得 %d" % (tail_length, len(tail))
            )
        doc["tail"] = util.make_blob_field(
            tail, note="版本 200 的 FlatBuffers 尾块 (根表 FBSave::BluePrint)"
        )
        doc["tail"]["length"] = tail_length
    elif rest:
        raise util.ScaffoldError(
            "VMOF 版本 %d 在体素数组之后还有 %d 字节, 结构未定" % (version, len(rest))
        )
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "vmof",
        "project": target,
        "files": [target],
        "version": version,
        "size": [size_x, size_y, size_z],
        "source_sha256": doc["source"]["sha256"],
    }


def _build_vmof(doc: dict) -> bytes:
    header = doc.get("header") or {}
    version = int(header.get("version", 100))
    size = header.get("size") or [0, 0, 0]
    if not isinstance(size, list) or len(size) != 3:
        raise util.ScaffoldError("header.size 必须是三个整数")
    size_x, size_y, size_z = [int(value) for value in size]
    count = size_x * size_y * size_z
    voxels = util.blob_field_bytes(doc.get("voxels"), "voxels")
    if len(voxels) != count * 2:
        raise util.ScaffoldError(
            "体素数组长度 %d 与尺寸 %dx%dx%d 不符 (应为 %d)"
            % (len(voxels), size_x, size_y, size_z, count * 2)
        )
    body = bytearray(MAGIC_VMOF)
    body += struct.pack("<IIII", version, size_x, size_y, size_z)
    body += voxels
    if version == 200:
        tail = util.blob_field_bytes(doc.get("tail"), "tail")
        body += struct.pack("<I", len(tail)) + tail
    return bytes(body)


"""MagicaVoxel VOX。"""


def _parse_vox_chunks(buf: bytes, start: int, end: int) -> list:
    chunks = []
    pos = start
    while pos < end:
        if pos + 12 > end:
            raise util.ScaffoldError("VOX 块头越界: 偏移 %d" % pos)
        raw_id = buf[pos : pos + 4]
        size, child_bytes = struct.unpack_from("<II", buf, pos + 4)
        content_start = pos + 12
        children_start = content_start + size
        if children_start + child_bytes > end:
            raise util.ScaffoldError("VOX 块 %r 越界" % raw_id)
        chunk = {
            "id": raw_id.decode("latin-1"),
            "content": buf[content_start:children_start],
            "children": [],
        }
        if child_bytes:
            chunk["children"] = _parse_vox_chunks(buf, children_start, children_start + child_bytes)
        chunks.append(chunk)
        pos = children_start + child_bytes
    if pos != end:
        raise util.ScaffoldError("VOX 块序列没有铺满到末尾: %d 对 %d" % (pos, end))
    return chunks


def _vox_chunk_doc(chunk: dict) -> dict:
    chunk_id = chunk["id"]
    content = chunk["content"]
    entry = {"id": chunk_id}
    if chunk_id == "SIZE" and len(content) == 12:
        entry["size"] = list(struct.unpack_from("<3I", content, 0))
    elif chunk_id == "XYZI" and len(content) >= 4:
        count = struct.unpack_from("<I", content, 0)[0]
        if len(content) >= 4 + count * 4:
            voxels = [
                list(content[4 + index * 4 : 8 + index * 4]) for index in range(count)
            ]
            entry["voxels"] = voxels
            residual = content[4 + count * 4 :]
            if residual:
                entry["content_tail"] = util.make_blob_field(residual, note="XYZI 计数之后的残余")
        else:
            entry["content"] = util.make_blob_field(content)
    elif chunk_id == "RGBA" and len(content) == 1024:
        entry["palette"] = [
            list(content[index * 4 : index * 4 + 4]) for index in range(256)
        ]
    else:
        entry["content"] = util.make_blob_field(content)
    if chunk["children"]:
        entry["children"] = [_vox_chunk_doc(child) for child in chunk["children"]]
    return entry


def _vox_chunk_bytes(entry: dict) -> bytes:
    chunk_id = entry.get("id")
    if not isinstance(chunk_id, str) or len(chunk_id.encode("latin-1")) != 4:
        raise util.ScaffoldError("VOX 块标识必须是 4 个 ASCII 字符: %r" % chunk_id)
    if "size" in entry:
        size = [int(value) for value in entry["size"]]
        if len(size) != 3:
            raise util.ScaffoldError("SIZE 块需要三个整数")
        content = struct.pack("<3I", *size)
    elif "voxels" in entry:
        voxels = entry["voxels"]
        content = bytearray(struct.pack("<I", len(voxels)))
        for voxel in voxels:
            if len(voxel) != 4:
                raise util.ScaffoldError("体素必须是 [x, y, z, 颜色索引] 四元组: %r" % (voxel,))
            content += bytes(int(channel) for channel in voxel)
        if "content_tail" in entry:
            content += util.blob_field_bytes(entry["content_tail"], "content_tail")
    elif "palette" in entry:
        palette = entry["palette"]
        if len(palette) != 256:
            raise util.ScaffoldError("RGBA 调色板必须是 256 项, 实得 %d" % len(palette))
        content = b"".join(bytes(int(channel) for channel in color) for color in palette)
    else:
        content = util.blob_field_bytes(entry.get("content"), "VOX 块 %s 的 content" % chunk_id)
    children = b"".join(_vox_chunk_bytes(child) for child in entry.get("children") or [])
    return (
        chunk_id.encode("latin-1")
        + struct.pack("<II", len(content), len(children))
        + content
        + children
    )


def _export_vox(data: bytes, doc: dict, project_dir: str) -> dict:
    if len(data) < 8:
        raise util.ScaffoldError("VOX 头不足 8 字节")
    magic, version = struct.unpack_from("<4sI", data, 0)
    chunks = _parse_vox_chunks(data, 8, len(data))
    doc["format"] = "vox"
    doc["header"] = {"magic": magic.decode("latin-1"), "version": version}
    doc["chunks"] = [_vox_chunk_doc(chunk) for chunk in chunks]
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "vox",
        "project": target,
        "files": [target],
        "chunks": len(chunks),
        "source_sha256": doc["source"]["sha256"],
    }


def _build_vox(doc: dict) -> bytes:
    header = doc.get("header") or {}
    magic = str(header.get("magic", "VOX ")).encode("latin-1")
    if len(magic) != 4:
        raise util.ScaffoldError("VOX 魔数必须是 4 字节")
    body = b"".join(_vox_chunk_bytes(chunk) for chunk in doc.get("chunks") or [])
    return magic + struct.pack("<I", int(header.get("version", 150))) + body


"""世界区块 .r。"""


def _world_candidates(cipher: bytes) -> list:
    """对 11 字节坐标密文枚举单字节密钥, 返回结构自洽的候选。"""
    out = []
    for key in range(256):
        plain = bytes(value ^ key for value in cipher)
        if plain[1] != 0:
            continue
        if plain[0] > 64 or plain[2] > 64:
            continue
        x, z = struct.unpack_from("<ii", plain, 3)
        if abs(x) > 4096 or abs(z) > 4096:
            continue
        out.append((key, plain[0], plain[2], x, z))
    return out


def _export_world_chunk(data: bytes, src: str, doc: dict, project_dir: str) -> dict:
    if len(data) < WORLD_HEADER_SIZE:
        raise util.ScaffoldError("世界区块头不足 %d 字节: %d" % (WORLD_HEADER_SIZE, len(data)))
    magic = data[:4]
    cipher = data[4:15]
    payload_length = struct.unpack_from("<I", data, 15)[0]
    extra = data[19:WORLD_HEADER_SIZE]
    body = data[WORLD_HEADER_SIZE:]
    candidates = _world_candidates(cipher)
    match = WORLD_CONST_RE.search(os.path.basename(src))
    chosen = None
    if match:
        target_x, target_z = int(match.group(1)), int(match.group(2))
        for candidate in candidates:
            if candidate[3] == target_x and candidate[4] == target_z:
                chosen = candidate
                break
    if chosen is None and candidates:
        chosen = candidates[0]
    doc["format"] = "world-chunk"
    doc["header"] = {
        "magic": magic.hex(),
        "payload_length": payload_length,
        "constant_hex": extra[3:].hex(),
        "unknown_hex": extra[:3].hex(),
        "note": "常量区实测只有两种取值, 原样保留; 载荷长度与数据区末非零字节相差 20 或 21",
    }
    if chosen is None:
        doc["coordinate"] = {
            "key": None,
            "cipher_hex": cipher.hex(),
            "note": "单字节密钥未能解出结构自洽的坐标, 只保留密文",
        }
    else:
        key, b_value, c_value, x_value, z_value = chosen
        doc["coordinate"] = {
            "key": key,
            "b": b_value,
            "c": c_value,
            "x": x_value,
            "z": z_value,
            "cipher_hex": cipher.hex(),
            "note": "明文 11 字节为 [B, 0, C, i32le X, i32le Z], 与单字节密钥 K 逐字节异或; K 实测等于路径 A + 32 * B",
        }
    doc["data"] = util.make_blob_field(
        body, note="数据区原样字节 (含尾部零填充), 编码未定, base64 保存"
    )
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "world-chunk",
        "project": target,
        "files": [target],
        "coordinate": None if chosen is None else {"x": chosen[3], "z": chosen[4], "key": chosen[0]},
        "source_sha256": doc["source"]["sha256"],
    }


def _export_world_raw(data: bytes, doc: dict, project_dir: str) -> dict:
    doc["format"] = "world-chunk-raw"
    doc["payload"] = util.make_blob_field(
        data, note="头部形态不符合 0C1C0706 的世界区块, 整份原样保留"
    )
    doc["magic_hex"] = data[:8].hex()
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "world-chunk-raw",
        "project": target,
        "files": [target],
        "source_sha256": doc["source"]["sha256"],
    }


def _world_cipher(doc: dict) -> bytes:
    coordinate = doc.get("coordinate") or {}
    key = coordinate.get("key")
    if isinstance(key, int):
        plain = bytearray()
        plain.append(int(coordinate.get("b", 0)) & 0xFF)
        plain.append(0)
        plain.append(int(coordinate.get("c", 0)) & 0xFF)
        plain += struct.pack("<ii", int(coordinate.get("x", 0)), int(coordinate.get("z", 0)))
        if len(plain) != 11:
            raise util.ScaffoldError("坐标区必须还原成 11 字节")
        return bytes(value ^ key for value in plain)
    cipher_hex = coordinate.get("cipher_hex")
    if not cipher_hex:
        raise util.ScaffoldError("coordinate 既没有 key 也没有 cipher_hex")
    cipher = bytes.fromhex(cipher_hex)
    if len(cipher) != 11:
        raise util.ScaffoldError("coordinate.cipher_hex 必须是 11 字节")
    return cipher


def _build_world_chunk(doc: dict) -> bytes:
    header = doc.get("header") or {}
    magic = bytes.fromhex(header.get("magic", "0c1c0706"))
    if len(magic) != 4:
        raise util.ScaffoldError("header.magic 必须是 4 字节")
    cipher = _world_cipher(doc)
    payload_length = int(header.get("payload_length", 0))
    unknown = bytes.fromhex(header.get("unknown_hex", "000000"))
    constant = bytes.fromhex(header.get("constant_hex", ""))
    if len(unknown) != 3 or len(constant) != 13:
        raise util.ScaffoldError("header.unknown_hex 需 3 字节, constant_hex 需 13 字节")
    body = util.blob_field_bytes(doc.get("data"), "data")
    return magic + cipher + struct.pack("<I", payload_length) + unknown + constant + body


"""CUST 自定义模型。"""


def _cust_decrypt(data: bytes) -> bytes:
    if data[:4] != MAGIC_CUST:
        raise util.ScaffoldError("CUST 魔数不符: %s" % data[:4].hex())
    return bytes(value ^ CUST_KEY[index % 16] for index, value in enumerate(data[4:]))


def _cust_encrypt(plain: bytes) -> bytes:
    return MAGIC_CUST + bytes(value ^ CUST_KEY[index % 16] for index, value in enumerate(plain))


def _parse_cust(plain: bytes) -> dict:
    """解析 CUST 明文里的 FlatBuffers 根表, 失败只记录错误。"""
    info = {"plain_prefix": plain[:4].hex()}
    if plain[:4] != CUST_PLAIN_PREFIX:
        info["error"] = "明文首 4 字节不是 a91306ed, 与解密口径不符"
        return info
    if len(plain) < 8:
        info["error"] = "明文过短"
        return info
    buffer = plain[4:]
    root_offset = struct.unpack_from("<I", buffer, 0)[0]
    info["root_offset"] = root_offset
    try:
        table = util.fb_table(buffer, root_offset)
    except util.ScaffoldError as exc:
        info["error"] = str(exc)
        return info
    info["root_table"] = table
    fields = []
    strings = {}
    blocks = []
    for index, slot in enumerate(table["slots"]):
        if slot == 0:
            continue
        field_off = table["table"] + slot
        described = util.fb_describe_slot(buffer, field_off)
        described["slot"] = index
        fields.append(described)
        if described.get("type") == "string":
            strings["slot_%d" % index] = described["value"]
        if described.get("type") == "vector" and index == 10:
            vector_off = field_off + struct.unpack_from("<I", buffer, field_off)[0]
            count = struct.unpack_from("<I", buffer, vector_off)[0]
            for element in range(count):
                element_off = vector_off + 4 + element * 4
                rel = struct.unpack_from("<I", buffer, element_off)[0]
                table_off = element_off + rel
                if table_off + 12 > len(buffer):
                    continue
                block_id = struct.unpack_from("<H", buffer, table_off + 6)[0]
                packed = struct.unpack_from("<I", buffer, table_off + 8)[0]
                blocks.append(
                    {
                        "block_id": block_id,
                        "x": packed & 0xFF,
                        "y": (packed >> 8) & 0xFF,
                        "z": (packed >> 16) & 0xFF,
                    }
                )
    info["fields"] = fields
    info["strings"] = strings
    info["blocks"] = blocks
    info["block_count"] = len(blocks)
    return info


def _export_cust(data: bytes, doc: dict, project_dir: str) -> dict:
    plain = _cust_decrypt(data)
    doc["format"] = "cust"
    doc["shell"] = {"magic": "CUST", "xor_key": CUST_KEY.hex(),
                    "note": "载荷按 16 字节周期密钥从 O[4:] 起逐字节异或"}
    doc["flatbuffer"] = _parse_cust(plain)
    doc["payload"] = util.make_blob_field(
        plain, note="解密后的正文 (含首 4 字节 a91306ed), base64 保存"
    )
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "cust",
        "project": target,
        "files": [target],
        "block_count": doc["flatbuffer"].get("block_count", 0),
        "source_sha256": doc["source"]["sha256"],
    }


def _build_cust(doc: dict) -> bytes:
    plain = util.blob_field_bytes(doc.get("payload"), "payload")
    if plain[:4] != CUST_PLAIN_PREFIX:
        raise util.ScaffoldError("payload 首 4 字节应为 a91306ed, 实得 %s" % plain[:4].hex())
    return _cust_encrypt(plain)


"""FlatBuffers 容器 .fcm / .bp。"""


def _flatbuffers_root(data: bytes) -> dict:
    if len(data) < 12:
        raise util.ScaffoldError("FlatBuffers 过短: %d 字节" % len(data))
    root = struct.unpack_from("<I", data, 0)[0]
    if not 4 <= root < len(data):
        raise util.ScaffoldError("FlatBuffers 根表偏移越界: %d" % root)
    table = util.fb_table(data, root)
    if table["vlen"] < 4 or table["vlen"] > 512:
        raise util.ScaffoldError("FlatBuffers 根表 vtable 长度非法: %d" % table["vlen"])
    return table


def _export_flatbuffers(data: bytes, doc: dict, project_dir: str) -> dict:
    table = _flatbuffers_root(data)
    fields = []
    for index, slot in enumerate(table["slots"]):
        if slot == 0:
            continue
        described = util.fb_describe_slot(data, table["table"] + slot)
        described["slot"] = index
        fields.append(described)
    doc["format"] = "flatbuffers"
    doc["root_table"] = table
    doc["fields"] = fields
    doc["payload"] = util.make_blob_field(
        data, note="FlatBuffers 缓冲区原样字节; 结构视图只读, 回写以本字段为准"
    )
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "flatbuffers",
        "project": target,
        "files": [target],
        "slots": len(table["slots"]),
        "source_sha256": doc["source"]["sha256"],
    }


def export(src: str, project_dir: str) -> dict:
    data = util.read_bytes(src)
    fmt = detect(src, data)
    doc = util.make_doc(NAME, fmt, src, data)
    if fmt == "vmof":
        return _export_vmof(data, doc, project_dir)
    if fmt == "vox":
        return _export_vox(data, doc, project_dir)
    if fmt == "world-chunk":
        return _export_world_chunk(data, src, doc, project_dir)
    if fmt == "world-chunk-raw":
        return _export_world_raw(data, doc, project_dir)
    if fmt == "cust":
        return _export_cust(data, doc, project_dir)
    return _export_flatbuffers(data, doc, project_dir)


def build(project_dir: str, out_path: str) -> dict:
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    fmt = doc["format"]
    if fmt == "vmof":
        data = _build_vmof(doc)
    elif fmt == "vox":
        data = _build_vox(doc)
    elif fmt == "world-chunk":
        data = _build_world_chunk(doc)
    elif fmt == "world-chunk-raw":
        data = util.blob_field_bytes(doc.get("payload"), "payload")
    elif fmt == "cust":
        data = _build_cust(doc)
    else:
        data = util.blob_field_bytes(doc.get("payload"), "payload")
    util.write_bytes(out_path, data)
    return {
        "category": NAME,
        "format": fmt,
        "project": doc_path,
        "out": out_path,
        "size": len(data),
        "sha256": util.sha256_bytes(data),
        "source_sha256": source.get("sha256"),
        "byte_identical": util.sha256_bytes(data) == source.get("sha256"),
    }
