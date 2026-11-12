#include "player/buff_state.h"

#include "player/generated/buff_data_gen.h"

namespace immirror::player {
namespace {

/* 效果类别, 与生成表一致. */
constexpr uint8_t kKindSpeed = 1;
constexpr uint8_t kKindDig = 2;
constexpr uint8_t kKindJump = 3;
constexpr uint8_t kKindRegen = 4;

/* 把百分比增量累加成倍率: 100% 表示翻倍. */
float AccumulatePercent(float percent) {
    const float multiplier = 1.0f + percent / 100.0f;
    /* 减速叠到负数时兜底到一成的速度, 不允许完全停住. */
    return multiplier < 0.1f ? 0.1f : multiplier;
}

}  /* namespace */

void BuffState::Clear() {
    for (int32_t index = 0; index < count_; ++index) {
        entries_[index] = Entry{};
    }
    count_ = 0;
}

void BuffState::Apply(uint32_t buffId) {
    const gen::BuffEntry* entry = gen::BuffOf(buffId);
    if (entry == nullptr || entry->duration <= 0.0f) {
        return;
    }
    /* 已挂着同一编号时刷新持续时间, 不占新槽位. */
    for (int32_t index = 0; index < count_; ++index) {
        if (entries_[index].id == buffId) {
            if (entries_[index].remaining < entry->duration) {
                entries_[index].remaining = entry->duration;
            }
            return;
        }
    }
    if (count_ >= kMaxActive) {
        return;
    }
    entries_[count_].id = buffId;
    entries_[count_].remaining = entry->duration;
    ++count_;
}

void BuffState::Tick(float deltaSeconds) {
    if (!(deltaSeconds > 0.0f) || count_ <= 0) {
        return;
    }
    int32_t alive = 0;
    for (int32_t index = 0; index < count_; ++index) {
        Entry entry = entries_[index];
        entry.remaining -= deltaSeconds;
        if (entry.remaining <= 0.0f) {
            continue;
        }
        entries_[alive] = entry;
        ++alive;
    }
    for (int32_t index = alive; index < count_; ++index) {
        entries_[index] = Entry{};
    }
    count_ = alive;
}

float BuffState::SpeedMultiplier() const {
    float percent = 0.0f;
    for (int32_t index = 0; index < count_; ++index) {
        const gen::BuffEntry* entry = gen::BuffOf(entries_[index].id);
        if (entry != nullptr && entry->kind == kKindSpeed) {
            percent += entry->value;
        }
    }
    return AccumulatePercent(percent);
}

float BuffState::DigMultiplier() const {
    float percent = 0.0f;
    for (int32_t index = 0; index < count_; ++index) {
        const gen::BuffEntry* entry = gen::BuffOf(entries_[index].id);
        if (entry != nullptr && entry->kind == kKindDig) {
            percent += entry->value;
        }
    }
    return AccumulatePercent(percent);
}

float BuffState::JumpBonus() const {
    float bonus = 0.0f;
    for (int32_t index = 0; index < count_; ++index) {
        const gen::BuffEntry* entry = gen::BuffOf(entries_[index].id);
        if (entry != nullptr && entry->kind == kKindJump) {
            bonus += entry->value;
        }
    }
    return bonus;
}

float BuffState::RegenPerSecond() const {
    float total = 0.0f;
    for (int32_t index = 0; index < count_; ++index) {
        const gen::BuffEntry* entry = gen::BuffOf(entries_[index].id);
        if (entry != nullptr && entry->kind == kKindRegen) {
            total += entry->value;
        }
    }
    return total;
}

bool BuffState::At(int32_t index, uint32_t* buffId, float* remaining) const {
    if (index < 0 || index >= count_) {
        return false;
    }
    if (buffId != nullptr) {
        *buffId = entries_[index].id;
    }
    if (remaining != nullptr) {
        *remaining = entries_[index].remaining;
    }
    return true;
}

}  /* namespace immirror::player */
