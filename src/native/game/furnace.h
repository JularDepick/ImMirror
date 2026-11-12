#pragma once

#include <cstdint>
#include <unordered_map>

#include "game/container_tuning.h"
#include "game/item/item_def.h"

namespace immirror::game {

using item::ItemId;
using item::ItemStack;

/*
 * 熔炉数据表与熔炉实例.
 *
 * 数据来自 scripts/gen_furnace_data.py 生成的 furnace_data_gen.h, 与原版
 * furnace.csv 对齐: 配方行给出材料到产出的折算, 燃料行给出可燃烧物与燃烧值.
 * 表是进程内单例只读, 熔炼与燃料查询都从这里走.
 *
 * 简化说明: 原版燃烧模型包含热量, 氧气与燃烧值的多级折算, 客户端首版只取
 * 燃烧值一个标量, 熔炼一件固定 10 秒, 燃料烧完即熄; 热量与氧气不建模,
 * 接入完整模型时只改 FurnaceState::Tick 与 container_tuning 的折算常量.
 */
class FurnaceTable {
public:
    static const FurnaceTable& Instance();

    /* 配方条目数. */
    int32_t Count() const;

    /* 按材料物品反查配方下标, 未收录返回 -1. */
    int32_t FindByMaterial(ItemId item) const;

    /* 配方下标的产出物品 id, 越界返回空物品. */
    ItemId ResultOf(int32_t recipeIndex) const;

    /* 某物品的燃烧值, 未收录返回 0 表示不可燃烧. */
    int32_t CombustionOf(ItemId item) const;

private:
    FurnaceTable() = default;
};

/*
 * 单个熔炉实例的运行状态. 输入格与燃料格由玩家经面板放置,
 * Tick 按固定间隔把输入熔成产出; 燃料按燃烧值折算燃烧时长, 烧尽扣格.
 */
class FurnaceState {
public:
    /* 一件物品的熔炼秒数, 折算常量见 container_tuning.h. */
    static constexpr float kSmeltSeconds = tuning::kSmeltSecondsPerItem;

    /* 燃料燃烧值到燃烧秒数的折算系数, 见 container_tuning.h. */
    static constexpr float kFuelSecondsPerUnit = tuning::kFuelSecondsPerUnit;

    /* 空炉构造. */
    FurnaceState() = default;

    /*
     * 推进熔炉. dt 为本帧秒数:
     *   没有燃烧中的燃料且输入与燃料齐备时, 点燃一格燃料;
     *   燃烧期间推进熔炼进度, 满一段即把一件输入按配方熔成产出;
     *   输入耗尽, 没有配方或产出堆满时熔炼暂停, 燃料继续烧完.
     */
    void Tick(float dt);

    /* 是否可以熔炼: 输入有配方且产出未堆满, 供面板进度条与日志使用. */
    bool Smeltable() const;

    /* 输入格: 待熔炼的堆叠. */
    ItemStack input;
    /* 燃料格: 燃烧中的燃料堆叠, 烧尽扣一格. */
    ItemStack fuel;
    /* 产出格: 熔炼结果堆叠. */
    ItemStack output;
    /* 熔炼进度 0 到 1, 由累计秒数折算, 供面板显示. */
    float progress = 0.0f;
    /* 累计熔炼秒数, 达到 kSmeltSeconds 出货一件并扣减, 避免浮点卡线. */
    float progressSeconds = 0.0f;
    /* 当前燃烧的剩余秒数, 0 表示没有燃烧. */
    float fuelTicks = 0.0f;
    /* 当前燃烧的单格总秒数, 供面板画燃料余量比例. */
    float fuelBurnTotal = 0.0f;
};

/* 世界里全部熔炉的容器: 键是世界方块坐标编码. */
using FurnaceMap = std::unordered_map<uint64_t, FurnaceState>;

/* 世界方块坐标到容器键的编码, 熔炉与箱子共用同一套键. */
uint64_t ContainerKeyOf(int32_t x, int32_t y, int32_t z);

}  /* namespace immirror::game */
