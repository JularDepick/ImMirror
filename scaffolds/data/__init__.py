# -*- coding: utf-8 -*-
"""数据表脚手架的模块实现。

受理数据表与数据定义类文本素材: csv / csvdef / 明文或文本族容器包裹的 xml 数据定义,
以及 toc / ini / txt / json / cfg 等同类文本。素材本身的编码与换行按索引保存在
data.json 里, 工程文件是 data.json 加一份 UTF-8 的文本文件, 回写时按原编码还原。
"""

from __future__ import annotations

import os

from .. import util
from forge.codecs.base import Payload
from forge.registry import all_codecs, select

NAME = "data"
TITLE = "数据表"
EXTENSIONS = (".csv", ".xml", ".txt", ".toc", ".ini", ".json", ".cfg", ".dat")
FORMATS = {"text-table": "文本数据表或数据定义, 保留原始编码与换行"}

PROJECT_FILE = "data.json"

"""受理的编解码器: 明文文本与文本族容器, 其余一律拒绝, 避免把二进制当表格读。"""
TEXT_CODECS = ("plain-text", "container-text-xor16-zlib")


def _codec_by_name(name: str):
    for codec in all_codecs():
        if codec.name == name:
            return codec
    raise util.ScaffoldError("未知编解码器: %s" % name)


def _columns_hint(text: str) -> list:
    """表格首行的列名提示, 只取前 64 列, 供人工对照。"""
    first = text.split("\n", 1)[0].strip("\r")
    if not first:
        return []
    return [cell.strip().strip('"') for cell in first.split(",")][:64]


def export(src: str, project_dir: str) -> dict:
    data = util.read_bytes(src)
    codec = select(os.path.basename(src), data)
    if codec.name not in TEXT_CODECS:
        """注册表按扩展名分派文本, 未登记的文本 (如 .toc) 在这里按内容补判。"""
        from forge.codecs.plain import PlainTextCodec
        from forge.util.textio import is_probably_text

        if not is_probably_text(data[:8192]):
            raise util.ScaffoldError(
                "%s 不是文本数据表 (编解码器 %s); 本分类受理 %s"
                % (src, codec.name, " ".join(EXTENSIONS))
            )
        codec = PlainTextCodec()
    payload = codec.decode(data)
    if payload.kind != "text":
        raise util.ScaffoldError("%s 解码结果不是文本" % src)
    text = payload.text or ""
    text_name = os.path.basename(src)
    doc = util.make_doc(NAME, "text-table", src, data)
    doc["codec"] = codec.name
    doc["encoding"] = payload.meta.get("encoding")
    doc["bom"] = bool(payload.meta.get("bom"))
    doc["eol"] = payload.meta.get("eol")
    doc["text_file"] = text_name
    doc["text_size"] = payload.meta.get("plain_size")
    doc["text_sha256"] = util.sha256_bytes(text.encode("utf-8"))
    doc["line_count"] = text.count("\n") + (0 if text.endswith("\n") else 1) if text else 0
    if text_name.lower().endswith(".csv"):
        doc["columns"] = _columns_hint(text)
    project_dir = util.ensure_dir(project_dir)
    text_path = os.path.join(project_dir, text_name)
    util.write_text(text_path, text)
    target = os.path.join(project_dir, PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "text-table",
        "project": target,
        "files": [target, text_path],
        "codec": codec.name,
        "encoding": doc["encoding"],
        "eol": doc["eol"],
        "source_sha256": doc["source"]["sha256"],
    }


def build(project_dir: str, out_path: str) -> dict:
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    text_file = doc.get("text_file")
    if not text_file:
        raise util.ScaffoldError("data.json 缺少 text_file")
    text_path = os.path.join(project_dir, text_file)
    text = util.read_text(text_path)
    payload = Payload(
        kind="text",
        text=text,
        meta={
            "container": "text-family" if str(doc.get("codec", "")).startswith("container-") else "plain",
            "encoding": doc.get("encoding", "utf-8"),
            "bom": bool(doc.get("bom")),
            "eol": doc.get("eol"),
        },
    )
    data = _codec_by_name(doc.get("codec")).encode(payload)
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
