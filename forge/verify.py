# -*- coding: utf-8 -*-
"""工程树与索引的一致性校验。

三级校验口径:

1. 结构级: 索引里的每个素材在工程树里都有对应文件, 工程树里没有索引之外的多余文件,
   记录声明了视图的素材其视图文件必须存在;
2. 内容级 (--deep): 把工程文件重新编码回原版容器, 与索引里的源摘要逐条比对;
3. 溯源级 (--source): 在候选源树里复查源文件的摘要, 确认索引本身没有失真。
"""

from __future__ import annotations

import hashlib
import json
import os

from .codecs.base import CodecError
from .project import TARGET_ORIGINAL, VIEW_DIR, pack_file, read_index

"""每类问题在报告里保留的样本条数, 总量单独统计。"""
MAX_LISTED = 20

"""单次读取的块大小, 摘要计算保持流式。"""
CHUNK = 1 << 20


def sha256_file(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(CHUNK), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _project_files(project_root: str):
    """枚举工程树的载荷文件。

    下划线开头的目录是派生视图 (`_view` 结构视图, `_png` 像素视图),
    它们由记录里的 view 与 side_files 字段单独核对, 不算索引之外的多余文件。
    """
    for dirpath, dirnames, filenames in os.walk(project_root):
        dirnames[:] = sorted(d for d in dirnames if d != VIEW_DIR and not d.startswith("_"))
        for name in sorted(filenames):
            path = os.path.join(dirpath, name)
            yield os.path.relpath(path, project_root).replace(os.sep, "/"), path


def verify_tree(project_root: str, index_path: str, deep: bool = False,
                source=None, limit=None) -> dict:
    """校验工程树与索引, 返回结构化结果。"""
    records = read_index(index_path)
    if limit is not None:
        records = records[:limit]
    indexed = {record["path"] for record in records}
    report = {
        "records": len(records),
        "project_files": 0,
        "missing": [],
        "missing_total": 0,
        "extra": [],
        "extra_total": 0,
        "view_missing": [],
        "view_missing_total": 0,
        "side_missing": [],
        "side_missing_total": 0,
        "deep": None,
        "source": None,
    }
    for rel, _path in _project_files(project_root):
        report["project_files"] += 1
        if limit is not None:
            """限流校验只看前 N 条记录, 此时全树多余文件没有意义, 不做判定。"""
            continue
        if rel not in indexed:
            report["extra_total"] += 1
            if len(report["extra"]) < MAX_LISTED:
                report["extra"].append(rel)
    for record in records:
        rel = record["path"]
        path = os.path.join(project_root, *rel.split("/"))
        if not os.path.isfile(path):
            report["missing_total"] += 1
            if len(report["missing"]) < MAX_LISTED:
                report["missing"].append(rel)
            continue
        view = record.get("view")
        if view and not os.path.isfile(os.path.join(project_root, *view.split("/"))):
            report["view_missing_total"] += 1
            if len(report["view_missing"]) < MAX_LISTED:
                report["view_missing"].append(view)
        for side in record.get("side_files") or ():
            if not os.path.isfile(os.path.join(project_root, *side.split("/"))):
                report["side_missing_total"] += 1
                if len(report["side_missing"]) < MAX_LISTED:
                    report["side_missing"].append(side)
    if deep:
        report["deep"] = _verify_deep(project_root, records)
    if source is not None:
        report["source"] = _verify_source(records, source)
    report["ok"] = not (report["missing_total"] or report["extra_total"]
                        or report["view_missing_total"] or report["side_missing_total"]
                        or (report["deep"] and report["deep"]["mismatch_total"])
                        or (report["source"] and report["source"]["mismatch_total"]))
    return report


def _verify_deep(project_root: str, records) -> dict:
    """重新编码每条记录并与源摘要比对。"""
    result = {"checked": 0, "mismatch_total": 0, "error_total": 0,
              "mismatch": [], "errors": []}
    for record in records:
        rel = record["path"]
        path = os.path.join(project_root, *rel.split("/"))
        if not os.path.isfile(path):
            continue
        try:
            data = pack_file(project_root, record, TARGET_ORIGINAL)
        except (CodecError, OSError, ValueError) as exc:
            result["error_total"] += 1
            if len(result["errors"]) < MAX_LISTED:
                result["errors"].append({"path": rel, "error": str(exc)})
            continue
        result["checked"] += 1
        digest = hashlib.sha256(data).hexdigest()
        if digest != record["source_sha256"]:
            result["mismatch_total"] += 1
            if len(result["mismatch"]) < MAX_LISTED:
                result["mismatch"].append({"path": rel, "codec": record["codec"],
                                           "size": len(data)})
    return result


def _verify_source(records, source) -> dict:
    """在候选源树里复查源文件摘要。"""
    result = {"checked": 0, "missing_total": 0, "mismatch_total": 0,
              "missing": [], "mismatch": []}
    for record in records:
        rel = record["path"]
        path = source.locate(rel)
        if path is None:
            result["missing_total"] += 1
            if len(result["missing"]) < MAX_LISTED:
                result["missing"].append(rel)
            continue
        digest = sha256_file(path)
        result["checked"] += 1
        if digest != record["source_sha256"]:
            result["mismatch_total"] += 1
            if len(result["mismatch"]) < MAX_LISTED:
                result["mismatch"].append({"path": rel, "size": os.path.getsize(path)})
    return result


def write_report(report: dict, out_path: str) -> str:
    """把校验结果写成 JSON, 便于后续工具消费。"""
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(report, fh, ensure_ascii=False, indent=1, sort_keys=True)
        fh.write("\n")
    return out_path
