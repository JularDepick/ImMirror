#pragma once

#include <cstdint>

/*
 * 设置键总表. 键名, 默认值与取值范围全部取自客户端
 * temp/original/fake-mini-BUT-true-moments/src/lua/ui/mobile/setting.lua:
 * 该文件第 9 至 12 行的 t_DefaultSetData 共 30 个键, 另加 1 个由设置界面写入
 * 但不在默认表内的 hideui, 合计 31 个键.
 *
 * 维护方式: 新增或调整设置项只改下方 X 宏表的一行,
 * 枚举, 元数据表, 持久化文件与 JNI 接口都会自动跟着变化.
 * 宏参数顺序依次为: 枚举名, 落盘键名, 默认值, 最小值, 最大值, 中文语义.
 * 宏参数内不得出现半角逗号, 否则会被当成新的参数.
 *
 * 与的语义对应:
 * configure 画质总档, 滑块分三段, 1 低 2 中 3 高;
 * view 视角, 1 主视角 2 背视角 3 正视角;
 * view_distance 视野距离 5 档, 取值为 1 近 2 中 3 远 5 更远 8 最远;
 * fog 雾效, 0 关闭 1 淡雾 2 浓雾;
 * classical 与 rocker 互斥, 默认 rocker 为 1 表示浮动摇杆;
 * sensitivity 与 volume 与 screenbright 均为 0 到 100 的滑杆取值.
 */
#define IMMIRROR_SETTING_KEY_TABLE(X)                                              \
    X(kConfigure, "configure", 2, 1, 3, "画质总档")                                \
    X(kView, "view", 1, 1, 3, "视角")                                              \
    X(kPeaceModel, "peacemodel", 0, 0, 1, "和平模式")                              \
    X(kVolume, "volume", 80, 0, 100, "音量")                                       \
    X(kMusicOpen, "musicopen", 1, 0, 1, "音乐开关")                                \
    X(kSoundOpen, "soundopen", 1, 0, 1, "音效开关")                                \
    X(kVoiceOpen, "voiceopen", 1, 0, 1, "语音开关")                                \
    X(kSensitivity, "sensitivity", 50, 0, 100, "操作灵敏度")                       \
    X(kReversalY, "reversalY", 0, 0, 1, "反转 Y 轴")                               \
    X(kLeftHanded, "lefthanded", 0, 0, 1, "左手模式")                              \
    X(kSight, "sight", 0, 0, 1, "准心模式")                                        \
    X(kAutoJump, "autojump", 1, 0, 1, "自动跳跃")                                  \
    X(kRadarSteering, "radarSteering", 0, 0, 1, "雷达转向")                        \
    X(kCameraShake, "camerashake", 1, 0, 1, "镜头摇晃")                            \
    X(kScreenBright, "screenbright", 0, 0, 100, "画面亮度")                        \
    X(kViewDistance, "view_distance", 2, 1, 8, "视野距离")                         \
    X(kFog, "fog", 0, 0, 2, "雾效")                                                \
    X(kTreeShape, "treeshape", 1, 0, 1, "树形")                                    \
    X(kVibrate, "vibrate", 1, 0, 1, "震动")                                        \
    X(kClassical, "classical", 0, 0, 1, "经典操作模式")                            \
    X(kRocker, "rocker", 1, 0, 1, "摇杆操作模式")                                  \
    X(kReflect, "reflect", 0, 0, 1, "水面反射")                                    \
    X(kShadow, "shadow", 0, 0, 1, "实时光影")                                      \
    X(kLimiteFps, "limiteFps", 1, 0, 1, "限制帧率")                                \
    X(kPopUps, "popups", 1, 0, 1, "邀请弹窗")                                      \
    X(kFpsButton, "fpsbuttom", 1, 0, 1, "帧率显示按钮")                            \
    X(kPhysxParam, "physxparam", 1, 0, 1, "物理参数面板")                          \
    X(kCanTrace, "cantrace", 1, 0, 1, "允许好友追踪")                              \
    X(kDamageSwitch, "damageswitch", 0, 0, 1, "伤害数值显示")                      \
    X(kBloodSwitch, "bloodswitch", 0, 0, 1, "血条显示")                            \
    X(kHideUi, "hideui", 0, 0, 1, "隐藏界面")

namespace immirror::settings {

/*
 * 设置键枚举. 顺序与上方宏表严格一致, 枚举值同时用作内部数组下标,
 * 因此只能追加到表尾, 不得重排或删除已有行.
 */
enum class Key : int32_t {
#define IMMIRROR_SETTING_KEY_ENUM(enumName, keyName, defValue, minRange, maxRange, labelText) enumName,
    IMMIRROR_SETTING_KEY_TABLE(IMMIRROR_SETTING_KEY_ENUM)
#undef IMMIRROR_SETTING_KEY_ENUM
    kCount,
};

constexpr int32_t kSettingKeyCount = static_cast<int32_t>(Key::kCount);

/*
 * 单个设置键的元数据. name 同时用于配置文件行首与 JNI 查表,
 * 与setting.lua 的 setGameData 键名完全一致, 便于逐键对照.
 */
struct KeyInfo {
    Key key;
    const char* name;
    int32_t defaultValue;
    int32_t minValue;
    int32_t maxValue;
    const char* label;
};

/* 越界键回落到第一个键, 调用方不需要判空. */
const KeyInfo& Info(Key key);

/* 按落盘键名查找, 未知键返回空指针. */
const KeyInfo* FindKey(const char* name);

/* 把任意整数裁剪到合法范围, 视野距离额外吸附到 1 2 3 5 8 五档. */
int32_t NormalizeValue(Key key, int32_t value);

}  /* namespace immirror::settings */
