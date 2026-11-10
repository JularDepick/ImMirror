#pragma once

#include <cstdint>

namespace immirror::platform {

/*
 * 同时在册的指针上限. 实战最多同时出现摇杆指, 视角指与两个按钮指,
 * 超出上限的触点直接丢弃, 避免池满时顶掉已经绑定的角色.
 */
constexpr int32_t kMaxPointers = 4;

/*
 * 单路指针. 位置为归一化屏幕坐标, 原点在左上角, 横纵均为 0 至 1.
 * motionX 与 motionY 是该指针自上次被逻辑层消费以来的累计位移:
 * 平台层按触摸事件携带的历史样本逐段累加, 一帧内多次回调不会丢动量,
 * 手指快速来回滑动又回到原点时也仍然能累积出转角.
 */
struct PointerState {
    int32_t id = -1;         /* 平台侧指针 id, 跨帧稳定, 用于区分不同手指 */
    float x = 0.0f;          /* 当前横坐标, 以屏宽为 1 */
    float y = 0.0f;          /* 当前纵坐标, 以屏高为 1 */
    float motionX = 0.0f;    /* 累计横向位移, 以屏宽为 1 */
    float motionY = 0.0f;    /* 累计纵向位移, 以屏高为 1 */
    bool down = false;
    bool uiClaimed = false;  /* 已被界面按钮认领, 不参与摇杆与视角分配 */
};

/*
 * 浮动摇杆的视觉状态. 由玩家控制器写入, 界面层只读,
 * 两侧共用同一个锚点与半径, 界面不会与真实输入错位.
 */
struct StickVisual {
    bool active = false;
    float anchorX = 0.0f;  /* 锚点横坐标, 归一化屏幕坐标 */
    float anchorY = 0.0f;  /* 锚点纵坐标, 归一化屏幕坐标 */
    float valueX = 0.0f;   /* 摇杆量, 向右为正, 范围 -1 至 1 */
    float valueY = 0.0f;   /* 摇杆量, 向前为正, 范围 -1 至 1 */
    float radius = 0.0f;   /* 满量程半径, 以屏宽为 1 */
};

/*
 * 输入快照. 平台层在原生线程上解析 AInputEvent 后写入指针池,
 * 逻辑层与渲染层只读这些数据, 逐帧路径上不产生任何 JNI 调用.
 */
struct InputSnapshot {
    PointerState pointers[kMaxPointers];
    int32_t pointerCount = 0;   /* 当前按下的指针数 */
    uint64_t downMask = 0;      /* 按下的指针 id 位图, 第 n 位对应 id n */
    int32_t surfaceWidth = 0;   /* 归一化所用的窗口宽度 */
    int32_t surfaceHeight = 0;  /* 归一化所用的窗口高度 */
    int64_t touchEvents = 0;    /* 累计触摸事件数 */
    int64_t keyEvents = 0;      /* 累计按键事件数 */
    int32_t lastKeyCode = 0;    /* 最近一次按键的键值, 便于调试观察 */

    /* 按 id 查找正在按下的指针, 找不到返回空. */
    const PointerState* Find(int32_t id) const {
        if (id < 0) {
            return nullptr;
        }
        for (int32_t i = 0; i < kMaxPointers; ++i) {
            if (pointers[i].down && pointers[i].id == id) {
                return &pointers[i];
            }
        }
        return nullptr;
    }

    /* 按 id 查找指针, 不要求在按下状态, 供界面认领标记回写使用. */
    PointerState* FindMutable(int32_t id) {
        if (id < 0) {
            return nullptr;
        }
        for (int32_t i = 0; i < kMaxPointers; ++i) {
            if (pointers[i].down && pointers[i].id == id) {
                return &pointers[i];
            }
        }
        return nullptr;
    }

    /* 位图判定, id 超出 64 视为未按下. */
    bool IsDown(int32_t id) const {
        if (id < 0 || id >= 64) {
            return false;
        }
        return ((downMask >> id) & 1ull) != 0ull;
    }

    bool IsUiClaimed(int32_t id) const {
        const PointerState* pointer = Find(id);
        return pointer != nullptr && pointer->uiClaimed;
    }

    bool AnyDown() const { return pointerCount > 0; }

    /* 窗口宽高比, 窗口尺寸未知时按 1 处理. */
    float Aspect() const {
        return surfaceHeight > 0 ? static_cast<float>(surfaceWidth) /
                                       static_cast<float>(surfaceHeight)
                                 : 1.0f;
    }

    /* 逻辑层读完累计位移后整体清零, 避免同一段位移被重复计入. */
    void ClearMotion() {
        for (int32_t i = 0; i < kMaxPointers; ++i) {
            pointers[i].motionX = 0.0f;
            pointers[i].motionY = 0.0f;
        }
    }
};

}  /* namespace immirror::platform */
