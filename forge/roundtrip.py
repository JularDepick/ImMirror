# -*- coding: utf-8 -*-
"""端到端往返验证: 解码 -> 明文工程文件 -> 编码 -> 与原文件逐字节比对。

抽样按素材类别分层, 每个类别内部再按编解码器轮转, 保证一次验证覆盖全部格式族,
而不会因为某类素材数量多就把样本吃满。样本在临时工作目录里独立走一遍工程树
落盘与回读, 不触碰正式的 project/ 树。
"""

from __future__ import annotations

import collections
import csv
import json
import os
import random
import shutil
import time

from .codecs.base import CodecError, Payload
from .project import TARGET_ORIGINAL, pack_file, read_index, sha256_bytes
from .registry import select

"""类别划分按扩展名与编解码器判定, 顺序即优先级。"""
CATEGORIES = (
    "数据表 csv",
    "命名区块容器",
    "纹理 ktx",
    "音频 ogg",
    "图标集与界面",
    "体素与世界存档",
    "Android 二进制 XML",
    "标准格式与其他",
)

"""每个类别至少抽这么多条 (类别总量不足时按总量取)。"""
MIN_PER_CATEGORY = 4

"""体素与世界存档的扩展名; .r119085 与 .r119179 是命名区块容器, 但归属世界存档。"""
WORLD_EXTS = (".vmo", ".vox", ".r", ".r119085", ".r119179")


def category_of(rel_path: str, codec_name: str, ext: str) -> str:
    if ext == ".csv":
        return "数据表 csv"
    if ext in WORLD_EXTS or rel_path.split("/")[0] in ("voxel", "worlds"):
        return "体素与世界存档"
    if codec_name == "named-block":
        return "命名区块容器"
    if ext == ".ktx" or codec_name == "ktx11":
        return "纹理 ktx"
    if ext == ".ogg" or codec_name == "ogg":
        return "音频 ogg"
    if codec_name == "axml":
        return "Android 二进制 XML"
    if rel_path.split("/")[0] == "ui":
        return "图标集与界面"
    return "标准格式与其他"


def _codec_order(name: str):
    return name


def plan_samples(records, total: int, seed: int = 0):
    """按类别分层抽样, 返回抽样记录列表 (顺序稳定)。"""
    buckets = collections.defaultdict(lambda: collections.defaultdict(list))
    for record in records:
        ext = os.path.splitext(record["path"])[1].lower()
        category = category_of(record["path"], record["codec"], ext)
        buckets[category][record["codec"]].append(record)
    counts = {category: sum(len(v) for v in codecs.values())
              for category, codecs in buckets.items()}
    quota = _allocate(counts, total)
    rng = random.Random(seed)
    picked = []
    for category in CATEGORIES:
        if category not in buckets or quota.get(category, 0) <= 0:
            continue
        picked.extend(_pick_in_category(buckets[category], quota[category], rng))
    return picked


def _allocate(counts, total: int):
    """按总量比例分配名额, 先保底再按最大余数补齐。"""
    available = sum(counts.values())
    if total >= available:
        return dict(counts)
    quota = {}
    for category, count in counts.items():
        quota[category] = min(count, MIN_PER_CATEGORY)
    assigned = sum(quota.values())
    remaining = total - assigned
    if remaining < 0:
        """名额比保底还少时按比例缩小。"""
        scale = total / float(assigned)
        return {c: max(1, int(v * scale)) for c, v in quota.items()}
    if remaining == 0:
        return quota
    weight = {category: min(counts[category], quota[category] * 4) for category in counts}
    weight_total = sum(weight.values()) or 1
    shares = []
    for category, count in counts.items():
        ideal = remaining * weight[category] / float(weight_total)
        room = count - quota[category]
        add = min(room, int(ideal))
        quota[category] += add
        shares.append((ideal - int(ideal), category))
    leftover = total - sum(quota.values())
    shares.sort(reverse=True)
    while leftover > 0:
        progressed = False
        for _frac, category in shares:
            if leftover <= 0:
                break
            if quota[category] < counts[category]:
                quota[category] += 1
                leftover -= 1
                progressed = True
        if not progressed:
            break
    return quota


def _pick_in_category(by_codec, quota: int, rng):
    """类别内部按编解码器轮转抽取, 各编码器内部随机打散。"""
    names = sorted(by_codec)
    pools = {name: list(by_codec[name]) for name in names}
    for name in names:
        rng.shuffle(pools[name])
    picked = []
    index = 0
    while len(picked) < quota:
        name = names[index % len(names)]
        index += 1
        if pools[name]:
            picked.append(pools[name].pop())
        elif all(not pools[n] for n in names):
            break
    return picked


def _read(path: str) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _write(path: str, data: bytes) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(data)


def _load_source(record, source, project_root):
    """取样本的原始字节: 磁盘源优先, 定位不到时按索引从工程树回写。"""
    path = source.locate(record["path"]) if source is not None else None
    if path is not None:
        return _read(path), "disk"
    if project_root is not None:
        return pack_file(project_root, record, TARGET_ORIGINAL), "rebuilt"
    raise CodecError("源文件缺失且未提供工程树: %s" % record["path"])


def run_sample(record, source, work_root: str, project_root: str = None) -> dict:
    """跑一个样本的完整往返, 返回结果行。"""
    rel = record["path"]
    ext = os.path.splitext(rel)[1].lower()
    row = {
        "path": rel,
        "category": category_of(rel, record.get("codec", ""), ext),
        "codec": record.get("codec", ""),
        "fidelity": "",
        "readability": "",
        "size": 0,
        "encoding": "",
        "eol": "",
        "plain_kind": "",
        "plain_bytes": 0,
        "source_mode": "",
        "decode_ms": 0,
        "encode_ms": 0,
        "status": "ok",
        "detail": "",
    }
    plain_path = os.path.join(work_root, "plain", *rel.split("/"))
    try:
        data, row["source_mode"] = _load_source(record, source, project_root)
        row["size"] = len(data)
        codec = select(rel, data)
        row["codec"] = codec.name
        row["fidelity"] = codec.fidelity
        row["readability"] = codec.readability
        row["category"] = category_of(rel, codec.name, ext)
        started = time.perf_counter()
        payload = codec.decode(data)
        row["decode_ms"] = round((time.perf_counter() - started) * 1000.0, 3)
        row["plain_kind"] = payload.kind
        row["encoding"] = payload.meta.get("encoding", "")
        row["eol"] = payload.meta.get("eol", "")
        plain = (payload.text or "").encode("utf-8") if payload.kind == "text" else (payload.blob or b"")
        _write(plain_path, plain)
        row["plain_bytes"] = len(plain)
        """回读明文工程文件, 重建载荷; 文本以 UTF-8 落盘, 索引里的原始编码负责还原。"""
        back_raw = _read(plain_path)
        if payload.kind == "text":
            back = Payload(kind="text", text=back_raw.decode("utf-8"), meta=dict(payload.meta))
        else:
            back = Payload(kind="binary", blob=back_raw, meta=dict(payload.meta))
        started = time.perf_counter()
        out = codec.encode(back)
        row["encode_ms"] = round((time.perf_counter() - started) * 1000.0, 3)
        if out != data:
            row["status"] = "mismatch"
            row["detail"] = "编码结果不同: %d 字节 vs 源 %d 字节, 源摘要 %s, 产物摘要 %s" % (
                len(out), len(data), sha256_bytes(data)[:16], sha256_bytes(out)[:16])
    except (CodecError, OSError, UnicodeDecodeError, ValueError) as exc:
        row["status"] = "error"
        row["detail"] = "%s: %s" % (type(exc).__name__, exc)
    return row


def run(records, source, work_root: str, total: int, seed: int = 0,
        project_root: str = None) -> dict:
    """抽样并逐个验证, 返回汇总。"""
    if os.path.isdir(work_root):
        shutil.rmtree(work_root)
    os.makedirs(work_root, exist_ok=True)
    picked = plan_samples(records, total, seed)
    rows = [run_sample(record, source, work_root, project_root) for record in picked]
    return {"rows": rows, "planned": len(picked), "available": len(records)}


def summarize(rows):
    """按类别与编解码器汇总。"""
    by_category = collections.OrderedDict()
    for name in CATEGORIES:
        by_category[name] = {"total": 0, "ok": 0, "fail": 0}
    by_codec = collections.OrderedDict()
    encodings = collections.Counter()
    for row in rows:
        cat = by_category.setdefault(row["category"], {"total": 0, "ok": 0, "fail": 0})
        cat["total"] += 1
        if row["status"] == "ok":
            cat["ok"] += 1
        else:
            cat["fail"] += 1
        codec = by_codec.setdefault(row["codec"], {
            "total": 0, "ok": 0, "fail": 0,
            "fidelity": row["fidelity"], "readability": row["readability"]})
        codec["total"] += 1
        if row["status"] == "ok":
            codec["ok"] += 1
        else:
            codec["fail"] += 1
        if row["encoding"]:
            encodings[row["encoding"]] += 1
    return {"by_category": by_category, "by_codec": by_codec, "encodings": encodings}


def write_csv(rows, out_path: str) -> str:
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    columns = ("path", "category", "codec", "fidelity", "readability", "size",
               "encoding", "eol", "plain_kind", "plain_bytes", "source_mode",
               "decode_ms", "encode_ms", "status", "detail")
    with open(out_path, "w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=columns, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    return out_path


def write_json(payload, out_path: str) -> str:
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(payload, fh, ensure_ascii=False, indent=1, sort_keys=True)
        fh.write("\n")
    return out_path

def write_markdown(summary, rows, meta, out_path: str) -> str:
    """把验证结果写成 markdown 报告, 供 reports/ 入库。"""
    lines = []
    failures = [row for row in rows if row["status"] != "ok"]
    passed = len(rows) - len(failures)
    lines.append("# 端到端往返验证报告")
    lines.append("")
    lines.append("口径: 解码 -> 明文工程文件 (文本以 UTF-8 落盘) -> 回读 -> 编码回原版容器 "
                 "-> 与原文件逐字节比对。样本按类别分层, 类别内按编解码器轮转抽取。")
    lines.append("")
    lines.append("## 一. 结论")
    lines.append("")
    lines.append("| 项 | 数值 |")
    lines.append("|:---:|:---:|")
    lines.append("| 抽样条数 | %d |" % len(rows))
    lines.append("| 索引可用条数 | %d |" % meta.get("available", 0))
    lines.append("| 覆盖类别 | %d |" % len(summary["by_category"]))
    lines.append("| 覆盖编解码器 | %d |" % len(summary["by_codec"]))
    lines.append("| 逐字节一致 | %d |" % passed)
    lines.append("| 失败 | %d |" % len(failures))
    lines.append("| 样本取自磁盘源树 | %d |" % (len(rows) - meta.get("rebuilt", 0)))
    lines.append("| 样本按索引由工程树回写 | %d |" % meta.get("rebuilt", 0))
    lines.append("")
    lines.append("## 二. 类别汇总")
    lines.append("")
    lines.append("| 类别 | 抽样 | 通过 | 失败 |")
    lines.append("|:---:|:---:|:---:|:---:|")
    for name, group in summary["by_category"].items():
        if not group["total"]:
            continue
        lines.append("| %s | %d | %d | %d |" % (name, group["total"], group["ok"], group["fail"]))
    lines.append("")
    lines.append("## 三. 编解码器汇总")
    lines.append("")
    lines.append("| 编解码器 | 保真级别 | 可读性 | 抽样 | 通过 | 失败 |")
    lines.append("|:---:|:---:|:---:|:---:|:---:|:---:|")
    for name, group in summary["by_codec"].items():
        lines.append("| %s | %s | %s | %d | %d | %d |"
                     % (name, group["fidelity"] or "-", group["readability"] or "-",
                        group["total"], group["ok"], group["fail"]))
    lines.append("")
    lines.append("## 四. 文本编码分布")
    lines.append("")
    if summary["encodings"]:
        lines.append("| 编码 | 条数 |")
        lines.append("|:---:|:---:|")
        for name, count in summary["encodings"].most_common():
            lines.append("| %s | %d |" % (name, count))
    else:
        lines.append("抽样里没有文本载荷。")
    lines.append("")
    lines.append("## 五. 失败清单")
    lines.append("")
    if failures:
        lines.append("| 路径 | 类别 | 编解码器 | 阶段 | 详情 |")
        lines.append("|:---:|:---:|:---:|:---:|:---|")
        for row in failures[:50]:
            lines.append("| %s | %s | %s | %s | %s |"
                         % (row["path"], row["category"], row["codec"], row["status"],
                            row["detail"].replace("|", "/")))
    else:
        lines.append("无失败样本。")
    lines.append("")
    lines.append("## 六. 复现")
    lines.append("")
    lines.append("```bash")
    lines.append("python -m forge.cli mirror")
    lines.append("python -m forge.cli roundtrip --samples %d --seed %d"
                 % (len(rows), meta.get("seed", 0)))
    lines.append("```")
    lines.append("")
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines))
    return out_path
