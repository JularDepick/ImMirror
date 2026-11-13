#include "entity/monster_table.h"

#include <cstddef>

#include "entity/generated/monster_data_gen.h"

namespace immirror::entity {
namespace {

/* 生成表条目过少说明过滤规则或数据表出了问题, 编译期就拦住. */
static_assert(gen::kMonsterDataCount >= 20, "生物数据表条目过少, 请重新运行 scripts/gen_monster_data.py");

/* 把生成表的单条记录翻译成运行期定义. */
MonsterDef ToDef(const gen::MonsterData& data) {
    MonsterDef def;
    def.sourceId = data.sourceId;
    def.name = data.displayName;
    def.life = data.life;
    def.attack = data.attack;
    def.speed = data.speed;
    def.height = data.height;
    def.width = data.width;
    def.thickness = data.thickness;
    def.viewDistance = data.viewDistance;
    def.attackDistance = data.attackDistance;
    def.attackInterval = data.attackInterval;
    def.attackType = data.attackType;
    def.armorPunch = data.armorPunch;
    def.armorRange = data.armorRange;
    def.armorMagic = data.armorMagic;
    def.type = data.type;
    def.hostile = data.hostile != 0;
    def.spawnWeight = data.spawnWeight;
    def.lootBlock = static_cast<voxel::BlockId>(data.lootBlock);
    def.lootCount = static_cast<int32_t>(data.lootCount);
    return def;
}

}  /* namespace */

const MonsterTable& MonsterTable::Instance() {
    static const MonsterTable instance;
    return instance;
}

MonsterTable::MonsterTable() {
    defs_.reserve(static_cast<size_t>(gen::kMonsterDataCount));
    cumulativeWeights_.reserve(static_cast<size_t>(gen::kMonsterDataCount));
    for (int32_t i = 0; i < gen::kMonsterDataCount; ++i) {
        const MonsterDef def = ToDef(gen::kMonsterData[i]);
        defs_.push_back(def);
        if (def.hostile) {
            ++hostileCount_;
        }
        if (def.spawnWeight > 0.0f) {
            totalWeight_ += def.spawnWeight;
            ++spawnableCount_;
        }
        cumulativeWeights_.push_back(totalWeight_);
    }
}

const MonsterDef& MonsterTable::Def(int32_t index) const {
    if (index < 0 || index >= Count()) {
        return defs_.front();
    }
    return defs_[static_cast<size_t>(index)];
}

const MonsterDef& MonsterTable::DefBySourceId(uint32_t sourceId) const {
    const int32_t index = IndexOfSourceId(sourceId);
    return index >= 0 ? defs_[static_cast<size_t>(index)] : defs_.front();
}

int32_t MonsterTable::IndexOfSourceId(uint32_t sourceId) const {
    for (size_t i = 0; i < defs_.size(); ++i) {
        if (defs_[i].sourceId == sourceId) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

const MonsterDef& MonsterTable::PickByWeight(uint32_t randomValue) const {
    const float target = static_cast<float>(randomValue);
    for (size_t i = 0; i < defs_.size(); ++i) {
        if (defs_[i].spawnWeight <= 0.0f) {
            continue;
        }
        if (target < cumulativeWeights_[i]) {
            return defs_[i];
        }
    }
    /* 数值落在累计权重之外 (浮点累加误差) 时回落到最后一条可生成的条目. */
    for (size_t i = defs_.size(); i > 0; --i) {
        if (defs_[i - 1].spawnWeight > 0.0f) {
            return defs_[i - 1];
        }
    }
    return defs_.front();
}

}  /* namespace immirror::entity */
