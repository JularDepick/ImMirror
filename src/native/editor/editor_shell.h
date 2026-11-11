#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/vec3.h"
#include "editor/editor_hud.h"
#include "editor/editor_types.h"
#include "editor/trigger_runtime.h"
#include "editor/undo_stack.h"
#include "editor/project_format.h"
#include "platform/input_state.h"
#include "voxel/voxel_types.h"

namespace immirror::render {
class ChunkRenderer;
}

namespace immirror::world {
class World;
}

namespace immirror::player {
class PlayerController;
}

namespace immirror::settings {
class Settings;
}

namespace immirror::editor {

class TriggerHost;

/*
 * 编辑器接管游戏总装所需的运行时对象.
 * 世界, 玩家与区块渲染器都由游戏总装持有, 编辑器只借用:
 * 这样编辑与试玩共用同一份世界数据与同一条渲染管线, 不需要第二套内存.
 */
struct EditorContext {
    world::World* world = nullptr;
    player::PlayerController* player = nullptr;
    render::ChunkRenderer* renderer = nullptr;
    settings::Settings* settings = nullptr;
    const char* dataPath = nullptr;
    int32_t viewDistanceChunks = limits::kDefaultViewDistance;
};

/* 编辑工具: 放置, 删除与框选填充. */
enum class EditTool {
    kPlace = 0,
    kRemove = 1,
    kFill = 2,
};

/*
 * 编辑器外壳.
 * 负责工程加载与落盘, 相机与拾取 (与游戏同一套控制器), 方块编辑与撤销重做,
 * 触发器运行时驱动, 覆盖层界面绘制与触摸命中, 以及试玩模式的进出.
 * 所有方法都在原生主线程调用.
 */
class EditorShell {
public:
    EditorShell();
    ~EditorShell();

    EditorShell(const EditorShell&) = delete;
    EditorShell& operator=(const EditorShell&) = delete;

    /*
     * 绑定运行时对象并载入工程.
     * 返回 false 时 error 给出原因, 调用方应当放弃进入编辑器.
     */
    bool Attach(const EditorContext& context, const std::string& projectDirectory,
                std::string& error);

    /* 释放界面资源并把工程落盘; 表面失效与退出编辑器时调用. */
    void Detach();

    /* 保存工程, 返回是否成功. */
    bool SaveNow();

    void SetViewport(int32_t width, int32_t height);

    /* 推进一帧: 界面认领, 世界同步, 玩家与手势, 触发器. */
    void Update(double deltaSeconds, platform::InputSnapshot& input);

    /* 在区块绘制之后调用: 绘制覆盖层. 场景绘制由 RenderScene 完成. */
    void RenderScene();

    bool Active() const { return active_; }
    bool Playtest() const { return mode_ == EditorMode::kPlaytest; }
    const std::string& ProjectDirectory() const { return project_.Directory(); }
    const std::string& LastError() const { return lastError_; }

    /* 原生侧是否请求回到 Java 界面. */
    bool ExitRequested() const { return exitRequested_; }
    void ClearExitRequest() { exitRequested_ = false; }

    /* 供 Java 侧调用的编辑操作, 全部是低频调用. */
    bool Undo();
    bool Redo();
    bool SetTool(int32_t tool);
    bool SetPaletteIndex(int32_t index);
    bool ExportWorld(const std::string& path, std::string& error);
    std::string DescribeState() const;

    /* 触发器增删; 返回新建触发器的 id, 失败返回 -1. */
    int32_t AddTrigger();
    bool DeleteTrigger(int32_t id);

    /* PlayerController 的拾取与编辑参数在编辑器内保持一致, 供外部查询. */
    bool TargetBlock(voxel::BlockPos& out) const;

private:
    /* 一帧界面数据的组装与按钮定义. */
    void BuildFrame();
    void BuildPanelRows();
    void BuildStatusLines();
    void FillButtons();

    /* 触摸: 界面按钮与面板认领, 编辑手势, 试玩手势. */
    void HandleUiPointers(platform::InputSnapshot& input);
    void UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds);
    void UpdatePlaytestGesture(platform::InputSnapshot& input, double deltaSeconds);

    /* 按钮与面板行的动作. */
    void OnButton(EditorButton button);
    void OnRowDelta(int32_t row, int32_t delta);
    void OnPalette(int32_t index);

    /* 拾取: 与游戏控制器同一套 DDA, 但命中非空气方块, 便于删除水与草. */
    bool Pick(const world::World& world, const core::Vec3& origin, const core::Vec3& direction,
              float maxDistance, voxel::BlockPos& block, voxel::BlockPos& face) const;

    /* 相机视点与视线方向, 与游戏内取值一致. */
    core::Vec3 EyePosition() const;
    core::Vec3 LookDirection() const;

    /* 写一格: 先进工程再投影到世界, 两侧保持同一份事实. */
    bool WriteBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id);

    /* 撤销栈回调. */
    static void UndoApply(void* user, int32_t x, int32_t y, int32_t z, voxel::BlockId id);

    /* 单格编辑与框选填充, 两者都会压入撤销栈. */
    bool ApplySingle(voxel::BlockPos target, voxel::BlockId id, const char* label);
    bool ApplyFill(const voxel::BlockPos& first, const voxel::BlockPos& second, voxel::BlockId id);

    /* 试玩模式的进出与出生点落位. */
    void SetPlaytest(bool enabled);
    void RespawnAtProjectSpawn();

    /* 提示文本与计时. */
    void ShowHint(const char* format, ...);
    void TickHint(double deltaSeconds);

    /* 控制参数同步: 灵敏度, 反转 Y, 摇杆模式与半径. */
    void ApplyControlSettings();

    /* 面板行追加的辅助, 越界时忽略; kind 与 extra 记录该行的编辑语义. */
    void PushRow(const char* label, const char* value, bool steppable, int32_t kind,
                 int32_t extra);
    void PushRowInt(const char* label, int32_t value, bool steppable, int32_t kind,
                    int32_t extra);
    void PushRowFloat(const char* label, float value, bool steppable, int32_t kind,
                      int32_t extra);

    /* 当前激活的触发器, 没有时返回空指针. */
    Trigger* ActiveTrigger();

    /* 触发器改动后的收尾: 重建脚本并同步运行时. */
    void TouchTrigger(Trigger& trigger);

    EditorContext context_;
    Project project_;
    UndoStack undo_;
    EditorHud hud_;
    std::unique_ptr<TriggerHost> host_;
    std::unique_ptr<TriggerRuntime> runtime_;

    EditorMode mode_ = EditorMode::kEdit;
    EditTool tool_ = EditTool::kPlace;
    int32_t selectedPalette_ = 0;
    int32_t activeTriggerId_ = -1;
    bool panelVisible_ = false;
    int32_t highlightedRow_ = -1;

    /* 框选: 第一次点选记录角点, 第二次点选直接填充. */
    bool fillHasFirst_ = false;
    voxel::BlockPos fillFirst_{};

    /* 拾取结果, 每帧更新一次供界面与编辑共用. */
    bool hasTarget_ = false;
    voxel::BlockPos targetBlock_{};
    voxel::BlockPos targetFace_{};

    /* 试玩时记录本次按住要破坏的格子, 破坏完成后按同一格写回工程数据. */
    voxel::BlockPos mineTarget_{};

    /* 输入跟踪: 与游戏总装一致, 记录上一帧的指针位图以识别新按下. */
    uint64_t previousDownMask_ = 0ull;
    int64_t lastProcessedTouchEvents_ = -1;
    int32_t editPointerId_ = -1;
    float editDownX_ = 0.0f;
    float editDownY_ = 0.0f;
    float editLastX_ = 0.0f;
    float editLastY_ = 0.0f;
    double editDownSeconds_ = 0.0;

    /* 界面数据. */
    EditorFrame frame_{};
    EditorButtonState buttons_[10] = {};
    int32_t buttonCount_ = 0;
    EditorRowState rows_[limits::kPanelRows] = {};
    int32_t rowCount_ = 0;
    int32_t rowKind_[limits::kPanelRows] = {};
    int32_t rowExtra_[limits::kPanelRows] = {};
    char statusLines_[limits::kStatusLines][96] = {};
    const char* statusPointers_[limits::kStatusLines] = {};
    char hint_[96] = {0};
    double hintRemainSeconds_ = 0.0;
    char title_[64] = {0};
    char panelTitle_[48] = {0};

    float viewProjection_[16] = {0.0f};
    core::Vec3 lastFeet_{};
    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t viewDistanceChunks_ = limits::kDefaultViewDistance;
    double elapsedSeconds_ = 0.0;
    bool active_ = false;
    bool exitRequested_ = false;
    std::string lastError_;
};

}  /* namespace immirror::editor */
