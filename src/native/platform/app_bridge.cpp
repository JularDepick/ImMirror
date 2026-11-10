#include "platform/app_bridge.h"

#include <cstdlib>
#include <cstring>

#include <android/input.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include "core/log.h"

namespace immirror::platform {
namespace {

constexpr const char* kTag = "ImMirrorPlatform";

/* 存档状态魔数与版本, 用于校验恢复数据的来源, 避免读到旧格式脏数据. */
constexpr uint32_t kSavedStateMagic = 0x494D5331u;
constexpr uint32_t kSavedStateVersion = 1u;

/*
 * 最小存档状态. 当前只保存累计命令数,
 * 正式版本会在这里挂接世界与玩家状态.
 */
struct SavedState {
    uint32_t magic;
    uint32_t version;
    int64_t commandCount;
};

}  /* namespace */

AppBridge::AppBridge(android_app* app, AppLifecycleListener* listener)
    : app_(app), listener_(listener) {
    if (app_ == nullptr) {
        IM_LOGE(kTag, "AppBridge created without android_app handle");
        return;
    }
    app_->userData = this;
    app_->onAppCmd = &AppBridge::OnAppCommand;
    app_->onInputEvent = &AppBridge::OnInputEvent;

    if (app_->savedState != nullptr && app_->savedStateSize == sizeof(SavedState)) {
        SavedState restored{};
        std::memcpy(&restored, app_->savedState, sizeof(restored));
        if (restored.magic == kSavedStateMagic && restored.version == kSavedStateVersion) {
            IM_LOGI(kTag, "saved state restored, commands=%lld", static_cast<long long>(restored.commandCount));
        } else {
            IM_LOGW(kTag, "saved state rejected, magic=0x%x version=%u", restored.magic, restored.version);
        }
    }
    IM_LOGI(kTag, "app bridge attached, activityState=%d", app_->activityState);
}

AppBridge::~AppBridge() {
    if (app_ != nullptr) {
        app_->userData = nullptr;
        app_->onAppCmd = nullptr;
        app_->onInputEvent = nullptr;
    }
}

bool AppBridge::PumpEvents(int32_t timeoutMillis) {
    if (app_ == nullptr) {
        return false;
    }
    /* 每轮只取一个事件源, 取完后把等待时长降为 0, 尽量把队列排空再回到主循环. */
    int32_t waitMillis = timeoutMillis;
    for (;;) {
        int events = 0;
        android_poll_source* source = nullptr;
        const int result = ALooper_pollOnce(waitMillis, nullptr, &events, reinterpret_cast<void**>(&source));
        if (result < 0) {
            break;
        }
        if (source != nullptr) {
            source->process(app_, source);
        }
        if (app_->destroyRequested != 0) {
            return false;
        }
        waitMillis = 0;
    }
    return app_->destroyRequested == 0;
}

bool AppBridge::DestroyRequested() const {
    return app_ == nullptr || app_->destroyRequested != 0;
}

bool AppBridge::WindowAttached() const {
    return app_ != nullptr && app_->window != nullptr;
}

bool AppBridge::Foreground() const {
    return app_ != nullptr && app_->activityState == APP_CMD_RESUME;
}

int32_t AppBridge::SurfaceWidth() const {
    if (app_ == nullptr || app_->window == nullptr) {
        return 0;
    }
    return ANativeWindow_getWidth(app_->window);
}

int32_t AppBridge::SurfaceHeight() const {
    if (app_ == nullptr || app_->window == nullptr) {
        return 0;
    }
    return ANativeWindow_getHeight(app_->window);
}

const InputSnapshot& AppBridge::Input() const {
    return input_;
}

android_app* AppBridge::Handle() const {
    return app_;
}

void AppBridge::OnAppCommand(android_app* app, int32_t command) {
    if (app == nullptr || app->userData == nullptr) {
        return;
    }
    static_cast<AppBridge*>(app->userData)->HandleCommand(command);
}

int32_t AppBridge::OnInputEvent(android_app* app, AInputEvent* event) {
    if (app == nullptr || app->userData == nullptr || event == nullptr) {
        return 0;
    }
    return static_cast<AppBridge*>(app->userData)->HandleInput(event);
}

void AppBridge::HandleCommand(int32_t command) {
    switch (command) {
        case APP_CMD_INIT_WINDOW: {
            const int32_t width = SurfaceWidth();
            const int32_t height = SurfaceHeight();
            IM_LOGI(kTag, "window created size=%dx%d", width, height);
            if (listener_ != nullptr) {
                listener_->OnWindowCreated(app_->window);
            }
            break;
        }
        case APP_CMD_TERM_WINDOW:
            IM_LOGI(kTag, "window terminating");
            if (listener_ != nullptr) {
                listener_->OnWindowDestroyed();
            }
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_WINDOW_REDRAW_NEEDED:
        case APP_CMD_CONTENT_RECT_CHANGED: {
            const int32_t width = SurfaceWidth();
            const int32_t height = SurfaceHeight();
            IM_LOGD(kTag, "window resized size=%dx%d", width, height);
            if (listener_ != nullptr) {
                listener_->OnWindowResized(width, height);
            }
            break;
        }
        case APP_CMD_GAINED_FOCUS:
            IM_LOGI(kTag, "window focus gained");
            break;
        case APP_CMD_LOST_FOCUS:
            IM_LOGI(kTag, "window focus lost");
            break;
        case APP_CMD_START:
        case APP_CMD_RESUME:
            IM_LOGI(kTag, "activity state foreground");
            if (listener_ != nullptr) {
                listener_->OnForegroundChanged(true);
            }
            break;
        case APP_CMD_PAUSE:
        case APP_CMD_STOP:
            IM_LOGI(kTag, "activity state paused");
            if (listener_ != nullptr) {
                listener_->OnForegroundChanged(false);
            }
            break;
        case APP_CMD_SAVE_STATE:
            SaveState();
            break;
        case APP_CMD_LOW_MEMORY:
            IM_LOGW(kTag, "system reported low memory");
            break;
        case APP_CMD_CONFIG_CHANGED:
            IM_LOGI(kTag, "configuration changed");
            break;
        case APP_CMD_DESTROY:
            IM_LOGI(kTag, "activity destroy requested");
            break;
        default:
            IM_LOGD(kTag, "unhandled app command %d", command);
            break;
    }
}

int32_t AppBridge::HandleInput(AInputEvent* event) {
    switch (AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_MOTION:
            HandleMotion(event);
            return 1;
        case AINPUT_EVENT_TYPE_KEY:
            HandleKey(event);
            return 1;
        default:
            return 0;
    }
}

void AppBridge::HandleMotion(AInputEvent* event) {
    const int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    const size_t pointerCount = AMotionEvent_getPointerCount(event);
    input_.pointerCount = static_cast<int32_t>(pointerCount);
    ++input_.touchEvents;

    if (pointerCount > 0) {
        const float x = AMotionEvent_getX(event, 0);
        const float y = AMotionEvent_getY(event, 0);
        const int32_t width = SurfaceWidth();
        const int32_t height = SurfaceHeight();
        if (width > 0 && height > 0) {
            input_.pointerX = x / static_cast<float>(width);
            input_.pointerY = y / static_cast<float>(height);
        }
    }
    input_.pointerDown = action == AMOTION_EVENT_ACTION_DOWN ||
                         action == AMOTION_EVENT_ACTION_MOVE ||
                         action == AMOTION_EVENT_ACTION_POINTER_DOWN;
}

void AppBridge::HandleKey(AInputEvent* event) {
    const int32_t keyCode = AKeyEvent_getKeyCode(event);
    input_.lastKeyCode = keyCode;
    ++input_.keyEvents;

    if (AKeyEvent_getAction(event) != AKEY_EVENT_ACTION_UP) {
        return;
    }
    /* 音量键交还系统处理, 其余按键在原生侧吞掉, 避免触发系统默认行为. */
    if (keyCode == AKEYCODE_VOLUME_UP || keyCode == AKEYCODE_VOLUME_DOWN ||
        keyCode == AKEYCODE_VOLUME_MUTE) {
        return;
    }
    IM_LOGI(kTag, "key up code=%d touches=%lld", keyCode, static_cast<long long>(input_.touchEvents));
    if (keyCode == AKEYCODE_BACK) {
        RequestExit();
    }
}

void AppBridge::SaveState() {
    if (app_ == nullptr || app_->savedState != nullptr) {
        return;
    }
    app_->savedState = std::malloc(sizeof(SavedState));
    if (app_->savedState == nullptr) {
        IM_LOGE(kTag, "saved state allocation failed");
        return;
    }
    auto* state = static_cast<SavedState*>(app_->savedState);
    state->magic = kSavedStateMagic;
    state->version = kSavedStateVersion;
    state->commandCount = input_.touchEvents + input_.keyEvents;
    app_->savedStateSize = sizeof(SavedState);
    IM_LOGI(kTag, "saved state written, commands=%lld", static_cast<long long>(state->commandCount));
}

void AppBridge::RequestExit() {
    if (app_ == nullptr || exiting_) {
        return;
    }
    exiting_ = true;
    IM_LOGI(kTag, "exit requested by back key");
    ANativeActivity_finish(app_->activity);
}

}  /* namespace immirror::platform */
