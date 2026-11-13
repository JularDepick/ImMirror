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
 * 物品类别, 取值与itemdef 的背包筛选类型一致:
 * 1 装备, 2 工具, 3 方块, 4 材料. 表里留空时按Type 字段兜底.
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
 * key 为英文键, 界面点阵字只收录 ASCII, 因此标签一律用 key 而不是中文名;
 * displayName 保留中文名, 供日志与后续界面贴图使用.
 */
struct ItemInfo {
    uint32_t sourceId;       /* itemdef 的物品 ID */
    const char* key;         /* 英文键, 也用作界面标签 */
    const char* displayName; /* 中文名, UTF-8 编码 */
    const char* icon;        /* 图标名, 空串表示没有独立图标 */
    uint16_t maxStack;       /* 单格最大堆叠数量 */
    ItemCategory category;   /* 背包筛选类别 */
    uint8_t itemType;        /* Type 字段, 保留便于回溯 */
    int16_t block;           /* 放置出的客户端方块 id, -1 表示不可放置 */
    /*
     * 界面字段: 界面脚本按物品定义读取, 这里原样带下来, 不做语义解释.
     * 未填写的列取 0, 文本列取空串.
     */
    const char* desc;        /* 物品描述文本 */
    uint16_t involvedId;     /* 关联物品编号 */
    uint16_t unlockFlag;     /* 解锁标记 */
    uint16_t unlockType;     /* 解锁方式 */
    uint16_t createType;     /* 获取方式 */
    uint16_t model;          /* 模型编号 */
    uint16_t replaceId;      /* 替换物品编号 */
    uint16_t enchantTag;     /* 附魔标签 */
    uint16_t stuffType;      /* 材料类别 */
    uint16_t quality;        /* 品质档位 */
};

/* 一格物品堆叠: 物品 id 与数量, 空堆叠两者都是 0. */
struct ItemStack {
    ItemId item = kNone;
    int32_t count = 0;
    /*
     * 剩余耐久, 只对工具类物品有意义. -1 表示该物品不用耐久 (方块与材料),
     * 工具进背包时由物品栏按物品表填满耐久. 工具不堆叠, 一格一把.
     */
    int16_t durability = -1;
    /*
     * 附魔: enchantId 为附魔表编号, 0 表示没有附魔; enchantLevel 为该编号的等级.
     * 物品栏不进存档, 因此不涉及存档格式.
     */
    uint16_t enchantId = 0;
    uint8_t enchantLevel = 0;

    bool Empty() const { return item == kNone || count <= 0; }

    /* 是否带耐久且已经耗尽. */
    bool Broken() const { return durability == 0; }

    void Clear() {
        item = kNone;
        count = 0;
        durability = -1;
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
