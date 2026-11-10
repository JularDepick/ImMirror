# -*- coding: utf-8 -*-
"""脚手架命令行入口。

用法:

    python -m scaffolds list
    python -m scaffolds <分类> export <源二进制或目录> [-o 工程目录]
    python -m scaffolds <分类> build  <工程目录> [-o 输出二进制]
    python -m scaffolds <分类> verify <源二进制...> [--work 目录]
    python -m scaffolds textures iconset <图集目录> -o <工程目录>
    python -m scaffolds textures iconset-build <工程目录> -o <输出目录>

export 的默认工程目录是 temp/scaffolds/<分类>/<文件名>; build 的默认输出是
工程目录下的 build/<源文件名>。verify 会对每个样本做 导出 -> 回写 -> 逐字节比对。
"""

from __future__ import annotations

import argparse
import os
import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

from . import CATEGORY_NAMES, list_categories, load_category
from . import util

"""assets 仓库根目录, 默认产物落在这里的 temp/scaffolds 下。"""
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_WORK = os.path.join(ROOT, "temp", "scaffolds")


def _flat_name(path: str) -> str:
    """把相对路径压成可用于目录名的一层名字。"""
    name = os.path.basename(os.path.normpath(path))
    stem, ext = os.path.splitext(name)
    return (stem or name) + (ext.replace(".", "_") if ext else "")


def _default_export_dir(category: str, src: str) -> str:
    return os.path.join(DEFAULT_WORK, category, _flat_name(src))


def _default_build_path(project_dir: str, module) -> str:
    doc_path = util.find_project_json(project_dir, getattr(module, "PROJECT_FILE", "project.json"))
    doc = util.read_json(doc_path)
    source = doc.get("source") or {}
    name = source.get("name") or (os.path.basename(os.path.normpath(project_dir)) + ".bin")
    return os.path.join(project_dir, "build", name)


def _print_export(result: dict) -> None:
    print("导出 %s (%s)" % (result["project"], result["format"]))
    for path in result.get("files", []):
        print("  + %s" % path)
    if "blocks" in result:
        print("  块数 %d" % result["blocks"])
    if "chunks" in result:
        print("  块数 %d" % result["chunks"])
    if "pixels" in result:
        print("  像素视图 %s" % ("有" if result["pixels"] else "无"))
    if "encoding" in result:
        print("  编码 %s, 换行 %s" % (result.get("encoding"), result.get("eol")))
    if "envelope" in result and result["envelope"]:
        print("  外包壳 %s" % result["envelope"])


def _print_build(result: dict) -> None:
    status = "逐字节一致" if result["byte_identical"] else "与源不同"
    print("回写 %s -> %s" % (result["project"], result["out"]))
    print("  大小 %d 字节, sha256 %s, %s" % (result["size"], result["sha256"][:16], status))
    if result.get("reencoded"):
        print("  像素已重新压块 (PNG 有改动)")
    if result.get("header_size_updated"):
        print("  文件头 size 已按块序列重算")


def cmd_list(args) -> int:
    print("分类        标题        受理扩展名                              格式")
    for item in list_categories():
        print(
            "%-11s %-11s %-40s %s"
            % (
                item["name"],
                item["title"],
                " ".join(item["extensions"]),
                ", ".join(item["formats"].keys()),
            )
        )
    return 0


def cmd_export(args) -> int:
    module = args.module
    category = args.category
    sources = util.iter_source_files(args.sources)
    if not sources:
        print("没有可导出的文件", file=sys.stderr)
        return 2
    multiple = len(sources) > 1 or os.path.isdir(args.sources[0])
    failures = 0
    for src in sources:
        if args.out and not multiple:
            project_dir = args.out
        elif args.out:
            project_dir = os.path.join(args.out, _flat_name(src))
        else:
            project_dir = _default_export_dir(category, src)
        try:
            result = module.export(src, project_dir)
        except (util.ScaffoldError, OSError) as exc:
            failures += 1
            print("[失败] %s: %s" % (src, exc), file=sys.stderr)
            continue
        _print_export(result)
    return 0 if not failures else 1


def cmd_build(args) -> int:
    module = args.module
    project_dir = args.project
    out_path = args.out or _default_build_path(project_dir, module)
    try:
        result = module.build(project_dir, out_path)
    except (util.ScaffoldError, OSError) as exc:
        print("[失败] %s: %s" % (project_dir, exc), file=sys.stderr)
        return 1
    _print_build(result)
    return 0 if result["byte_identical"] else 0


def cmd_verify(args) -> int:
    module = args.module
    category = args.category
    sources = util.iter_source_files(args.sources)
    if not sources:
        print("没有可验证的文件", file=sys.stderr)
        return 2
    work_root = args.work or os.path.join(DEFAULT_WORK, "_verify", category)
    total = 0
    matched = 0
    rows = []
    for src in sources:
        total += 1
        project_dir = os.path.join(work_root, _flat_name(src))
        out_path = os.path.join(project_dir, "build", os.path.basename(src))
        try:
            module.export(src, project_dir)
            result = module.build(project_dir, out_path)
            original = util.read_bytes(src)
            same = util.sha256_bytes(original) == result["sha256"]
        except (util.ScaffoldError, OSError) as exc:
            rows.append((os.path.relpath(src, ROOT), "失败", str(exc)))
            continue
        if same:
            matched += 1
        rows.append(
            (
                os.path.relpath(src, ROOT),
                "一致" if same else "不一致",
                "%d 字节, %s" % (len(original), result["format"]),
            )
        )
    for path, status, detail in rows:
        print("%-6s %-70s %s" % (status, path, detail))
    print("汇总: %d/%d 逐字节一致 (%s)" % (matched, total, category))
    return 0 if matched == total else 1


def cmd_iconset(args) -> int:
    module = args.module
    if args.action == "iconset":
        result = module.export_iconset(args.path, args.out, getattr(args, "prefix", "") or "")
        print("导出图集 %s: %d 页" % (result["project"], result["pages"]))
        return 0
    result = module.build_iconset(args.path, args.out)
    failures = 0
    for item in result["results"]:
        status = "一致" if item["byte_identical"] else "不同"
        if not item["byte_identical"]:
            failures += 1
        print("%-4s %s" % (status, item["out"]))
    print("汇总: %d/%d 逐字节一致" % (len(result["results"]) - failures, len(result["results"])))
    return 0 if not failures else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m scaffolds",
        description="按分类把素材二进制与明文工程目录双向转换",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("list", help="列出全部分类与受理扩展名").set_defaults(func=cmd_list)

    for category in CATEGORY_NAMES:
        module = load_category(category)
        title = getattr(module, "TITLE", category)
        category_parser = subparsers.add_parser(category, help="%s (%s)" % (title, category))
        actions = category_parser.add_subparsers(dest="action", required=True)

        export_parser = actions.add_parser("export", help="二进制 -> 明文工程目录")
        export_parser.add_argument("sources", nargs="+", help="源二进制文件或目录")
        export_parser.add_argument("-o", "--out", default=None, help="工程目录")
        export_parser.set_defaults(func=cmd_export, module=module, category=category)

        build_parser_ = actions.add_parser("build", help="明文工程目录 -> 二进制")
        build_parser_.add_argument("project", help="工程目录")
        build_parser_.add_argument("-o", "--out", default=None, help="输出二进制路径")
        build_parser_.set_defaults(func=cmd_build, module=module, category=category)

        verify_parser = actions.add_parser("verify", help="导出后回写并逐字节比对")
        verify_parser.add_argument("sources", nargs="+", help="源二进制文件或目录")
        verify_parser.add_argument("--work", default=None, help="验证工作目录")
        verify_parser.set_defaults(func=cmd_verify, module=module, category=category)

        if category == "textures":
            iconset_parser = actions.add_parser("iconset", help="把图集目录逐页导出为一个工程")
            iconset_parser.add_argument("path", help="图集目录")
            iconset_parser.add_argument("-o", "--out", required=True, help="工程目录")
            iconset_parser.add_argument("--prefix", default="", help="只导出以该前缀开头的页")
            iconset_parser.set_defaults(func=cmd_iconset, module=module, category=category,
                                        action="iconset")

            build_set_parser = actions.add_parser("iconset-build", help="按图集工程回写全部页")
            build_set_parser.add_argument("path", help="图集工程目录")
            build_set_parser.add_argument("-o", "--out", required=True, help="输出目录")
            build_set_parser.set_defaults(func=cmd_iconset, module=module, category=category,
                                          action="iconset-build")
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return args.func(args)
    except util.ScaffoldError as exc:
        print("[失败] %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
