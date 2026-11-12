#pragma once

namespace immirror::game::tuning {

/*
 * 容器系统调参. furnace.csv 的 Heat 列是燃料热量, 分布 160 到 24000,
 * furnace.lua 按 Heat>0 判燃料. 熔炼一件固定时长, 燃料秒数按热量线性折算,
 * 取每 100 热量 1 秒: 160 热量烧 1.6 秒, 24000 热量烧 240 秒.
 * 折算关系只在本文件, 调手感只改这里.
 */
constexpr float kSmeltSecondsPerItem = 10.0f;
constexpr float kFuelSecondsPerUnit = 0.01f;

/* 储物箱格数, storagebox.lua 的 STORAGEBOX_GRID_MAX = 60. */
constexpr int32_t kChestSlots = 60;

}  /* namespace immirror::game::tuning */
