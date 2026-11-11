#pragma once

#include <cstdint>

#include "core/vec3.h"

namespace immirror::entity {

/* 实体标识, 0 为无效值, 由 EntityManager 单调分配, 进程内不重复. */
using EntityId = uint32_t;
constexpr EntityId kInvalidEntityId = 0u;

/* 实体类别, 当前只有生物一类, 后续接入投掷物与时装实体时在此追加. */
enum class EntityKind : uint8_t {
    kCreature = 0,
};

/*
 * 实体基类. 只承载标识, 类别与包围盒, 位置与尺寸交给派生类提供,
 * 因此生物实体的物理状态可以放在自己的碰撞体里, 基类不必再存一份.
 * 包围盒统一按脚底中心加高度推出, 射线拾取, 接触判定与后续的渲染接入共用这一份.
 */
class Entity {
public:
    virtual ~Entity() = default;

    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    EntityId Id() const { return id_; }
    EntityKind Kind() const { return kind_; }

    /* 脚底中心的世界坐标. */
    virtual core::Vec3 Position() const = 0;
    /* 碰撞盒半宽与高度, 单位米. */
    virtual float BoundsHalfWidth() const = 0;
    virtual float BoundsHeight() const = 0;

    /* 世界空间轴对齐包围盒, 供射线拾取与命中判定复用. */
    void Bounds(core::Vec3& minCorner, core::Vec3& maxCorner) const;

    /* 已被标记移除的实体不再参与更新与拾取. */
    bool Removed() const { return removed_; }
    void MarkRemoved() { removed_ = true; }

protected:
    Entity(EntityId id, EntityKind kind);

private:
    EntityId id_ = kInvalidEntityId;
    EntityKind kind_ = EntityKind::kCreature;
    bool removed_ = false;
};

/*
 * 射线与轴对齐包围盒求交, 采用 slab 判定.
 * 命中返回进入距离, 未命中或包围盒在射线后方时返回负值, direction 需为单位向量.
 */
float RayHitsBox(const core::Vec3& origin, const core::Vec3& direction,
                 const core::Vec3& minCorner, const core::Vec3& maxCorner);

}  /* namespace immirror::entity */
