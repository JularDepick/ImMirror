#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "core/vec3.h"
#include "entity/creature_entity.h"
#include "entity/entity_tuning.h"
#include "voxel/voxel_types.h"

namespace immirror::world {
class World;
}

namespace immirror::entity {

/* 生物死亡后待入背包的掉落. 掉落物当前按生物类别折算为方块, 见生成表注释. */
struct EntityDrop {
    voxel::BlockId block = 0;
    int32_t count = 0;
    /* 死亡点坐标: 掉落物从这里落地, 而不是从玩家脚下冒出. */
    core::Vec3 position;
};

/* 一帧的统计: 对玩家的伤害合计, 以及生成, 死亡与回收数量. */
struct EntityFrameStats {
    float playerDamage = 0.0f;
    /* 本帧对玩家的伤害类型, 取值见 game::DamageType; 多种来源取最后一条. */
    uint8_t playerDamageType = 1;
    /* 本帧对玩家出手的生物标识, 0 表示无. */
    uint32_t playerDamageSource = 0;
    /* 本帧的出手是否应改为飞行物: 远程生物为真. */
    bool rangedShot = false;
    int32_t spawned = 0;
    int32_t deaths = 0;
    int32_t despawned = 0;
};

/*
 * 实体管理器. 持有全部生物, 负责按生物表在地表生成, 按距离休眠与回收,
 * 逐帧推进 AI 与物理, 并提供视线拾取与伤害结算给玩家攻击路径使用.
 * 生物对玩家的伤害不在这里施加, 而是汇总到 EntityFrameStats 由总装转交玩家控制器,
 * 这样实体层不依赖玩家层, 更新顺序完全由总装掌握.
 * 只在原生主线程使用, 不做加锁.
 */
class EntityManager {
public:
    EntityManager();
    ~EntityManager();

    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    /* 清空全部生物并重新播种; 进入新世界或重新载入存档时调用. */
    void Reset(uint32_t seed);

    /*
     * 生成环带的外半径, 由总装按视距下发: 视距外的列没有区块数据,
     * 按地表生成会全部落空, 因此半径不能超过已加载范围.
     */
    void SetSpawnExtent(float maxRadius);

    /*
     * 是否允许主动攻击的生物自然生成. 总装按世界时间下发: 白天只放动物,
     * 夜间才放怪物, 与昼夜刷怪口径一致.
     */
    void SetHostileSpawnAllowed(bool allowed) { hostileSpawnAllowed_ = allowed; }

    /*
     * 是否允许自然生成. 作为房间客人时关闭: 世界里的生物由房主上报,
     * 本地再生成一份会出现同一只生物被画两遍.
     */
    void SetSpawnEnabled(bool enabled) { spawnEnabled_ = enabled; }

    /* 清空全部生物与待处理事件, 切到客人模式时调用. */
    void DespawnAll();

    /* 推进全部生物并补充生成, 返回本帧统计. */
    EntityFrameStats Update(double dt, const world::World& world, const PlayerProbe& player);

    /* 视线拾取最近的生物, 命中返回实体指针, 距离写入 outDistance; 不修改状态. */
    const Entity* PickRay(const core::Vec3& origin, const core::Vec3& direction, float maxDistance,
                          float& outDistance) const;

    /* 对指定生物造成伤害; 致死时把掉落放进待收列表并返回 true. */
    bool Damage(EntityId id, float amount);

    /* 取走累计的掉落, 取走后列表清空. */
    std::vector<EntityDrop> TakeDrops();

    /*
     * 生物音效事件: 受击与死亡各记一条, 由装配层取出后交给音频引擎播放.
     * 实体层不直接依赖音频层, 事件只带生物种类与动作编号.
     */
    struct SoundEvent {
        uint32_t sourceId = 0;
        uint8_t action = 0; /* 0 受击, 1 死亡 */
        /* 发生位置: 播放侧据此做距离衰减. */
        core::Vec3 position;
    };
    std::vector<SoundEvent> TakeSoundEvents();

    int32_t Count() const { return static_cast<int32_t>(creatures_.size()); }

    /* 按下标取生物, 越界返回空; 供只读遍历使用. */
    const CreatureEntity* At(int32_t index) const {
        if (index < 0 || index >= static_cast<int32_t>(creatures_.size())) {
            return nullptr;
        }
        return creatures_[static_cast<size_t>(index)].get();
    }
    int32_t AwakeCount() const { return awake_; }
    /* 同时存在的生物上限. */
    int32_t Capacity() const;

    /* 按标识查生物, 未找到返回空指针; 供战斗框刷新目标血量. */
    const CreatureEntity* FindById(EntityId id) const;

    /*
     * 对指定生物施加一次外力位移. 生物内部持有速度, 但管理器不暴露可变引用,
     * 因此击退一类效果统一走这个入口.
     */
    void ApplyImpulse(EntityId id, const core::Vec3& impulse, float seconds, float liftSpeed);

    /* 只读访问全部生物, 供渲染侧接入生物网格时逐个体取位置, 朝向与状态. */
    const std::vector<std::unique_ptr<CreatureEntity>>& Creatures() const { return creatures_; }

private:
    CreatureEntity* Find(EntityId id);
    /* 回收已死亡或已被标记移除的实体. */
    void RemoveGone();
    /* 在玩家周围的地表尝试生成一只生物, 地表不合法或概率未命中时直接返回. */
    void TrySpawn(const world::World& world, const PlayerProbe& player, EntityFrameStats& stats);
    /* 把死亡生物的掉落放进待收列表. */
    void QueueLoot(const CreatureEntity& creature);
    float NextUnit();

    std::vector<std::unique_ptr<CreatureEntity>> creatures_;
    std::vector<EntityDrop> drops_;
    std::vector<SoundEvent> soundEvents_;
    /* 环境叫声计时器, 到点随机挑一只清醒生物发声. */
    double idleAccumulator_ = 0.0;
    uint32_t rng_ = 1u;
    EntityId nextId_ = 1u;
    double spawnAccumulator_ = 0.0;
    int32_t awake_ = 0;
    int32_t deathsPending_ = 0;
    float spawnMaxRadius_ = tuning::kSpawnMaxRadius;
    bool hostileSpawnAllowed_ = true;
    /* 自然生成总开关, 客人模式下关闭. */
    bool spawnEnabled_ = true;
};

}  /* namespace immirror::entity */
