#pragma once

#include <cstdint>
#include <memory>

struct ANativeWindow;

namespace immirror::render {

/*
 * 渲染后端的统一抽象. 当前只有 GLES 实现,
 * 后续 Vulkan 后端实现同一接口即可在启动阶段按设备能力切换.
 */
class Device {
public:
    virtual ~Device() = default;

    /* 绑定原生窗口并创建图形上下文; 重复调用前应当先 Shutdown. */
    virtual bool Initialize(ANativeWindow* window) = 0;

    /* 释放上下文与表面; 未初始化时调用同样安全. */
    virtual void Shutdown() = 0;

    /* 同步新的窗口尺寸到视口. */
    virtual void Resize(int32_t width, int32_t height) = 0;

    /* 提交一帧; 返回 false 表示本帧失败, 调用方据此跳过帧率统计. */
    virtual bool RenderFrame(double elapsedSeconds) = 0;

    virtual bool Ready() const = 0;

    /*
     * 场景绘制回调: 在清屏之后, 交换缓冲之前调用.
     * 传空指针表示不绘制场景, 仅保留清屏行为.
     */
    using SceneCallback = void (*)(void* user);
    virtual void SetSceneCallback(SceneCallback callback, void* user) = 0;

    /* 后端名称, 例如 gles3. */
    virtual const char* BackendName() const = 0;

    /* 驱动与版本描述, 来自图形 API 自身上报, 用于日志与后续画质分级. */
    virtual const char* DriverName() const = 0;

    virtual int32_t Width() const = 0;
    virtual int32_t Height() const = 0;
};

/* 创建默认渲染后端; 当前固定返回 GLES 3.0 实现. */
std::unique_ptr<Device> CreateDefaultDevice();

}  /* namespace immirror::render */
