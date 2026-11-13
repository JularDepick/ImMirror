#include "game/item/enchant_table.h"

#include <cstring>

namespace immirror::game::item {

const EnchantTable& EnchantTable::Instance() {
    static const EnchantTable table;
    return table;
}

const gen::EnchantEntry& EnchantTable::At(int32_t index) const {
    if (index < 0 || index >= gen::kEnchantEntryCount) {
        return gen::kEnchantEntries[0];
    }
    return gen::kEnchantEntries[index];
}

const gen::EnchantEntry* EnchantTable::Find(uint32_t sourceId, uint16_t level) const {
    /* 表按 (编号, 等级) 排列, 同一编号的行连续, 线性扫描足够: 120 条. */
    const gen::EnchantEntry* best = nullptr;
    for (int32_t index = 0; index < gen::kEnchantEntryCount; ++index) {
        const gen::EnchantEntry& entry = gen::kEnchantEntries[index];
        if (entry.sourceId != sourceId) {
            continue;
        }
        if (entry.level == level) {
            return &entry;
        }
        /* 记下该编号里等级最高的一条, 供越界回落. */
        if (best == nullptr || entry.level > best->level) {
            best = &entry;
        }
    }
    return best;
}

bool EnchantTable::IsType(const gen::EnchantEntry& entry, const char* type) {
    if (entry.type == nullptr || type == nullptr) {
        return false;
    }
    return std::strcmp(entry.type, type) == 0;
}

int32_t EnchantTable::CountOfType(const char* type) const {
    int32_t total = 0;
    for (int32_t index = 0; index < gen::kEnchantEntryCount; ++index) {
        if (IsType(gen::kEnchantEntries[index], type)) {
            total += 1;
        }
    }
    return total;
}

bool EnchantTable::AllowsToolType(const gen::EnchantEntry& entry, uint8_t toolType) {
    if (entry.toolMask == 0) {
        return true;
    }
    if (toolType == 0 || toolType > 32) {
        return false;
    }
    return (entry.toolMask & (1u << (toolType - 1))) != 0;
}

}  /* namespace immirror::game::item */
