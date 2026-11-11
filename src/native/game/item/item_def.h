#pragma once

#include <cstdint>

namespace immirror::game::item {

/*
 * 客户端物品 id. 0 恒为空物品, 1 到 14 与客户端方块 id 同值:
 * 破坏方块掉落的物品与放置消耗的物品因此不需要额外换算,
 * 15 起依次是可合成闭包与常见材料, 具体顺序由生成表给出.
 */
using ItemId = uint16_t;

/* 空物品 id, 表示该格没有东西. */
constexpr ItemId kNone = 0;

/*
 * 物品类别, 取值与原版 itemdef 的背包筛选类型一致:
 * 1 装备, 2 工具, 3 方块, 4 材料. 表里留空时按原版 Type 字段兜底.
 */
enum class ItemCategory : uint8_t {
    kUnknown = 0,
    kEquipment = 1,
    kTool = 2,
    kBlock = 3,
    kMaterial = 4,
};

/*
 * 物品静态属性, 由 ItemTable 持有, 生存期与进程一致.
 * key 为原版英文键, 界面点阵字只收录 ASCII, 因此标签一律用 key 而不是中文名;
 * displayName 保留中文名, 供日志与后续界面贴图使用.
 */
struct ItemInfo {
    uint32_t sourceId;       /* 原版 itemdef 的物品 ID */
    const char* key;         /* 原版英文键, 也用作界面标签 */
    const char* displayName; /* 原版中文名, UTF-8 编码 */
    const char* icon;        /* 原版图标名, 空串表示没有独立图标 */
    uint16_t maxStack;       /* 单格最大堆叠数量 */
    ItemCategory category;   /* 背包筛选类别 */
    uint8_t itemType;        /* 原版 Type 字段, 保留便于回溯 */
    int16_t block;           /* 放置出的客户端方块 id, -1 表示不可放置 */
};

/* 一格物品堆叠: 物品 id 与数量, 空堆叠两者都是 0. */
struct ItemStack {
    ItemId item = kNone;
    int32_t count = 0;

    bool Empty() const { return item == kNone || count <= 0; }

    void Clear() {
        item = kNone;
        count = 0;
    }
};

/* 造一个堆叠, 数量非正时返回空堆叠. */
inline ItemStack MakeStack(ItemId item, int32_t count) {
    ItemStack stack;
    if (item != kNone && count > 0) {
        stack.item = item;
        stack.count = count;
    }
    return stack;
}

/* 同种物品才能合并, 空堆叠不参与合并判定. */
inline bool SameItem(const ItemStack& a, const ItemStack& b) {
    return !a.Empty() && !b.Empty() && a.item == b.item;
}

}  /* namespace immirror::game::item */
