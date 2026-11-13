"""界面字符串表生成: stringdef.csv -> 客户端字符串表."""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
CSV_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project", "data",
                                          "csv", "csvdef", "autogen", "stringdef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "string_data_gen.h")

HEADER = """#pragma once

#include <cstdint>

/*
 * 由 scripts/gen_string_data.py 生成, 请勿手改.
 * 界面字符串表: 编号到英文文案. 界面点阵字只有 ASCII 字形, 因此取英文列;
 * 中文列保留在数据源里, 供将来接入中文字形时使用.
 */
namespace immirror::ui::gen {

struct StringEntry {
    uint32_t id;
    const char* text;
};
"""


def escape(text):
    out = []
    for ch in text:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            continue
        elif ch == "\t":
            out.append(" ");
        elif ord(ch) < 32:
            continue
        else:
            out.append(ch)
    return "".join(out)


def main():
    raw = open(CSV_PATH, "rb").read().decode("utf-8-sig")
    rows = list(csv.reader(io.StringIO(raw)))
    if len(rows) < 3:
        print("字符串表为空")
        return 1
    header = rows[1]
    id_index = header.index("ID")
    en_index = header.index("ENDesc")
    # 中文列用于英文缺失时回落, 列名固定为第二列.
    cn_index = 1 if len(header) > 1 else -1
    entries = {}
    for row in rows[2:]:
        if len(row) <= max(id_index, en_index):
            continue
        key = row[id_index].strip()
        if not key.isdigit():
            continue
        # 英文列缺失时回落到中文列: 当前字形只有 ASCII, 中文暂时画不出来,
        # 但条目要留在表里, 否则接入中文字形时还得重新生成一遍.
        text = row[en_index].strip()
        if not text and cn_index >= 0 and cn_index < len(row):
            text = row[cn_index].strip()
        if not text:
            continue
        entries[int(key)] = text
    ordered = sorted(entries.items())
    out = [HEADER]
    out.append("constexpr int32_t kStringEntryCount = %d;" % len(ordered))
    out.append("constexpr StringEntry kStringEntries[kStringEntryCount == 0 ? 1 : kStringEntryCount] = {")
    for key, text in ordered:
        out.append('    {%du, "%s"},' % (key, escape(text)))
    if not ordered:
        out.append('    {0u, ""},')
    out.append("};")
    out.append("")
    out.append("/* 按编号取英文文案, 找不到返回空串. 表内按编号升序, 用二分查找. */")
    out.append("inline const char* StringOf(uint32_t id) {")
    out.append("    int32_t low = 0;")
    out.append("    int32_t high = kStringEntryCount - 1;")
    out.append("    while (low <= high) {")
    out.append("        const int32_t mid = (low + high) / 2;")
    out.append("        if (kStringEntries[mid].id == id) {")
    out.append("            return kStringEntries[mid].text;")
    out.append("        }")
    out.append("        if (kStringEntries[mid].id < id) {")
    out.append("            low = mid + 1;")
    out.append("        } else {")
    out.append("            high = mid - 1;")
    out.append("        }")
    out.append("    }")
    out.append("    return \"\";")
    out.append("}")
    out.append("")
    out.append("}  // namespace immirror::ui::gen")
    out.append("")
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    print("字符串表: %d 条" % len(ordered))
    return 0


if __name__ == "__main__":
    sys.exit(main())
