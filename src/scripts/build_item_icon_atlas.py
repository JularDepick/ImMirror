"""把物品图标打包成图集, 并生成 物品编号 -> 格位 的对照头文件."""

import json
import os
import sys

from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project"))
MANIFEST = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_manifest.json")
BUFF_MANIFEST = os.path.join(SRC_ROOT, "native", "ui", "generated", "buff_icon_manifest.json")
OUT_DIR = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "generated"))
GEN_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_atlas_gen.h")

GRID = 64
CELL = 32
SIZE = GRID * CELL


def main():
    if not os.path.isfile(MANIFEST):
        print("图标清单不存在, 先运行 gen_item_icon_manifest.py")
        return 1
    manifest = json.load(open(MANIFEST, encoding="utf-8"))
    items = manifest.get("items", {})
    if not items:
        print("图标清单为空")
        return 1
    capacity = GRID * GRID
    ordered = sorted(items.items(), key=lambda pair: int(pair[0]))
    # 效果图标并入同一张图集: 图集格位还有富余, 分两张纹理反而多一次绑定.
    buffs = {}
    if os.path.isfile(BUFF_MANIFEST):
        buffs = json.load(open(BUFF_MANIFEST, encoding="utf-8")).get("buffs", {})
    buff_ordered = sorted(buffs.items(), key=lambda pair: int(pair[0]))
    if len(ordered) + len(buff_ordered) > capacity:
        print("图标数量 %d 超过图集容量 %d" % (len(ordered) + len(buff_ordered), capacity))
        return 1

    atlas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    slots = {}
    drawn = 0
    for slot, (item_id, relative) in enumerate(ordered):
        source = os.path.join(ASSETS, relative)
        if not os.path.isfile(source):
            continue
        try:
            image = Image.open(source).convert("RGBA")
        except Exception:
            continue
        # 图标可能是竖长条, 取上部正方形再缩放, 保持比例不失真.
        if image.height > image.width:
            image = image.crop((0, 0, image.width, image.width))
        image = image.resize((CELL, CELL), Image.LANCZOS)
        col, row = slot % GRID, slot // GRID
        atlas.paste(image, (col * CELL, row * CELL))
        slots[int(item_id)] = slot
        drawn += 1

    buff_slots = {}
    for offset, (buff_id, relative) in enumerate(buff_ordered):
        slot = len(ordered) + offset
        source = os.path.join(ASSETS, relative)
        if not os.path.isfile(source):
            continue
        try:
            image = Image.open(source).convert("RGBA")
        except Exception:
            continue
        if image.height > image.width:
            image = image.crop((0, 0, image.width, image.width))
        image = image.resize((CELL, CELL), Image.LANCZOS)
        col, row = slot % GRID, slot // GRID
        atlas.paste(image, (col * CELL, row * CELL))
        buff_slots[int(buff_id)] = slot
        drawn += 1

    os.makedirs(OUT_DIR, exist_ok=True)
    atlas.save(os.path.join(OUT_DIR, "atlas_item_icon.png"))
    with open(os.path.join(OUT_DIR, "atlas_item_icon.rgba"), "wb") as handle:
        handle.write(atlas.tobytes())

    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/* 由 scripts/build_item_icon_atlas.py 生成, 请勿手改. */",
        "/* 物品编号 -> 图标图集格序号; 负数表示该物品没有图标. */",
        "namespace immirror::ui::gen {",
        "",
        "constexpr int32_t kItemIconGrid = %d;" % GRID,
        "constexpr int32_t kItemIconCell = %d;" % CELL,
        "constexpr int32_t kItemIconSize = %d;" % SIZE,
        "",
        "struct ItemIconEntry {",
        "    uint32_t item;",
        "    int16_t slot;",
        "};",
        "",
        "constexpr int32_t kItemIconCount = %d;" % len(slots),
        "constexpr ItemIconEntry kItemIcons[kItemIconCount == 0 ? 1 : kItemIconCount] = {",
    ]
    for item_id in sorted(slots):
        lines.append("    {%du, %d}," % (item_id, slots[item_id]))
    if not slots:
        lines.append("    {0u, -1},")
    lines += [
        "};",
        "",
        "/* 按物品编号取图标格序号, 没有图标返回 -1. 表内按编号升序, 用二分查找. */",
        "inline int32_t ItemIconSlot(uint32_t item) {",
        "    int32_t low = 0;",
        "    int32_t high = kItemIconCount - 1;",
        "    while (low <= high) {",
        "        const int32_t mid = (low + high) / 2;",
        "        if (kItemIcons[mid].item == item) {",
        "            return kItemIcons[mid].slot;",
        "        }",
        "        if (kItemIcons[mid].item < item) {",
        "            low = mid + 1;",
        "        } else {",
        "            high = mid - 1;",
        "        }",
        "    }",
        "    return -1;",
        "}",
        "",
        "constexpr int32_t kBuffIconCount = %d;" % len(buff_slots),
        "constexpr ItemIconEntry kBuffIcons[kBuffIconCount == 0 ? 1 : kBuffIconCount] = {",
    ]
    for buff_id in sorted(buff_slots):
        lines.append("    {%du, %d}," % (buff_id, buff_slots[buff_id]))
    if not buff_slots:
        lines.append("    {0u, -1},")
    lines += [
        "};",
        "",
        "/* 按效果编号取图标格序号, 没有图标返回 -1. 表内按编号升序, 用二分查找. */",
        "inline int32_t BuffIconSlot(uint32_t buff) {",
        "    int32_t low = 0;",
        "    int32_t high = kBuffIconCount - 1;",
        "    while (low <= high) {",
        "        const int32_t mid = (low + high) / 2;",
        "        if (kBuffIcons[mid].item == buff) {",
        "            return kBuffIcons[mid].slot;",
        "        }",
        "        if (kBuffIcons[mid].item < buff) {",
        "            low = mid + 1;",
        "        } else {",
        "            high = mid - 1;",
        "        }",
        "    }",
        "    return -1;",
        "}",
        "",
        "}  // namespace immirror::ui::gen",
        "",
    ]
    os.makedirs(os.path.dirname(GEN_PATH), exist_ok=True)
    with open(GEN_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("物品图标图集: %d 个图标, 格位 %d, 尺寸 %d" % (drawn, GRID * GRID, SIZE))
    return 0


if __name__ == "__main__":
    sys.exit(main())