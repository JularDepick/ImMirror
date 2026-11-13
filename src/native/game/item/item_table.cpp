#include "game/item/item_table.h"

#include "ui/generated/item_icon_atlas_gen.h"

#include <cstring>

#include "game/generated/block_data_gen.h"
#include "game/generated/item_data_gen.h"
#include "game/generated/tool_data_gen.h"
#include "voxel/block_registry.h"

namespace immirror::game::item {
namespace {

/* 越界 id 的回落条目, 与生成表的首条 (空气) 语义一致. */
constexpr ItemInfo kFallbackItem = {0u, "Air", "空气", "", 64, ItemCategory::kBlock, 1u, 0};

/* 生成表条数的编译期校验, 越界回落与静态数组按同一上界工作. */
static_assert(gen::kItemInfoCount == static_cast<int32_t>(sizeof(gen::kItemInfos) /
                                                          sizeof(gen::kItemInfos[0])),
              "物品表条目数与声明条数不一致, 请重新运行 scripts/gen_item_data.py");
static_assert(gen::kBlockItemCount == static_cast<int32_t>(voxel::block::Count),
              "方块到物品的映射条数与方块注册表不一致, 请重新运行 scripts/gen_item_data.py");

}  /* namespace */

const ItemTable& ItemTable::Instance() {
    static const ItemTable instance;
    return instance;
}

const ItemInfo& ItemTable::Info(ItemId id) const {
    if (id >= static_cast<ItemId>(gen::kItemInfoCount)) {
        return kFallbackItem;
    }
    return gen::kItemInfos[id];
}

int32_t ItemTable::Count() const {
    return gen::kItemInfoCount;
}

ItemId ItemTable::FindByKey(const char* key) const {
    if (key == nullptr) {
        return kNone;
    }
    for (int32_t index = 0; index < gen::kItemInfoCount; ++index) {
        if (std::strcmp(gen::kItemInfos[index].key, key) == 0) {
            return static_cast<ItemId>(index);
        }
    }
    return kNone;
}

ItemId ItemTable::OfBlock(voxel::BlockId block) const {
    const int32_t index = static_cast<int32_t>(block);
    if (index < 0 || index >= gen::kBlockItemCount) {
        return kNone;
    }
    return gen::kBlockItems[index];
}

ItemId ItemTable::ItemOfBlock(voxel::BlockId block) const {
    if (block == voxel::block::Air) {
        return kNone;
    }
    /* 物品表按物品编号升序, 逐条比对关联方块, 命中即返回. */
    for (int32_t index = 0; index < Count(); ++index) {
        const ItemInfo& info = Info(static_cast<ItemId>(index + 1));
        if (info.block >= 0 && static_cast<voxel::BlockId>(info.block) == block) {
            return static_cast<ItemId>(index + 1);
        }
    }
    return kNone;
}

voxel::BlockId ItemTable::BlockOf(ItemId id) const {
    const int16_t block = Info(id).block;
    return block < 0 ? voxel::block::Air : static_cast<voxel::BlockId>(block);
}

bool ItemTable::ToolOf(ItemId id, ToolInfo& out) const {
    const gen::ToolEntry* entry = gen::ToolOfItem(static_cast<uint16_t>(id));
    if (entry == nullptr) {
        return false;
    }
    out.harvest = entry->harvest;
    out.level = entry->level;
    out.efficiency = entry->efficiency;
    out.durability = entry->durability;
    return true;
}

ItemId ItemTable::DropOfBlock(voxel::BlockId block) const {
    /*
     * 方块数据表已按掉落字段折算好掉落方块, 这里只做方块到物品的最后一步换算,
     * 两处都以生成表为准, 不存在第二份掉落规则.
     */
    const voxel::BlockId drop = static_cast<voxel::BlockId>(gen::DropBlockOf(block));
    if (drop == voxel::block::Air) {
        return kNone;
    }
    return OfBlock(drop);
}

int32_t ItemTable::MaxStack(ItemId id) const {
    const int32_t maxStack = static_cast<int32_t>(Info(id).maxStack);
    return maxStack > 0 ? maxStack : 64;
}

char ItemTable::Initial(ItemId id) const {
    const char* key = Info(id).key;
    if (key == nullptr || key[0] == '\0') {
        return '?';
    }
    char initial = key[0];
    if (initial >= 'a' && initial <= 'z') {
        initial = static_cast<char>(initial - 'a' + 'A');
    }
    return initial;
}

int32_t ItemTable::IconCellOf(ItemId id) const {
    return ui::gen::ItemIconSlot(static_cast<uint32_t>(id));
}

int32_t ItemTable::AtlasCellOf(ItemId id) const {
    const voxel::BlockId block = BlockOf(id);
    if (block == voxel::block::Air) {
        return -1;
    }
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    constexpr uint16_t kAtlasFallback = 0xFFFF;
    return info.atlasTop == kAtlasFallback ? static_cast<int32_t>(info.atlasIndex)
                                           : static_cast<int32_t>(info.atlasTop);
}

}  /* namespace immirror::game::item */
