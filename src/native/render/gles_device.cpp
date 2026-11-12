#include "render/gles_device.h"

#include <cerrno>
#include <cmath>
#include <ctime>

#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>

#include "core/log.h"

namespace immirror::render {
namespace {

constexpr const char* kTag = "ImMirrorRender";

/* 清屏底色的不透明度; 三个颜色分量由总装按昼夜下发, 见 SetClearColor. */
constexpr float kClearAlpha = 1.0f;

/* 取图形 API 上报的字符串, 空指针统一转成空串, 避免日志出现 (null). */
std::string ReadGlString(GLenum name) {
    const GLubyte* value = glGetString(name);
    return value != nullptr ? std::string(reinterpret_cast<const char*>(value)) : std::string();
}

}  /* namespace */

GlesDevice::~GlesDevice() {
    Shutdown();
}

bool GlesDevice::Initialize(ANativeWindow* window) {
    if (window == nullptr) {
        IM_LOGE(kTag, "initialize without native window");
        return false;
    }
    Shutdown();

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        IM_LOGE(kTag, "eglGetDisplay failed");
        return false;
    }
    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(display_, &major, &minor) != EGL_TRUE) {
        IM_LOGE(kTag, "eglInitialize failed, error=0x%x", eglGetError());
        display_ = EGL_NO_DISPLAY;
        return false;
    }

    /* 优先 8 位色深与 16 位深度, 满足后续三维场景的深度测试需求. */
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE,
    };
    EGLint configCount = 0;
    if (eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) != EGL_TRUE || configCount < 1) {
        IM_LOGE(kTag, "eglChooseConfig failed, error=0x%x", eglGetError());
        Shutdown();
        return false;
    }

    /* 先申请 ES3 上下文; 极少数驱动只给 ES2, 此时回退并记录实际版本. */
    if (!CreateContext(3)) {
        IM_LOGW(kTag, "GLES3 context unavailable, falling back to GLES2");
        if (!CreateContext(2)) {
            IM_LOGE(kTag, "eglCreateContext failed, error=0x%x", eglGetError());
            Shutdown();
            return false;
        }
    }

    const EGLint surfaceAttribs[] = {EGL_NONE};
    surface_ = eglCreateWindowSurface(display_, config_, window, surfaceAttribs);
    if (surface_ == EGL_NO_SURFACE) {
        IM_LOGE(kTag, "eglCreateWindowSurface failed, error=0x%x", eglGetError());
        Shutdown();
        return false;
    }
    if (eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
        IM_LOGE(kTag, "eglMakeCurrent failed, error=0x%x", eglGetError());
        Shutdown();
        return false;
    }

    width_ = ANativeWindow_getWidth(window);
    height_ = ANativeWindow_getHeight(window);
    glViewport(0, 0, width_, height_);

    renderer_ = ReadGlString(GL_RENDERER);
    version_ = ReadGlString(GL_VERSION);
    driver_ = renderer_ + " | " + version_;
    ready_ = true;
    swapFailureLogged_ = false;
    IM_LOGI(kTag, "gles context ready, client=%d renderer=%s version=%s size=%dx%d", clientVersion_,
            renderer_.c_str(), version_.c_str(), width_, height_);
    return true;
}

bool GlesDevice::CreateContext(int32_t clientVersion) {
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, clientVersion,
        EGL_NONE,
    };
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        return false;
    }
    clientVersion_ = clientVersion;
    return true;
}

void GlesDevice::Shutdown() {
    const bool hadContext = display_ != EGL_NO_DISPLAY;
    if (hadContext) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        eglTerminate(display_);
    }
    display_ = EGL_NO_DISPLAY;
    config_ = nullptr;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    width_ = 0;
    height_ = 0;
    clientVersion_ = 0;
    ready_ = false;
    renderer_.clear();
    version_.clear();
    driver_.clear();
    if (hadContext) {
        IM_LOGI(kTag, "gles context released");
    }
}

void GlesDevice::Resize(int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    width_ = width;
    height_ = height;
    if (!ready_) {
        return;
    }
    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_TRUE) {
        glViewport(0, 0, width_, height_);
    }
}

void GlesDevice::SetFrameLimit(int32_t fps) {
    if (fps < 0) {
        fps = 0;
    }
    if (fps == frameLimit_) {
        return;
    }
    frameLimit_ = fps;
    IM_LOGI(kTag, "frame limit set to %d fps", fps);
}

bool GlesDevice::RenderFrame(double frameDeltaSeconds) {
    /*
     * 帧门控: 交换间隔保持 0, 由本函数按目标帧率把本帧富余的时间睡掉.
     * 入参是本帧相对上一帧的现实秒数, 富余量等于目标周期减去本帧耗时;
     * 富余为正说明本帧够快, 睡掉差额让帧间距贴近目标周期,
     * 富余为负说明本帧已超预算, 直接进入绘制不再补睡, 避免卡顿叠加.
     * 逐帧重新计算而不跨帧累计, 长时间运行也不会因误差积累而漂移.
     * 目标为 0 时不做任何节流. 门控只作用于交换节奏, 场景绘制照常执行,
     * 逻辑侧的固定步长推进不受影响.
     */
    if (frameLimit_ > 0 && frameDeltaSeconds > 0.0) {
        const double targetSeconds = 1.0 / static_cast<double>(frameLimit_);
        const double surplusMillis = (targetSeconds - frameDeltaSeconds) * 1000.0;
        /* 留 1 毫秒余量, 睡眠本身有调度误差, 睡满反而容易超过目标周期. */
        const int32_t sleepMillis = static_cast<int32_t>(std::floor(surplusMillis - 1.0));
        if (sleepMillis > 0) {
            /*
             * nanosleep 在安卓上会被信号打断并提前返回, 打断后剩余时长写回 remain,
             * 必须循环续睡直到睡满, 否则门控形同虚设.
             */
            struct timespec request;
            request.tv_sec = sleepMillis / 1000;
            request.tv_nsec = (sleepMillis % 1000) * 1000000L;
            struct timespec remain = request;
            while (nanosleep(&remain, &remain) == -1 && errno == EINTR) {
                /* 继续睡剩余部分 */
            }
        }
    }
    if (!ready_) {
        return false;
    }
    /* 上下文归属于本线程, 窗口重建或尺寸变化后可能已经失效, 这里做一次廉价校验. */
    if (eglGetCurrentContext() != context_) {
        if (eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
            if (!swapFailureLogged_) {
                IM_LOGE(kTag, "eglMakeCurrent failed during frame, error=0x%x", eglGetError());
                swapFailureLogged_ = true;
            }
            return false;
        }
    }
    glViewport(0, 0, width_, height_);
    glClearColor(clearRed_, clearGreen_, clearBlue_, kClearAlpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (sceneCallback_ != nullptr) {
        sceneCallback_(sceneUser_);
    }
    if (eglSwapBuffers(display_, surface_) != EGL_TRUE) {
        if (!swapFailureLogged_) {
            IM_LOGE(kTag, "eglSwapBuffers failed, error=0x%x", eglGetError());
            swapFailureLogged_ = true;
        }
        return false;
    }
    return true;
}

bool GlesDevice::Ready() const {
    return ready_;
}

void GlesDevice::SetClearColor(float red, float green, float blue) {
    /* 颜色只做范围裁剪, 不做伽马处理: 总装给的就是线性归一化值. */
    clearRed_ = red < 0.0f ? 0.0f : (red > 1.0f ? 1.0f : red);
    clearGreen_ = green < 0.0f ? 0.0f : (green > 1.0f ? 1.0f : green);
    clearBlue_ = blue < 0.0f ? 0.0f : (blue > 1.0f ? 1.0f : blue);
}

void GlesDevice::SetSceneCallback(SceneCallback callback, void* user) {
    sceneCallback_ = callback;
    sceneUser_ = user;
}

const char* GlesDevice::BackendName() const {
    /* 上下文尚未创建时只报后端族名, 避免日志出现与实际版本不符的描述. */
    if (clientVersion_ >= 3) {
        return "gles3";
    }
    if (clientVersion_ == 2) {
        return "gles2";
    }
    return "gles";
}

const char* GlesDevice::DriverName() const {
    return driver_.c_str();
}

int32_t GlesDevice::Width() const {
    return width_;
}

int32_t GlesDevice::Height() const {
    return height_;
}

std::unique_ptr<Device> CreateDefaultDevice() {
    return std::unique_ptr<Device>(new GlesDevice());
}

}  /* namespace immirror::render */
