#!/usr/bin/env python3
"""协议层断言: proto 里的每个 message 与 enum 都必须出现在生成的头文件里."""

import glob
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROTO_DIR = os.path.normpath(os.path.join(
    ROOT, "..", "temp", "original", "fake-mini-BUT-true-moments", "src", "protos"))
GEN = os.path.join(ROOT, "native", "net", "generated", "proto_gen.h")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "net-baseline", "proto.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    proto_msgs = set()
    proto_enums = set()
    for path in sorted(glob.glob(os.path.join(PROTO_DIR, "*.proto"))):
        body = io.open(path, encoding="utf-8", errors="replace").read()
        proto_msgs |= set(re.findall(r"^message\s+(\w+)", body, re.M))
        proto_enums |= set(re.findall(r"^enum\s+(\w+)", body, re.M))
    print("      proto: message %d, enum %d" % (len(proto_msgs), len(proto_enums)))

    body = io.open(GEN, encoding="utf-8", errors="replace").read()
    gen_msgs = set(re.findall(r"^struct\s+(\w+)\s*\{", body, re.M))
    gen_enums = set(re.findall(r"^enum class\s+(\w+)\s*:", body, re.M))
    print("      生成: struct %d, enum class %d" % (len(gen_msgs), len(gen_enums)))

    miss_msg = sorted(proto_msgs - gen_msgs)
    miss_enum = sorted(proto_enums - gen_enums)
    check("message 全部生成", len(miss_msg) == 0,
          "缺 %d: %s" % (len(miss_msg), miss_msg[:5]))
    check("enum 全部生成", len(miss_enum) == 0,
          "缺 %d: %s" % (len(miss_enum), miss_enum[:5]))
    check("message 数量一致", len(proto_msgs) == len(gen_msgs),
          "proto=%d 生成=%d" % (len(proto_msgs), len(gen_msgs)))

    # 字段数对比
    field_re = re.compile(r"^\s*(optional|required|repeated)\s+[\w.]+\s+\w+\s*=\s*\d+", re.M)
    proto_fields = 0
    for path in sorted(glob.glob(os.path.join(PROTO_DIR, "*.proto"))):
        proto_fields += len(field_re.findall(io.open(path, encoding="utf-8", errors="replace").read()))
    gen_fields = len(re.findall(r"/\* = \d+ \*/", body))
    print("      字段: proto %d, 生成 %d" % (proto_fields, gen_fields))
    check("字段数一致", proto_fields == gen_fields, "proto=%d 生成=%d" % (proto_fields, gen_fields))

    lines_out = ["# 协议层断言", "",
                 "| 项 | 值 |", "|:---:|:---:|",
                 "| proto message | %d |" % len(proto_msgs),
                 "| 生成 struct | %d |" % len(gen_msgs),
                 "| proto enum | %d |" % len(proto_enums),
                 "| 生成 enum class | %d |" % len(gen_enums),
                 "| proto 字段 | %d |" % proto_fields,
                 "| 生成字段 | %d |" % gen_fields,
                 "| 失败项 | %d |" % len(failures), ""]
    if miss_msg:
        lines_out.append("## 未生成的 message")
        lines_out.append("")
        for n in miss_msg[:20]:
            lines_out.append("- %s" % n)
        lines_out.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
