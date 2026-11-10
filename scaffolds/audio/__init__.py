# -*- coding: utf-8 -*-
"""音频脚手架的模块实现。

受理 Ogg (可套二进制族外壳)。工程文件是 audio.json: 头部字段与页表摘要可读,
音频正文以 base64 原样保存。回写按 base64 还原, 未改动时逐字节一致;
换音频就是整文件替换, 音频编码本身不在管线职责内。
"""

from __future__ import annotations

import os
import struct

from .. import util
from forge.codecs import standard

NAME = "audio"
TITLE = "音频"
EXTENSIONS = (".ogg",)
FORMATS = {"ogg": "Ogg Vorbis 或 Opus, 正文原样保留"}

PROJECT_FILE = "audio.json"
MAGIC_OGG = b"OggS"

"""页头 27 字节, 其后是段表; 摘要只统计页数与序列号, 不搬运正文。"""
PAGE_HEADER_SIZE = 27


def _page_summary(data: bytes) -> dict:
    """扫描 Ogg 页头, 返回页数, 序列号与首末粒度位置。"""
    offset = 0
    pages = 0
    serials = set()
    first_granule = None
    last_granule = None
    while offset + PAGE_HEADER_SIZE <= len(data) and data[offset : offset + 4] == MAGIC_OGG:
        version = data[offset + 4]
        if version != 0:
            break
        segments = data[offset + 26]
        header_len = PAGE_HEADER_SIZE + segments
        if offset + header_len > len(data):
            break
        body_len = sum(data[offset + PAGE_HEADER_SIZE : offset + header_len])
        granule = struct.unpack_from("<q", data, offset + 6)[0]
        serial = struct.unpack_from("<I", data, offset + 14)[0]
        serials.add(serial)
        if first_granule is None:
            first_granule = granule
        last_granule = granule
        pages += 1
        offset += header_len + body_len
    return {
        "pages": pages,
        "serials": sorted(serials),
        "first_granule": first_granule,
        "last_granule": last_granule,
        "tiles_to_eof": offset == len(data),
        "bytes": len(data),
    }


def export(src: str, project_dir: str) -> dict:
    raw = util.read_bytes(src)
    plain, envelope = util.unwrap_envelope(raw)
    if plain[:4] != MAGIC_OGG:
        raise util.ScaffoldError(
            "%s 不是 Ogg (剥壳后魔数 %s); 本分类受理 %s"
            % (src, plain[:4].hex(), " ".join(EXTENSIONS))
        )
    doc = util.make_doc(NAME, "ogg", src, raw)
    doc["envelope"] = envelope or None
    view = standard.OggCodec().view(PayloadBinary(plain)) or {}
    doc["stream"] = {
        "codec": view.get("codec", "unknown"),
        "channels": view.get("channels"),
        "sample_rate": view.get("sampleRate"),
        "note": "由 Ogg 头派生的只读视图; 回写以正文 base64 为准",
    }
    doc["pages"] = _page_summary(plain)
    doc["payload"] = util.make_blob_field(plain, note="Ogg 正文原样字节, base64 保存")
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "ogg",
        "project": target,
        "files": [target],
        "codec": view.get("codec", "unknown"),
        "envelope": envelope or None,
        "source_sha256": doc["source"]["sha256"],
    }


def PayloadBinary(blob: bytes):
    """构造一个只带 blob 的载荷, 供标准格式的视图函数使用。"""
    from forge.codecs.base import Payload

    return Payload(kind="binary", blob=blob)


def build(project_dir: str, out_path: str) -> dict:
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    plain = util.blob_field_bytes(doc.get("payload"), "payload")
    if plain[:4] != MAGIC_OGG:
        raise util.ScaffoldError("payload 不是 Ogg 数据 (魔数 %s)" % plain[:4].hex())
    data = util.wrap_envelope(plain, doc.get("envelope") or {})
    util.write_bytes(out_path, data)
    return {
        "category": NAME,
        "format": doc["format"],
        "project": doc_path,
        "out": out_path,
        "size": len(data),
        "sha256": util.sha256_bytes(data),
        "source_sha256": source.get("sha256"),
        "byte_identical": util.sha256_bytes(data) == source.get("sha256"),
    }
