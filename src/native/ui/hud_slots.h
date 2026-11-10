#pragma once

#include <cstdint>

#include "ui/hud_layout.h"
#include "voxel/block_registry.h"

namespace immirror::ui {

/*
 * 底部方块栏的唯一槽位表. 槽位顺序在本表定义, 名称与图集格从方块注册表派生,
 * 界面绘制与玩法编辑共用这一份定义, 不再在界面层与总装各硬编码一份,
 * 方块顺序或图集编号调整时只改这里.
 */
constexpr int32_t kBlockSlotCount = kHudSlotCount;

voxel::BlockId BlockSlotId(int32_t slot);

/* 方块名来自注册表, 生存期与进程一致. */
const char* BlockSlotName(int32_t slot);

/* 方块栏图标使用的图集格, 取顶面, 未指定顶面时回落到 atlasIndex. */
int32_t BlockSlotAtlasCell(int32_t slot);

/* 反查槽位序号, 未收录返回 -1. */
int32_t BlockSlotIndexOf(voxel::BlockId id);

}  /* namespace immirror::ui */
