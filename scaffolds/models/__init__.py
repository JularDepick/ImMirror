# -*- coding: utf-8 -*-
"""模型与实体脚手架的模块实现。

受理命名区块容器 `89 67 45 23` 及其二进制族外壳 (omod/emo/ent/oanim,
以及 `.omod.rNNNNNN` 增量)。导出把容器切成块序列, 每块写字段视图与载荷;
回写按块序列重建容器头与块头, 因此可以增删块、改载荷, 不改动时逐字节还原。
"""

from __future__ import annotations

import os
import struct

from .. import util
from forge.codecs import named_block

NAME = "models"
TITLE = "模型与实体"
EXTENSIONS = (".omod", ".emo", ".ent", ".oanim", ".r119085", ".r119179")
FORMATS = {"named-block": "89 67 45 23 命名区块容器, 可含 FF D9 FF D8 外壳"}

PROJECT_FILE = "model.json"
MAGIC_NAMED_BLOCK = b"\x89gE#"


def detect(path: str, data: bytes) -> str:
    """判断这份素材是否归本分类, 返回格式名; 不受理时抛 ScaffoldError。"""
    plain, envelope = util.unwrap_envelope(data)
    if plain[:4] != MAGIC_NAMED_BLOCK:
        shell = "含二进制族外壳" if envelope else "无外壳"
        raise util.ScaffoldError(
            "%s 不是命名区块容器 (%s, 剥壳后魔数 %s); 本分类受理 %s"
            % (path, shell, plain[:4].hex(), " ".join(EXTENSIONS))
        )
    if len(plain) < 12:
        raise util.ScaffoldError("命名区块容器过短: %s (%d 字节)" % (path, len(plain)))
    return "named-block"


def export(src: str, project_dir: str) -> dict:
    """把单个二进制容器导出为工程目录 (一个 model.json)。"""
    data = util.read_bytes(src)
    fmt = detect(src, data)
    plain, envelope = util.unwrap_envelope(data)
    doc = util.make_doc(NAME, fmt, src, data)
    doc["envelope"] = envelope or None
    version, header_flag = struct.unpack_from("<II", plain, 4)
    doc["container"] = {
        "magic": plain[:4].hex(),
        "version": version,
        "header_flag": header_flag,
        "note": "version 恒为 100, header_flag 实测恒为 1, 语义未定, 原样保留",
    }
    spans = named_block.split_blocks(plain)
    blocks = []
    covered = 0
    for index, span in enumerate(spans):
        payload = plain[span.payload_offset : span.end]
        covered += span.length
        blocks.append(
            {
                "index": index,
                "name": span.name,
                "type_id": span.type_id,
                "mode": span.mode,
                "form": span.form,
                "payload": util.make_blob_field(
                    payload, note="区块载荷原样字节, 改它就等于改块内容"
                ),
                "fields": named_block.block_fields(plain, span),
            }
        )
    doc["blocks"] = blocks
    doc["coverage"] = {
        "size": len(plain),
        "header_bytes": 12,
        "block_bytes": covered,
        "tiles_to_eof": 12 + covered == len(plain),
        "blocks": len(blocks),
        "closed_blocks": sum(1 for span in spans if span.mode == "closed"),
    }
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": fmt,
        "project": target,
        "files": [target],
        "blocks": len(blocks),
        "envelope": envelope or None,
        "source_sha256": doc["source"]["sha256"],
    }


def build(project_dir: str, out_path: str) -> dict:
    """由 model.json 重建容器字节, 并写出到 out_path。"""
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    body = util.build_named_block(doc.get("container") or {}, util.named_block_units(doc))
    data = util.wrap_envelope(body, doc.get("envelope") or {})
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
