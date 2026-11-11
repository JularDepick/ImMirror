#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "progress/progress_types.h"

namespace immirror::progress {

/*
 * 累计统计.
 *
 * 只在原生主线程内读写, 因此不设锁; 落盘与上报都通过快照文本进行.
 * 所有取值单调递增, 客户端重启后从 state 文件恢复, 服务端按增量折算每日任务进度.
 */
class ProgressStats {
public:
    void Reset();

    void Add(StatKind kind, int64_t amount);
    void Set(StatKind kind, int64_t value);
    int64_t Get(StatKind kind) const;

    /*
     * 记录一个自然日键, 返回是否首次出现.
     * 在线天数由这张表的大小决定, 因此跨日只记录一次, 重复调用不增加天数.
     */
    bool RecordDayKey(const std::string& key);
    const std::vector<std::string>& DayKeys() const { return dayKeys_; }
    void SetDayKeys(const std::vector<std::string>& keys);

    /* 天数键表的容量上限: 超出后丢弃最早的日期, 成就只需要天数总和与今日判定. */
    static constexpr size_t kMaxDayKeys = 400;

private:
    int64_t values_[kStatCount] = {};
    std::vector<std::string> dayKeys_;
};

}  /* namespace immirror::progress */
