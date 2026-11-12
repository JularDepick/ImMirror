#!/usr/bin/env python3
"""把界面精灵页里的九宫模板素材批量导入客户端界面素材索引.

输入:
  single-branch/client/temp/ui_extract/build/report/templates.tsv  模板表 (draw/tile)
  single-branch/client/temp/ui_extract/build/report/sprites.tsv    精灵表 (页内矩形)
  single-branch/client/temp/ui_extract/build/png/_png/mobile/<page>.png 解码后的精灵页
输出:
  src/assets/ui/<category>/<name>.png      裁好的素材
  src/assets/ui/ui_manifest.json           追加的素材条目 (已存在则不重复)

导入范围由 TEMPLATE_PAGES 限定: 目前界面层真正用到的页 (通用件, 消息框, 大厅, 背包, 设置).
draw 取模板的 draw 字段折算: ninesquare 与 ninesquare_center 按模板的 tile 中心块
反推九宫边界, 其余按整体拉伸登记. 索引条目写入后由 gen_ui_resources.py 生成两侧常量.
"""

import csv
import json
import os
import sys

try:
    from PIL import Image
except ImportError:
    raise SystemExit("缺少 Pillow")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
CLIENT_ROOT = os.path.dirname(SRC_ROOT)
REPORT_ROOT = os.path.join(CLIENT_ROOT, "temp", "ui_extract", "build", "report")
PAGE_ROOT = os.path.join(CLIENT_ROOT, "temp", "ui_extract", "build", "png", "_png", "mobile")
ASSET_ROOT = os.path.join(SRC_ROOT, "assets", "ui")
MANIFEST_PATH = os.path.join(ASSET_ROOT, "ui_manifest.json")

# 页 -> 目标分类目录. 只导入这几页: 界面层现有 screens 用到的是通用件与大堂件.
TEMPLATE_PAGES = {
    "texture2/messagebox": "panel",
    "texture2/common": "button",
    "texture0/main_lobby": "lobby",
    "texture0/backpack": "slot",
}

# 按名导入的精灵: 页 -> (目标分类, 精灵名列表). 这些是原版大厅入口按钮与顶部图标,
# 名字与 main_lobby_ex.xml 的 uvname 一一对应; 拉伸方式按素材原尺寸 1:1 使用, 记 stretch.
NAMED_SPRITES = {
    "texture0/main_lobby": ("lobby", [
        "btn_orange_hall", "btn_blue_hall", "btn_lgreen_hall", "btn_dgreen_hall",
        "board_enter_workshop", "board_workshop",
    ]),
    "texture2/main_lobby": ("lobby", [
        "icon_halltop_set", "icon_halltop_pack", "icon_halltop_letter", "icon_halltop_friend",
    ]),
}

DRAW_MAP = {
    "ninesquare": "nine_slice",
    "ninesquare_center": "nine_slice",
    "center": "stretch",
    "height": "stretch",
    "width": "stretch",
}


def read_tsv(path):
    with open(path, "r", encoding="utf-8") as handle:
        return list(csv.DictReader(handle, delimiter="	"))


def parse_tile(text):
    parts = [int(value) for value in text.split(",")] if text.strip() else []
    return parts if len(parts) == 4 else None


def nine_slice_insets(width, height, tile):
    """按九宫中心块反推边界: 中心块内的像素是拉伸区, 其余是固定边."""
    left, top, center_width, center_height = tile
    right = max(0, width - left - center_width)
    bottom = max(0, height - top - center_height)
    return left, top, right, bottom


def load_manifest():
    with open(MANIFEST_PATH, "r", encoding="utf-8") as handle:
        return json.load(handle)


def sanitize(name):
    return "".join(ch if ch.isalnum() or ch in "_-" else "_" for ch in name).strip("_").lower()


def main():
    templates = read_tsv(os.path.join(REPORT_ROOT, "templates.tsv"))
    sprites = read_tsv(os.path.join(REPORT_ROOT, "sprites.tsv"))
    pages = set(TEMPLATE_PAGES)
    by_page_sprite = {(row["page"], row["sprite"]): row for row in sprites}

    manifest = load_manifest()
    known = {asset["path"] for asset in manifest["assets"]}
    added = 0
    missing = []

    for template in templates:
        page = template["page"] + ".xml" if not template["page"].endswith(".xml") else template["page"]
        page_key = template["page"]
        if page_key not in pages:
            continue
        sprite = template["sprite"]
        info = by_page_sprite.get((page, sprite))
        if info is None:
            missing.append("%s/%s" % (page_key, sprite))
            continue
        width, height = int(info["w"]), int(info["h"])
        left, top, w, h = int(info["x"]), int(info["y"]), width, height

        category = TEMPLATE_PAGES[page_key]
        name = sanitize("%s_%s" % (page_key.split("/", 1)[1], sprite)) + ".png"
        path = category + "/" + name
        if path in known:
            continue

        page_png = os.path.join(PAGE_ROOT, *(page_key.split("/") + [".png"]))
        page_png = os.path.join(PAGE_ROOT, page_key + ".png")
        if not os.path.exists(page_png):
            missing.append(page_key + ".png")
            continue
        image = Image.open(page_png).convert("RGBA")
        crop = image.crop((left, top, left + w, top + h))

        draw = DRAW_MAP.get(template["draw"], "stretch")
        insets = {"left": 0, "top": 0, "right": 0, "bottom": 0}
        tile = parse_tile(template["tile"]) if draw == "nine_slice" else None
        if tile is not None:
            l, t, r, b = nine_slice_insets(width, height, tile)
            insets = {"left": l, "top": t, "right": r, "bottom": b}
        elif draw == "nine_slice":
            draw = "stretch"

        target = os.path.join(ASSET_ROOT, category, name)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        crop.save(target)

        manifest["assets"].append({
            "path": path,
            "width": width,
            "height": height,
            "category": category,
            "purpose": "原版模板 %s 的 %s" % (template["template"], sprite),
            "source_page": page_key + ".ktx",
            "source_sprite": sprite,
            "source_rect": [left, top, w, h],
            "draw": draw,
            "insets": insets,
            "template": template["template"],
        })
        known.add(path)
        added += 1

    # 按名导入的大厅按钮与顶部图标: 与模板件同一条写盘与登记路径
    for page_key, (category, names) in NAMED_SPRITES.items():
        page_png = os.path.join(PAGE_ROOT, page_key + ".png")
        if not os.path.exists(page_png):
            missing.append(page_key + ".png")
            continue
        image = Image.open(page_png).convert("RGBA")
        for sprite in names:
            info = by_page_sprite.get((page_key + ".xml", sprite))
            if info is None:
                missing.append("%s/%s" % (page_key, sprite))
                continue
            width, height = int(info["w"]), int(info["h"])
            left, top = int(info["x"]), int(info["y"])
            name = sanitize(page_key.split("/", 1)[1] + "_" + sprite) + ".png"
            path = category + "/" + name
            if path in known:
                continue
            crop = image.crop((left, top, left + width, top + height))
            target = os.path.join(ASSET_ROOT, category, name)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            crop.save(target)
            manifest["assets"].append({
                "path": path,
                "width": width,
                "height": height,
                "category": category,
                "purpose": "原版大厅素材 %s" % sprite,
                "source_page": page_key + ".ktx",
                "source_sprite": sprite,
                "source_rect": [left, top, width, height],
                "draw": "stretch",
                "insets": {"left": 0, "top": 0, "right": 0, "bottom": 0},
                "template": "",
            })
            known.add(path)
            added += 1

    manifest["assets"].sort(key=lambda asset: asset["path"])
    with open(MANIFEST_PATH, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(manifest, handle, ensure_ascii=False, indent=1)
        handle.write("\n")

    print("导入模板素材 %d 个, 索引总数 %d" % (added, len(manifest["assets"])))
    if missing:
        print("缺页或缺精灵 %d 项: %s" % (len(missing), ", ".join(missing[:8])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
