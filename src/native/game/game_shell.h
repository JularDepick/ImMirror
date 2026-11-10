#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "audio/audio_engine.h"
#include "platform/input_state.h"
#include "ui/hud_renderer.h"

struct AAssetManager;

namespace immirror {

namespace platform {
struct InputSnapshot;
}

namespace render {
class ChunkRenderer;
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
 * 所有方法都在原生主线程调用.
 */
class GameShell {
public:
    GameShell();
    ~GameShell();

    GameShell(const GameShell&) = delete;
    GameShell& operator=(const GameShell&) = delete;

    /* 图形上下文就绪后调用; dataPath 为应用内部存储目录, 用于世界存档. */
    void OnSurfaceReady(AAssetManager* assets, const char* dataPath);

    /* 图形上下文失效前调用, 释放全部 GPU 资源. */
    void OnSurfaceLost();

    void SetViewportSize(int32_t width, int32_t height);

    /* 按设备分级 (high/medium/low) 调整视距等画质参数. */
    void SetQualityFromGrade(const char* grade);

    void Update(double deltaSeconds, const platform::InputSnapshot& input);

    /* 在图形上下文中绘制一帧场景; 由渲染设备在清屏后回调. */
    void RenderScene();

private:
    /* 触摸动作到世界编辑的映射: 短按破坏, 长按放置. */
    void HandleTapActions(const platform::InputSnapshot& input);

    void LogStatus(double deltaSeconds);

    /* 把世界与玩家状态写入存档. */
    void SaveWorld();

    /* 组装存档路径, 必要时创建目录. */
    void PrepareSavePath(const char* dataPath);

    std::unique_ptr<world::World> world_;
    std::unique_ptr<player::PlayerController> player_;
    std::unique_ptr<render::ChunkRenderer> renderer_;
    ui::HudRenderer hud_;
    platform::InputSnapshot lastInput_;

    int32_t width_ = 0;
    int32_t height_ = 0;
    bool surfaceReady_ = false;

    bool previousPointerDown_ = false;
    double pointerDownSeconds_ = 0.0;
    float pointerDownX_ = 0.0f;
    float pointerDownY_ = 0.0f;

    double elapsedSeconds_ = 0.0;
    double logAccumulator_ = 0.0;
    double saveAccumulator_ = 0.0;
    std::string savePath_;
    int32_t viewDistanceChunks_ = 4;
    const char* qualityGrade_ = "high";
};

}  /* namespace game */
}  /* namespace immirror */
