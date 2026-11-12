#!/usr/bin/env python3
"""方块贴图合成图集的兼容入口.

历史上的 16x16 图集脚本已被全量图集取代: 渲染端固定按 64x64 格, 每格 64 像素
读取 generated/atlas.rgba, 旧脚本写出的 1024x1024 图集会直接让图集尺寸校验失败.
这里只做转发, 保证文档与快捷命令里的入口仍然可用, 且不会产出错误规格的资产.

输出: android/app/src/main/assets/generated/ 下的 atlas.rgba, atlas.png 与 atlas.json,
 以及 native/voxel/generated/atlas_index_gen.h.
"""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

import build_atlas_full  # noqa: E402  (需要先改好 sys.path 再导入)


def main():
    print("build_atlas.py 已转发到 build_atlas_full.py (图集规格 4096x4096, 64x64 格)")
    return build_atlas_full.main()


if __name__ == "__main__":
    sys.exit(main())
