#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "audio/audio_engine.h"
#include "core/vec3.h"
#include "entity/entity_manager.h"
#include "game/inventory.h"
#include "game/item/inventory_screen.h"
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

namespace editor {

/* 编辑器外壳, 由总装在表面就绪时按 Java 侧的请求接管世界与渲染. */
class EditorShell;

}  /* namespace editor */

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

    /* 供 JNI 桥读取背包与选中格; 装配前调用无副作用. */
    Inventory& InventoryRef() { return inventory_; }
    int32_t SelectedSlot() const { return selectedSlot_; }
    void SelectSlotPublic(int32_t index) { SelectSlot(index); }

private:
    /*
     * 把设置项折算成控制器参数: 灵敏度与反转 Y 进 LookTuning,
     * 左手与经典模式的固定底座进 StickTuning. 逐帧调用, 设置改变即时生效.
     */
    void ApplyControlSettings();

    /* 界面按钮与方块栏的认领与结算: 跳跃按钮按下即跳, 方块栏短按选中, 长按显示名称. */
    void UpdateHudButtons(platform::InputSnapshot& input, double deltaSeconds);

    /*
     * 背包面板的指针认领与结算. 面板打开时独占全部指针:
     * 命中面板交给背包界面处理, 点在面板以外则关闭面板并归还合成材料.
     */
    void UpdateInventoryPanel(platform::InputSnapshot& input);

    /* 开局物品: 快捷栏每种方块一叠, 背包里放几样常用材料, 方便直接试合成. */
    void SeedStartingItems();

    /* 选中槽位的物品变化后同步编辑用方块, 物品不可放置时回落到空气. */
    void RefreshSelectedItem();

    /*
     * 编辑手势: 右半屏按住不松手按硬度累积破坏进度, 轻点放置当前材料,
     * 与摇杆和视角各自独立.
     */
    void UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds);

    /* 对准视线命中的方块开始挖掘; 目标不可破坏时返回 false, 本次手势只能结算为放置. */
    bool BeginEditMining();

    /* 推进挖掘进度: 按节拍播放挖掘音, 挖满时破坏方块并收取掉落物. */
    void AdvanceEditMining(double deltaSeconds);

    /* 把这次破坏的掉落物放进物品栏, 堆满或没有对应槽位时只累计丢弃数. */
    void CollectDrop(voxel::BlockId brokenBlock);

    /*
     * 生物与战斗: 每帧推进生物管理器, 把生物对玩家的伤害转交玩家控制器,
     * 并把死亡掉落收进物品栏. 实体层不反向依赖玩家层, 两者在这里对接.
     */
    void UpdateEntities(double deltaSeconds);

    /* 视线拾取生物并造成一次伤害; 命中返回 true, 未命中时本次手势仍按挖掘与放置走. */
    bool TryAttackCreature();

    /* 按住攻击时的连续出手结算; 目标消失后回落到挖掘. */
    void AdvanceHeldAttack(double deltaSeconds);

    /* 把生物死亡掉落的物品放进物品栏. */
    void CollectCreatureDrops();

    /* 用当前选中槽位的方块放置一次; 数量为 0 时拒绝放置并给出提示. */
    bool TryPlaceSelectedBlock();

    /* 方块栏提示: 登记提示文本并重置显示时长, 到时自动消失. */
    void ShowTip(const char* text);

    /* 方块名标签与方块栏提示的显示时长倒计时. */
    void UpdateNamePlate(double deltaSeconds);
    void UpdateTip(double deltaSeconds);

    /* 自动跳跃: 前推量足够且前方是一格高台阶时补一次跳跃请求. */
    void MaybeRequestAutoJump();

    /* 切换方块栏选中项, 同时刷新编辑所用方块. */
    void SelectSlot(int32_t slot);

    /* 设置改动后的视距与音量同步. */
    void SyncViewDistance();

    /* 把视距折算成生物生成半径下发, 视距外没有区块数据, 生成会落空. */
    void SyncEntitySpawnExtent();
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

    /* 物品栏: 前九格与快捷栏一一对应, 放置时扣减, 破坏与生物掉落入栏. */
    Inventory inventory_;

    /* 背包界面状态与它的一帧绘制数据, 面板打开时才填充. */
    InventoryScreen inventoryScreen_;
    ui::InventoryPanelFrame inventoryFrame_;

    /* 背包按钮持有的指针, 按下期间保持按压反馈. */
    int32_t bagPointerId_ = -1;
    bool bagHeld_ = false;

    /* 生物实体管理器, 持有全部生物并负责生成, 休眠, 回收与伤害结算. */
    entity::EntityManager entities_;

    /* 攻击手势状态: 本次手势是否已转为攻击, 以及是否出手过 (抬手不再放置方块). */
    bool attackHeld_ = false;
    bool editAttacked_ = false;
    double attackAccumulator_ = 0.0;

    /* 方块栏提示文本与剩余显示秒数, 空指针表示不显示. */
    const char* tipText_ = nullptr;
    double tipRemainSeconds_ = 0.0;

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