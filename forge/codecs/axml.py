# -*- coding: utf-8 -*-
"""Android 二进制 XML (AXML) 的识别与可读化。

AXML 是 Android 资源编译产物, 不是游戏自定义格式: 文件由 RES_XML 根块加字符串池、
资源映射与一串 XML 节点块组成。管线把它还原成可读 XML 文本供人阅读与 diff,
但字节仍原样保留, 因此再编码逐字节相同。

节点顺序与块头字段均按 AOSP ResourceTypes.h 的定义读取, 不依赖外部工具。
"""

from __future__ import annotations

import struct

from .base import Codec, Payload

"""根块恒为 RES_XML, 头长 8, 与 589 个样本全部吻合。"""
MAGIC_AXML = b"\x03\x00\x08\x00"

RES_STRING_POOL_TYPE = 0x0001
RES_XML_TYPE = 0x0003
RES_XML_START_NAMESPACE = 0x0100
RES_XML_END_NAMESPACE = 0x0101
RES_XML_START_ELEMENT = 0x0102
RES_XML_END_ELEMENT = 0x0103
RES_XML_CDATA = 0x0104

NO_ENTRY = 0xFFFFFFFF

"""字符串池的标志位: 0x100 表示 UTF-8, 否则 UTF-16。"""
UTF8_FLAG = 1 << 8

"""视图里 XML 文本的长度上限, 防止异常资源把视图撑大。"""
MAX_XML_CHARS = 200000

"""单文件节点数上限, 防御性停止条件。"""
MAX_NODES = 200000

"""值类型名, 与 Res_value::Type 一致; 未列出的按原始整数呈现。"""
TYPE_NAMES = {
    0x00: "null",
    0x01: "reference",
    0x02: "attribute",
    0x03: "string",
    0x04: "float",
    0x05: "dimension",
    0x06: "fraction",
    0x10: "int",
    0x11: "hex",
    0x12: "boolean",
    0x1C: "argb8",
    0x1D: "rgb8",
    0x1E: "argb4",
    0x1F: "rgb4",
}

DIMENSION_UNITS = ("px", "dip", "sp", "pt", "in", "mm")
FRACTION_UNITS = ("%", "%p")


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _chunk_header(data: bytes, offset: int):
    """返回 (类型, 头长, 块长); 越界返回 None。"""
    if offset + 8 > len(data):
        return None
    return _u16(data, offset), _u16(data, offset + 2), _u32(data, offset + 4)


def _read_varint8(data: bytes, offset: int):
    """UTF-8 字符串池的变长长度: 单字节或高位置位的双字节。"""
    first = data[offset]
    if first & 0x80:
        return ((first & 0x7F) << 8) | data[offset + 1], offset + 2
    return first, offset + 1


def parse_string_pool(data: bytes, offset: int, size: int):
    """解析 RES_STRING_POOL, 返回 (字符串列表, 编码名)。"""
    header_size = _u16(data, offset + 2)
    count = _u32(data, offset + 8)
    flags = _u32(data, offset + 16)
    strings_start = _u32(data, offset + 20)
    utf8 = bool(flags & UTF8_FLAG)
    base = offset + strings_start
    offsets = [_u32(data, offset + header_size + 4 * index) for index in range(count)]
    out = []
    for rel in offsets:
        pos = base + rel
        if pos >= len(data):
            out.append("")
            continue
        if utf8:
            _chars, pos = _read_varint8(data, pos)
            byte_len, pos = _read_varint8(data, pos)
            raw = data[pos:pos + byte_len]
            out.append(raw.decode("utf-8", "replace"))
        else:
            length = _u16(data, pos)
            pos += 2
            if length & 0x8000:
                length = ((length & 0x7FFF) << 16) | _u16(data, pos)
                pos += 2
            raw = data[pos:pos + 2 * length]
            out.append(raw.decode("utf-16-le", "replace"))
    return out, ("utf-8" if utf8 else "utf-16")


def _pool_get(pool, index: int) -> str:
    if index == NO_ENTRY or index >= len(pool):
        return ""
    return pool[index]


def _format_value(data_type: int, value, pool) -> str:
    """把 Res_value 的 (类型, 数据) 字段渲染成属性值文本。"""
    if data_type == 0x03:
        if isinstance(value, int):
            return _pool_get(pool, value)
        return ""
    if data_type == 0x01:
        return "@0x%08x" % value
    if data_type == 0x02:
        return "?0x%08x" % value
    if data_type == 0x04:
        return repr(struct.unpack("<f", struct.pack("<I", value & 0xFFFFFFFF))[0])
    if data_type == 0x05:
        unit = DIMENSION_UNITS[value & 0xF] if (value & 0xF) < len(DIMENSION_UNITS) else "?"
        return "%s%s" % (_complex_to_float(value), unit)
    if data_type == 0x06:
        unit = FRACTION_UNITS[value & 0xF] if (value & 0xF) < len(FRACTION_UNITS) else "?"
        return "%s%s" % (_complex_to_float(value), unit)
    if data_type == 0x12:
        return "true" if value else "false"
    if data_type == 0x11:
        return "0x%x" % value
    if data_type in (0x1C, 0x1D, 0x1E, 0x1F):
        return "#%08x" % value
    return str(value)


def _complex_to_float(value: int) -> str:
    """Android 的 complex 值: 高 24 位为整数部分, 低 8 位为分数。"""
    mantissa = value >> 8
    if mantissa & 0x800000:
        mantissa -= 0x1000000
    return "%g" % (mantissa / 256.0)


def _escape(text: str) -> str:
    return (text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace('"', "&quot;"))


def parse_nodes(data: bytes):
    """按块序列还原 XML 文本与统计信息, 返回字典。"""
    header = _chunk_header(data, 0)
    if header is None or header[0] != RES_XML_TYPE:
        raise ValueError("不是 RES_XML 根块")
    pool = []
    pool_encoding = ""
    position = header[1]
    while position + 8 <= len(data):
        chunk = _chunk_header(data, position)
        if chunk is None:
            break
        chunk_type, _chunk_head, chunk_size = chunk
        if chunk_size < 8 or position + chunk_size > len(data):
            break
        if chunk_type == RES_STRING_POOL_TYPE:
            pool, pool_encoding = parse_string_pool(data, position, chunk_size)
        position += chunk_size
    position = header[1]
    prefix_of = {}
    lines = []
    stats = {"elements": 0, "attributes": 0, "texts": 0, "namespaces": 0, "depth": 0}
    depth = 0
    nodes = 0
    while position + 8 <= len(data):
        chunk = _chunk_header(data, position)
        if chunk is None:
            break
        chunk_type, chunk_head, chunk_size = chunk
        if chunk_size < 8 or position + chunk_size > len(data):
            break
        nodes += 1
        if nodes > MAX_NODES:
            lines.append("<!-- 节点数超过上限, 余下略去 -->")
            break
        if chunk_type == RES_XML_START_NAMESPACE:
            uri = _pool_get(pool, _u32(data, position + chunk_head))
            prefix = _pool_get(pool, _u32(data, position + chunk_head + 4))
            prefix_of[uri] = prefix
            stats["namespaces"] += 1
        elif chunk_type == RES_XML_START_ELEMENT:
            name_index = _u32(data, position + chunk_head + 4)
            name = _pool_get(pool, name_index)
            ns_index = _u32(data, position + chunk_head)
            prefix = prefix_of.get(_pool_get(pool, ns_index), "") if ns_index != NO_ENTRY else ""
            label = "%s%s" % (prefix + ":" if prefix else "", name)
            attributes = _read_attributes(data, position, chunk_head, pool, prefix_of)
            stats["elements"] += 1
            stats["attributes"] += len(attributes)
            indent = "  " * depth
            if attributes:
                head = "<%s" % label
                body = "".join(' %s="%s"' % (key, _escape(value)) for key, value in attributes)
                lines.append("%s%s%s>" % (indent, head, body))
            else:
                lines.append("%s<%s>" % (indent, label))
            depth += 1
            stats["depth"] = max(stats["depth"], depth)
        elif chunk_type == RES_XML_END_ELEMENT:
            depth = max(0, depth - 1)
            name_index = _u32(data, position + chunk_head + 4)
            name = _pool_get(pool, name_index)
            ns_index = _u32(data, position + chunk_head)
            prefix = prefix_of.get(_pool_get(pool, ns_index), "") if ns_index != NO_ENTRY else ""
            lines.append("%s</%s>" % ("  " * depth, "%s%s" % (prefix + ":" if prefix else "", name)))
        elif chunk_type == RES_XML_CDATA:
            index = _u32(data, position + chunk_head)
            text = _pool_get(pool, index)
            if text.strip():
                lines.append("%s%s" % ("  " * depth, _escape(text)))
                stats["texts"] += 1
        position += chunk_size
    xml = "\n".join(lines)
    truncated = len(xml) > MAX_XML_CHARS
    if truncated:
        xml = xml[:MAX_XML_CHARS]
    stats["string_count"] = len(pool)
    stats["string_encoding"] = pool_encoding
    stats["xml_chars"] = len(xml)
    stats["truncated"] = truncated
    return {"xml": xml, "stats": stats}


def _read_attributes(data: bytes, position: int, chunk_head: int, pool, prefix_of):
    """读取开始元素块里的属性表, 返回 (名称, 值) 序列。"""
    attribute_start = _u16(data, position + chunk_head + 8)
    attribute_size = _u16(data, position + chunk_head + 10)
    attribute_count = _u16(data, position + chunk_head + 12)
    base = position + chunk_head + attribute_start
    out = []
    for index in range(attribute_count):
        offset = base + index * attribute_size
        if offset + 20 > len(data):
            break
        ns_index = _u32(data, offset)
        name = _pool_get(pool, _u32(data, offset + 4))
        raw_value = _u32(data, offset + 8)
        data_type = data[offset + 15]
        value = _u32(data, offset + 16)
        prefix = prefix_of.get(_pool_get(pool, ns_index), "") if ns_index != NO_ENTRY else ""
        key = "%s:%s" % (prefix, name) if prefix else name
        if raw_value != NO_ENTRY:
            text = _pool_get(pool, raw_value)
        else:
            text = _format_value(data_type, value, pool)
        out.append((key, text))
    return out


class AxmlCodec(Codec):
    name = "axml"
    fidelity = "lossless-passthrough"
    readability = "binary-standard"
    extensions = (".xml",)
    magics = (MAGIC_AXML,)

    def sniff(self, data: bytes) -> bool:
        return data[:4] == MAGIC_AXML

    def decode(self, data: bytes) -> Payload:
        return Payload(kind="binary", blob=data,
                       meta={"container": "axml", "plain_size": len(data)})

    def view(self, payload: Payload):
        data = payload.blob or b""
        try:
            parsed = parse_nodes(data)
        except (ValueError, struct.error, IndexError) as exc:
            return {"container": "axml", "error": str(exc)}
        out = {"container": "axml", "bytes": len(data)}
        out.update(parsed["stats"])
        out["xml"] = parsed["xml"]
        return out
