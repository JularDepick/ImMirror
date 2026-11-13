#pragma once

#include <cstdint>
#include <vector>

#include "game/item/item_def.h"
#include "voxel/voxel_types.h"

namespace immirror::entity {

/*
 * 一条生物定义. 由 scripts/gen_monster_data.py 从monster 表折算后
 * 编译进客户端, 运行期只读, 生存期与进程一致.
 * 长度单位米, 速度单位米每秒, 攻击间隔单位秒.
 */
struct MonsterDef {
    uint32_t sourceId = 0;      /* monster 表的 ID */
    const char* name = "";      /* 中文名, UTF-8 */
    float life = 1.0f;
    float attack = 1.0f;
    float speed = 1.0f;
    float height = 1.8f;
    float width = 0.6f;
    float thickness = 0.6f;
    float viewDistance = 12.0f;
    float attackDistance = 2.0f;
    float attackInterval = 1.0f;
    uint8_t attackType = 0;     /* 0 近程, 1 远程, 2 爆炸 */
    /* 按伤害类型的百分比减伤, 取值 0 到 95. */
    float armorPunch = 0.0f;
    float armorRange = 0.0f;
    float armorMagic = 0.0f;
    uint8_t type = 0;           /* 生物类别, 取值见生成表注释 */
    bool hostile = false;       /* true 主动攻击玩家, false 遇人躲避 */
    float spawnWeight = 0.0f;   /* 生成权重, 0 表示不参与生成 */
    /*
     * 三组掉落: 物品编号为 0 表示按 lootBlock 的方块占位.
     * odds 是万分比, 10000 表示必掉.
     */
    struct DropEntry {
        game::item::ItemId item = game::item::kNone;
        int32_t odds = 0;
        int32_t count = 0;
    };
    DropEntry drops[3];
    /* 占位掉落使用的方块编号, 表里没配物品的怪物按它产出. */
    voxel::BlockId lootBlock = 0;
    /* 命中玩家时挂上的状态效果编号, 0 表示无. */
    uint32_t buffId = 0;
    /* 攻击附带的火焰档位, 大于 0 时伤害按燃烧类型结算. */
    uint8_t attackFire = 0;
    /* 质量: 越大越难被击退, 1 为常规. */
    float mass = 1.0f;
    int32_t lootCount = 0;

    /*
     * 碰撞盒半宽取身宽与身厚中较大者的一半. 两个字段分别描述正面宽度与
     * 侧面厚度, 体素碰撞只支持轴对齐的方盒, 取较大者保证包裹住模型.
     */
    float HalfWidth() const { return (width > thickness ? width : thickness) * 0.5f; }
};

/*
 * 生物定义注册表. 首次访问时从生成表装配, 之后只读,
 * 生物实体持有其中条目的指针, 因此实例必须活到进程结束.
 */
class MonsterTable {
public:
    static const MonsterTable& Instance();

    /* 全部条目数, 也是 Def 的合法索引上界. */
    int32_t Count() const { return static_cast<int32_t>(defs_.size()); }

    /* 越界索引回落到首条, 调用方不必判空. */
    const MonsterDef& Def(int32_t index) const;

    /* 按ID 取定义, 找不到时回落到首条. */
    const MonsterDef& DefBySourceId(uint32_t sourceId) const;

    /* 按ID 查索引, 找不到返回 -1. */
    int32_t IndexOfSourceId(uint32_t sourceId) const;

    /*
     * 按累计权重抽取生成用的生物. randomValue 取 [0, TotalSpawnWeight) 的整数,
     * 权重为 0 的条目不会被抽中.
     */
    const MonsterDef& PickByWeight(uint32_t randomValue) const;

    float TotalSpawnWeight() const { return totalWeight_; }
    /* 权重非零的条目数, 即可以生成的生物种类数. */
    int32_t SpawnableCount() const { return spawnableCount_; }
    /* 会主动攻击玩家的条目数. */
    int32_t HostileCount() const { return hostileCount_; }

private:
    MonsterTable();

    std::vector<MonsterDef> defs_;
    std::vector<float> cumulativeWeights_; /* 与 defs_ 等长, 只对权重非零的条目递增 */
    float totalWeight_ = 0.0f;
    int32_t spawnableCount_ = 0;
    int32_t hostileCount_ = 0;
};

}  /* namespace immirror::entity */
