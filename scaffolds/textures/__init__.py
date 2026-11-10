# -*- coding: utf-8 -*-
"""纹理脚手架的模块实现。

受理两类:

- `ktx11`: KTX 11 纹理 (可套二进制族外壳), 工程文件是 manifest.json 加 mip0.png。
  压缩块按 mip 级原样保存在 manifest 里, PNG 是可编辑的 0 级像素视图;
  PNG 未改动时按原块回写 (逐字节一致), 改动后按原像素格式重新压块并重算 mip 链。
- `otex`: 命名区块容器里的 TextureData 加 SurfaceData 纹理, 工程文件同上。
  像素格式实测为 10 (RGB8) 与 12 (RGBA8), 像素可经 PNG 编辑后回写。

另提供图集页批量入口 export_iconset / build_iconset: 图标集是 ui/mobile/textureN
下的一组 KTX 图集页, 按目录整体导出与回写, 单页格式与 ktx11 完全相同。
"""

from __future__ import annotations

import os
import struct

from .. import util
from forge.codecs import etc as etccodec
from forge.codecs import ktx as ktxcodec
from forge.codecs import named_block
from forge.util import png as pngutil

NAME = "textures"
TITLE = "纹理"
EXTENSIONS = (".ktx", ".otex")
FORMATS = {
    "ktx11": "KTX 11 纹理, 单面非数组, 压缩块可原样回写",
    "ktx11-raw": "KTX 11 纹理的整份兜底形态 (多面或数组, 只保留原始载荷)",
    "otex": "89 67 45 23 容器内的 TextureData 加 SurfaceData 纹理",
}

PROJECT_FILE = "manifest.json"
PNG_FILE = "mip0.png"
MAGIC_KTX11 = b"\xabKTX 11\xbb\r\n\x1a\n"
MAGIC_NAMED_BLOCK = b"\x89gE#"
HEADER_SIZE = 64

"""头部 13 个 u32 的字段名, 顺序与 Khronos KTX 11 规范一致。"""
HEADER_FIELDS = (
    "endianness",
    "glType",
    "glTypeSize",
    "glFormat",
    "glInternalFormat",
    "glBaseInternalFormat",
    "pixelWidth",
    "pixelHeight",
    "pixelDepth",
    "numberOfArrayElements",
    "numberOfFaces",
    "numberOfMipmapLevels",
    "bytesOfKeyValueData",
)

"""otex 的 SurfaceData 像素格式号, 实测只出现 10 与 12。"""
OTEX_FORMATS = {10: ("rgb8", 3), 12: ("rgba8", 4)}


def detect(path: str, data: bytes) -> str:
    plain, envelope = util.unwrap_envelope(data)
    if plain[:12] == MAGIC_KTX11:
        return "ktx11"
    if plain[:4] == MAGIC_NAMED_BLOCK:
        spans = named_block.split_blocks(plain)
        if any(span.name == "SurfaceData" for span in spans):
            return "otex"
        raise util.ScaffoldError("%s 是命名区块容器但没有 SurfaceData 块, 不属于纹理分类" % path)
    raise util.ScaffoldError(
        "%s 不是 KTX 或 otex 纹理 (剥壳后魔数 %s); 本分类受理 %s"
        % (path, plain[:8].hex(), " ".join(EXTENSIONS))
    )


"""KTX 容器层: 自己切分而不是只用 forge.codecs.ktx, 因为回写需要保留每级的填充字节。"""


def _parse_ktx(plain: bytes) -> dict:
    if len(plain) < HEADER_SIZE:
        raise util.ScaffoldError("KTX 头不足 64 字节: %d" % len(plain))
    little = struct.unpack_from("<I", plain, 12)[0] == 0x04030201
    layout = "<" if little else ">"
    values = struct.unpack_from(layout + "13I", plain, 12)
    header = dict(zip(HEADER_FIELDS, values))
    header["endianness"] = 0x04030201
    offset = HEADER_SIZE + header["bytesOfKeyValueData"]
    key_values = plain[HEADER_SIZE:offset]
    faces = max(1, header["numberOfFaces"])
    array = max(1, header["numberOfArrayElements"])
    levels = []
    for level in range(max(1, header["numberOfMipmapLevels"])):
        if offset + 4 > len(plain):
            raise util.ScaffoldError("KTX 第 %d 级缺少长度字段" % level)
        size = struct.unpack_from(layout + "I", plain, offset)[0]
        offset += 4
        total = size * faces * array
        if offset + total > len(plain):
            raise util.ScaffoldError("KTX 第 %d 级数据越界: 声明 %d 字节" % (level, total))
        payload = plain[offset : offset + total]
        offset += total
        levels.append(
            {
                "level": level,
                "width": max(1, header["pixelWidth"] >> level),
                "height": max(1, header["pixelHeight"] >> level),
                "size": size,
                "payload": payload,
            }
        )
    trailing = plain[offset:]
    return {"header": header, "layout": layout, "key_values": key_values,
            "levels": levels, "trailing": trailing}


def _assemble_ktx(doc: dict) -> bytes:
    header = doc.get("header") or {}
    missing = [name for name in HEADER_FIELDS if name != "endianness" and name not in header]
    if missing:
        raise util.ScaffoldError("header 缺字段: %s" % ", ".join(missing))
    layout = "<" if doc.get("endianness", "little") == "little" else ">"
    """端序标记按文件字节序写回 0x04030201, 与 forge.codecs.ktx 的口径一致。"""
    values = [0x04030201]
    for name in HEADER_FIELDS[1:]:
        value = header[name]
        if not isinstance(value, int):
            raise util.ScaffoldError("header.%s 必须是整数" % name)
        values.append(value)
    key_values = util.blob_field_bytes(doc.get("key_value_data"), "key_value_data")
    if len(key_values) != header["bytesOfKeyValueData"]:
        raise util.ScaffoldError(
            "key_value_data 长度 %d 与 header.bytesOfKeyValueData %d 不符"
            % (len(key_values), header["bytesOfKeyValueData"])
        )
    body = bytearray(MAGIC_KTX11)
    body += struct.pack(layout + "13I", *values)
    body += key_values
    for index, level in enumerate(doc.get("mips") or []):
        payload = util.blob_field_bytes(level.get("payload"), "第 %d 级载荷" % index)
        if level.get("size") != len(payload):
            raise util.ScaffoldError(
                "第 %d 级 size 字段 %r 与载荷长度 %d 不符"
                % (index, level.get("size"), len(payload))
            )
        body += struct.pack(layout + "I", len(payload)) + payload
    body += util.blob_field_bytes(doc.get("trailing"), "trailing")
    return bytes(body)


def _pixel_codec_name(header: dict):
    codec = etccodec.codec_for_format(header["glInternalFormat"])
    return codec.name if codec else None


def _export_ktx(plain: bytes, envelope: dict, doc: dict, project_dir: str) -> dict:
    try:
        parsed = _parse_ktx(plain)
    except util.ScaffoldError:
        return _export_ktx_raw(plain, envelope, doc, project_dir)
    faces = max(1, parsed["header"]["numberOfFaces"])
    array = max(1, parsed["header"]["numberOfArrayElements"])
    if faces != 1 or array != 1:
        return _export_ktx_raw(plain, envelope, doc, project_dir)
    doc["format"] = "ktx11"
    doc["envelope"] = envelope or None
    doc["endianness"] = "little" if parsed["layout"] == "<" else "big"
    doc["header"] = {key: value for key, value in parsed["header"].items() if key != "endianness"}
    doc["key_value_data"] = util.make_blob_field(parsed["key_values"], note="KTX 键值区原样字节")
    doc["pixel"] = {
        "internal_format": parsed["header"]["glInternalFormat"],
        "format_name": ktxcodec.FORMAT_NAMES.get(
            parsed["header"]["glInternalFormat"], "unknown"
        ),
        "codec": _pixel_codec_name(parsed["header"]),
    }
    doc["mips"] = [
        {
            "level": level["level"],
            "width": level["width"],
            "height": level["height"],
            "size": level["size"],
            "payload": util.make_blob_field(
                level["payload"], note="压缩块原样字节, 未改 PNG 时按此回写"
            ),
        }
        for level in parsed["levels"]
    ]
    doc["trailing"] = util.make_blob_field(parsed["trailing"], note="最后一级之后的残余字节")
    files = []
    png_path = os.path.join(project_dir, PNG_FILE)
    codec = _pixel_codec_name(parsed["header"])
    if codec:
        try:
            texture = ktxcodec.parse(plain)
            image = ktxcodec.to_png(texture)
            width, height, rgba = pngutil.read_png(image)
        except (ktxcodec.CodecError, pngutil.PngError) as exc:
            doc["pixels"] = {"error": str(exc)}
        else:
            util.write_bytes(png_path, image)
            files.append(png_path)
            doc["pixels"] = {
                "file": PNG_FILE,
                "width": width,
                "height": height,
                "rgba_sha256": util.sha256_bytes(rgba),
                "note": "PNG 是 0 级像素的可编辑视图; 像素与压缩块解码结果不一致时以 PNG 为准重压块",
            }
    else:
        doc["pixels"] = {"note": "像素格式未支持, 只保留压缩块"}
    doc["files"] = [PROJECT_FILE] + [os.path.basename(path) for path in files]
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": doc["format"],
        "project": target,
        "files": [target] + files,
        "pixels": bool(codec),
        "envelope": envelope or None,
        "source_sha256": doc["source"]["sha256"],
    }


def _export_ktx_raw(plain: bytes, envelope: dict, doc: dict, project_dir: str) -> dict:
    doc["format"] = "ktx11-raw"
    doc["envelope"] = envelope or None
    doc["payload"] = util.make_blob_field(
        plain, note="多面或数组 KTX 的结构切分未支持, 整份载荷原样保留"
    )
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "ktx11-raw",
        "project": target,
        "files": [target],
        "pixels": False,
        "envelope": envelope or None,
        "source_sha256": doc["source"]["sha256"],
    }


def _otex_texture(plain: bytes, blocks) -> dict:
    """从 otex 的 TextureData 与 SurfaceData 块解出尺寸与像素。"""
    texture = {}
    for block in blocks:
        name = block["name"]
        payload = block["payload"]
        if name == "TextureData" and len(payload) >= 32:
            fields = struct.unpack_from("<8I", payload, 0)
            texture.update(
                {
                    "width": fields[1],
                    "height": fields[2],
                    "format_id": fields[5],
                    "texture_data": list(fields),
                }
            )
        elif name == "SurfaceData" and len(payload) >= 32:
            fields = struct.unpack_from("<8I", payload, 0)
            texture.setdefault("width", fields[1])
            texture.setdefault("height", fields[2])
            texture["surface_fields"] = list(fields)
            texture["pixels"] = payload[32:]
    if "pixels" not in texture:
        return texture
    format_id = texture.get("format_id", texture.get("surface_fields", [0])[0])
    name, bpp = OTEX_FORMATS.get(format_id, ("unknown", 0))
    texture["pixel_format"] = name
    texture["bytes_per_pixel"] = bpp
    expected = texture["width"] * texture["height"] * bpp if bpp else -1
    texture["pixels_bytes_match"] = expected == len(texture["pixels"])
    return texture


def _rgba_from_otex_pixels(texture: dict) -> bytes:
    width = texture["width"]
    height = texture["height"]
    pixels = texture["pixels"]
    if texture["pixel_format"] == "rgba8":
        return bytes(pixels)
    if texture["pixel_format"] == "rgb8":
        out = bytearray(width * height * 4)
        for index in range(width * height):
            out[index * 4 : index * 4 + 3] = pixels[index * 3 : index * 3 + 3]
            out[index * 4 + 3] = 255
        return bytes(out)
    raise util.ScaffoldError("otex 像素格式未支持: %r" % texture.get("pixel_format"))


def _pixels_from_otex_rgba(texture: dict, rgba: bytes) -> bytes:
    width = texture["width"]
    height = texture["height"]
    if texture["pixel_format"] == "rgba8":
        return bytes(rgba)
    if texture["pixel_format"] == "rgb8":
        out = bytearray(width * height * 3)
        for index in range(width * height):
            out[index * 3 : index * 3 + 3] = rgba[index * 4 : index * 4 + 3]
        return bytes(out)
    raise util.ScaffoldError("otex 像素格式未支持: %r" % texture.get("pixel_format"))


def _export_otex(plain: bytes, envelope: dict, doc: dict, project_dir: str) -> dict:
    spans = named_block.split_blocks(plain)
    blocks = [
        {
            "index": index,
            "name": span.name,
            "type_id": span.type_id,
            "mode": span.mode,
            "payload": util.make_blob_field(plain[span.payload_offset : span.end]),
            "fields": named_block.block_fields(plain, span),
        }
        for index, span in enumerate(spans)
    ]
    version, header_flag = struct.unpack_from("<II", plain, 4)
    doc["format"] = "otex"
    doc["envelope"] = envelope or None
    doc["container"] = {"magic": plain[:4].hex(), "version": version, "header_flag": header_flag}
    doc["blocks"] = blocks
    raw_units = [(block["name"], block["type_id"], plain[span.payload_offset:span.end])
                 for block, span in zip(blocks, spans)]
    texture = _otex_texture(plain, [
        {"name": name, "payload": payload} for name, _type_id, payload in raw_units
    ])
    files = []
    if texture.get("pixel_format") in ("rgb8", "rgba8") and texture.get("pixels_bytes_match"):
        rgba = _rgba_from_otex_pixels(texture)
        image = pngutil.write_rgba(texture["width"], texture["height"], rgba)
        png_path = os.path.join(project_dir, PNG_FILE)
        util.write_bytes(png_path, image)
        files.append(png_path)
        doc["pixels"] = {
            "file": PNG_FILE,
            "width": texture["width"],
            "height": texture["height"],
            "rgba_sha256": util.sha256_bytes(rgba),
        }
    else:
        doc["pixels"] = {"note": "像素格式或长度未支持, 只保留块载荷"}
    doc["texture"] = {
        "width": texture.get("width"),
        "height": texture.get("height"),
        "format_id": texture.get("format_id"),
        "pixel_format": texture.get("pixel_format", "unknown"),
        "bytes_per_pixel": texture.get("bytes_per_pixel", 0),
    }
    doc["files"] = [PROJECT_FILE] + [os.path.basename(path) for path in files]
    target = os.path.join(util.ensure_dir(project_dir), PROJECT_FILE)
    util.write_json(target, doc)
    return {
        "category": NAME,
        "format": "otex",
        "project": target,
        "files": [target] + files,
        "pixels": bool(files),
        "envelope": envelope or None,
        "source_sha256": doc["source"]["sha256"],
    }


def export(src: str, project_dir: str) -> dict:
    data = util.read_bytes(src)
    fmt = detect(src, data)
    plain, envelope = util.unwrap_envelope(data)
    doc = util.make_doc(NAME, fmt, src, data)
    if fmt == "ktx11":
        return _export_ktx(plain, envelope, doc, project_dir)
    return _export_otex(plain, envelope, doc, project_dir)


def _build_ktx(doc: dict, project_dir: str) -> bytes:
    if doc.get("format") == "ktx11-raw":
        plain = util.blob_field_bytes(doc.get("payload"), "payload")
        return util.wrap_envelope(plain, doc.get("envelope") or {})
    plain = _assemble_ktx(doc)
    pixels = doc.get("pixels") or {}
    file_name = pixels.get("file")
    if not file_name:
        return util.wrap_envelope(plain, doc.get("envelope") or {})
    png_path = os.path.join(project_dir, file_name)
    if not os.path.isfile(png_path):
        raise util.ScaffoldError("工程声明了像素视图但文件缺失: %s" % png_path)
    width, height, wanted = pngutil.read_png(util.read_bytes(png_path))
    texture = ktxcodec.parse(plain)
    if (width, height) != (texture.width, texture.height):
        raise util.ScaffoldError(
            "PNG 尺寸 %dx%d 与 KTX %dx%d 不符" % (width, height, texture.width, texture.height)
        )
    current = ktxcodec.to_rgba(texture)
    if current == wanted:
        return util.wrap_envelope(plain, doc.get("envelope") or {})
    rebuilt = ktxcodec.rebuild(texture, wanted)
    rebuilt += util.blob_field_bytes(doc.get("trailing"), "trailing")
    doc["_reencoded"] = True
    doc["_pixel_diff"] = util.pixel_diff(wanted, ktxcodec.to_rgba(ktxcodec.parse(rebuilt)))
    return util.wrap_envelope(rebuilt, doc.get("envelope") or {})


def _build_otex(doc: dict, project_dir: str) -> bytes:
    units = util.named_block_units(doc)
    pixels = doc.get("pixels") or {}
    file_name = pixels.get("file")
    if file_name:
        png_path = os.path.join(project_dir, file_name)
        if not os.path.isfile(png_path):
            raise util.ScaffoldError("工程声明了像素视图但文件缺失: %s" % png_path)
        width, height, rgba = pngutil.read_png(util.read_bytes(png_path))
        texture = {
            "width": (doc.get("texture") or {}).get("width"),
            "height": (doc.get("texture") or {}).get("height"),
            "pixel_format": (doc.get("texture") or {}).get("pixel_format"),
        }
        if (width, height) != (texture["width"], texture["height"]):
            raise util.ScaffoldError(
                "PNG 尺寸 %dx%d 与 TextureData %rx%r 不符"
                % (width, height, texture["width"], texture["height"])
            )
        new_pixels = _pixels_from_otex_rgba(texture, rgba)
        rebuilt_units = []
        for name, type_id, payload in units:
            if name == "SurfaceData" and len(payload) >= 32:
                current = payload[32:]
                if current != new_pixels:
                    payload = payload[:32] + new_pixels
                    doc["_reencoded"] = True
            rebuilt_units.append((name, type_id, payload))
        units = rebuilt_units
    body = util.build_named_block(doc.get("container") or {}, units)
    return util.wrap_envelope(body, doc.get("envelope") or {})


def build(project_dir: str, out_path: str) -> dict:
    doc_path = util.find_project_json(project_dir, PROJECT_FILE)
    doc = util.read_json(doc_path)
    source = util.check_doc(doc, NAME, FORMATS)
    if doc["format"] in ("ktx11", "ktx11-raw"):
        data = _build_ktx(doc, project_dir)
    else:
        data = _build_otex(doc, project_dir)
    util.write_bytes(out_path, data)
    result = {
        "category": NAME,
        "format": doc["format"],
        "project": doc_path,
        "out": out_path,
        "size": len(data),
        "sha256": util.sha256_bytes(data),
        "source_sha256": source.get("sha256"),
        "byte_identical": util.sha256_bytes(data) == source.get("sha256"),
        "reencoded": bool(doc.get("_reencoded")),
    }
    if "_pixel_diff" in doc:
        result["pixel_diff"] = doc["_pixel_diff"]
    return result


"""图集页批量入口: 图标集是 ui/mobile/textureN 下的一组 KTX, 按目录整体处理。"""


def export_iconset(asset_dir: str, project_dir: str, prefix: str = "") -> dict:
    """把一个图集目录下的 KTX 逐页导出到 atlas.json 加 pages/ 子目录。"""
    pages = []
    os.makedirs(os.path.join(project_dir, "pages"), exist_ok=True)
    for dirpath, dirnames, filenames in os.walk(asset_dir):
        dirnames.sort()
        for name in sorted(filenames):
            if not name.lower().endswith(".ktx"):
                continue
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, asset_dir).replace(os.sep, "/")
            if prefix and not rel.startswith(prefix):
                continue
            flat = rel.replace("/", "__")
            page_dir = os.path.join(project_dir, "pages", flat)
            summary = export(path, page_dir)
            pages.append(
                {
                    "rel": rel,
                    "project": "pages/" + flat,
                    "format": summary["format"],
                    "source_sha256": summary["source_sha256"],
                }
            )
    doc = {
        "project": util.PROJECT_NAME,
        "version": util.PROJECT_VERSION,
        "category": NAME,
        "format": "ktx-atlas-set",
        "pages": pages,
    }
    target = os.path.join(util.ensure_dir(project_dir), "atlas.json")
    util.write_json(target, doc)
    return {"category": NAME, "format": "ktx-atlas-set", "project": target,
            "pages": len(pages)}


def build_iconset(project_dir: str, out_dir: str) -> dict:
    """按 atlas.json 把每一页回写成 KTX, 路径与图集目录一致。"""
    atlas_path = os.path.join(project_dir, "atlas.json")
    doc = util.read_json(atlas_path)
    if doc.get("format") != "ktx-atlas-set":
        raise util.ScaffoldError("atlas.json 的 format 不是 ktx-atlas-set")
    results = []
    for page in doc.get("pages") or []:
        page_dir = os.path.join(project_dir, *page["project"].split("/"))
        out_path = os.path.join(out_dir, *page["rel"].split("/"))
        results.append(build(page_dir, out_path))
    return {"category": NAME, "format": "ktx-atlas-set", "pages": len(results),
            "results": results}
