#!/usr/bin/env python3
"""从 proto2 文件生成 C++ 结构体骨架(单文件, 按依赖拓扑排序)."""

import glob
import io
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROTO_DIR = os.path.normpath(os.path.join(
    ROOT, "..", "temp", "original", "fake-mini-BUT-true-moments", "src", "protos"))
OUT = os.path.join(ROOT, "native", "net", "generated", "proto_gen.h")

SCALAR = {
    "int32": "int32_t", "int64": "int64_t", "uint32": "uint32_t", "uint64": "uint64_t",
    "sint32": "int32_t", "sint64": "int64_t", "fixed32": "uint32_t", "fixed64": "uint64_t",
    "sfixed32": "int32_t", "sfixed64": "int64_t",
    "float": "float", "double": "double", "bool": "bool", "string": "std::string",
    "bytes": "std::string",
}
FIELD_RE = re.compile(r"^\s*(optional|required|repeated)\s+([\w.]+)\s+(\w+)\s*=\s*(\d+)", re.M)


def blocks(body, kw):
    out = []
    for hit in re.finditer(r"^" + kw + r"\s+(\w+)\s*\{", body, re.M):
        depth = 0
        i = hit.end() - 1
        start = i
        while i < len(body):
            if body[i] == "{":
                depth += 1
            elif body[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        out.append((hit.group(1), body[start + 1:i]))
    return out


def ref_name(raw):
    return raw[1:].split(".")[-1] if raw.startswith(".") else raw


def main():
    files = sorted(glob.glob(os.path.join(PROTO_DIR, "*.proto")))
    if not files:
        print("找不到 proto: %s" % PROTO_DIR)
        return 1

    messages = {}
    enums = {}
    order_hint = []
    for path in files:
        body = io.open(path, encoding="utf-8", errors="replace").read()
        for name, inner in blocks(body, "enum"):
            enums[name] = inner
        for name, inner in blocks(body, "message"):
            messages[name] = inner
            order_hint.append(name)

    # 依赖图: message -> 它引用的其他 message
    deps = defaultdict(set)
    for name, inner in messages.items():
        for hit in FIELD_RE.finditer(inner):
            ref = ref_name(hit.group(2))
            if ref in messages and ref != name:
                deps[name].add(ref)

    # 拓扑排序(无环已确认), 依赖少的先出
    state = {}
    ordered = []

    def visit(node):
        if state.get(node) == 1:
            return
        if state.get(node) == 2:
            return
        state[node] = 1
        for dep in sorted(deps.get(node, ())):
            visit(dep)
        state[node] = 2
        ordered.append(node)

    for name in order_hint:
        visit(name)

    lines = ["#pragma once", "", "#include <cstdint>", "#include <string>",
             "#include <vector>", "", "/*",
             " * 由 scripts/gen_proto_cpp.py 从逆向工程的 4 个 proto 文件生成, 请勿手改.",
             " * 只含字段声明与 has_ 标志; 按依赖拓扑排序, 无前向引用.",
             " * 序列化在后续阶段实现.",
             " */",
             "namespace immirror::net::gen {", ""]

    for name in sorted(enums):
        lines.append("enum class %s : int32_t {" % name)
        for hit in re.finditer(r"^\s*(\w+)\s*=\s*(-?\d+)", enums[name], re.M):
            lines.append("    k%s = %s," % (hit.group(1), hit.group(2)))
        lines.append("};")
        lines.append("")

    for name in ordered:
        inner = messages[name]
        lines.append("struct %s {" % name)
        for hit in FIELD_RE.finditer(inner):
            label, raw, field, number = hit.group(1), hit.group(2), hit.group(3), hit.group(4)
            ctype = SCALAR.get(raw) or ref_name(raw)
            if label == "repeated":
                lines.append("    std::vector<%s> %s;  /* = %s */" % (ctype, field, number))
            else:
                lines.append("    %s %s{};  /* = %s */" % (ctype, field, number))
                lines.append("    bool has_%s = false;" % field)
        lines.append("};")
        lines.append("")

    lines.append("}  /* namespace immirror::net::gen */")
    lines.append("")
    payload = "\n".join(lines)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    old = io.open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
    if old != payload:
        io.open(OUT, "w", encoding="utf-8", newline="\n").write(payload)
        print("写出 %s" % OUT)
    print("message %d, enum %d, 拓扑序 %d" % (len(messages), len(enums), len(ordered)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
