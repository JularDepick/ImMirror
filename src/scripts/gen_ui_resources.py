#!/usr/bin/env python3
"""从界面素材索引生成 Java 常量类与原生头文件.

输入: src/assets/ui/ui_manifest.json, 75 个素材的原始尺寸, 拉伸方式与九宫边界.
输出:
  src/android/app/src/main/java/com/julardepick/immirror/generated/UiResources.java
  src/native/ui/generated/ui_resources_gen.h
  src/android/app/src/main/assets/ui/<类别>/<名称>.png (默认复制, 加 --no-assets 跳过)

由 Gradle 的 genUiResources 任务与 scripts/build-native.sh 在构建期调用.
生成物列入 .gitignore, 尺寸与边界只维护 ui_manifest.json 一处, 避免两侧手写坐标.
"""

import json
import os
import shutil
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
MANIFEST_PATH = os.path.join(SRC_ROOT, "assets", "ui", "ui_manifest.json")
JAVA_OUT_PATH = os.path.join(SRC_ROOT, "android", "app", "src", "main", "java",
                             "com", "julardepick", "immirror", "generated", "UiResources.java")
NATIVE_OUT_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "ui_resources_gen.h")
ASSET_SOURCE_ROOT = os.path.join(SRC_ROOT, "assets", "ui")
ASSET_TARGET_ROOT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "ui")

JAVA_PACKAGE = "com.julardepick.immirror.generated"

# 索引里的 draw 字段与两侧枚举的对应关系, 未登记视为索引损坏
DRAW_TYPES = ("stretch", "three_slice", "nine_slice")

# 生成文件头部固定声明, 两侧共用同一句话说明来源与不可手改
GENERATED_NOTICE = "由 scripts/gen_ui_resources.py 从 assets/ui/ui_manifest.json 生成, 请勿手改"


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def load_assets():
    """读取索引并做基本校验, 返回按路径排序的素材列表."""
    with open(MANIFEST_PATH, "r", encoding="utf-8") as handle:
        document = json.load(handle)
    assets = document.get("assets")
    if not assets:
        raise SystemExit("索引中没有 assets 字段: %s" % MANIFEST_PATH)
    seen = set()
    for asset in assets:
        path = asset["path"]
        if path in seen:
            raise SystemExit("索引中存在重复素材: %s" % path)
        seen.add(path)
        if asset["draw"] not in DRAW_TYPES:
            raise SystemExit("未知拉伸方式 %s: %s" % (asset["draw"], path))
        insets = asset["insets"]
        # 少数素材的四边之和正好等于或略超原始边长, 例如 2 像素宽的竖线声明左右各 2,
        # 此时中段为零宽, 由渲染器按比例压缩四边, 生成阶段只提示不阻断.
        if insets["left"] + insets["right"] >= asset["width"]:
            emit("提示: %s 横向无拉伸区, 渲染时按比例压缩左右边界" % path)
        if insets["top"] + insets["bottom"] >= asset["height"]:
            emit("提示: %s 纵向无拉伸区, 渲染时按比例压缩上下边界" % path)
        if not os.path.exists(os.path.join(ASSET_SOURCE_ROOT, path)):
            raise SystemExit("索引登记了不存在的素材文件: %s" % path)
    assets.sort(key=lambda item: item["path"])
    return assets


def widget_name(path):
    """素材路径转枚举名, 例如 button/primary_blue.png 得 BUTTON_PRIMARY_BLUE."""
    stem = path.rsplit(".", 1)[0]
    parts = []
    for chunk in stem.split("/"):
        cleaned = "".join(character if character.isalnum() else "_" for character in chunk)
        parts.append(cleaned.strip("_").upper())
    return "_".join(parts)


def draw_enum_name(draw):
    return {"stretch": "STRETCH", "three_slice": "THREE_SLICE", "nine_slice": "NINE_SLICE"}[draw]


def draw_native_name(draw):
    return {"stretch": "Stretch", "three_slice": "ThreeSlice", "nine_slice": "NineSlice"}[draw]


def pascal_name(upper_name):
    """下划线大写转帕斯卡命名, 供原生枚举使用."""
    return "".join(chunk.capitalize() for chunk in upper_name.split("_") if chunk)


def write_if_changed(path, text):
    """内容变化时才写盘, 避免每次构建都刷新时间戳."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8", newline="") as handle:
            if handle.read() == text:
                return False
    temporary = path + ".tmp"
    with open(temporary, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    os.replace(temporary, path)
    return True


def render_java(assets):
    """生成 Java 常量类: 控件枚举携带路径, 尺寸与九宫边界."""
    lines = []
    lines.append("package %s;" % JAVA_PACKAGE)
    lines.append("")
    lines.append("/*")
    lines.append(" * 界面素材参数表, %s." % GENERATED_NOTICE)
    lines.append(" * 覆盖写入; 需要调整尺寸或边界请改素材索引后重新构建.")
    lines.append(" */")
    lines.append("public final class UiResources {")
    lines.append("")
    lines.append("    /* 素材在 APK assets 内的根目录, 与 path 字段拼接得到完整路径. */")
    lines.append('    public static final String ASSET_ROOT = "ui";')
    lines.append("")
    lines.append("    /* 拉伸方式, 与索引的 draw 字段一一对应. */")
    lines.append("    public enum DrawType {")
    lines.append("        STRETCH,")
    lines.append("        THREE_SLICE,")
    lines.append("        NINE_SLICE")
    lines.append("    }")
    lines.append("")
    lines.append("    /*")
    lines.append("     * 控件枚举: 常量名由素材路径派生, 尺寸与九宫边界均为素材原始像素,")
    lines.append("     * 渲染时由 NineSliceDrawable 按 artScale 换算成屏幕像素.")
    lines.append("     */")
    lines.append("    public enum Widget {")
    body = []
    for index, asset in enumerate(assets):
        insets = asset["insets"]
        tail = ";" if index == len(assets) - 1 else ","
        body.append('        %s("%s", %d, %d, DrawType.%s, %d, %d, %d, %d)%s' % (
            widget_name(asset["path"]), asset["path"], asset["width"], asset["height"],
            draw_enum_name(asset["draw"]), insets["left"], insets["top"],
            insets["right"], insets["bottom"], tail))
    lines.extend(body)
    lines.append("")
    lines.append("        public final String path;")
    lines.append("        public final int width;")
    lines.append("        public final int height;")
    lines.append("        public final DrawType draw;")
    lines.append("        public final int insetLeft;")
    lines.append("        public final int insetTop;")
    lines.append("        public final int insetRight;")
    lines.append("        public final int insetBottom;")
    lines.append("")
    lines.append("        Widget(String path, int width, int height, DrawType draw,")
    lines.append("                int insetLeft, int insetTop, int insetRight, int insetBottom) {")
    lines.append("            this.path = path;")
    lines.append("            this.width = width;")
    lines.append("            this.height = height;")
    lines.append("            this.draw = draw;")
    lines.append("            this.insetLeft = insetLeft;")
    lines.append("            this.insetTop = insetTop;")
    lines.append("            this.insetRight = insetRight;")
    lines.append("            this.insetBottom = insetBottom;")
    lines.append("        }")
    lines.append("")
    lines.append("        /* 素材在 APK assets 内的完整路径. */")
    lines.append("        public String assetPath() {")
    lines.append('            return ASSET_ROOT + "/" + path;')
    lines.append("        }")
    lines.append("")
    lines.append("        /* 是否需要切片绘制; 整体拉伸的素材可直接用位图绘制. */")
    lines.append("        public boolean sliced() {")
    lines.append("            return draw != DrawType.STRETCH;")
    lines.append("        }")
    lines.append("    }")
    lines.append("")
    lines.append("    private UiResources() {")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def render_native(assets):
    """生成 C++ 头文件: 同名枚举与常量表, 供原生侧九宫绘制取参数."""
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("/*")
    lines.append(" * 界面素材参数表, %s." % GENERATED_NOTICE)
    lines.append(" * 覆盖写入; 原生侧只用尺寸与九宫边界, 路径留作日志定位.")
    lines.append(" */")
    lines.append("namespace immirror::ui::gen {")
    lines.append("")
    lines.append("/* 拉伸方式, 与索引的 draw 字段一一对应. */")
    lines.append("enum class UiDraw : uint8_t {")
    lines.append("    Stretch,")
    lines.append("    ThreeSlice,")
    lines.append("    NineSlice,")
    lines.append("};")
    lines.append("")
    lines.append("/* 控件枚举, 顺序与素材路径的字典序一致, 与 kUiAssets 逐项对应. */")
    lines.append("enum class UiWidget : uint16_t {")
    for asset in assets:
        lines.append("    %s," % pascal_name(widget_name(asset["path"])))
    lines.append("    Count,")
    lines.append("};")
    lines.append("")
    lines.append("/* 一个控件素材: 素材原始宽高与四边固定像素, 单位都是素材像素. */")
    lines.append("struct UiAsset {")
    lines.append("    const char* path;")
    lines.append("    float width;")
    lines.append("    float height;")
    lines.append("    UiDraw draw;")
    lines.append("    float left;")
    lines.append("    float top;")
    lines.append("    float right;")
    lines.append("    float bottom;")
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr UiAsset kUiAssets[] = {")
    for asset in assets:
        insets = asset["insets"]
        lines.append('    {"%s", %.1ff, %.1ff, UiDraw::%s, %.1ff, %.1ff, %.1ff, %.1ff},' % (
            asset["path"], asset["width"], asset["height"], draw_native_name(asset["draw"]),
            insets["left"], insets["top"], insets["right"], insets["bottom"]))
    lines.append("};")
    lines.append("")
    lines.append("inline constexpr size_t kUiAssetCount = sizeof(kUiAssets) / sizeof(kUiAssets[0]);")
    lines.append("")
    lines.append('static_assert(kUiAssetCount == static_cast<size_t>(UiWidget::Count),')
    lines.append('              "控件枚举与素材表条目数不一致, 请重新运行生成脚本");')
    lines.append("")
    lines.append("/* 按枚举取素材参数, 越界时回落到首项, 避免绘制路径读到脏数据. */")
    lines.append("inline const UiAsset& UiAssetOf(UiWidget widget) {")
    lines.append("    const size_t index = static_cast<size_t>(widget);")
    lines.append("    return index < kUiAssetCount ? kUiAssets[index] : kUiAssets[0];")
    lines.append("}")
    lines.append("")
    lines.append("}  /* namespace immirror::ui::gen */")
    lines.append("")
    return "\n".join(lines)


def copy_assets(assets):
    """把索引内登记的素材复制到 APK assets, 目标目录由 .gitignore 忽略."""
    copied = 0
    for asset in assets:
        source = os.path.join(ASSET_SOURCE_ROOT, asset["path"])
        target = os.path.join(ASSET_TARGET_ROOT, asset["path"])
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copy2(source, target)
        copied += 1
    return copied


def main(argv):
    copy = "--no-assets" not in argv
    unknown = [item for item in argv[1:] if item not in ("--no-assets",)]
    if unknown:
        emit("忽略未知参数: %s" % " ".join(unknown))
    assets = load_assets()
    java_changed = write_if_changed(JAVA_OUT_PATH, render_java(assets))
    native_changed = write_if_changed(NATIVE_OUT_PATH, render_native(assets))
    emit("界面参数表: %d 个素材, 九宫 %d, 三片 %d, 整体拉伸 %d" % (
        len(assets),
        sum(1 for item in assets if item["draw"] == "nine_slice"),
        sum(1 for item in assets if item["draw"] == "three_slice"),
        sum(1 for item in assets if item["draw"] == "stretch")))
    emit("  Java  %s (%s)" % (os.path.relpath(JAVA_OUT_PATH, SRC_ROOT),
                              "已更新" if java_changed else "未变化"))
    emit("  C++   %s (%s)" % (os.path.relpath(NATIVE_OUT_PATH, SRC_ROOT),
                              "已更新" if native_changed else "未变化"))
    if copy:
        emit("  素材  %d 个已复制到 %s" % (copy_assets(assets),
                                          os.path.relpath(ASSET_TARGET_ROOT, SRC_ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
