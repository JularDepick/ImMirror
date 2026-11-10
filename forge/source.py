# -*- coding: utf-8 -*-
"""源树定位与重建。

管线需要原始二进制作为校验基准, 但原版素材可能只保留在别处 (逆向工程树),
或已被客户端自身的素材替换。本模块把「索引里的相对路径 -> 磁盘上的真实文件」
这段定位逻辑集中到一处, 并提供按索引重建源树的镜像命令。

镜像默认使用硬链接, 同一磁盘上不占额外空间; 跨卷或权限不足时退回复制。
"""

from __future__ import annotations

import hashlib
import os
import shutil

from .codecs.base import CodecError

"""客户端资源树, 管线默认输入。"""
CLIENT_ASSETS = os.path.join("..", "client", "src", "assets")

"""原版逆向工程树的候选根。

逆向树把素材分散在 src/assets 与 src/res、src/media、src/data 下,
按顺序尝试即可覆盖全部索引路径。
"""
RE_ROOT = os.path.join("..", "client", "temp", "original", "fake-mini-BUT-true-moments", "src")
DEFAULT_SOURCE_ROOTS = (
    CLIENT_ASSETS,
    os.path.join(RE_ROOT, "assets"),
    RE_ROOT,
)


def normalize_roots(roots, base: str) -> list:
    """把相对路径展开成绝对路径并去重, 保持给定顺序。"""
    out = []
    seen = set()
    for root in roots:
        full = os.path.abspath(os.path.join(base, root))
        if full in seen:
            continue
        seen.add(full)
        out.append(full)
    return out


class SourceTree:
    """按索引相对路径在多个候选根里定位源文件。"""

    def __init__(self, roots):
        self.roots = list(roots)

    def locate(self, rel_path: str):
        """返回第一个存在的源文件路径, 全部缺失时返回 None。"""
        parts = rel_path.split("/")
        for root in self.roots:
            path = os.path.join(root, *parts)
            if os.path.isfile(path):
                return path
        return None

    def resolve(self, rel_path: str) -> str:
        path = self.locate(rel_path)
        if path is None:
            raise CodecError("源文件缺失: %s (候选根 %d 个)" % (rel_path, len(self.roots)))
        return path

    def coverage(self, rel_paths) -> dict:
        """统计候选根对给定路径集合的覆盖率, 用于镜像前的体检。"""
        per_root = [0] * len(self.roots)
        missing = 0
        for rel in rel_paths:
            parts = rel.split("/")
            for index, root in enumerate(self.roots):
                if os.path.isfile(os.path.join(root, *parts)):
                    per_root[index] += 1
                    break
            else:
                missing += 1
        return {"roots": self.roots, "per_root": per_root, "missing": missing}


def _same_file(target: str, source: str) -> bool:
    """目标与源是否为同一份文件 (硬链接也算)。"""
    try:
        return os.path.samefile(target, source)
    except OSError:
        return False


def mirror_tree(records, source: SourceTree, out_root: str, hardlink: bool = True,
                project_root: str = None) -> dict:
    """按索引路径把源树重建到 out_root。

    磁盘上能定位到的源文件优先硬链接 (同卷不占额外空间); 定位不到的, 若给了工程树,
    就按索引里的编码参数把原版容器重新写出来, 并逐条核对源摘要, 保证镜像可信。
    返回统计与缺失清单 (缺失清单截断到 50 条, 总数单独记录)。
    """
    stats = {"linked": 0, "copied": 0, "rebuilt": 0, "rebuilt_mismatch": 0,
             "missing": [], "missing_total": 0, "bytes": 0}
    for record in records:
        rel = record["path"]
        src = source.locate(rel)
        dst = os.path.join(out_root, *rel.split("/"))
        if src is not None:
            os.makedirs(os.path.dirname(dst) or ".", exist_ok=True)
            if os.path.exists(dst) and _same_file(dst, src):
                stats["linked"] += 1
                continue
            if os.path.exists(dst):
                os.remove(dst)
            if hardlink:
                try:
                    os.link(src, dst)
                    stats["linked"] += 1
                    continue
                except OSError:
                    pass
            shutil.copyfile(src, dst)
            stats["copied"] += 1
            stats["bytes"] += os.path.getsize(dst)
            continue
        if project_root is not None and _rebuild(project_root, record, dst, stats):
            continue
        stats["missing_total"] += 1
        if len(stats["missing"]) < 50:
            stats["missing"].append(rel)
    return stats


def _rebuild(project_root: str, record, dst: str, stats: dict) -> bool:
    """由工程文件回写原版容器; 源摘要不符时计数并仍然写出。"""
    from .project import TARGET_ORIGINAL, pack_file

    src_path = os.path.join(project_root, *record["path"].split("/"))
    if not os.path.isfile(src_path):
        return False
    data = pack_file(project_root, record, TARGET_ORIGINAL)
    os.makedirs(os.path.dirname(dst) or ".", exist_ok=True)
    if os.path.exists(dst):
        os.remove(dst)
    with open(dst, "wb") as fh:
        fh.write(data)
    stats["rebuilt"] += 1
    stats["bytes"] += len(data)
    if hashlib.sha256(data).hexdigest() != record.get("source_sha256"):
        stats["rebuilt_mismatch"] += 1
    return True
