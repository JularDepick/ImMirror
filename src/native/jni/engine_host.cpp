#include "jni/engine_host.h"

#include "core/log.h"

namespace immirror::jni {
namespace {

constexpr const char* kTag = "ImMirrorHost";

}  /* namespace */

EngineHost& EngineHost::Instance() {
    /* 函数内静态对象由编译器保证线程安全的一次性构造. */
    static EngineHost host;
    return host;
}

void EngineHost::SetDeviceProfile(const std::string& payload) {
    {
        std::lock_guard<std::mutex> guard(mutex_);
        profile_ = payload;
    }
    IM_LOGI(kTag, "device profile stored, bytes=%zu", payload.size());
}

std::string EngineHost::CopyDeviceProfile() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return profile_;
}

void EngineHost::RequestStart() {
    stopRequested_.store(false);
    startRequested_.store(true);
    IM_LOGI(kTag, "start requested by java");
}

void EngineHost::RequestStop() {
    startRequested_.store(false);
    stopRequested_.store(true);
    IM_LOGI(kTag, "stop requested by java");
}

bool EngineHost::StartRequested() const {
    return startRequested_.load();
}

bool EngineHost::StopRequested() const {
    return stopRequested_.load();
}

void EngineHost::MarkRuntimeActive(bool active) {
    runtimeActive_.store(active);
    IM_LOGI(kTag, "runtime active=%d", active ? 1 : 0);
}

bool EngineHost::RuntimeActive() const {
    return runtimeActive_.load();
}

}  /* namespace immirror::jni */
