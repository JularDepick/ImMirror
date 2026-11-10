# -*- coding: utf-8 -*-
"""界面脚手架的模块实现。

受理两类:

- axml: 二进制 Android XML (魔数 03 00 08 00), 工程文件是 layout.json 加 layout.xml。
  layout.json 记录文件头与块序列 (每块原样字节), layout.xml 是解码出的可读布局。
  回写按块序列拼接; 改 layout.xml 目前不支持回写, 会在 build 时明确报错。
- ui-xml-text: 明文或被文本族容器包裹的布局 XML, 工程文件是 layout.json 加 layout.xml。
  回写按索引里的编码与换行还原, 未改动时逐字节一致。
"""

from __future__ import annotations

import os
import struct

from .. import util
from forge.codecs import axml as axmlcodec
from forge.codecs.base import Payload
from forge.registry import all_codecs, select

NAME = "ui"
TITLE = "界面"
EXTENSIONS = (".xml",)
FORMATS = {
    "axml": "二进制 Android XML, 块序列可原样回写",
    "ui-xml-text": "明文或文本族容器包裹的布局 XML, 按原编码回写",
}

PROJECT_FILE = "layout.json"
XML_FILE = "layout.xml"
MAGIC_AXML = b"\x03\x00\x08\x00"

"""块类型编号的可读名, 未列出的按十六进制显示。"""
CHUNK_TYPES = {
    0x0001: "RES_STRING_POOL",
    0x0002: "RES_TABLE",
    0x0180: "RES_XML_RESOURCE_MAP",
    0x0100: "START_NAMESPACE",
    0x0101: "END_NAMESPACE",
    0x0102: "START_ELEMENT",
    0x0103: "END_ELEMENT",
    0x0104: "CDATA",
}


def _split_chunks(data: bytes):
    """按 RES_XML 的块头切分 8 字节文件头之后的块序列。"""
    chunks = []
    offset = 8
    while offset + 8 <= len(data):
        chunk_type, header_size, size = struct.unpack_from("<HHI", data, offset)
        if size < 8 or header_size < 8 or offset + size > len(data):
            raise util.ScaffoldError(
                "axml 块越界: 偏移 %d 类型 %#x 大小 %d" % (offset, chunk_type, size)
            )
        chunks.append(
            {
                "index": len(chunks),
                "type": chunk_type,
                "type_name": CHUNK_TYPES.get(chunk_type, "0x%04x" % chunk_type),
                "header_size": header_size,
                "size": size,
                "payload": util.make_blob_field(
                    data[offset : offset + size], note="块原样字节, 含块头"
                ),
            }
        )
        offset += size
    return chunks, offset


def _xml_view(data: bytes) -> dict:
    """调用 forge.codecs.axml 的视图拿到可读 XML, 失败时只记录错误。"""
    try:
        view = axmlcodec.AxmlCodec().view(Payload(kind="binary", blob=data))
    except (ValueError, struct.error, IndexError) as exc:
        return {"error": str(exc)}
    return view or {}


def _export_axml(data: bytes, doc: dict, project_dir: str) -> dict:
    if len(data) < 8:
        raise util.ScaffoldError("axml 过短: %d 字节" % len(data))
    file_type, header_size, file_size = struct.unpack_from("<HHI", data, 0)
    chunks, end = _split_chunks(data)
    doc["format"] = "axml"
    doc["header"] = {"type": file_type, "header_size": header_size, "size": file_size}
    doc["chunks"] = chunks
    doc["coverage"] = {
        "bytes": len(data),
        "chunk_bytes": end - 8,
        "tiles_to_eof": end == len(data),
        "header_size_declared": file_size,
        "header_size_actual": len(data),
        "chunks": len(chunks),
    }
    view = _xml_view(data)
    xml_text = view.get("xml") or ""
    doc["string_pool"] = {
        "encoding": view.get("string_encoding"),
        "count": view.get("string_count"),
        "note": "字符串池是块内派生的只读视图, 回写以块原样字节为准",
    }
    doc["stats"] = {
        key: view.get(key)
        for key in ("elements", "attributes", "texts", "namespaces", "xml_chars", "truncated")
        if key in view
    }
    files = []
    xml_path = os.path.join(project_dir, XML_FILE)
    util.write_text(xml_path, xml_text)
    files.append(xml_path)
    doc["xml"] = {
        "file": XML_FILE,
        "sha256": util.sha256_bytes(xml_text.encode("utf-8")),
        "note": "可读布局, 仅供阅读与对照; 改它不会改变回写结果",
    }
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "axml",
        "project": target,
        "files": [target] + files,
        "chunks": len(chunks),
        "source_sha256": doc["source"]["sha256"],
    }


def _export_text(data: bytes, src: str, doc: dict, project_dir: str) -> dict:
    codec = select(os.path.basename(src), data)
    if codec.name not in ("plain-text", "container-text-xor16-zlib"):
        raise util.ScaffoldError(
            "%s 不是可读的布局 XML (编解码器 %s), 本分类受理 %s"
            % (src, codec.name, " ".join(EXTENSIONS))
        )
    payload = codec.decode(data)
    if payload.kind != "text":
        raise util.ScaffoldError("%s 解码结果不是文本" % src)
    text = payload.text or ""
    doc["format"] = "ui-xml-text"
    doc["codec"] = codec.name
    doc["encoding"] = payload.meta.get("encoding")
    doc["bom"] = bool(payload.meta.get("bom"))
    doc["eol"] = payload.meta.get("eol")
    doc["text_file"] = XML_FILE
    doc["text_size"] = payload.meta.get("plain_size")
    doc["text_sha256"] = util.sha256_bytes(text.encode("utf-8"))
    xml_path = os.path.join(util.ensure_dir(project_dir), XML_FILE)
    util.write_text(xml_path, text)
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "ui-xml-text",
        "project": target,
        "files": [target, xml_path],
        "codec": codec.name,
        "encoding": doc["encoding"],
        "eol": doc["eol"],
        "source_sha256": doc["source"]["sha256"],
    }


def export(src: str, project_dir: str) -> dict:
    data = util.read_bytes(src)
    if data[:4] == MAGIC_AXML:
        doc = util.make_doc(NAME, "axml", src, data)
        return _export_axml(data, doc, project_dir)
    doc = util.make_doc(NAME, "ui-xml-text", src, data)
    return _export_text(data, src, doc, project_dir)


def _codec_by_name(name: str):
    for codec in all_codecs():
        if codec.name == name:
            return codec
    raise util.ScaffoldError("未知编解码器: %s" % name)


def _build_axml(doc: dict, project_dir: str) -> bytes:
    header = doc.get("header") or {}
    file_type = int(header.get("type", 3))
    header_size = int(header.get("header_size", 8))
    xml_path = os.path.join(project_dir, (doc.get("xml") or {}).get("file", XML_FILE))
    if os.path.isfile(xml_path):
        current = util.sha256_bytes(util.read_bytes(xml_path))
        declared = (doc.get("xml") or {}).get("sha256")
        if declared and current != declared:
            raise util.ScaffoldError(
                "layout.xml 已被编辑, AXML 结构回写尚未支持; "
                "请还原 layout.xml, 或改块序列后重新导出 (见 scaffolds/ui/README.md)"
            )
    body = bytearray()
    for index, chunk in enumerate(doc.get("chunks") or []):
        payload = util.blob_field_bytes(chunk.get("payload"), "第 %d 块" % index)
        if len(payload) != chunk.get("size"):
            raise util.ScaffoldError(
                "第 %d 块 size 字段 %r 与载荷长度 %d 不符"
                % (index, chunk.get("size"), len(payload))
            )
        body += payload
    total = 8 + len(body)
    doc["_header_size_updated"] = total != int(header.get("size", total))
    return struct.pack("<HHI", file_type, header_size, total) + bytes(body)


def _build_text(doc: dict, project_dir: str) -> bytes:
    text_file = doc.get("text_file") or XML_FILE
    text_path = os.path.join(project_dir, text_file)
    text = util.read_text(text_path)
    payload = Payload(
        kind="text",
        text=text,
        meta={
            "container": "text-family" if doc.get("codec", "").startswith("container-") else "plain",
            "encoding": doc.get("encoding", "utf-8"),
            "bom": bool(doc.get("bom")),
            "eol": doc.get("eol"),
        },
    )
    return _codec_by_name(doc.get("codec")).encode(payload)


def build(project_dir: str, out_path: str) -> dict:
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    if doc["format"] == "axml":
        data = _build_axml(doc, project_dir)
    else:
        data = _build_text(doc, project_dir)
    util.write_bytes(out_path, data)
    result = {
        "category": NAME,
        "format": doc["format"],
        "project": doc_path,
        "out": out_path,
        "size": len(data),
        "sha256": util.sha256_bytes(data),
        "source_sha256": source.get("sha256"),
        "byte_identical": util.sha256_bytes(data) == source.get("sha256"),
    }
    if doc.get("_header_size_updated"):
        result["header_size_updated"] = True
    return result
