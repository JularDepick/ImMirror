#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/vec3.h"
#include "editor/trigger_model.h"
#include "script/binding.h"
#include "script/bytecode.h"
#include "script/sandbox.h"
#include "script/vm.h"
#include "voxel/voxel_types.h"

namespace immirror::editor {

class Project;
class TriggerHost;

/*
 * 触发器运行时.
 * 每帧按三种事件推进: 进入区域看玩家位置是否越界, 破坏方块由编辑器的破坏路径通知,
 * 定时按间隔累计秒数. 命中后把触发器的脚本源码交给运行时虚拟机执行,
 * 脚本里的原生函数调用经 TriggerHost 落到工程与世界.
 *
 * 脚本按文本指纹缓存编译结果, 属性面板改过动作后脚本文本变化才会重新编译;
 * 编译失败时保留错误文本供界面显示, 不影响其他触发器.
 */
class TriggerRuntime {
public:
    TriggerRuntime(Project& project, TriggerHost& host);
    ~TriggerRuntime();

    TriggerRuntime(const TriggerRuntime&) = delete;
    TriggerRuntime& operator=(const TriggerRuntime&) = delete;

    /* 清空全部缓存与计时, 工程重新加载后调用. */
    void Reset();

    /* 与工程的触发器列表同步, 新增或脚本变化时重新编译, 删除时移除缓存. */
    void Sync();

    /* 推进一帧; playerFeet 是玩家脚底中心的世界坐标. */
    void Tick(double deltaSeconds, const core::Vec3& playerFeet);

    /* 玩家破坏方块时由编辑器通知, 命中"被破坏"事件时执行动作. */
    void NotifyBlockBroken(voxel::BlockId block, int32_t x, int32_t y, int32_t z);

    /* 立即执行一条触发器, 供属性面板的测试按钮使用; 返回是否执行成功. */
    bool FireOnce(int32_t triggerId);

    /* 界面显示用的统计与最近一次错误. */
    int32_t TriggerCount() const { return static_cast<int32_t>(entries_.size()); }
    int32_t TotalFires() const { return totalFires_; }
    const std::string& LastError() const { return lastError_; }
    const std::string& LastLabel() const { return lastLabel_; }

    /* 单条触发器的编译状态, 界面据此标记哪条脚本有错. */
    bool TriggerHasError(int32_t triggerId) const;

private:
    /* 一条触发器的运行期缓存. */
    struct Entry {
        int32_t id = 0;
        std::string name;
        uint64_t scriptHash = 0;
        script::Program program;
        bool compiled = false;
        std::string error;
        double timerSeconds = 0.0;
        bool playerInside = false;
        bool regionEvaluated = false;
    };

    Entry* FindEntry(int32_t id);
    const Entry* FindEntry(int32_t id) const;

    /* 编译一条触发器的脚本; 失败时 entry.error 写入诊断文本. */
    bool Compile(Entry& entry, const Trigger& trigger);

    /* 执行一条触发器, 返回是否执行成功. */
    bool Fire(Entry& entry);

    Project& project_;
    TriggerHost& host_;
    script::BindingRegistry registry_;
    script::Vm vm_;
    script::SandboxLimits sandboxLimits_;
    std::vector<std::unique_ptr<Entry>> entries_;
    std::string lastError_;
    std::string lastLabel_;
    int32_t firesThisFrame_ = 0;
    int32_t totalFires_ = 0;
};

}  /* namespace immirror::editor */
