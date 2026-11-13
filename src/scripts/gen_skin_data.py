"""角色皮肤表: 由皮肤数据表生成 皮肤编号 -> 名称与模型 的对照表."""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
CSV_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project", "csvdef",
                                          "autogen", "roleskin.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "skin_data_gen.h")


def escape(text):
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main():
    if not os.path.isfile(CSV_PATH):
        print("皮肤表不存在")
        return 1
    rows = list(csv.reader(io.StringIO(open(CSV_PATH, "rb").read().decode("utf-8-sig"))))
    if len(rows) < 3:
        print("皮肤表为空")
        return 1
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    entries = {}
    for row in rows[2:]:
        if len(row) <= index["ID"]:
            continue
        raw_id = row[index["ID"]].strip()
        if not raw_id.isdigit():
            continue
        skin_id = int(raw_id)
        name = row[index["ENName"]].strip() if "ENName" in index else ""
        if not name:
            name = row[index["Name"]].strip()
        model = row[index["Model"]].strip() if "Model" in index else ""
        head = row[index["Head"]].strip() if "Head" in index else ""
        entries[skin_id] = (name, model, head)
    ordered = sorted(entries.items())
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/* 由 scripts/gen_skin_data.py 生成, 请勿手改. */",
        "/* 角色皮肤: 编号, 英文名与模型编号; 英文名用于界面显示. */",
        "namespace immirror::game::gen {",
        "",
        "struct SkinEntry {",
        "    uint16_t id;",
        "    uint32_t model;",
        "    uint16_t head;",
        "    const char* name;",
        "};",
        "",
        "constexpr int32_t kSkinEntryCount = %d;" % len(ordered),
        "constexpr SkinEntry kSkinEntries[kSkinEntryCount == 0 ? 1 : kSkinEntryCount] = {",
    ]
    for skin_id, (name, model, head) in ordered:
        model_value = int(model) if model.isdigit() else 0
        head_value = int(head) if head.isdigit() else 0
        lines.append('    {%du, %du, %du, "%s"},' % (skin_id, model_value, head_value,
                                                  escape(name)))
    if not ordered:
        lines.append('    {0u, 0u, 0u, ""},')
    lines += [
        "};",
        "",
        "/* 按编号取皮肤, 找不到返回空指针. 表内按编号升序, 用二分查找. */",
        "inline const SkinEntry* SkinOf(uint32_t id) {",
        "    int32_t low = 0;",
        "    int32_t high = kSkinEntryCount - 1;",
        "    while (low <= high) {",
        "        const int32_t mid = (low + high) / 2;",
        "        if (kSkinEntries[mid].id == id) {",
        "            return &kSkinEntries[mid];",
        "        }",
        "        if (kSkinEntries[mid].id < id) {",
        "            low = mid + 1;",
        "        } else {",
        "            high = mid - 1;",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "}  // namespace immirror::game::gen",
        "",
    ]
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("皮肤表: %d 条" % len(ordered))
    return 0


if __name__ == "__main__":
    sys.exit(main())