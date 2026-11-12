#pragma once

#include <string>

#include <EGL/egl.h>

#include "render/render_device.h"

namespace immirror::render {

/*
 * 基于 EGL 与 GLES 3.0 的渲染后端.
 * 每帧只做清屏与缓冲交换, 用于验证窗口, 上下文与交换链可用;
 * 后续的场景绘制接入相同接口.
 */
class GlesDevice final : public Device {
public:
    GlesDevice() = default;
    ~GlesDevice() override;

    GlesDevice(const GlesDevice&) = delete;
    GlesDevice& operator=(const GlesDevice&) = delete;

    bool Initialize(ANativeWindow* window) override;
    void Shutdown() override;
    void Resize(int32_t width, int32_t height) override;
    bool RenderFrame(double frameDeltaSeconds) override;
    void SetFrameLimit(int32_t fps) override;

    bool Ready() const override;
    void SetClearColor(float red, float green, float blue) override;
    void SetSceneCallback(SceneCallback callback, void* user) override;
    const char* BackendName() const override;
    const char* DriverName() const override;
    int32_t Width() const override;
    int32_t Height() const override;

private:
    /* 按给定客户端主版本创建上下文, 失败时由调用方尝试回退版本. */
    bool CreateContext(int32_t clientVersion);

    /*
     * 帧门控状态. 上限 0 表示不限帧; 门控只延迟交换, 不跳过场景回调,
     * 因此逻辑侧仍按自己的固定步长推进, 不受限帧影响.
     */
    int32_t frameLimit_ = 0;

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t clientVersion_ = 0;
    /* 清屏颜色, 由总装按昼夜下发, 默认值与旧版的深蓝一致. */
    float clearRed_ = 0.07f;
    float clearGreen_ = 0.12f;
    float clearBlue_ = 0.20f;
    bool ready_ = false;
    bool swapFailureLogged_ = false;
    SceneCallback sceneCallback_ = nullptr;
    void* sceneUser_ = nullptr;
    std::string renderer_;
    std::string version_;
    std::string driver_;
};

}  /* namespace immirror::render */
