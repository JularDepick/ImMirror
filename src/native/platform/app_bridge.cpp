#include "platform/app_bridge.h"

#include <cmath>
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

/* 单步位移上限, 超出视为触点位置跳变, 该步不计入累计位移. */
constexpr float kPointerJumpLimit = 0.35f;

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
    /*
     * 上一帧的累计位移已经被逻辑层消费, 在这里清零.
     * 位移只在收到新事件时产生, 因此清零点放在取事件之前, 本帧新事件照常累加.
     */
    for (int32_t i = 0; i < kMaxPointers; ++i) {
        tracks_[i].motionX = 0.0f;
        tracks_[i].motionY = 0.0f;
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

/*
 * 触摸事件分派. 按下与抬起按指针 id 增删池内条目, 移动按事件携带的历史样本
 * 逐段累加位移, 因此同一帧内连续多次回调不会丢动量, 多根手指也互不覆盖.
 */
void AppBridge::HandleMotion(AInputEvent* event) {
    const int32_t action = AMotionEvent_getAction(event);
    const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
    const int32_t width = SurfaceWidth();
    const int32_t height = SurfaceHeight();
    ++input_.touchEvents;

    switch (masked) {
        case AMOTION_EVENT_ACTION_DOWN:
            HandlePointerDown(event, 0);
            break;
        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            const size_t index = static_cast<size_t>(
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            HandlePointerDown(event, index);
            break;
        }
        case AMOTION_EVENT_ACTION_MOVE:
            HandlePointerMove(event);
            break;
        case AMOTION_EVENT_ACTION_POINTER_UP: {
            const size_t index = static_cast<size_t>(
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            HandlePointerUp(event, index);
            break;
        }
        case AMOTION_EVENT_ACTION_UP: {
            /* ACTION_UP 表示最后一根手指离开, 按规范清空整池, 不留下悬空角色. */
            const size_t index = static_cast<size_t>(
                (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            HandlePointerUp(event, index);
            ReleaseAllPointers();
            break;
        }
        case AMOTION_EVENT_ACTION_CANCEL:
            ReleaseAllPointers();
            IM_LOGI(kTag, "touch cancelled");
            break;
        default:
            break;
    }
    PublishPointers(width, height);
}

void AppBridge::HandlePointerDown(AInputEvent* event, size_t index) {
    if (index >= AMotionEvent_getPointerCount(event)) {
        return;
    }
    const int32_t id = AMotionEvent_getPointerId(event, index);
    if (TrackById(id) == nullptr && TrackForSlot() == nullptr) {
        /* 指针池已满, 忽略多出来的触点, 已绑定的摇杆与视角不受影响. */
        IM_LOGW(kTag, "pointer pool full, drop id=%d", id);
        return;
    }
    RecordPointer(id, AMotionEvent_getX(event, index), AMotionEvent_getY(event, index), true);
    IM_LOGI(kTag, "pointer down id=%d count=%d", id,
            static_cast<int32_t>(AMotionEvent_getPointerCount(event)));
}

void AppBridge::HandlePointerMove(AInputEvent* event) {
    const size_t count = AMotionEvent_getPointerCount(event);
    const size_t history = AMotionEvent_getHistorySize(event);
    for (size_t index = 0; index < count; ++index) {
        const int32_t id = AMotionEvent_getPointerId(event, index);
        if (TrackById(id) == nullptr) {
            continue;
        }
        /* 先按批次内的历史样本推进, 再落到本次事件的当前位置. */
        for (size_t step = 0; step < history; ++step) {
            RecordPointer(id, AMotionEvent_getHistoricalX(event, index, step),
                          AMotionEvent_getHistoricalY(event, index, step), true);
        }
        RecordPointer(id, AMotionEvent_getX(event, index), AMotionEvent_getY(event, index), true);
    }
}

void AppBridge::HandlePointerUp(AInputEvent* event, size_t index) {
    const size_t count = AMotionEvent_getPointerCount(event);
    if (index >= count) {
        return;
    }
    const int32_t id = AMotionEvent_getPointerId(event, index);
    PointerState* track = TrackById(id);
    if (track == nullptr) {
        return;
    }
    /* 抬起前的最后一段位移仍然计入, 手指快速划动不会丢掉结尾的转角. */
    AccumulateMotion(*track, AMotionEvent_getX(event, index), AMotionEvent_getY(event, index));
    track->down = false;
    IM_LOGI(kTag, "pointer up id=%d count=%d", id, static_cast<int32_t>(count));
}

void AppBridge::ReleaseAllPointers() {
    for (int32_t i = 0; i < kMaxPointers; ++i) {
        tracks_[i] = PointerState{};
    }
}

PointerState* AppBridge::TrackById(int32_t id) {
    for (int32_t i = 0; i < kMaxPointers; ++i) {
        if (tracks_[i].down && tracks_[i].id == id) {
            return &tracks_[i];
        }
    }
    return nullptr;
}

PointerState* AppBridge::TrackForSlot() {
    for (int32_t i = 0; i < kMaxPointers; ++i) {
        if (!tracks_[i].down) {
            return &tracks_[i];
        }
    }
    return nullptr;
}

/*
 * 写入一路指针的状态. 指针已按下且继续移动时累加位移, 新按下的指针从头计位移,
 * 单步位移异常大时视为位置跳变丢弃该步, 避免一帧内转头过度.
 */
void AppBridge::RecordPointer(int32_t id, float x, float y, bool down) {
    PointerState* track = TrackById(id);
    if (track == nullptr) {
        if (!down) {
            return;
        }
        track = TrackForSlot();
        if (track == nullptr) {
            return;
        }
        *track = PointerState{};
        track->id = id;
        track->x = x;
        track->y = y;
        track->down = true;
        return;
    }
    if (!down) {
        return;
    }
    AccumulateMotion(*track, x, y);
}

void AppBridge::AccumulateMotion(PointerState& track, float x, float y) {
    const float deltaX = x - track.x;
    const float deltaY = y - track.y;
    track.x = x;
    track.y = y;
    /*
     * 跟踪表里存的是设备像素, 跳变阈值按屏宽折算成像素再比较;
     * 超过阈值的单步位移视为触点位置跳变, 不参与累计.
     */
    const int32_t width = SurfaceWidth();
    const float limit = kPointerJumpLimit * static_cast<float>(width > 0 ? width : 1);
    if (std::fabs(deltaX) < limit && std::fabs(deltaY) < limit) {
        track.motionX += deltaX;
        track.motionY += deltaY;
    }
}

/* 把跟踪表整理成对外快照, 坐标在此处一次性归一化. */
void AppBridge::PublishPointers(int32_t width, int32_t height) {
    for (int32_t i = 0; i < kMaxPointers; ++i) {
        input_.pointers[i] = PointerState{};
    }
    input_.pointerCount = 0;
    input_.downMask = 0ull;
    input_.surfaceWidth = width;
    input_.surfaceHeight = height;

    for (int32_t i = 0; i < kMaxPointers; ++i) {
        const PointerState& track = tracks_[i];
        if (!track.down) {
            continue;
        }
        PointerState& out = input_.pointers[input_.pointerCount];
        out = track;
        if (width > 0 && height > 0) {
            out.x = track.x / static_cast<float>(width);
            out.y = track.y / static_cast<float>(height);
            out.motionX = track.motionX / static_cast<float>(width);
            out.motionY = track.motionY / static_cast<float>(height);
        }
        if (track.id >= 0 && track.id < 64) {
            input_.downMask |= (1ull << track.id);
        }
        input_.pointerCount += 1;
    }
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
