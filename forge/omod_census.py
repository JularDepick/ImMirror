"""模型容器语料普查: 统计 omod 文件的块结构, 骨骼与网格规模, 以及同目录贴图.

只读. 用 forge.codecs.named_block 解码容器头, 统计各类块的出现次数,
为运行时模型加载器评估可行性提供数字.

用法: python -m forge.omod_census
"""

import collections
import os
import sys

from .codecs.named_block import container_of, describe

PROJECT_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "project")
ENTITY_ROOT = os.path.join(PROJECT_ROOT, "entity")


def iter_omod():
    for root, _dirs, files in os.walk(ENTITY_ROOT):
        for name in files:
            if name.endswith(".omod"):
                yield os.path.join(root, name)


def main():
    files = 0
    failed = 0
    blocks = collections.Counter()
    bones_total = 0
    bones_max = 0
    with_skeleton = 0
    with_mesh = 0
    ktx_files = 0
    for path in iter_omod():
        files += 1
        try:
            view = describe(container_of(open(path, "rb").read()))
        except Exception:
            failed += 1
            continue
        names = [block.get("name", "") for block in view.get("blocks", [])]
        blocks.update(names)
        if "SkeletonData" in names:
            with_skeleton += 1
            for block in view["blocks"]:
                if block.get("name") != "SkeletonData":
                    continue
                count = block.get("fields", {}).get("bone_count", 0)
                try:
                    count = int(count)
                except (TypeError, ValueError):
                    count = 0
                bones_total += count
                bones_max = max(bones_max, count)
        if "MeshData" in names:
            with_mesh += 1
    for _root, _dirs, files_in_dir in os.walk(ENTITY_ROOT):
        ktx_files += len([name for name in files_in_dir if name.endswith(".ktx")])
    print("omod 文件数: %d, 解码失败: %d" % (files, failed))
    print("带骨骼: %d, 带网格: %d, 同目录 ktx: %d" % (with_skeleton, with_mesh, ktx_files))
    print("骨骼总数: %d, 单个最多: %d" % (bones_total, bones_max))
    print("块出现次数 (前 12):")
    for name, count in blocks.most_common(12):
        print("  %-16s %d" % (name, count))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
