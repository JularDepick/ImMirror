#pragma once

#include <cstdint>

namespace immirror::platform {

/*
 * 输入快照. 平台层在原生线程上直接解析 AInputEvent 后写入,
 * 逻辑层与渲染层只读这些标量, 逐帧路径上不产生任何 JNI 调用.
 */
struct InputSnapshot {
    float pointerX = 0.0f;      /* 归一化到 [0,1] 的触点横坐标 */
    float pointerY = 0.0f;      /* 归一化到 [0,1] 的触点纵坐标 */
    int32_t pointerCount = 0;
    bool pointerDown = false;
    int64_t touchEvents = 0;    /* 累计触摸事件数 */
    int64_t keyEvents = 0;      /* 累计按键事件数 */
    int32_t lastKeyCode = 0;    /* 最近一次按键的键值, 便于调试观察 */
};

}  /* namespace immirror::platform */
