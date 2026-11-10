# -*- coding: utf-8 -*-
"""工程树与二进制之间的双向转换。

约定:
- project/<rel> 是可读工程文件, 文本一律 UTF-8, 二进制保留解密后的原始载荷
- project/_view/<rel>.json 是结构化视图, 只供阅读
- 索引是唯一权威 (默认 reports/project-index.jsonl), 记录重新编码所需全部参数
- 两种编码目标:
  plain     解密后的载荷, 与源同源同字节 (客户端默认消费); 文本默认保持原始编码与换行,
            可选 utf8-lf 策略把它规范成 UTF-8 无 BOM 且 LF 结尾, 便于 git diff
  original  逐字节还原原版容器 (校验路径, 不作默认输出)

文本策略只在 plain 目标生效: verbatim 保留源编码与换行, utf8-lf 转码并统一换行。
original 目标始终按索引里记录的编码与换行还原, 与策略无关。
"""

from __future__ import annotations

import hashlib
import json
import os

from .codecs.base import Codec, CodecError, Payload
from .registry import select

INDEX_NAME = "project-index.jsonl"
VIEW_DIR = "_view"

TARGET_PLAIN = "plain"
TARGET_ORIGINAL = "original"
TARGETS = (TARGET_PLAIN, TARGET_ORIGINAL)

"""明文目标的文本策略: 保持原始字节, 或统一成 UTF-8 无 BOM 加 LF。"""
TEXT_VERBATIM = "verbatim"
TEXT_UTF8_LF = "utf8-lf"
TEXT_POLICIES = (TEXT_VERBATIM, TEXT_UTF8_LF)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _read(path: str) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _write(path: str, data: bytes) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(data)


def _write_json(path: str, obj) -> None:
    _write(path, (json.dumps(obj, ensure_ascii=False, indent=1, sort_keys=True) + "\n").encode("utf-8"))


def _iter_source(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            yield os.path.join(dirpath, name)


def _side_files(rel: str, payload: Payload):
    """编解码器之外的派生工程文件 (目前只有 KTX 的 PNG 像素视图)。

    派生文件不进索引的权威字段, 只记录路径; 缺模块或像素格式未支持时返回空表。
    """
    try:
        from .ktx_png import side_files
    except ImportError:
        return ()
    try:
        return side_files(rel, payload)
    except CodecError:
        return ()


def unpack_file(asset_root: str, project_root: str, path: str):
    """解码单个素材到工程树, 返回索引记录; 失败抛 CodecError。"""
    rel = os.path.relpath(path, asset_root).replace(os.sep, "/")
    data = _read(path)
    codec = select(rel, data)
    payload = codec.decode(data)
    record = {
        "path": rel,
        "codec": codec.name,
        "fidelity": codec.fidelity,
        "source_size": len(data),
        "source_sha256": sha256_bytes(data),
        "payload_kind": payload.kind,
        "meta": payload.meta,
    }
    if payload.kind == "text":
        _write(os.path.join(project_root, rel), (payload.text or "").encode("utf-8"))
    else:
        _write(os.path.join(project_root, rel), payload.blob or b"")
    view = codec.view(payload)
    if view is not None:
        view_rel = "%s/%s.json" % (VIEW_DIR, rel)
        _write_json(os.path.join(project_root, view_rel), view)
        record["view"] = view_rel
    for side_rel, side_data in _side_files(rel, payload):
        _write(os.path.join(project_root, side_rel), side_data)
        record.setdefault("side_files", []).append(side_rel)
    return record


def unpack_tree(asset_root: str, project_root: str, include_prefixes=None, limit=None):
    """批量解码整棵树。返回 (索引记录列表, 失败列表)。"""
    records = []
    failures = []
    for path in _iter_source(asset_root):
        rel = os.path.relpath(path, asset_root).replace(os.sep, "/")
        if include_prefixes and not any(rel.startswith(p) for p in include_prefixes):
            continue
        if limit is not None and len(records) >= limit:
            break
        try:
            records.append(unpack_file(asset_root, project_root, path))
        except (CodecError, OSError) as exc:
            failures.append({"path": rel, "error": str(exc)})
    return records, failures


def write_index(index_path: str, records) -> str:
    os.makedirs(os.path.dirname(index_path) or ".", exist_ok=True)
    with open(index_path, "w", encoding="utf-8", newline="") as fh:
        for record in sorted(records, key=lambda r: r["path"]):
            fh.write(json.dumps(record, ensure_ascii=False, sort_keys=True) + "\n")
    return index_path


def read_index(index_path: str):
    records = []
    with open(index_path, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if line:
                records.append(json.loads(line))
    return records


def _payload_from_project(project_root: str, record):
    data = _read(os.path.join(project_root, record["path"]))
    if record["payload_kind"] == "text":
        return Payload(kind="text", text=data.decode("utf-8"), meta=dict(record["meta"]))
    return Payload(kind="binary", blob=data, meta=dict(record["meta"]))


def pack_file(project_root: str, record, target: str = TARGET_PLAIN,
              text_policy: str = TEXT_VERBATIM) -> bytes:
    """由工程文件编码出二进制。

    target=plain 时输出解密后的载荷 (默认原样, 可选 UTF-8 规范化);
    target=original 时重新套回原版容器, 与源逐字节一致。
    """
    if text_policy not in TEXT_POLICIES:
        raise ValueError("未知文本策略: %s" % text_policy)
    payload = _payload_from_project(project_root, record)
    if target == TARGET_ORIGINAL:
        return _codec_by_name(record["codec"]).encode(payload)
    if payload.kind == "text" and text_policy == TEXT_UTF8_LF:
        """UTF-8 规范化: 去 BOM, 统一 LF; 只用于阅读与 diff, 不进客户端。"""
        return (payload.text or "").replace("\r\n", "\n").encode("utf-8")
    return payload.raw_bytes()


def pack_tree(project_root: str, index_path: str, out_root: str, target: str = TARGET_PLAIN,
              text_policy: str = TEXT_VERBATIM, include_prefixes=None, limit=None):
    """按索引编码整棵工程树, 返回逐文件结果。"""
    if target not in TARGETS:
        raise ValueError("未知编码目标: %s" % target)
    results = []
    for record in read_index(index_path):
        rel = record["path"]
        if include_prefixes and not any(rel.startswith(p) for p in include_prefixes):
            continue
        if limit is not None and len(results) >= limit:
            break
        try:
            data = pack_file(project_root, record, target, text_policy)
        except (CodecError, OSError) as exc:
            results.append({"path": rel, "ok": False, "error": str(exc)})
            continue
        _write(os.path.join(out_root, rel), data)
        same_as_source = sha256_bytes(data) == record["source_sha256"]
        results.append({"path": rel, "ok": target == TARGET_PLAIN or same_as_source,
                        "bytes": len(data), "byte_identical_to_source": same_as_source,
                        "sha256": sha256_bytes(data)})
    return results


def _codec_by_name(name: str) -> Codec:
    from .registry import all_codecs

    for codec in all_codecs():
        if codec.name == name:
            return codec
    raise CodecError("未知编解码器: %s" % name)
