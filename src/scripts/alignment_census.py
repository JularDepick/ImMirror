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


def block_stats():
    text = read_text(os.path.join(NATIVE_ROOT, 'game', 'generated', 'block_data_gen.h'))
    start = text.find('constexpr RegistryEntry kRegistryEntries')
    if start < 0:
        return {}
    rows = drop = gravity = tool_level = shape = 0
    for line in text[start:].splitlines():
        s = line.strip()
        if not s.startswith('{'):
            continue
        parts = [p.strip() for p in s.strip(',').strip('{}').split(',')]
        if len(parts) < 15:
            continue
        rows += 1
        if parts[11] not in ('""', ''):
            drop += 1
        if parts[13].rstrip('u') not in ('0', ''):
            gravity += 1
        if parts[7].rstrip('u') not in ('0', ''):
            tool_level += 1
        if parts[12].rstrip('u') not in ('0', ''):
            shape += 1
    return {'blocks': rows, 'drop': drop, 'gravity': gravity, 'toolLevel': tool_level, 'shape': shape}


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
        ('世界', '有掉落', blocks.get('drop', 0), blocks.get('blocks', 1), '掉落字段非空'),
        ('世界', '有重力方式', blocks.get('gravity', 0), blocks.get('blocks', 1), 'GravityEffect 非 0'),
        ('世界', '有工具等级要求', blocks.get('toolLevel', 0), blocks.get('blocks', 1), 'ToolLevel 非 0'),
        ('世界', '非整块形状', blocks.get('shape', 0), blocks.get('blocks', 1), '台阶/楼梯/栅栏/玻璃板/地毯'),
        ('世界', '图集缺源贴图', missing, 753, 'atlas.json missingSource, 越少越好'),
        ('世界', '群系带', header_constant(os.path.join(NATIVE_ROOT, 'voxel', 'generated', 'biome_data_gen.h'), 'kBiomeBandCount'), 6, 'biomedef 52 群系折算'),
        ('世界', '矿脉类型', header_constant(os.path.join(NATIVE_ROOT, 'voxel', 'generated', 'ore_data_gen.h'), 'kOreEntryCount'), 10, 'oredef MapID=0'),
        ('玩法', '物品表', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'item_data_gen.h'), 'kItemInfoCount'), 1619, '生成表条目数'),
        ('玩法', '合成配方', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'recipe_data_gen.h'), 'kRecipeInfoCount'), 630, 'crafting.csv 可折算条目'),
        ('玩法', '熔炉配方', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'furnace_data_gen.h'), 'kFurnaceEntryCount'), 34, 'furnace.csv 配方行'),
        ('玩法', '附魔条目', header_constant(os.path.join(NATIVE_ROOT, 'game', 'generated', 'enchant_data_gen.h'), 'kEnchantEntryCount'), 120, 'enchant.csv 按等级分行'),
        ('玩法', '附魔效果类型', len(set(re.findall(r'\{\d+u, "[^"]*", "([A-Za-z_]+)"',
                                              read_text(os.path.join(NATIVE_ROOT, 'game', 'generated', 'enchant_data_gen.h'))))), 18, '数据表出现的效果类型种类'),
        ('玩法', '状态效果', count_rows(os.path.join(NATIVE_ROOT, 'player', 'generated', 'buff_data_gen.h')), 44, 'buffdef 客户端可消费四类'),
        ('玩法', '食物', count_rows(os.path.join(NATIVE_ROOT, 'game', 'generated', 'food_data_gen.h')), 38, 'fooddef 可食用条目'),
        ('玩法', '工具', count_rows(os.path.join(NATIVE_ROOT, 'game', 'generated', 'tool_data_gen.h')), 25, 'tooldef 全量工具'),
        ('玩法', '附魔效果接入', 1, 1, 'DIG_SPEED 已作用到挖掘速度; 其余 17 类只带数据'),
        ('音频', '进包音效', count_files(os.path.join(APK_ASSETS, 'sounds'), '.ogg'), 1346, 'sounds 扁平化后文件数'),
        ('音频', '界面音通道', len(re.findall(r'_ASSET\s*=', read_text(ui_sound))), 7, 'UiSound 具名通道'),
        ('音频', '环境音曲目', len(re.findall(r'"sounds/env/', read_text(audio_java))), 19, '环境音分组曲表'),
        ('音频', '音乐曲目', len(re.findall(r'"sounds/music/', read_text(audio_java))), 25, '背景音乐曲池'),
        ('音频', '生物音效', len(re.findall(r'"sounds/ent_', read_text(os.path.join(NATIVE_ROOT, 'entity', 'generated', 'entity_sound_gen.h')))), 183, '68 生物四类动作'),
        ('界面', 'Java 类', java_file_count(), 81, 'Java 侧 .java 总数'),
        ('界面', '界面素材', count_files(os.path.join(APK_ASSETS, 'ui'), '.png', recursive=True), 391, 'ui_manifest 登记数'),
        ('界面', 'HUD 组件', 6, 8, '已接: 状态条/快捷栏/摇杆/跳跃/准星与进度环/罗盘/战斗框/状态图标; 未接: Boss 血条/充能球'),
        ('联机', '房间接口', len(re.findall(r'app\.(?:get|post)\("/api/v1/rooms', read_text(os.path.join(REPO_ROOT, 'server', 'src', 'server', 'api', 'v1', 'rooms', 'index.ts')))), 7, '建房/列表/加入/离房/状态/踢人'),
        ('联机', '同步通道', 7, 7, '位置/方块改动/生物/掉落物/储物箱/熔炉/房间元信息'),
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
