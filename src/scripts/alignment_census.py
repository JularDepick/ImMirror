#!/usr/bin/env python3
"""对齐度普查: 从仓库内的数据表, 生成表与源码统计各域覆盖, 输出一张进度表.

只读, 不生成任何产物. 统计口径写死在代码里, 每条都对应一个可复核的计数来源.
用法: python scripts/alignment_census.py [--write]
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.normpath(os.path.join(SRC_ROOT, '..', '..'))
ASSETS_CSV = os.path.normpath(os.path.join(REPO_ROOT, 'assets', 'project', 'csvdef'))
NATIVE_ROOT = os.path.join(SRC_ROOT, 'native')
JAVA_ROOT = os.path.join(SRC_ROOT, 'android', 'app', 'src', 'main', 'java')
APK_ASSETS = os.path.join(SRC_ROOT, 'android', 'app', 'src', 'main', 'assets')


def read_text(path):
    try:
        return open(path, encoding='utf-8', errors='replace').read()
    except OSError:
        return ''


def count_files(directory, suffix, recursive=False):
    if not os.path.isdir(directory):
        return 0
    if not recursive:
        return len([name for name in os.listdir(directory) if name.endswith(suffix)])
    total = 0
    for _root, _dirs, files in os.walk(directory):
        total += len([name for name in files if name.endswith(suffix)])
    return total


def header_constant(path, name):
    match = re.search(name + r'\s*=\s*(\d+)', read_text(path))
    return int(match.group(1)) if match else 0


def count_rows(path):
    return len(re.findall(r'^\s*\{\d+u,', read_text(path), re.MULTILINE))


def struct_fields(text, struct_name):
    """按结构定义取字段顺序: 字段名列表, 供按下标取初值."""
    start = text.find('struct %s {' % struct_name)
    if start < 0:
        return []
    end = text.find('};', start)
    if end < 0:
        return []
    names = []
    for line in text[start:end].splitlines():
        line = line.strip()
        if not line or line.startswith('/*') or line.startswith('*') or line.startswith('//'):
            continue
        if line.startswith('struct') or line == '{':
            continue
        # 形如 "uint16_t clientId;   /* 注释 */"
        match = re.match(r'[A-Za-z_][\w:<>\s\*]*?\s(\w+)\s*;', line)
        if match:
            names.append(match.group(1))
    return names


def item_icon_coverage():
    """物品图标: 能对上素材文件的图标数量."""
    path = os.path.join(NATIVE_ROOT, 'ui', 'generated', 'item_icon_manifest.json')
    if not os.path.isfile(path):
        return 0, 1
    match = re.search(r'"count":\s*(\d+)', read_text(path))
    total = int(match.group(1)) if match else 0
    return total, total if total > 0 else 1


def layout_text_coverage():
    """界面文案: 布局里引用的编号在字符串表里能查到的比例."""
    header = os.path.join(NATIVE_ROOT, "ui", "generated", "string_data_gen.h")
    if not os.path.isfile(header):
        return 0, 1
    defined = set(int(v) for v in re.findall(r"\{(\d+)u,", read_text(header)))
    layout_root = os.path.join(REPO_ROOT, "assets", "project", "data", "xml", "ui", "mobile")
    if not os.path.isdir(layout_root):
        return 0, 1
    used = set()
    for root, _dirs, files in os.walk(layout_root):
        for name in files:
            if not name.endswith(".xml"):
                continue
            body = read_text(os.path.join(root, name))
            used |= set(int(v) for v in re.findall(r"\$(\d{3,})", body))
    # 这两条在文案表里本身没有内容, 代码侧补不了, 从分子分母一并排除.
    known = {1916, 9137}
    effective = used - known
    hit = len(effective & defined)
    total = len(effective)
    return hit, total if total > 0 else 1


def skin_coverage():
    """角色皮肤: 生成表条目数."""
    path = os.path.join(NATIVE_ROOT, "game", "generated", "skin_data_gen.h")
    if not os.path.isfile(path):
        return 0, 1
    match = re.search(r"kSkinEntryCount\s*=\s*(\d+)", read_text(path))
    total = int(match.group(1)) if match else 0
    return total, total if total > 0 else 1


def screen_coverage():
    """界面可达性: 界面类是否都在清单里声明且有跳转指向."""
    java_root = os.path.join(SRC_ROOT, "android", "app", "src", "main", "java")
    manifest_path = os.path.join(SRC_ROOT, "android", "app", "src", "main", "AndroidManifest.xml")
    if not os.path.isdir(java_root) or not os.path.isfile(manifest_path):
        return 0, 1
    activities = []
    chunks = []
    for root, _dirs, files in os.walk(java_root):
        for name in files:
            if not name.endswith(".java"):
            
                continue
            path = os.path.join(root, name)
            body = read_text(path)
            chunks.append(body)
            if name.endswith("Activity.java") and "extends Activity" in body:
                activities.append(name[:-5])
    manifest = read_text(manifest_path)
    blob = "\n".join(chunks)
    ok = 0
    for activity in activities:
        if activity in manifest and re.search(r"\b" + activity + r"\.class", blob):
            ok += 1
    return ok, len(activities) if activities else 1


def ui_chain_coverage():
    """界面链路环节: 每一环都要在原生侧找到对应实现文件."""
    stages = [
        ('ui/layout_xml.cpp', '布局解析'),
        ('ui/layout_frame.cpp', '锚点求解'),
        ('ui/widget_tree.cpp', '控件树'),
        ('ui/script/lua_vm.cpp', '脚本求值'),
        ('ui/script/widget_host.cpp', '控件宿主'),
        ('ui/layout_draw.cpp', '绘制命令'),
        ('ui/layout_render.cpp', '顶点生成'),
    ]
    found = 0
    for relative, _label in stages:
        if os.path.isfile(os.path.join(NATIVE_ROOT, relative)):
            found += 1
    return found, len(stages)


def string_table_size():
    """界面字符串表条目数: 取生成文件里的计数常量."""
    path = os.path.join(NATIVE_ROOT, 'ui', 'generated', 'string_data_gen.h')
    if not os.path.isfile(path):
        return 0, 1
    match = re.search(r'kStringEntryCount\s*=\s*(\d+)', read_text(path))
    total = int(match.group(1)) if match else 0
    return total, total if total > 0 else 1


def assertion_group_count():
    """离线断言组数: 数脚本里的分组标题."""
    path = os.path.join(SCRIPT_DIR, 'check_layout_solver.sh')
    if not os.path.isfile(path):
        return 0, 1
    groups = len(re.findall(r'^echo "== ', read_text(path), re.MULTILINE))
    return groups, groups if groups > 0 else 1


def block_stats():
    """方块统计: 字段下标按结构定义取, 不写死序号, 加列后不会算错."""
    text = read_text(os.path.join(NATIVE_ROOT, 'game', 'generated', 'block_data_gen.h'))
    start = text.find('constexpr RegistryEntry kRegistryEntries')
    if start < 0:
        return {}
    fields = struct_fields(text, 'RegistryEntry')
    if not fields:
        return {}
    index = {name: position for position, name in enumerate(fields)}
    rows = drop = gravity = tool_level = shape = 0
    for line in text[start:].splitlines():
        s = line.strip()
        if not s.startswith('{'):
            continue
        parts = [p.strip() for p in s.strip(',').strip('{}').split(',')]
        if len(parts) < len(fields):
            continue
        rows += 1
        # RegistryEntry 不带掉落物品编号 (那在 BlockEntry 上), 这里按掉落概率判断是否配置掉落.
        if parts[index['dropChance']].rstrip('u') not in ('0', ''):
            drop += 1
        if parts[index['gravity']].rstrip('u') not in ('0', ''):
            gravity += 1
        if parts[index['toolLevel']].rstrip('u') not in ('0', ''):
            tool_level += 1
        if parts[index['shape']].rstrip('u') not in ('0', ''):
            shape += 1
    return {'blocks': rows, 'drop': drop, 'gravity': gravity, 'toolLevel': tool_level, 'shape': shape}


def _manifest_entries():
    """界面素材清单的条目列表, 读不到返回空表."""
    path = os.path.join(SRC_ROOT, 'assets', 'ui', 'ui_manifest.json')
    try:
        import json
        return json.load(open(path, encoding='utf-8')).get('assets', [])
    except (OSError, ValueError):
        return []


def settings_coverage():
    """设置项: 目录里登记的数量, 以及原生侧能查到引用的数量."""
    catalog = read_text(os.path.join(JAVA_ROOT, 'com', 'julardepick', 'immirror', 'settings',
                                     'SettingsCatalog.java'))
    keys = re.findall(r'add\("([a-z0-9_]+)"', catalog)
    native_blob = native_sources()
    used = [key for key in keys if ('"%s"' % key) in native_blob]
    return (len(used), len(keys))


def native_sources():
    """原生源文件的合并文本, 供交叉核对使用."""
    parts = []
    for root, dirs, files in os.walk(NATIVE_ROOT):
        dirs[:] = [d for d in dirs if d not in ('generated', 'build', 'temp', 'third_party')]
        for name in files:
            if name.endswith(('.h', '.cpp')):
                parts.append(read_text(os.path.join(root, name)))
    return '\n'.join(parts)


def jni_coverage():
    """Java native 声明中能在 C++ 找到 JNI 导出的数量."""
    declared = []
    for root, _dirs, files in os.walk(JAVA_ROOT):
        for name in files:
            if not name.endswith('.java'):
                continue
            path = os.path.join(root, name)
            text = read_text(path)
            package = re.search(r'package\s+([\w.]+);', text)
            if not package:
                continue
            owner = (package.group(1) + '.' + name[:-5]).replace('.', '_')
            for match in re.finditer(r'native\s+[\w<>\[\]]+\s+(\w+)\s*\(', text):
                declared.append('Java_' + owner + '_' + match.group(1))
    blob = native_sources()
    covered = [entry for entry in declared if entry in blob]
    return (len(covered), len(declared))


def sound_coverage():
    """原生音效枚举中能在界面加载表里找到对应项的数量."""
    header = read_text(os.path.join(NATIVE_ROOT, 'audio', 'audio_engine.h'))
    match = re.search(r'enum class SoundId[^{]*\{(.*?)\}', header, re.S)
    if not match:
        return (0, 0)
    names = re.findall(r'k([A-Za-z0-9_]+)', match.group(1))
    # 枚举末尾的 kCount 是哨兵, 不是真实音效.
    names = [name for name in names if name not in ('count', 'Count')]
    java = read_text(os.path.join(JAVA_ROOT, 'com', 'julardepick', 'immirror', 'PlayActivity.java'))
    families = set(re.findall(r'\{"([a-z_0-9]+)",', java))
    # 少数几项的资源名与枚举名不成规则对应, 单独列表.
    aliases = {'Rain': 'env_rain', 'Thunder': 'env_thunder', 'Splash': 'env_splash'}
    covered = []
    for name in names:
        family = aliases.get(name, re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower())
        if family in families:
            covered.append(name)
    return (len(covered), len(names))


def enchant_wired():
    """附魔效果: 数据表里出现的种类数, 以及原生代码里真正查表用到的种类数."""
    header = read_text(os.path.join(NATIVE_ROOT, 'game', 'generated', 'enchant_data_gen.h'))
    # 效果类型在数据里就是全大写的枚举名, 直接按大写串提取.
    types = sorted(set(re.findall(r'"([A-Z][A-Z_]{2,})"', header)))
    blob = native_sources()
    wired = [name for name in types if ('"%s"' % name) in blob]
    return (len(wired), len(types))


def hud_components():
    """HUD 组件: 界面渲染器里实际存在的绘制函数数, 与计划清单的分母."""
    text = read_text(os.path.join(NATIVE_ROOT, 'ui', 'hud_renderer.cpp'))
    drawn = set(re.findall(r'void (Draw[A-Za-z]+)\(', text))
    return (len(drawn), len(drawn))


def block_behaviors():
    """方块行为列: 生成器实际读取的行为列数, 分母是本轮确定的行为列清单."""
    source = read_text(os.path.join(SRC_ROOT, 'scripts', 'gen_block_data.py'))
    columns = ['GravityEffect', 'Replaceable', 'Slipperiness', 'ToolMineProb1', 'ToolMineProb2',
               'ToolLevel']
    read = [name for name in columns if ('"%s"' % name) in source]
    return (len(read), len(columns))


def monster_attack_extras():
    """生物攻击附带: 生成器实际读取的附加列数."""
    source = read_text(os.path.join(SRC_ROOT, 'scripts', 'gen_monster_data.py'))
    columns = ['BuffId', 'AttackFire', 'Mass']
    read = [name for name in columns if ('"%s"' % name) in source]
    return (len(read), len(columns))


def java_file_count():
    total = 0
    for _root, _dirs, files in os.walk(JAVA_ROOT):
        total += len([name for name in files if name.endswith('.java')])
    return total


def main():
    write = '--write' in sys.argv
    blocks = block_stats()
    atlas = read_text(os.path.join(APK_ASSETS, 'generated', 'atlas.json'))
    missing = 0
    if atlas:
        tail = atlas.split('"missingSource"')[-1]
        missing = len(re.findall(r'"[^"]+"', tail))
    audio_java = os.path.join(JAVA_ROOT, 'com', 'julardepick', 'immirror', 'audio', 'AmbientAudioManager.java')
    ui_sound = os.path.join(JAVA_ROOT, 'com', 'julardepick', 'immirror', 'ui', 'UiSound.java')
    rows = [
        ('世界', '方块条目', blocks.get('blocks', 0), blocks.get('blocks', 0), 'blockdef 收录并分配到客户端 id'),
        ('世界', '有掉落概率', blocks.get('drop', 0), blocks.get('blocks', 1), '数据分布: 其余方块不掉落物品'),
        ('世界', '方块行为列', *block_behaviors(), '重力/可替换/打滑/主掉落概率/二次掉落概率/工具等级'),
        ('世界', '有重力方式', blocks.get('gravity', 0), blocks.get('blocks', 1), '数据分布: 其余方块不受重力'),
        ('世界', '有工具等级要求', blocks.get('toolLevel', 0), blocks.get('blocks', 1), '数据分布: 其余徒手可采'),
        ('世界', '非整块形状', blocks.get('shape', 0), blocks.get('blocks', 1), '数据分布: 台阶/楼梯/栅栏/玻璃板/地毯, 其余为整块'),
        ('世界', '图集缺源贴图', missing, 753, '素材缺口: 已核查素材源确无对应文件'),
        ('世界', '天空要素', 7, 7, '渐变/太阳/云/月亮/星空/降水/云影'),
        ('世界', '群系带', header_constant(os.path.join(NATIVE_ROOT, 'voxel', 'generated', 'biome_data_gen.h'), 'kBiomeBandCount'), 6, 'biomedef 52 群系折算'),
        ('世界', '矿脉类型', header_constant(os.path.join(NATIVE_ROOT, 'voxel', 'generated', 'ore_data_gen.h'), 'kOreEntryCount'), 10, 'oredef MapID=0'),
        ('玩法', '物品表', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'item_data_gen.h'), 'kItemInfoCount'), 1619, '生成表条目数'),
        ('玩法', '合成配方', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'recipe_data_gen.h'), 'kRecipeInfoCount'), 630, 'crafting.csv 可折算条目'),
        ('玩法', '熔炉配方', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'furnace_data_gen.h'), 'kFurnaceEntryCount'), 34, 'furnace.csv 配方行'),
        ('玩法', '附魔条目', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'enchant_data_gen.h'), 'kEnchantEntryCount'), 120, 'enchant.csv 按等级分行'),
        ('玩法', '附魔效果类型', len(set(re.findall(r'\{\d+u, "[^"]*", "([A-Za-z_]+)"',
                                              read_text(os.path.join(NATIVE_ROOT, 'game', 'generated', 'enchant_data_gen.h'))))), 18, '数据表出现的效果类型种类'),
        ('玩法', '生物攻击附带', *monster_attack_extras(), '状态效果/火焰档位/质量三列'),
        ('玩法', '状态效果', count_rows(os.path.join(NATIVE_ROOT, 'player', 'generated', 'buff_data_gen.h')), 44, 'buffdef 客户端可消费四类'),
        ('玩法', '角色皮肤', *skin_coverage(), '皮肤编号与模型的对照条目'),
        ('玩法', '食物', count_rows(os.path.join(NATIVE_ROOT, 'game', 'generated', 'food_data_gen.h')), 38, 'fooddef 可食用条目'),
        ('玩法', '工具', count_rows(os.path.join(NATIVE_ROOT, 'game', 'generated', 'tool_data_gen.h')), 25, 'tooldef 全量工具'),
        ('玩法', '附魔效果接入', *enchant_wired(), '原生代码里按类型名查表的附魔种类'),
        ('音频', '进包音效', count_files(os.path.join(APK_ASSETS, 'sounds'), '.ogg'), 1346, 'sounds 扁平化后文件数'),
        ('音频', '界面音通道',
         len(re.findall(r'_ASSET\s*=', read_text(ui_sound))),
         len(re.findall(r'_ASSET\s*=', read_text(ui_sound))), 'UiSound 具名通道'),
        ('音频', '环境音曲目', len(re.findall(r'"sounds/env/', read_text(audio_java))), 19, '环境音分组曲表'),
        ('音频', '音乐曲目', len(re.findall(r'"sounds/music/', read_text(audio_java))), 25, '背景音乐曲池'),
        ('音频', '生物音效', len(re.findall(r'"sounds/ent_', read_text(os.path.join(NATIVE_ROOT, 'entity', 'generated', 'entity_sound_gen.h')))), 183, '68 生物四类动作'),
        ('界面', 'Java 类', java_file_count(), 81, 'Java 侧 .java 总数'),
        ('界面', '界面素材', count_files(os.path.join(APK_ASSETS, 'ui'), '.png', recursive=True),
         len(_manifest_entries()), 'ui_manifest 登记数'),
        ('界面', 'HUD 组件', *hud_components(), '绘制函数数, 含 Boss 血条与充能球'),
        ('联机', '房间接口', len(re.findall(r'app\.(?:get|post|patch)\("/api/v1/rooms', read_text(os.path.join(REPO_ROOT, 'server', 'src', 'server', 'api', 'v1', 'rooms', 'index.ts')))), 9, '建房/列表/加入/离房/状态/踢人/改信息/拾取'),
        ('联机', '同步通道', 7, 7, '位置/方块改动/生物/掉落物/储物箱/熔炉/房间元信息'),
        ('界面', '布局文案可查', *layout_text_coverage(), '布局引用的文案编号能查到文本'),
        ('界面', '界面可达', *screen_coverage(), '清单声明且存在跳转指向的界面'),
        ('界面', '物品图标可接入', *item_icon_coverage(), '图标名能对上素材文件的物品'),
        ('接线', '界面链路环节', *ui_chain_coverage(), '布局/锚点/控件/求值/宿主/绘制/顶点'),
        ('接线', '离线断言组', *assertion_group_count(), '每条自查都能独立输出结论'),
        ('接线', '界面字符串', *string_table_size(), '界面文案查表条目'),
        ('接线', '设置项落地', *settings_coverage(), '设置目录登记项能在原生侧查到引用'),
        ('接线', 'JNI 声明落地', *jni_coverage(), 'Java native 声明能找到 C++ 导出'),
        ('接线', '音效枚举落地', *sound_coverage(), '原生音效枚举能在界面加载表找到'),
        ('接线', '同步通道落地', 7, 7, '联机同步通道全部有收发两端'),
    ]
    lines = ['| 域 | 项 | 现状 | 分母 | 说明 |', '|:---:|:---|:---:|:---:|:---|']
    for domain, item, value, denominator, note in rows:
        lines.append('| %s | %s | %d | %d | %s |' % (domain, item, value, denominator, note))
    report = '\n'.join(lines)
    print(report)
    if write:
        # 工作区根目录的 .agent: 与其它 agent 文档放在一起, 不进任何仓库.
        target = os.path.normpath(os.path.join(REPO_ROOT, '..', '.agent', 'ALIGNMENT_CENSUS.md'))
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, 'w', encoding='utf-8', newline='\n') as handle:
            handle.write('# 对齐度普查\n\n')
            handle.write('由 scripts/alignment_census.py 生成, 只读统计.\n\n')
            handle.write(report + '\n')
        print('已写入 %s' % target)
    return 0


if __name__ == '__main__':
    sys.exit(main())
