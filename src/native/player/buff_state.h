#pragma once

#include <cstdint>

namespace immirror::player {

/*
 * 状态效果集合.
 *
 * 只保存"当前挂着哪些效果"与剩余时间, 效果的具体换算由控制器在消费时完成:
 * 移动速度与挖掘速度是百分比增量, 跳跃是高度增量, 生命恢复是每秒点数.
 * 同类效果按加和叠加, 不做优先级覆盖, 与原版多个状态同时生效的语义一致.
 */
class BuffState {
public:
    /* 同时挂着的效果上限: 吃药与食物叠加的常见数量, 超出后忽略新效果. */
    static constexpr int32_t kMaxActive = 8;

    /* 清空全部效果, 进入新世界或死亡重生时调用. */
    void Clear();

    /*
     * 挂上一个效果, 重复挂同一编号时刷新持续时间并取较高等级.
     * 成功挂上时返回该效果的音效路径 (可能为空), 由调用方决定播放.
     */
    const char* Apply(uint32_t buffId);

    /* 推进剩余时间, 到期的效果自动移除. */
    void Tick(float deltaSeconds);

    /* 聚合倍率: 1.0 为无加成, 可能小于 1 (减速类效果). */
    float SpeedMultiplier() const;
    float DigMultiplier() const;
    /* 跳跃高度增量, 单位与方块高度一致. */
    float JumpBonus() const;
    /* 每秒恢复的生命点数. */
    float RegenPerSecond() const;

    /* 当前挂着的效果数量, 供界面与调试显示. */
    int32_t Count() const { return count_; }

    /* 第 index 个效果的编号与剩余时间, 越界返回 false. */
    bool At(int32_t index, uint32_t* buffId, float* remaining) const;

private:
    struct Entry {
        uint32_t id = 0;
        float remaining = 0.0f;
    };

    Entry entries_[kMaxActive] = {};
    int32_t count_ = 0;
};

}  /* namespace immirror::player */
