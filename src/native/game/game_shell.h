#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "audio/audio_engine.h"
#include "platform/input_state.h"
#include "ui/hud_renderer.h"
#include "voxel/block_registry.h"
#include "voxel/voxel_types.h"

struct AAssetManager;

namespace immirror {

namespace platform {
struct InputSnapshot;
}

namespace render {
class ChunkRenderer;
}

namespace settings {
class Settings;
}

namespace world {
class World;
}

namespace player {
class PlayerController;
}

namespace game {

/*
 * 游戏总装: 持有世界, 玩家与区块渲染器,
 * 把主循环的固定步长更新与每帧场景绘制接到具体玩法上.
 * 触摸按指针 id 分别跟踪: 先由界面按钮认领, 剩下的交给玩家控制器分配摇杆与视角角色,
 * 因此多根手指同时操作时各自的动作互不打断.
 * 所有方法都在原生主线程调用.
 */
class GameShell {
public:
    GameShell();
    ~GameShell();

    GameShell(const GameShell&) = delete;
    GameShell& operator=(const GameShell&) = delete;

    /* 图形上下文就绪后调用; dataPath 为应用内部存储目录, 用于世界存档与设置落盘. */
    void OnSurfaceReady(AAssetManager* assets, const char* dataPath);

    /* 图形上下文失效前调用, 释放全部 GPU 资源. */
    void OnSurfaceLost();

    void SetViewportSize(int32_t width, int32_t height);

    /* 按设备分级 (high/medium/low) 播种视距档位. */
    void SetQualityFromGrade(const char* grade);

    void Update(double deltaSeconds, const platform::InputSnapshot& input);

    /* 在图形上下文中绘制一帧场景; 由渲染设备在清屏后回调. */
    void RenderScene();

private:
    /*
     * 把设置项折算成控制器参数: 灵敏度与反转 Y 进 LookTuning,
     * 左手与经典模式的固定底座进 StickTuning. 逐帧调用, 设置改变即时生效.
     */
    void ApplyControlSettings();

    /* 界面按钮与方块栏的认领与结算: 跳跃按钮按下即跳, 方块栏短按选中, 长按显示名称. */
    void UpdateHudButtons(platform::InputSnapshot& input, double deltaSeconds);

    /* 编辑手势: 右半屏短按破坏, 长按放置, 与摇杆和视角各自独立. */
    void UpdateEditGesture(platform::InputSnapshot& input);

    /* 方块名标签的显示时长倒计时. */
    void UpdateNamePlate(double deltaSeconds);

    /* 自动跳跃: 前推量足够且前方是一格高台阶时补一次跳跃请求. */
    void MaybeRequestAutoJump();

    /* 切换方块栏选中项, 同时刷新编辑所用方块. */
    void SelectSlot(int32_t slot);

    /* 设置改动后的视距与音量同步. */
    void SyncViewDistance();
    void SyncAudioSettings();

    void LogStatus(double deltaSeconds);

    /* 把世界与玩家状态写入存档. */
    void SaveWorld();

    /* 组装存档路径, 必要时创建目录. */
    void PrepareSavePath(const char* dataPath);

    /* 表面重建后复位指针快照与手势持有的状态. */
    void ResetInputState();

    std::unique_ptr<world::World> world_;
    std::unique_ptr<player::PlayerController> player_;
    std::unique_ptr<render::ChunkRenderer> renderer_;
    ui::HudRenderer hud_;
    platform::InputSnapshot lastInput_;

    /* 设置单例, 构造后即绑定, 逐帧只读原子快照. */
    settings::Settings* settings_ = nullptr;

    int32_t width_ = 0;
    int32_t height_ = 0;
    bool surfaceReady_ = false;

    /* 上一帧按下的指针位图, 用于识别本帧新按下的指针. */
    uint64_t previousDownMask_ = 0ull;

    /* 已消费到的触摸事件计数, 防止同一份累计位移被两帧重复计入. */
    int64_t lastProcessedTouchEvents_ = -1;

    /* 跳跃按钮持有的指针, 按下期间保持按压反馈. */
    int32_t jumpPointerId_ = -1;
    bool jumpHeld_ = false;

    /* 方块栏持有的指针与长按计时. */
    int32_t slotPointerId_ = -1;
    int32_t slotHoldIndex_ = -1;
    double slotHoldSeconds_ = 0.0;
    bool slotNameShown_ = false;

    /* 方块名标签: 显示中的槽位与剩余秒数. */
    int32_t nameSlot_ = -1;
    double nameRemainSeconds_ = 0.0;

    /* 编辑手势持有的指针与按下点. */
    int32_t editPointerId_ = -1;
    float editDownX_ = 0.0f;
    float editDownY_ = 0.0f;
    float editLastX_ = 0.0f;
    float editLastY_ = 0.0f;
    double editDownSeconds_ = 0.0;

    /* 音频设置生效状态的缓存, 避免每帧重复下发. */
    int32_t audioVolumePercent_ = -1;
    int32_t audioSoundOpen_ = -1;
    int32_t audioMusicOpen_ = -1;

    /* 上次自动跳跃的时刻, 用于限制触发频率. */
    double lastAutoJumpSeconds_ = -1.0;

    double elapsedSeconds_ = 0.0;
    double logAccumulator_ = 0.0;
    double saveAccumulator_ = 0.0;
    std::string savePath_;
    int32_t viewDistanceChunks_ = 4;
    int32_t selectedSlot_ = 0;
    voxel::BlockId selectedBlock_ = voxel::block::Grass;
    const char* qualityGrade_ = "high";
};

}  /* namespace game */
}  /* namespace immirror */
