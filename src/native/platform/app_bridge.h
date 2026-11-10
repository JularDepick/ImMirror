#pragma once

#include <cstdint>

#include "platform/input_state.h"

struct android_app;
struct ANativeWindow;
struct AInputEvent;

namespace immirror::platform {

/*
 * 生命周期回调. 全部在原生主线程上触发,
 * 实现方不需要额外加锁, 也不应在此长时间阻塞.
 */
class AppLifecycleListener {
public:
    virtual ~AppLifecycleListener() = default;

    /* 窗口就绪, 可以创建图形上下文. */
    virtual void OnWindowCreated(ANativeWindow* window) = 0;

    /* 窗口即将失效, 必须在返回前释放所有与窗口绑定的图形资源. */
    virtual void OnWindowDestroyed() = 0;

    /* 窗口尺寸变化, 参数取自当前的原生窗口. */
    virtual void OnWindowResized(int32_t width, int32_t height) = 0;

    /* 前后台切换. 由 Activity 的 START/RESUME/PAUSE/STOP 推导. */
    virtual void OnForegroundChanged(bool foreground) = 0;
};

/*
 * android_native_app_glue 的薄封装:
 * 把胶水层的命令队列与输入队列翻译成引擎可用的回调与输入快照,
 * 屏蔽 ANativeActivity, ALooper 与 AInputQueue 的使用细节.
 */
class AppBridge {
public:
    AppBridge(android_app* app, AppLifecycleListener* listener);
    ~AppBridge();

    AppBridge(const AppBridge&) = delete;
    AppBridge& operator=(const AppBridge&) = delete;

    /* 排空事件队列; 返回 false 表示 Activity 已请求销毁. */
    bool PumpEvents(int32_t timeoutMillis);

    bool DestroyRequested() const;
    bool WindowAttached() const;
    bool Foreground() const;
    int32_t SurfaceWidth() const;
    int32_t SurfaceHeight() const;

    const InputSnapshot& Input() const;
    android_app* Handle() const;

private:
    /* 胶水层回调入口, 通过 userData 回到当前实例. */
    static void OnAppCommand(android_app* app, int32_t command);
    static int32_t OnInputEvent(android_app* app, AInputEvent* event);

    void HandleCommand(int32_t command);
    int32_t HandleInput(AInputEvent* event);
    void HandleMotion(AInputEvent* event);
    void HandleKey(AInputEvent* event);
    void SaveState();
    void RequestExit();

    android_app* app_ = nullptr;
    AppLifecycleListener* listener_ = nullptr;
    InputSnapshot input_;
    bool exiting_ = false;
};

}  /* namespace immirror::platform */
