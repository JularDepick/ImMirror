#!/usr/bin/env python3
"""从 proto 生成每个 message 的 Serialize / Deserialize 函数.

依赖 gen_proto_cpp.py 生成的 proto_gen.h 的字段顺序(按字段号升序).
"""

import glob
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROTO_DIR = os.path.normpath(os.path.join(
    ROOT, "..", "temp", "original", "fake-mini-BUT-true-moments", "src", "protos"))
OUT = os.path.join(ROOT, "native", "net", "generated", "proto_codec_gen.h")

SCALAR = {
    "int32": "int32_t", "int64": "int64_t", "uint32": "uint32_t", "uint64": "uint64_t",
    "sint32": "int32_t", "sint64": "int64_t", "fixed32": "uint32_t", "fixed64": "uint64_t",
    "sfixed32": "int32_t", "sfixed64": "int64_t",
    "float": "float", "double": "double", "bool": "bool", "string": "std::string",
    "bytes": "std::string",
}
# 标量 -> (写函数, 读函数)
SCALAR_CODEC = {
    "int32_t": ("WriteInt32", "ReadInt32"),
    "int64_t": ("WriteInt64", "ReadInt64"),
    "uint32_t": ("WriteUInt32", "ReadUInt32"),
    "uint64_t": ("WriteUInt64", "ReadUInt64"),
    "float": ("WriteFloat", "ReadFloat"),
    "double": ("WriteDouble", "ReadDouble"),
    "bool": ("WriteBool", "ReadBool"),
    "std::string": ("WriteString", "ReadString"),
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
    messages = {}
    order = []
    for path in files:
        body = io.open(path, encoding="utf-8", errors="replace").read()
        for name, inner in blocks(body, "message"):
            messages[name] = inner
            order.append(name)

    lines = ["#pragma once", "", "#include <cstdint>", "#include <string>",
             "#include <vector>", "", '#include "net/proto_wire.h"',
             '#include "net/generated/proto_gen.h"', "", "/*",
             " * 由 scripts/gen_proto_codec.py 生成, 请勿手改.",
             " * 每个 message 一对 Serialize / Deserialize, 只处理已知字段, 未知字段跳过.",
             " */",
             "namespace immirror::net::gen {", ""]

    for name in order:
        inner = messages[name]
        fields = []
        for hit in FIELD_RE.finditer(inner):
            label, raw, field, number = hit.group(1), hit.group(2), hit.group(3), int(hit.group(4))
            fields.append((number, label, raw, field))
        fields.sort()

        # 声明
        lines.append("void Serialize(const %s& value, ProtoWriter& writer);" % name)
        lines.append("bool Deserialize(ProtoReader& reader, %s* value);" % name)
        lines.append("")

    for name in order:
        inner = messages[name]
        fields = []
        for hit in FIELD_RE.finditer(inner):
            label, raw, field, number = hit.group(1), hit.group(2), hit.group(3), int(hit.group(4))
            fields.append((number, label, raw, field))
        fields.sort()

        # Serialize
        lines.append("inline void Serialize(const %s& value, ProtoWriter& writer) {" % name)
        for number, label, raw, field in fields:
            ctype = SCALAR.get(raw)
            if ctype is not None:
                write_fn = SCALAR_CODEC.get(ctype, ("WriteInt32", "ReadInt32"))[0]
                if label == "repeated":
                    lines.append("    for (const auto& item : value.%s) {" % field)
                    lines.append("        writer.%s(%d, item);" % (write_fn, number))
                    lines.append("    }")
                else:
                    lines.append("    if (value.has_%s) {" % field)
                    lines.append("        writer.%s(%d, value.%s);" % (write_fn, number, field))
                    lines.append("    }")
            else:
                nested = ref_name(raw)
                if label == "repeated":
                    lines.append("    for (const auto& item : value.%s) {" % field)
                    lines.append("        ProtoWriter inner;")
                    lines.append("        Serialize(item, inner);")
                    lines.append("        writer.WriteMessage(%d, inner.Bytes());" % number)
                    lines.append("    }")
                else:
                    lines.append("    if (value.has_%s) {" % field)
                    lines.append("        ProtoWriter inner;")
                    lines.append("        Serialize(value.%s, inner);" % field)
                    lines.append("        writer.WriteMessage(%d, inner.Bytes());" % number)
                    lines.append("    }")
        lines.append("}")
        lines.append("")

        # Deserialize
        lines.append("inline bool Deserialize(ProtoReader& reader, %s* value) {" % name)
        lines.append("    if (value == nullptr) {")
        lines.append("        return false;")
        lines.append("    }")
        lines.append("    while (!reader.Done()) {")
        lines.append("        uint32_t field = 0;")
        lines.append("        WireType type = WireType::kVarint;")
        lines.append("        if (!reader.ReadTag(&field, &type)) {")
        lines.append("            return false;")
        lines.append("        }")
        lines.append("        switch (field) {")
        for number, label, raw, field in fields:
            ctype = SCALAR.get(raw)
            lines.append("            case %d: {" % number)
            if ctype is not None:
                read_fn = SCALAR_CODEC.get(ctype, ("WriteInt32", "ReadInt32"))[1]
                if label == "repeated":
                    lines.append("                %s item{};" % ctype)
                    lines.append("                if (!reader.%s(&item)) {" % read_fn)
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                value->%s.push_back(item);" % field)
                else:
                    lines.append("                if (!reader.%s(&value->%s)) {" % (read_fn, field))
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                value->has_%s = true;" % field)
            else:
                nested = ref_name(raw)
                if label == "repeated":
                    lines.append("                std::vector<uint8_t> nested;")
                    lines.append("                if (!reader.ReadMessage(&nested)) {")
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                %s item{};" % nested)
                    lines.append("                ProtoReader inner(nested.data(), nested.size());")
                    lines.append("                if (!Deserialize(inner, &item)) {")
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                value->%s.push_back(item);" % field)
                else:
                    lines.append("                std::vector<uint8_t> nested;")
                    lines.append("                if (!reader.ReadMessage(&nested)) {")
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                ProtoReader inner(nested.data(), nested.size());")
                    lines.append("                if (!Deserialize(inner, &value->%s)) {" % field)
                    lines.append("                    return false;")
                    lines.append("                }")
                    lines.append("                value->has_%s = true;" % field)
            lines.append("                break;")
            lines.append("            }")
        lines.append("            default:")
        lines.append("                if (!reader.Skip(type)) {")
        lines.append("                    return false;")
        lines.append("                }")
        lines.append("                break;")
        lines.append("        }")
        lines.append("    }")
        lines.append("    return true;")
        lines.append("}")
        lines.append("")

    lines.append("}  /* namespace immirror::net::gen */")
    lines.append("")
    payload = "\n".join(lines)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    old = io.open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
    if old != payload:
        io.open(OUT, "w", encoding="utf-8", newline="\n").write(payload)
        print("写出 %s" % OUT)
    print("生成 %d 个 message 的序列化函数" % len(order))
    return 0


if __name__ == "__main__":
    sys.exit(main())
