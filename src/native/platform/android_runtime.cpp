#include "platform/android_runtime.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include "core/app_loop.h"
#include "core/frame_stats.h"
#include "core/log.h"
#include "game/game_shell.h"
#include "jni/engine_host.h"
#include "platform/app_bridge.h"
#include "render/render_device.h"

namespace immirror::platform {
namespace {

constexpr const char* kTag = "ImMirrorCore";

/* 帧率日志的统计窗口: 每 60 帧输出一次, 避免日志刷屏反过来影响帧率. */
constexpr int32_t kReportWindowFrames = 60;

using ProfileEntries = std::vector<std::pair<std::string, std::string>>;

/*
 * 把 Java 侧下发的扁平键值文本切成条目.
 * 空行与缺少分隔符的行直接跳过, 不做任何格式化报错.
 */
ProfileEntries ParseProfile(const std::string& text) {
    ProfileEntries entries;
    size_t cursor = 0;
    while (cursor < text.size()) {
        size_t end = text.find('\n', cursor);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = text.substr(cursor, end - cursor);
        const size_t separator = line.find('=');
        if (separator != std::string::npos && separator > 0) {
            entries.emplace_back(line.substr(0, separator), line.substr(separator + 1));
        }
        cursor = end + 1;
    }
    return entries;
}

std::string Lookup(const ProfileEntries& entries, const char* key) {
    for (const auto& entry : entries) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    return std::string();
}

/*
 * 引擎运行时. 同时实现主循环宿主与平台生命周期监听,
 * 是窗口事件, 逻辑推进与渲染提交三者唯一的交汇点.
 */
class Runtime final : public core::LoopHost, public AppLifecycleListener {
public:
    explicit Runtime(android_app* app)
        : bridge_(app, this),
          device_(render::CreateDefaultDevice()),
          shell_(std::make_unique<game::GameShell>()) {}

    void Run();
    void Shutdown();

    /* LoopHost */
    bool PumpEvents(int32_t timeoutMillis) override;
    void Tick(double fixedDeltaSeconds) override;
    void Render(double frameDeltaSeconds) override;
    bool IsActive() const override;
    bool ShouldExit() const override;

    /* AppLifecycleListener */
    void OnWindowCreated(ANativeWindow* window) override;
    void OnWindowDestroyed() override;
    void OnWindowResized(int32_t width, int32_t height) override;
    void OnForegroundChanged(bool foreground) override;

private:
    /* 未收到 Java 启动请求时不进入游戏, 仅保持事件循环运转. */
    void CheckStartGate();
    /* 设备画像到达时间不确定, 首帧之后才可能拿到, 因此按需补记一次. */
    void LogDeviceProfileOnce();
    /* 渲染设备在清屏后回调本函数绘制场景. */
    static void SceneCallbackThunk(void* user);

    AppBridge bridge_;
    std::unique_ptr<render::Device> device_;
    std::unique_ptr<game::GameShell> shell_;
    core::FrameStats stats_{kReportWindowFrames};
    ProfileEntries profile_;
    std::string abi_;
    std::string grade_;
    double elapsedSeconds_ = 0.0;
    bool started_ = false;
    bool profileLogged_ = false;
};

void Runtime::Run() {
    jni::EngineHost::Instance().MarkRuntimeActive(true);
    IM_LOGI(kTag, "engine runtime entering main loop, backend=%s",
            device_ != nullptr ? device_->BackendName() : "none");
    core::AppLoop loop(core::LoopConfig{});
    loop.Run(*this);
    IM_LOGI(kTag, "engine runtime left main loop, frames=%lld",
            static_cast<long long>(stats_.TotalFrames()));
    jni::EngineHost::Instance().MarkRuntimeActive(false);
}

void Runtime::Shutdown() {
    if (device_ != nullptr) {
        device_->Shutdown();
    }
}

bool Runtime::PumpEvents(int32_t timeoutMillis) {
    if (!bridge_.PumpEvents(timeoutMillis)) {
        return false;
    }
    return !jni::EngineHost::Instance().StopRequested();
}

bool Runtime::ShouldExit() const {
    return bridge_.DestroyRequested() || jni::EngineHost::Instance().StopRequested();
}

bool Runtime::IsActive() const {
    return bridge_.WindowAttached() && bridge_.Foreground();
}

void Runtime::Tick(double fixedDeltaSeconds) {
    elapsedSeconds_ += fixedDeltaSeconds;
    CheckStartGate();
    if (!started_ || shell_ == nullptr) {
        return;
    }
    shell_->Update(fixedDeltaSeconds, bridge_.Input());
}

void Runtime::Render(double frameDeltaSeconds) {
    CheckStartGate();
    if (!started_ || device_ == nullptr || !device_->Ready()) {
        return;
    }
    LogDeviceProfileOnce();
    if (!device_->RenderFrame(elapsedSeconds_)) {
        return;
    }
    stats_.AddFrame(frameDeltaSeconds);
    if (!stats_.ShouldReport()) {
        return;
    }
    const core::FrameReport report = stats_.Snapshot();
    IM_LOGI(kTag,
            "frame fps=%.1f avg=%.2fms max=%.2fms frames=%lld size=%dx%d gpu=%s abi=%s grade=%s",
            report.averageFps, report.averageMs, report.maxMs, static_cast<long long>(report.frames),
            device_->Width(), device_->Height(), device_->DriverName(),
            abi_.empty() ? "unknown" : abi_.c_str(), grade_.empty() ? "unknown" : grade_.c_str());
}

void Runtime::CheckStartGate() {
    if (started_ || !jni::EngineHost::Instance().StartRequested()) {
        return;
    }
    started_ = true;
    IM_LOGI(kTag, "engine started by java request");
}

void Runtime::LogDeviceProfileOnce() {
    if (profileLogged_) {
        return;
    }
    const std::string payload = jni::EngineHost::Instance().CopyDeviceProfile();
    if (payload.empty()) {
        return;
    }
    profile_ = ParseProfile(payload);
    abi_ = Lookup(profile_, "abi");
    grade_ = Lookup(profile_, "grade");
    for (const auto& entry : profile_) {
        IM_LOGI(kTag, "device profile %s=%s", entry.first.c_str(), entry.second.c_str());
    }
    profileLogged_ = true;
}

void Runtime::OnWindowCreated(ANativeWindow* window) {
    if (device_ == nullptr) {
        IM_LOGE(kTag, "render device missing on window creation");
        return;
    }
    if (!device_->Initialize(window)) {
        IM_LOGE(kTag, "render device initialize failed");
        return;
    }
    stats_.ResetWindow();
    if (shell_ != nullptr) {
        shell_->SetViewportSize(device_->Width(), device_->Height());
        AAssetManager* assets = nullptr;
        if (bridge_.Handle() != nullptr && bridge_.Handle()->activity != nullptr) {
            assets = bridge_.Handle()->activity->assetManager;
        }
        shell_->OnSurfaceReady(assets);
        device_->SetSceneCallback(&Runtime::SceneCallbackThunk, shell_.get());
    }
    IM_LOGI(kTag, "render surface ready, %s %dx%d", device_->BackendName(), device_->Width(),
            device_->Height());
}

void Runtime::SceneCallbackThunk(void* user) {
    auto* shell = static_cast<game::GameShell*>(user);
    if (shell != nullptr) {
        shell->RenderScene();
    }
}

void Runtime::OnWindowDestroyed() {
    if (device_ != nullptr) {
        device_->SetSceneCallback(nullptr, nullptr);
    }
    if (shell_ != nullptr) {
        shell_->OnSurfaceLost();
    }
    if (device_ != nullptr) {
        device_->Shutdown();
    }
    IM_LOGI(kTag, "render surface released");
}

void Runtime::OnWindowResized(int32_t width, int32_t height) {
    if (device_ != nullptr) {
        device_->Resize(width, height);
    }
    if (shell_ != nullptr) {
        shell_->SetViewportSize(width, height);
    }
}

void Runtime::OnForegroundChanged(bool foreground) {
    IM_LOGI(kTag, "foreground=%d", foreground ? 1 : 0);
}

}  /* namespace */

void RunApplication(android_app* app) {
    if (app == nullptr) {
        IM_LOGE(kTag, "RunApplication without android_app");
        return;
    }
    IM_LOGI(kTag, "engine runtime created");
    Runtime runtime(app);
    runtime.Run();
    runtime.Shutdown();
    IM_LOGI(kTag, "engine runtime disposed");
}

}  /* namespace immirror::platform */
