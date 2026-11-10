# -*- coding: utf-8 -*-
"""素材脚手架: 按分类把二进制资源与明文工程目录双向转换。

每个分类是一个子包, 对外只暴露三个入口:

- `export(源二进制路径, 工程目录)`  二进制 -> 明文工程目录
- `build(工程目录, 输出二进制路径)`  明文工程目录 -> 二进制
- `detect(路径, 字节)`              判断该分类是否受理这份素材

命令行入口为 `python -m scaffolds`, 具体命令见 README 与 docs/scaffolds.md。
"""

from __future__ import annotations

import importlib

"""分类名按资料族排列, 与 docs/scaffolds.md 的分类矩阵一一对应。"""
CATEGORY_NAMES = ("models", "textures", "ui", "data", "audio", "world")


def load_category(name: str):
    """按分类名导入子包; 未登记的分类抛 ValueError。"""
    if name not in CATEGORY_NAMES:
        raise ValueError("未知分类: %s (可选 %s)" % (name, ", ".join(CATEGORY_NAMES)))
    return importlib.import_module("scaffolds." + name)


def list_categories() -> list:
    """列出全部分类及其受理扩展名, 供 CLI 与文档生成使用。"""
    out = []
    for name in CATEGORY_NAMES:
        module = load_category(name)
        out.append(
            {
                "name": name,
                "title": getattr(module, "TITLE", name),
                "extensions": tuple(getattr(module, "EXTENSIONS", ())),
                "formats": dict(getattr(module, "FORMATS", {})),
            }
        )
    return out
