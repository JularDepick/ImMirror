#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace immirror::jni {

/*
 * Java 线程与原生主线程之间的共享状态容器.
 * Java 侧只在进入与退出游戏时写入一次;
 * 原生主线程逐帧只读取原子标量, 因此逐帧路径上没有任何 JNI 调用.
 */
class EngineHost {
public:
    static EngineHost& Instance();

    EngineHost(const EngineHost&) = delete;
    EngineHost& operator=(const EngineHost&) = delete;

    /* 保存 Java 侧采集的设备画像文本; 可在原生主循环已运行时调用. */
    void SetDeviceProfile(const std::string& payload);

    /* 取设备画像副本; 尚未设置时返回空串. */
    std::string CopyDeviceProfile() const;

    /* Java 侧请求启动与停止原生主循环. */
    void RequestStart();
    void RequestStop();
    bool StartRequested() const;
    bool StopRequested() const;

    /* 记录原生主循环是否已经接管控制权, 便于排查重复启动. */
    void MarkRuntimeActive(bool active);
    bool RuntimeActive() const;

private:
    EngineHost() = default;

    mutable std::mutex mutex_;
    std::string profile_;
    std::atomic<bool> startRequested_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> runtimeActive_{false};
};

}  /* namespace immirror::jni */
