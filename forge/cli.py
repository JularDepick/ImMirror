# -*- coding: utf-8 -*-
"""AssetForge 命令行入口。

    python -m forge.cli formats
    python -m forge.cli census    [--root 目录] [--out 报告.csv]
    python -m forge.cli mirror    [--index 文件] [--out 目录] [--source-root 目录 ...] [--copy]
    python -m forge.cli unpack    [--root 目录] [--project 目录] [--index 文件] [--include 前缀 ...]
    python -m forge.cli pack      [--target plain|original] [--project 目录] [--out 目录]
    python -m forge.cli verify    [--project 目录] [--index 文件] [--deep] [--source-root 目录 ...]
    python -m forge.cli roundtrip [--index 文件] [--samples 200] [--out 报告.md] [--csv 明细.csv]

默认路径: 资源树 ../client/src/assets, 工程树 ./project, 索引 ./reports/project-index.jsonl,
明文产物 ./build, 源树镜像 ./build-source, 往返工作区 ./temp/roundtrip-work。
"""

from __future__ import annotations

import argparse
import json
import os
import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

from .census import census
from .project import (TARGET_PLAIN, TARGETS, TEXT_POLICIES, TEXT_VERBATIM, pack_tree,
                      read_index, unpack_tree, write_index)
from .registry import all_codecs
from .roundtrip import run as run_roundtrip
from .roundtrip import summarize as summarize_roundtrip
from .roundtrip import write_csv as write_roundtrip_csv
from .roundtrip import write_json as write_roundtrip_json
from .roundtrip import write_markdown as write_roundtrip_markdown
from .source import DEFAULT_SOURCE_ROOTS, SourceTree, mirror_tree, normalize_roots
from .verify import verify_tree, write_report as write_verify_report

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_ASSET_ROOT = os.path.join(ROOT, "..", "client", "src", "assets")
DEFAULT_PROJECT = os.path.join(ROOT, "project")
DEFAULT_BUILD = os.path.join(ROOT, "build")
DEFAULT_SOURCE_MIRROR = os.path.join(ROOT, "build-source")
DEFAULT_CENSUS = os.path.join(ROOT, "reports", "census.csv")
DEFAULT_INDEX = os.path.join(ROOT, "reports", "project-index.jsonl")
DEFAULT_ROUNDTRIP_WORK = os.path.join(ROOT, "temp", "roundtrip-work")
DEFAULT_ROUNDTRIP_MD = os.path.join(ROOT, "reports", "roundtrip-200.md")
DEFAULT_ROUNDTRIP_CSV = os.path.join(ROOT, "reports", "roundtrip-200.csv")
DEFAULT_VERIFY_JSON = os.path.join(ROOT, "reports", "verify.json")


def _source(args, with_mirror: bool = False) -> SourceTree:
    """按参数或默认候选根构造源树定位器。

    校验类命令优先读本地镜像树, 再退回真实源树; 建镜像的命令不能把镜像自己当源,
    否则改不了任何缺失项。
    """
    if args.source_root:
        roots = normalize_roots(args.source_root, ROOT)
    elif with_mirror:
        roots = normalize_roots((DEFAULT_SOURCE_MIRROR,) + DEFAULT_SOURCE_ROOTS, ROOT)
    else:
        roots = normalize_roots(DEFAULT_SOURCE_ROOTS, ROOT)
    return SourceTree(roots)


def cmd_formats(args) -> int:
    print("%-28s %-22s %-20s %s" % ("编解码器", "保真级别", "可读性", "扩展名"))
    for codec in all_codecs():
        print("%-28s %-22s %-20s %s" % (codec.name, codec.fidelity, codec.readability,
                                        ",".join(codec.extensions) or "-"))
    return 0


def cmd_census(args) -> int:
    summary = census(args.root, args.out)
    print("%-30s %-8s %8s %14s  %s" % ("编解码器", "扩展名", "数量", "字节", "可读性分布"))
    for (codec, ext), g in sorted(summary["groups"].items(), key=lambda kv: -kv[1]["bytes"]):
        dist = " ".join("%s:%d" % (k, v) for k, v in g["readability"].most_common())
        print("%-30s %-8s %8d %14d  %s" % (codec, ext or "-", g["count"], g["bytes"], dist))
    total = summary["total"]
    print("合计: %d 个文件, %d 字节" % (total["count"], total["bytes"]))
    print("报告已写入: %s" % args.out)
    return 0


def cmd_mirror(args) -> int:
    records = read_index(args.index)
    source = _source(args)
    project = None if args.no_rebuild else args.project
    stats = mirror_tree(records, source, args.out, hardlink=not args.copy,
                        project_root=project)
    print("源树镜像: %d 个文件, 硬链接 %d, 复制 %d, 按索引回写 %d (新占用 %d 字节)"
          % (stats["linked"] + stats["copied"] + stats["rebuilt"], stats["linked"],
             stats["copied"], stats["rebuilt"], stats["bytes"]))
    print("候选根: %s" % ", ".join(source.roots))
    if stats["rebuilt_mismatch"]:
        print("回写结果与源摘要不符: %d 个" % stats["rebuilt_mismatch"])
    if stats["missing_total"]:
        print("无法重建的源文件: %d 个" % stats["missing_total"])
        for rel in stats["missing"][:10]:
            print("  [缺失] %s" % rel)
    print("镜像目录: %s" % args.out)
    return 0 if not (stats["missing_total"] or stats["rebuilt_mismatch"]) else 1


def cmd_unpack(args) -> int:
    records, failures = unpack_tree(
        args.root, args.project, include_prefixes=args.include or None, limit=args.limit
    )
    path = write_index(args.index, records)
    print("解码 %d 个素材, 失败 %d 个" % (len(records), len(failures)))
    for item in failures[:20]:
        print("  [失败] %s: %s" % (item["path"], item["error"]))
    print("索引: %s" % path)
    return 0 if not failures else 1


def cmd_pack(args) -> int:
    results = pack_tree(args.project, args.index, args.out, args.target,
                        text_policy=args.text_policy, include_prefixes=args.include or None)
    bad = [r for r in results if not r["ok"]]
    if args.target == "original":
        same = sum(1 for r in results if r.get("byte_identical_to_source"))
        print("编码目标 original: %d 个素材, 与源逐字节一致 %d 个" % (len(results), same))
    else:
        print("编码目标 plain (文本策略 %s): %d 个素材, 失败 %d 个"
              % (args.text_policy, len(results), len(bad)))
    for item in bad[:20]:
        print("  [失败] %s %s" % (item["path"], item.get("error", "")))
    print("产物目录: %s" % args.out)
    return 0 if not bad else 1


def cmd_verify(args) -> int:
    """--source 只查真实源树: 镜像里的回写项由工程树生成, 复查它会变成自证。"""
    source = _source(args) if args.source else None
    report = verify_tree(args.project, args.index, deep=args.deep, source=source,
                         limit=args.limit)
    print("索引记录: %d 条, 工程文件: %d 个" % (report["records"], report["project_files"]))
    print("工程文件缺失: %d 条" % report["missing_total"])
    for item in report["missing"][:10]:
        print("  [缺失] %s" % item)
    print("索引之外的多余文件: %d 条" % report["extra_total"])
    for item in report["extra"][:10]:
        print("  [多余] %s" % item)
    print("视图缺失: %d 条" % report["view_missing_total"])
    if report["deep"]:
        deep = report["deep"]
        print("深度重编码: %d 条, 摘要不符 %d 条, 编码异常 %d 条"
              % (deep["checked"], deep["mismatch_total"], deep["error_total"]))
        for item in (deep["mismatch"] + deep["errors"])[:10]:
            print("  [不符] %s %s" % (item["path"], item.get("error", "")))
    if report["source"]:
        src = report["source"]
        print("源文件复查: %d 条, 缺失 %d 条, 摘要不符 %d 条"
              % (src["checked"], src["missing_total"], src["mismatch_total"]))
    if args.json:
        write_verify_report(report, args.json)
        print("报告: %s" % args.json)
    print("结论: %s" % ("通过" if report["ok"] else "未通过"))
    return 0 if report["ok"] else 1


def cmd_roundtrip(args) -> int:
    records = read_index(args.index)
    source = _source(args, with_mirror=True)
    project = None if args.no_rebuild else args.project
    outcome = run_roundtrip(records, source, args.work, args.samples, args.seed, project)
    rows = outcome["rows"]
    summary = summarize_roundtrip(rows)
    meta = {
        "samples": outcome["planned"],
        "available": outcome["available"],
        "seed": args.seed,
        "roots": source.roots,
        "rebuilt": sum(1 for row in rows if row.get("source_mode") == "rebuilt"),
    }
    print("%-22s %6s %6s %6s %6s" % ("类别", "抽样", "通过", "失败", "总计"))
    for name, group in summary["by_category"].items():
        print("%-22s %6d %6d %6d %6d" % (name, group["total"], group["ok"], group["fail"], group["total"]))
    print("%-28s %6s %6s %s" % ("编解码器", "抽样", "通过", "保真级别"))
    for name, group in summary["by_codec"].items():
        print("%-28s %6d %6d %s" % (name, group["total"], group["ok"], group["fidelity"]))
    write_roundtrip_csv(rows, args.csv)
    write_roundtrip_markdown(summary, rows, meta, args.out)
    if args.json:
        write_roundtrip_json({"meta": meta, "summary": summary, "rows": rows}, args.json)
    failures = [row for row in rows if row["status"] != "ok"]
    print("明细: %s" % args.csv)
    print("报告: %s" % args.out)
    print("结论: 抽样 %d 个, 逐字节一致 %d 个, 失败 %d 个"
          % (len(rows), len(rows) - len(failures), len(failures)))
    return 0 if not failures else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="forge", description="ImMirror 资源管线")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("formats", help="列出已接入的编解码器")
    p.set_defaults(func=cmd_formats)

    p = sub.add_parser("census", help="清点资源树并产出 CSV 报告")
    p.add_argument("--root", default=DEFAULT_ASSET_ROOT)
    p.add_argument("--out", default=DEFAULT_CENSUS)
    p.set_defaults(func=cmd_census)

    p = sub.add_parser("mirror", help="按索引把候选源树重建成本地镜像树")
    p.add_argument("--index", default=DEFAULT_INDEX)
    p.add_argument("--out", default=DEFAULT_SOURCE_MIRROR)
    p.add_argument("--source-root", action="append", default=None,
                   help="候选源根, 可重复; 默认依次尝试客户端资源树与逆向工程树")
    p.add_argument("--project", default=DEFAULT_PROJECT,
                   help="工程树; 磁盘上定位不到的源文件按索引从这里回写")
    p.add_argument("--no-rebuild", action="store_true", help="只做硬链接与复制, 不回写缺失项")
    p.add_argument("--copy", action="store_true", help="逐文件复制而不是硬链接")
    p.set_defaults(func=cmd_mirror)

    p = sub.add_parser("unpack", help="解码资源树到工程树")
    p.add_argument("--root", default=DEFAULT_ASSET_ROOT)
    p.add_argument("--project", default=DEFAULT_PROJECT)
    p.add_argument("--index", default=DEFAULT_INDEX)
    p.add_argument("--include", nargs="*", default=None, help="只处理这些相对路径前缀")
    p.add_argument("--limit", type=int, default=None)
    p.set_defaults(func=cmd_unpack)

    p = sub.add_parser("pack", help="由工程树编码出二进制")
    p.add_argument("--project", default=DEFAULT_PROJECT)
    p.add_argument("--index", default=DEFAULT_INDEX)
    p.add_argument("--out", default=DEFAULT_BUILD)
    p.add_argument("--target", choices=TARGETS, default=TARGET_PLAIN)
    p.add_argument("--text-policy", choices=TEXT_POLICIES, default=TEXT_VERBATIM,
                   help="plain 目标的文本处理: verbatim 保留原编码, utf8-lf 转 UTF-8 并统一换行")
    p.add_argument("--include", nargs="*", default=None, help="只处理这些相对路径前缀")
    p.set_defaults(func=cmd_pack)

    p = sub.add_parser("verify", help="校验工程树与索引完整性")
    p.add_argument("--project", default=DEFAULT_PROJECT)
    p.add_argument("--index", default=DEFAULT_INDEX)
    p.add_argument("--deep", action="store_true", help="重新编码每条记录并与源摘要比对")
    p.add_argument("--source", action="store_true", help="在候选源树里复查源文件摘要")
    p.add_argument("--source-root", action="append", default=None)
    p.add_argument("--limit", type=int, default=None)
    p.add_argument("--json", default=None, help="把结构化结论写到该文件")
    p.set_defaults(func=cmd_verify)

    p = sub.add_parser("roundtrip", help="抽样做解码到编码的逐字节往返验证")
    p.add_argument("--index", default=DEFAULT_INDEX)
    p.add_argument("--project", default=DEFAULT_PROJECT,
                   help="工程树; 磁盘上定位不到的样本按索引从这里回写后再验")
    p.add_argument("--no-rebuild", action="store_true", help="磁盘上定位不到的样本直接判失败")
    p.add_argument("--source-root", action="append", default=None)
    p.add_argument("--samples", type=int, default=200)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--work", default=DEFAULT_ROUNDTRIP_WORK)
    p.add_argument("--out", default=DEFAULT_ROUNDTRIP_MD)
    p.add_argument("--csv", default=DEFAULT_ROUNDTRIP_CSV)
    p.add_argument("--json", default=None)
    p.set_defaults(func=cmd_roundtrip)
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
