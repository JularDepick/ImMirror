# -*- coding: utf-8 -*-
"""KTX 纹理与 PNG 工程文件之间的双向转换。

管线约定:

- 解码时旁挂 PNG: 工程树里 <rel> 是原始 KTX 载荷, <PNG_DIR>/<rel 换扩展名>.png 是 0 级像素;
- 回写时以 PNG 为准: 像素有改动就重压块并重算 mip 链, 像素没改动则原样保留 KTX;
- 客户端资源树里 .ktx 可能套了二进制族容器, 读取时自动剥壳, 写回一律写裸 KTX,
  由 forge pack 负责重新套容器。

命令行:

    python -m forge.ktx_png stats   [--root 资源树]
    python -m forge.ktx_png export  [--root 资源树] [--out 目录] [--limit 数量]
    python -m forge.ktx_png import  [--project 工程树] [--limit 数量]
    python -m forge.ktx_png check   [--project 工程树] [--limit 数量]
"""

from __future__ import annotations

import argparse
import os
import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

from .codecs import etc, ktx
from .codecs.base import CodecError
from .codecs.container import MAGIC_BINARY, xor_stream
from .util import png

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_ASSET_ROOT = os.path.join(ROOT, "..", "client", "src", "assets")
DEFAULT_PROJECT = os.path.join(ROOT, "project")

PNG_DIR = ktx.PNG_DIR

"""回写自检的像素误差阈值: 平均误差与最大误差都不得越界。"""
SELF_CHECK_MEAN_LIMIT = 6.0
SELF_CHECK_MAX_LIMIT = 96


def unwrap(data: bytes) -> bytes:
    """剥掉二进制族容器, 拿到裸 KTX; 本来就是裸 KTX 时原样返回。"""
    if data[:4] == MAGIC_BINARY:
        prefix = int.from_bytes(data[4:8], "big")
        if 8 + prefix > len(data):
            raise CodecError("二进制族容器声明的前缀长度越界: %d" % prefix)
        return xor_stream(data[8 : 8 + prefix]) + data[8 + prefix :]
    return data


def is_ktx(data: bytes) -> bool:
    return data[:12] == ktx.MAGIC_KTX11


def side_files(rel: str, payload):
    """给一个载荷产出旁挂 PNG; 只对含 KTX 载荷的素材生效。"""
    blob = payload.blob or b""
    if not blob:
        return ()
    try:
        plain = unwrap(blob)
    except CodecError:
        return ()
    if not is_ktx(plain):
        return ()
    try:
        texture = ktx.parse(plain)
    except CodecError:
        return ()
    if texture.codec() is None:
        return ()
    try:
        image = ktx.to_png(texture)
    except (CodecError, png.PngError):
        return ()
    name = rel[:-4] if rel.lower().endswith(".ktx") else rel
    return (("%s/%s.png" % (PNG_DIR, name), image),)


def png_rel(rel: str) -> str:
    """素材相对路径对应的 PNG 旁挂路径。"""
    name = rel[:-4] if rel.lower().endswith(".ktx") else rel
    return "%s/%s.png" % (PNG_DIR, name)


def diff_pixels(left: bytes, right: bytes):
    """比较两份 RGBA 像素, 返回 (平均差, 最大差, 不同像素数)。"""
    if len(left) != len(right):
        raise CodecError("像素长度不一致: %d 对 %d" % (len(left), len(right)))
    total = 0
    biggest = 0
    changed = 0
    for i in range(0, len(left), 4):
        pixel_diff = 0
        for channel in range(4):
            value = abs(left[i + channel] - right[i + channel])
            total += value
            pixel_diff += value
            if value > biggest:
                biggest = value
        if pixel_diff:
            changed += 1
    return total / float(len(left)), biggest, changed


def export_tree(root: str, out_root: str, limit=None):
    """把资源树里可解的 KTX 逐张导出 PNG, 返回统计。"""
    exported = 0
    skipped = 0
    failed = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.lower().endswith(".ktx"):
                continue
            if limit is not None and exported + len(failed) >= limit:
                return {"exported": exported, "skipped": skipped, "failed": failed}
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            with open(path, "rb") as handle:
                blob = handle.read()
            try:
                plain = unwrap(blob)
                if not is_ktx(plain):
                    skipped += 1
                    continue
                texture = ktx.parse(plain)
                if texture.codec() is None:
                    skipped += 1
                    continue
                image = ktx.to_png(texture)
            except (CodecError, png.PngError, OSError) as exc:
                failed.append({"path": rel, "error": str(exc)})
                continue
            target = os.path.join(out_root, png_rel(rel))
            os.makedirs(os.path.dirname(target), exist_ok=True)
            with open(target, "wb") as handle:
                handle.write(image)
            exported += 1
    return {"exported": exported, "skipped": skipped, "failed": failed}


def import_tree(project_root: str, limit=None):
    """以 PNG 为准回写工程树里的 KTX; 像素未变时保留原字节。"""
    rebuilt = 0
    untouched = 0
    failed = []
    for dirpath, dirnames, filenames in os.walk(project_root):
        dirnames.sort()
        if os.path.basename(dirpath) == PNG_DIR:
            continue
        for name in sorted(filenames):
            if not name.lower().endswith(".ktx"):
                continue
            if limit is not None and rebuilt + len(failed) >= limit:
                return {"rebuilt": rebuilt, "untouched": untouched, "failed": failed}
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, project_root).replace(os.sep, "/")
            image_path = os.path.join(project_root, png_rel(rel))
            if not os.path.isfile(image_path):
                untouched += 1
                continue
            with open(path, "rb") as handle:
                blob = handle.read()
            with open(image_path, "rb") as handle:
                image_bytes = handle.read()
            try:
                texture = ktx.parse(blob)
                if texture.codec() is None:
                    untouched += 1
                    continue
                width, height, pixels = png.read_png(image_bytes)
                if (width, height) != (texture.width, texture.height):
                    failed.append(
                        {
                            "path": rel,
                            "error": "PNG 尺寸 %dx%d 与 KTX %dx%d 不符"
                            % (width, height, texture.width, texture.height),
                        }
                    )
                    continue
                current = ktx.to_rgba(texture)
                mean, biggest, changed = diff_pixels(current, pixels)
                if changed == 0:
                    untouched += 1
                    continue
                new_blob = ktx.rebuild(texture, pixels)
            except (CodecError, png.PngError, OSError) as exc:
                failed.append({"path": rel, "error": str(exc)})
                continue
            with open(path, "wb") as handle:
                handle.write(new_blob)
            rebuilt += 1
    return {"rebuilt": rebuilt, "untouched": untouched, "failed": failed}


def check_tree(project_root: str, limit=None):
    """回写自检: 用 PNG 重压一遍 KTX, 报告像素误差, 不落盘。"""
    checked = 0
    failures = []
    worst = (0.0, 0, None)
    skipped = 0
    for dirpath, dirnames, filenames in os.walk(project_root):
        dirnames.sort()
        if os.path.basename(dirpath) == PNG_DIR:
            continue
        for name in sorted(filenames):
            if not name.lower().endswith(".ktx"):
                continue
            if limit is not None and checked + len(failures) >= limit:
                return {"checked": checked, "skipped": skipped, "failed": failures, "worst": worst}
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, project_root).replace(os.sep, "/")
            image_path = os.path.join(project_root, png_rel(rel))
            if not os.path.isfile(image_path):
                skipped += 1
                continue
            with open(path, "rb") as handle:
                blob = handle.read()
            with open(image_path, "rb") as handle:
                image_bytes = handle.read()
            try:
                texture = ktx.parse(blob)
                if texture.codec() is None:
                    skipped += 1
                    continue
                width, height, pixels = png.read_png(image_bytes)
                if (width, height) != (texture.width, texture.height):
                    failures.append({"path": rel, "error": "PNG 与 KTX 尺寸不符"})
                    continue
                new_blob = ktx.rebuild(texture, pixels)
                rebuilt = ktx.parse(new_blob)
                restored = ktx.to_rgba(rebuilt)
                mean, biggest, changed = diff_pixels(pixels, restored)
            except (CodecError, png.PngError, OSError) as exc:
                failures.append({"path": rel, "error": str(exc)})
                continue
            checked += 1
            if mean > SELF_CHECK_MEAN_LIMIT or biggest > SELF_CHECK_MAX_LIMIT:
                failures.append(
                    {
                        "path": rel,
                        "error": "像素误差超阈值: 平均 %.2f 最大 %d" % (mean, biggest),
                    }
                )
            if mean > worst[0]:
                worst = (mean, biggest, rel)
    return {"checked": checked, "skipped": skipped, "failed": failures, "worst": worst}


def stats_tree(root: str):
    """统计资源树里 KTX 的格式、尺寸与 mip 分布, 只读头部不读像素。"""
    import collections

    total = 0
    carriers = collections.Counter()
    formats = collections.Counter()
    mips = collections.Counter()
    sizes = collections.Counter()
    unsupported = collections.Counter()
    failures = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.lower().endswith(".ktx"):
                continue
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            with open(path, "rb") as handle:
                head = handle.read()
            carriers["binary-family" if head[:4] == MAGIC_BINARY else "bare"] += 1
            try:
                plain = unwrap(head)
                texture = ktx.parse(plain)
            except (CodecError, OSError) as exc:
                failures.append({"path": rel, "error": str(exc)})
                continue
            total += 1
            formats[texture.format_name] += 1
            mips[len(texture.levels)] += 1
            sizes["%dx%d" % (texture.width, texture.height)] += 1
            if texture.codec() is None:
                unsupported[texture.format_name] += 1
    return {
        "total": total,
        "carriers": dict(carriers),
        "formats": dict(formats),
        "mips": dict(sorted(mips.items())),
        "sizes": dict(sizes.most_common(20)),
        "unsupported": dict(unsupported),
        "failures": failures,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="forge.ktx_png", description="KTX 纹理与 PNG 工程文件互转")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("stats", help="统计 KTX 头部字段分布")
    p.add_argument("--root", default=DEFAULT_ASSET_ROOT)

    p = sub.add_parser("export", help="把 KTX 导出为 PNG 工程文件")
    p.add_argument("--root", default=DEFAULT_ASSET_ROOT)
    p.add_argument("--out", default=DEFAULT_PROJECT)
    p.add_argument("--limit", type=int, default=None)

    p = sub.add_parser("import", help="以 PNG 为准回写 KTX")
    p.add_argument("--project", default=DEFAULT_PROJECT)
    p.add_argument("--limit", type=int, default=None)

    p = sub.add_parser("check", help="回写像素自检, 不落盘")
    p.add_argument("--project", default=DEFAULT_PROJECT)
    p.add_argument("--limit", type=int, default=None)
    return parser


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    if args.command == "stats":
        summary = stats_tree(args.root)
        print("KTX 总数: %d" % summary["total"])
        print("容器分布: %s" % summary["carriers"])
        print("像素格式: %s" % summary["formats"])
        print("mip 级数: %s" % summary["mips"])
        print("前 20 尺寸: %s" % summary["sizes"])
        if summary["unsupported"]:
            print("未支持格式: %s" % summary["unsupported"])
        for item in summary["failures"][:10]:
            print("  [失败] %s: %s" % (item["path"], item["error"]))
        return 0 if not summary["failures"] else 1
    if args.command == "export":
        result = export_tree(args.root, args.out, args.limit)
        print("导出 PNG %d 张, 跳过 %d 张, 失败 %d 张" % (
            result["exported"], result["skipped"], len(result["failed"])))
        for item in result["failed"][:10]:
            print("  [失败] %s: %s" % (item["path"], item["error"]))
        return 0 if not result["failed"] else 1
    if args.command == "import":
        result = import_tree(args.project, args.limit)
        print("回写 KTX %d 张, 未改动 %d 张, 失败 %d 张" % (
            result["rebuilt"], result["untouched"], len(result["failed"])))
        for item in result["failed"][:10]:
            print("  [失败] %s: %s" % (item["path"], item["error"]))
        return 0 if not result["failed"] else 1
    result = check_tree(args.project, args.limit)
    print("自检 %d 张, 跳过 %d 张, 越界 %d 张" % (
        result["checked"], result["skipped"], len(result["failed"])))
    mean, biggest, rel = result["worst"]
    if rel:
        print("最大平均误差 %.2f (最大单通道差 %d), 样本 %s" % (mean, biggest, rel))
    for item in result["failed"][:10]:
        print("  [越界] %s: %s" % (item["path"], item["error"]))
    return 0 if not result["failed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
