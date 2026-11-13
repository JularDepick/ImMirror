#include "entity/entity_manager.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/log.h"
#include "entity/entity_tuning.h"
#include "entity/monster_table.h"
#include "voxel/block_registry.h"
#include "voxel/voxel_types.h"
#include "world/world.h"

namespace immirror::entity {
namespace {

constexpr const char* kTag = "ImMirrorEntity";
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

}  /* namespace */

EntityManager::EntityManager() { Reset(1u); }

EntityManager::~EntityManager() = default;

void EntityManager::Reset(uint32_t seed) {
    creatures_.clear();
    drops_.clear();
    rng_ = seed != 0u ? seed : 1u;
    nextId_ = 1u;
    awake_ = 0;
    deathsPending_ = 0;
    /* 先进一段生成计时, 世界就绪后的首帧就能看到生物, 不必等满一个间隔. */
    spawnAccumulator_ = tuning::kSpawnIntervalSeconds;
    const MonsterTable& table = MonsterTable::Instance();
    IM_LOGI(kTag, "entity manager reset, species=%d spawnable=%d hostile=%d",
            table.Count(), table.SpawnableCount(), table.HostileCount());
}

void EntityManager::SetSpawnExtent(float maxRadius) {
    /* 下限保证生成点不会贴到玩家身上, 上限沿用调参常量. */
    const float minimum = tuning::kSpawnMinRadius + 2.0f;
    if (maxRadius < minimum) {
        maxRadius = minimum;
    }
    if (maxRadius > tuning::kSpawnMaxRadius) {
        maxRadius = tuning::kSpawnMaxRadius;
    }
    spawnMaxRadius_ = maxRadius;
}

EntityFrameStats EntityManager::Update(double dt, const world::World& world,
                                       const PlayerProbe& player) {
    EntityFrameStats stats;
    float seconds = dt > 0.0 ? static_cast<float>(dt) : 0.0f;
    if (seconds > tuning::kMaxFrameSeconds) {
        seconds = tuning::kMaxFrameSeconds;
    }

    awake_ = 0;
    for (const auto& creature : creatures_) {
        if (creature == nullptr || creature->Removed()) {
            continue;
        }
        const core::Vec3 position = creature->Position();
        const float deltaX = position.x - player.feet.x;
        const float deltaZ = position.z - player.feet.z;
        const float distance = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);

        /* 走得太远的生物直接回收, 控制同时存在的实体数量. */
        if (distance > tuning::kDespawnRadius) {
            creature->MarkRemoved();
            ++stats.despawned;
            continue;
        }
        /* 休眠: 远离玩家的生物保留在场但不推进 AI 与物理, 死亡动画照常播完. */
        if (distance > tuning::kSleepRadius && !creature->Dying()) {
            continue;
        }
        ++awake_;
        const CreatureTick tick = creature->Update(seconds, world, player);
        stats.playerDamage += tick.playerDamage;
        if (tick.playerDamage > 0.0f) {
            stats.playerDamageType = tick.playerDamageType;
        }
    }

    RemoveGone();

    /*
     * 环境叫声: 每隔数秒随机挑一只清醒生物发一次待机音, 让生物在场时有听觉存在感.
     * 只挑清醒个体, 远处休眠的生物不发声, 避免声音从看不见的地方飘来.
     */
    idleAccumulator_ += seconds;
    if (idleAccumulator_ >= tuning::kIdleSoundIntervalSeconds) {
        idleAccumulator_ = 0.0;
        /*
         * 只从玩家的感知半径内挑: 半径外的生物处于休眠, 发声会让人以为身边有东西.
         * 先收集候选再随机取一只, 避免抽到休眠个体后整轮静默.
         */
        const CreatureEntity* candidates[tuning::kMaxCreatures];
        int32_t candidateCount = 0;
        for (const auto& creature : creatures_) {
            if (creature == nullptr || creature->Removed() || creature->Dying()) {
                continue;
            }
            const core::Vec3 position = creature->Position();
            const float deltaX = position.x - player.feet.x;
            const float deltaZ = position.z - player.feet.z;
            if (std::sqrt(deltaX * deltaX + deltaZ * deltaZ) > tuning::kSleepRadius) {
                continue;
            }
            if (candidateCount < tuning::kMaxCreatures) {
                candidates[candidateCount] = creature.get();
                ++candidateCount;
            }
        }
        if (candidateCount > 0) {
            const int32_t pick = static_cast<int32_t>(NextUnit() * static_cast<float>(candidateCount));
            const int32_t index = pick < candidateCount ? pick : candidateCount - 1;
            soundEvents_.push_back(SoundEvent{candidates[index]->Definition().sourceId, 2});
        }
    }

    /* 生成按固定间隔结算, 落后多个间隔时只补一轮, 避免卡顿后突然刷出大量生物. */
    spawnAccumulator_ += seconds;
    if (spawnAccumulator_ >= tuning::kSpawnIntervalSeconds) {
        spawnAccumulator_ -= tuning::kSpawnIntervalSeconds;
        if (spawnAccumulator_ > tuning::kSpawnIntervalSeconds) {
            spawnAccumulator_ = tuning::kSpawnIntervalSeconds;
        }
        for (int32_t attempt = 0; attempt < tuning::kSpawnAttemptsPerTick; ++attempt) {
            if (Count() >= tuning::kMaxCreatures) {
                break;
            }
            TrySpawn(world, player, stats);
        }
    }

    stats.deaths = deathsPending_;
    deathsPending_ = 0;
    return stats;
}

const Entity* EntityManager::PickRay(const core::Vec3& origin, const core::Vec3& direction,
                                     float maxDistance, float& outDistance) const {
    const Entity* best = nullptr;
    float bestDistance = maxDistance;
    for (const auto& creature : creatures_) {
        if (creature == nullptr || creature->Removed() || creature->Dying()) {
            continue;
        }
        core::Vec3 minCorner;
        core::Vec3 maxCorner;
        creature->Bounds(minCorner, maxCorner);
        const float distance = RayHitsBox(origin, direction, minCorner, maxCorner);
        if (distance < 0.0f || distance > maxDistance) {
            continue;
        }
        if (best == nullptr || distance < bestDistance) {
            best = creature.get();
            bestDistance = distance;
        }
    }
    outDistance = best != nullptr ? bestDistance : maxDistance;
    return best;
}

bool EntityManager::Damage(EntityId id, float amount) {
    CreatureEntity* creature = Find(id);
    if (creature == nullptr) {
        return false;
    }
    const uint32_t sourceId = creature->Definition().sourceId;
    if (!creature->Damage(amount)) {
        return false;
    }
    /* 未致死记一条受击音, 致死记一条死亡音, 两种动作不同时出现. */
    soundEvents_.push_back(SoundEvent{sourceId, 0});
    /* 致死一击额外记一条死亡音, 受击音已在上方记录. */
    soundEvents_.push_back(SoundEvent{sourceId, 1});
    QueueLoot(*creature);
    ++deathsPending_;
    IM_LOGI(kTag, "creature died id=%u name=%s life=%.0f", static_cast<uint32_t>(id),
            creature->Definition().name, static_cast<double>(creature->Definition().life));
    return true;
}

std::vector<EntityDrop> EntityManager::TakeDrops() {
    std::vector<EntityDrop> taken;
    taken.swap(drops_);
    return taken;
}

std::vector<EntityManager::SoundEvent> EntityManager::TakeSoundEvents() {
    std::vector<SoundEvent> taken;
    taken.swap(soundEvents_);
    return taken;
}

int32_t EntityManager::Capacity() const { return tuning::kMaxCreatures; }

CreatureEntity* EntityManager::Find(EntityId id) {
    for (const auto& creature : creatures_) {
        if (creature != nullptr && creature->Id() == id) {
            return creature.get();
        }
    }
    return nullptr;
}

const CreatureEntity* EntityManager::FindById(EntityId id) const {
    for (const auto& creature : creatures_) {
        if (creature != nullptr && creature->Id() == id) {
            return creature.get();
        }
    }
    return nullptr;
}

void EntityManager::RemoveGone() {
    creatures_.erase(
        std::remove_if(creatures_.begin(), creatures_.end(),
                       [](const std::unique_ptr<CreatureEntity>& creature) {
                           return creature == nullptr || creature->Removed() ||
                                  creature->ReadyToRemove();
                       }),
        creatures_.end());
}

void EntityManager::TrySpawn(const world::World& world, const PlayerProbe& player,
                             EntityFrameStats& stats) {
    /* 客人模式下不自然生成: 生物由房主上报, 本地再生成一份会画成两只. */
    if (!spawnEnabled_) {
        return;
    }
    const MonsterTable& table = MonsterTable::Instance();
    if (!(table.TotalSpawnWeight() > 0.0f)) {
        return;
    }

    /* 生成点取玩家周围的一个环带, 太近会突然出现在视野里, 太远则超出视距. */
    const float angle = NextUnit() * kTwoPi;
    const float radius = tuning::kSpawnMinRadius + NextUnit() * (spawnMaxRadius_ - tuning::kSpawnMinRadius);
    const int32_t x = FloorToInt(player.feet.x + std::cos(angle) * radius);
    const int32_t z = FloorToInt(player.feet.z + std::sin(angle) * radius);

    /* HeightAt 返回该列最高非空方块的上方一格, 返回 0 表示该列区块尚未加载. */
    const int32_t surface = world.HeightAt(x, z);
    if (surface <= 0 || surface >= voxel::kWorldHeight - 2) {
        return;
    }

    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    /* 脚下要有实心地表, 落脚与头顶两格必须是空的, 避免生在树冠或墙里. */
    if (!registry.Info(world.GetBlock(x, surface - 1, z)).solid) {
        return;
    }
    if (registry.Info(world.GetBlock(x, surface, z)).solid) {
        return;
    }
    if (registry.Info(world.GetBlock(x, surface + 1, z)).solid) {
        return;
    }
    if (NextUnit() > tuning::kSpawnChancePerAttempt) {
        return;
    }

    const float scaled = NextUnit() * table.TotalSpawnWeight();
    const uint32_t pick = static_cast<uint32_t>(scaled);
    const MonsterDef& def = table.PickByWeight(pick);
    /* 白天不生成主动攻击的生物: 抽到怪物直接放弃本次尝试, 不重新抽取. */
    if (def.hostile && !hostileSpawnAllowed_) {
        return;
    }

    const core::Vec3 spawn{static_cast<float>(x) + 0.5f, static_cast<float>(surface),
                           static_cast<float>(z) + 0.5f};
    const EntityId id = nextId_++;
    const uint32_t seed = (id * 2654435761u) ^ rng_;
    creatures_.push_back(std::make_unique<CreatureEntity>(id, def, spawn, seed));
    ++stats.spawned;
    IM_LOGD(kTag, "spawn id=%u name=%s kind=%d at %.1f,%.1f,%.1f", static_cast<uint32_t>(id),
            def.name, static_cast<int32_t>(def.type), static_cast<double>(spawn.x),
            static_cast<double>(spawn.y), static_cast<double>(spawn.z));
}

void EntityManager::DespawnAll() {
    creatures_.clear();
    awake_ = 0;
    drops_.clear();
    soundEvents_.clear();
}

void EntityManager::QueueLoot(const CreatureEntity& creature) {
    const MonsterDef& def = creature.Definition();
    if (def.lootBlock == 0 || def.lootCount <= 0) {
        return;
    }
    drops_.push_back(EntityDrop{def.lootBlock, def.lootCount, creature.Position()});
}

float EntityManager::NextUnit() {
    /* xorshift32: 布局只需要可复现的伪随机, 不引入标准库随机设备. */
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return static_cast<float>(rng_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

}  /* namespace immirror::entity */
