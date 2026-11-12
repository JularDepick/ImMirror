#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "settings/settings_keys.h"
#include "settings/settings_store.h"

namespace immirror::settings {

/*
 * 设置单例. 进程内唯一实例, 由 Java 侧通过 JNI 写入, 原生主线程逐帧只读取
 * 原子快照, 因此逐帧路径上没有锁也没有 JNI 调用.
 *
 * 默认值全部来自 settings_keys.h 的键表, 与setting.lua 的 t_DefaultSetData 一致.
 * 取值写入时立即裁剪到合法范围并置脏, 由主循环低频落盘, 拖滑杆时不会每帧写文件.
 */
class Settings {
public:
    static Settings& Instance();

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    /*
     * 指定应用内部存储目录并载入设置. 重复调用幂等: 目录未变化时不重复读盘.
     * 未初始化时 Get 仍返回默认值, 因此任何时序下读取都安全.
     */
    bool Initialize(const char* dataPath);

    /* 配置文件绝对路径, 未初始化时为空串. 返回副本, 调用方不必关心生命周期. */
    std::string StoragePath() const;

    int32_t Get(Key key) const;

    /* 按落盘键名读取, 未知键返回 fallback. */
    int32_t GetByName(const char* name, int32_t fallback) const;

    /* 写入并裁剪到合法范围, 返回落定后的值. 经典模式与摇杆模式在此保持互斥. */
    int32_t Set(Key key, int32_t value);

    /* 按落盘键名写入, 未知键返回 -1 表示未生效. */
    int32_t SetByName(const char* name, int32_t value);

    void Reset(Key key);
    void ResetAll();

    bool Dirty() const { return dirty_.load(); }

    /* 主循环低频调用: 有脏数据时落盘. 返回是否写了文件. */
    bool FlushIfDirty();

    /* 立即落盘, 忽略脏标记. */
    bool Save();

    /*
     * 首次运行时按设备分级播种视野距离档位; 已有落盘值时不覆盖.
     * 返回播种后生效的区块列半径.
     */
    int32_t SeedViewDistanceFromGrade(const char* grade);

    /* 全量导出 "键=值" 多行文本, 供设置界面一次读完. */
    std::string Export() const;

    /* 导出键表元数据, 每行 "键|默认值|最小值|最大值", 供设置界面生成控件. */
    std::string Describe() const;

    /*
     * 派生量. 消费方只读这里, 换算常量集中在 settings_tuning.h,
     * 设置值本身的含义不扩散到游戏逻辑里.
     */
    float LookFactor() const;
    float LookDegreesPerScreenWidth() const;
    float LookDegreesPerScreenHeight() const;
    float MasterVolume() const;
    bool SoundEnabled() const;
    bool MusicEnabled() const;
    bool VoiceEnabled() const;
    bool AutoJump() const;
    bool CameraShake() const;
    bool ReversalY() const;
    bool LeftHanded() const;
    bool HideUi() const;
    bool ClassicalMoveMode() const;
    int32_t ViewDistanceChunks() const;

private:
    Settings();

    /* 把读到的条目套用到原子快照, 未知键与非法行只记日志. */
    void ApplyEntries(const std::vector<SettingsStore::Entry>& entries);
    void ApplyDefaults();

    SettingsStore store_;

    /* 初始化与落盘串行化, 逐帧读路径不参与. */
    mutable std::mutex mutex_;
    std::atomic<int32_t> values_[kSettingKeyCount];
    std::atomic<bool> dirty_{false};
    std::atomic<bool> viewDistanceFromFile_{false};
    std::string loadedPath_;
    bool loaded_ = false;
};

}  /* namespace immirror::settings */
