#include "editor/trigger_runtime.h"

#include <cmath>
#include <utility>

#include "core/log.h"
#include "editor/editor_bindings.h"
#include "editor/editor_types.h"
#include "editor/project_format.h"
#include "script/compiler.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditorTrigger";

/* FNV-1a 指纹, 只用来判断脚本文本是否变化, 不用于安全用途. */
uint64_t HashText(const std::string& text) {
    uint64_t hash = 1469598103934665603ull;
    for (const char c : text) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ull;
    }
    return hash;
}

}  /* namespace */

TriggerRuntime::TriggerRuntime(Project& project, TriggerHost& host)
    : project_(project), host_(host) {
    RegisterEditorBindings(registry_);
    sandboxLimits_.instructionBudget = limits::kTriggerInstructionBudget;
    sandboxLimits_.timeoutMillis = limits::kTriggerTimeoutMillis;
    /* 脚本一次执行只跑几十条语句, 栈与调用深度按运行时默认上限即可. */
    vm_.SetBindingRegistry(&registry_);
    vm_.SetHost(&host_);
}

TriggerRuntime::~TriggerRuntime() = default;

void TriggerRuntime::Reset() {
    entries_.clear();
    lastError_.clear();
    lastLabel_.clear();
    firesThisFrame_ = 0;
    totalFires_ = 0;
}

TriggerRuntime::Entry* TriggerRuntime::FindEntry(int32_t id) {
    for (auto& entry : entries_) {
        if (entry->id == id) {
            return entry.get();
        }
    }
    return nullptr;
}

const TriggerRuntime::Entry* TriggerRuntime::FindEntry(int32_t id) const {
    for (const auto& entry : entries_) {
        if (entry->id == id) {
            return entry.get();
        }
    }
    return nullptr;
}

bool TriggerRuntime::Compile(Entry& entry, const Trigger& trigger) {
    script::CompileOptions options;
    const std::string chunkName = "trigger." + std::to_string(trigger.id);
    script::CompileResult result = script::CompileSource(trigger.script, chunkName, options);
    entry.scriptHash = HashText(trigger.script);
    if (!result.ok) {
        entry.compiled = false;
        entry.program = script::Program{};
        entry.error = result.diagnostics.empty()
                          ? std::string("脚本编译失败")
                          : script::FormatDiagnostic(chunkName.c_str(), result.diagnostics.front());
        lastError_ = "触发器 " + std::to_string(trigger.id) + ": " + entry.error;
        IM_LOGW(kTag, "%s", lastError_.c_str());
        return false;
    }
    entry.program = std::move(result.program);
    entry.compiled = true;
    entry.error.clear();
    return true;
}

void TriggerRuntime::Sync() {
    /* 先删掉工程里已经不存在的缓存. */
    const std::vector<Trigger>& triggers = project_.Triggers();
    for (auto it = entries_.begin(); it != entries_.end();) {
        bool alive = false;
        for (const Trigger& trigger : triggers) {
            if (trigger.id == (*it)->id) {
                alive = true;
                break;
            }
        }
        if (alive) {
            ++it;
        } else {
            it = entries_.erase(it);
        }
    }
    /* 再补齐新增与脚本变化的条目. */
    for (const Trigger& trigger : triggers) {
        Entry* entry = FindEntry(trigger.id);
        if (entry == nullptr) {
            auto created = std::make_unique<Entry>();
            created->id = trigger.id;
            created->name = trigger.name;
            entry = created.get();
            entries_.push_back(std::move(created));
        }
        entry->name = trigger.name;
        const uint64_t hash = HashText(trigger.script);
        if (!entry->compiled || entry->scriptHash != hash) {
            Compile(*entry, trigger);
        }
        if (trigger.fireImmediately && trigger.IsTimed() && entry->timerSeconds == 0.0) {
            /* 立即触发的定时器从间隔时长起步, 第一帧就会执行一次. */
            entry->timerSeconds = static_cast<double>(trigger.intervalSeconds);
        }
    }
}

bool TriggerRuntime::Fire(Entry& entry) {
    if (!entry.compiled) {
        return false;
    }
    if (firesThisFrame_ >= limits::kTriggerFiresPerFrame) {
        return false;
    }
    ++firesThisFrame_;
    const script::VmResult result = vm_.Execute(entry.program, sandboxLimits_);
    if (!result.ok()) {
        entry.error = result.message;
        lastError_ = "触发器 " + std::to_string(entry.id) + ": " + result.message;
        IM_LOGW(kTag, "%s", lastError_.c_str());
        return false;
    }
    ++totalFires_;
    lastLabel_ = entry.name;
    return true;
}

bool TriggerRuntime::FireOnce(int32_t triggerId) {
    Sync();
    Entry* entry = FindEntry(triggerId);
    if (entry == nullptr) {
        return false;
    }
    if (!entry->compiled) {
        return false;
    }
    firesThisFrame_ = 0;
    return Fire(*entry);
}

bool TriggerRuntime::TriggerHasError(int32_t triggerId) const {
    const Entry* entry = FindEntry(triggerId);
    return entry != nullptr && !entry->compiled;
}

void TriggerRuntime::Tick(double deltaSeconds, const core::Vec3& playerFeet) {
    firesThisFrame_ = 0;
    const std::vector<Trigger>& triggers = project_.Triggers();
    for (const Trigger& trigger : triggers) {
        if (!trigger.enabled) {
            continue;
        }
        Entry* entry = FindEntry(trigger.id);
        if (entry == nullptr) {
            continue;
        }
        switch (trigger.eventType) {
            case TriggerEventType::kEnterRegion: {
                const bool inside =
                    trigger.region.Contains(playerFeet.x, playerFeet.y + 1.0f, playerFeet.z);
                if (!entry->regionEvaluated) {
                    /* 首帧只记录状态: 进入编辑器时玩家已经在区域里不应该立刻触发. */
                    entry->regionEvaluated = true;
                    entry->playerInside = inside;
                    break;
                }
                if (inside && !entry->playerInside) {
                    Fire(*entry);
                }
                entry->playerInside = inside;
                break;
            }
            case TriggerEventType::kTimer: {
                if (!trigger.IsTimed()) {
                    break;
                }
                entry->timerSeconds += deltaSeconds;
                const double interval =
                    trigger.intervalSeconds > 0.1f ? trigger.intervalSeconds : 0.1f;
                while (entry->timerSeconds >= interval) {
                    entry->timerSeconds -= interval;
                    if (!Fire(*entry)) {
                        break;
                    }
                }
                break;
            }
            case TriggerEventType::kBlockBroken:
                /* 由 NotifyBlockBroken 驱动, 不影响每帧状态. */
                break;
        }
    }
}

void TriggerRuntime::NotifyBlockBroken(voxel::BlockId block, int32_t x, int32_t y, int32_t z) {
    (void)x;
    (void)y;
    (void)z;
    const std::vector<Trigger>& triggers = project_.Triggers();
    for (const Trigger& trigger : triggers) {
        if (!trigger.enabled || trigger.eventType != TriggerEventType::kBlockBroken) {
            continue;
        }
        if (trigger.brokenBlock != 0 && trigger.brokenBlock != block) {
            continue;
        }
        Entry* entry = FindEntry(trigger.id);
        if (entry != nullptr) {
            Fire(*entry);
        }
    }
}

}  /* namespace immirror::editor */
