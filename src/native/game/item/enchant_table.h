#pragma once

#include <cstdint>

#include "game/generated/enchant_data_gen.h"

namespace immirror::game::item {

/*
 * 附魔表查询.
 * 数据由 scripts/gen_enchant_data.py 生成, 同一附魔按等级分行,
 * 因此按 (编号, 等级) 取值; 等级越界时回落到该编号的最高等级.
 * 工具类型掩码的第 n 位对应工具类型 n+1, 0 表示不限工具.
 */
class EnchantTable {
public:
    static const EnchantTable& Instance();

    int32_t Count() const { return gen::kEnchantEntryCount; }

    /* 越界回落到首条; 表为空时由生成期保证至少一条. */
    const gen::EnchantEntry& At(int32_t index) const;

    /* 按编号与等级查; 找不到编号时返回空指针. */
    const gen::EnchantEntry* Find(uint32_t sourceId, uint16_t level) const;

    /* 工具类型 (1 到 12) 是否满足该附魔的适用工具要求. */
    static bool AllowsTool(const gen::EnchantEntry& entry, uint16_t toolType);

    /* 某个工具类型可用的附魔条数, 供界面筛选. */
    int32_t CountForTool(uint16_t toolType) const;
};

}  /* namespace immirror::game::item */
