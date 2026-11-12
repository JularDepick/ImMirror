#pragma once

namespace immirror::game::tuning {

/*
 * 容器系统调参. 原版熔炼包含热量, 氧气与燃烧值的多级模型, 首版按简化折算:
 * 熔炼一件固定时长; 燃料燃烧秒数按燃烧值线性折算. 原版 furnace.csv 的燃烧值
 * 实际分布 0 到 100 (木板类 30 最高), 折算取每单位 0.6 秒: 燃烧值 30 的木板
 * 烧 18 秒, 够熔 1 件余 8 秒, 与"一格燃料至少熔一件"的直觉一致.
 * 折算关系只在本文件, 调手感只改这里.
 */
constexpr float kSmeltSecondsPerItem = 10.0f;
constexpr float kFuelSecondsPerUnit = 0.6f;

/* 储物箱格数, 与原版大箱子的 27 格一致, 按 9 列 3 行排布. */
constexpr int32_t kChestSlots = 27;

}  /* namespace immirror::game::tuning */
