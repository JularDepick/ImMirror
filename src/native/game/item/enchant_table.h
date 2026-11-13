#pragma once

#include <cstdint>

#include "game/generated/enchant_data_gen.h"

namespace immirror::game::item {

/*
 * 附魔表查询.
 * 数据由 scripts/gen_enchant_data.py 生成, 同一附魔按等级分行,
 * 因此按 (编号, 等级) 取值; 等级越界时回落到该编号的最高等级.
 * 效果类型是枚举名 (SHARP / DIG_SPEED / DURABLE 一类), 用名字比较而不是数字.
 * toolMask 的第 n 位对应原版工具类型 n+1, 0 表示不限工具.
 */
class EnchantTable {
public:
    static const EnchantTable& Instance();

    int32_t Count() const { return gen::kEnchantEntryCount; }

    /* 越界回落到首条; 表为空时由生成期保证至少一条. */
    const gen::EnchantEntry& At(int32_t index) const;

    /* 按编号与等级查; 找不到编号时返回空指针. */
    const gen::EnchantEntry* Find(uint32_t sourceId, uint16_t level) const;

    /* 条目是否属于指定效果类型. */
    static bool IsType(const gen::EnchantEntry& entry, const char* type);

    /* 指定效果类型的条目数, 供界面分组与普查. */
    int32_t CountOfType(const char* type) const;

    /*
     * 某条附魔是否适用于给定原版工具类型编号 (1 起).
     * 掩码为 0 表示不限工具.
     */
    static bool AllowsToolType(const gen::EnchantEntry& entry, uint8_t toolType);
};

}  /* namespace immirror::game::item */
