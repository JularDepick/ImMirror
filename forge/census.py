# -*- coding: utf-8 -*-
"""素材清点: 逐文件判定容器族、编解码器与可读性, 产出 CSV 与汇总。

清点只读不写素材本体, 用于回答「哪些素材不可读、卡在哪个环节」。
"""

from __future__ import annotations

import collections
import csv
import os
import stat

from .codecs.base import CodecError
from .registry import select

"""CSV 列顺序固定, 供报告与后续工具消费。"""
COLUMNS = (
    "rel_path",
    "ext",
    "size",
    "magic",
    "codec",
    "fidelity",
    "readability",
    "encoding",
    "detail",
)


def iter_files(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            yield os.path.join(dirpath, name)


def readability_of(codec, payload) -> str:
    """以编解码器声明的可读性分级为准, 与是否产出视图无关。"""
    if codec.readability != "binary-opaque":
        return codec.readability
    return "binary-structured" if codec.view(payload) is not None else "binary-opaque"


def scan_file(root: str, path: str):
    rel = os.path.relpath(path, root).replace(os.sep, "/")
    try:
        size = os.path.getsize(path)
        with open(path, "rb") as fh:
            head = fh.read(64)
            fh.seek(0)
            block = fh.read()
    except OSError as exc:
        return {
            "rel_path": rel,
            "ext": os.path.splitext(rel)[1].lower(),
            "size": 0,
            "magic": "",
            "codec": "unreadable",
            "fidelity": "",
            "readability": "unreadable",
            "encoding": "",
            "detail": str(exc),
        }
    codec = select(rel, block)
    row = {
        "rel_path": rel,
        "ext": os.path.splitext(rel)[1].lower(),
        "size": size,
        "magic": head[:8].hex(),
        "codec": codec.name,
        "fidelity": codec.fidelity,
        "readability": "",
        "encoding": "",
        "detail": "",
    }
    try:
        payload = codec.decode(block)
    except CodecError as exc:
        row["readability"] = "decode-failed"
        row["detail"] = str(exc)
        return row
    row["readability"] = readability_of(codec, payload)
    row["encoding"] = payload.meta.get("encoding", "")
    view = codec.view(payload)
    """加密二进制族的载荷是标准格式或结构化容器, 下钻一层得到真实可读性。"""
    if payload.kind == "binary" and codec.name == "container-binary-xor16-head":
        inner = select(rel, payload.blob)
        if inner.name != codec.name:
            try:
                inner_payload = inner.decode(payload.blob)
            except CodecError as exc:
                row["detail"] = "载荷层解码失败: %s" % exc
                return row
            row["readability"] = readability_of(inner, inner_payload)
            view = inner.view(inner_payload)
            row["detail"] = "解密后为 " + inner.name
    if view is not None:
        row["detail"] = (row["detail"] + " 有结构视图").strip()
    return row


def census(root: str, out_csv: str):
    os.makedirs(os.path.dirname(out_csv) or ".", exist_ok=True)
    rows = []
    for path in iter_files(root):
        rows.append(scan_file(root, path))
    with open(out_csv, "w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=COLUMNS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    return summarize(rows)


def summarize(rows):
    """按 (编解码器, 扩展名) 汇总数量与体积, 供报告直接引用。"""
    groups = collections.defaultdict(lambda: {"count": 0, "bytes": 0, "readability": collections.Counter()})
    for row in rows:
        key = (row["codec"], row["ext"])
        g = groups[key]
        g["count"] += 1
        g["bytes"] += row["size"]
        g["readability"][row["readability"]] += 1
    total = {"count": len(rows), "bytes": sum(r["size"] for r in rows)}
    return {"groups": dict(groups), "total": total}
