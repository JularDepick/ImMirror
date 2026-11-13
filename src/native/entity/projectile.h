#pragma once

#include <cstdint>
#include <vector>

#include "core/vec3.h"

namespace immirror::entity {

/*
 * 飞行物: 箭一类直线飞行并受重力影响的投射物.
 *
 * 只做几何推进与命中判定, 伤害与特效由调用方按命中结果处理,
 * 因此这里不依赖生物管理器与方块注册表的写接口.
 */
struct Projectile {
    uint32_t id = 0;
    core::Vec3 position;
    core::Vec3 velocity;
    /* 累计存活秒数, 到上限自动失效. */
    float age = 0.0f;
    /* 命中时造成的伤害. */
    float damage = 0.0f;
    /* 是否由玩家射出: 用于区分命中归属. */
    bool fromPlayer = true;
};

/* 一次推进的命中结果. */
struct ProjectileHit {
    enum class Kind : uint8_t { kNone, kBlock, kCreature };
    Kind kind = Kind::kNone;
    /* 命中生物时的生物标识. */
    uint32_t creatureId = 0;
    /* 命中位置: 特效从这里生成. */
    core::Vec3 position;
    /* 该发飞行物的伤害, 由发射时决定. */
    float damage = 0.0f;
};

/*
 * 推进一批飞行物.
 *
 * 位置按速度积分, 竖直方向受重力; 每步按固定细分长度采样, 避免高速穿墙.
 * 命中方块或生物的飞行物会被移除, 命中结果追加到 hits.
 * 回调由调用方提供: 判断某点是否实心, 以及某点是否落在生物身上.
 */
class ProjectileSystem {
public:
    /* 命中判定回调: 返回 true 表示命中. */
    using SolidAt = bool (*)(void* context, int32_t x, int32_t y, int32_t z);
    using CreatureAt = uint32_t (*)(void* context, const core::Vec3& point, float radius);

    void Clear();

    /* 发射一发, 返回新飞行物的标识. */
    uint32_t Spawn(const core::Vec3& origin, const core::Vec3& direction, float speed,
                   float damage);

    /*
     * 推进一帧. context 透传给两个回调;
     * hits 收集本帧全部命中, 由调用方决定伤害与特效.
     */
    void Update(float seconds, void* context, SolidAt solidAt, CreatureAt creatureAt,
                std::vector<ProjectileHit>& hits);

    int32_t Count() const { return static_cast<int32_t>(items_.size()); }
    const Projectile& At(int32_t index) const { return items_[static_cast<size_t>(index)]; }

private:
    std::vector<Projectile> items_;
    uint32_t nextId_ = 1;
};

}  /* namespace immirror::entity */
