#include "game/game_shell.h"

#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "core/vec3.h"
#include "game/generated/block_data_gen.h"
#include "game/inventory.h"
#include "player/input_tuning.h"
#include "player/mining_tuning.h"
#include "player/player_controller.h"
#include "platform/input_state.h"
#include "render/chunk_renderer.h"
#include "settings/settings.h"
#include "settings/settings_tuning.h"
#include "ui/hud_slots.h"
#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::game {
namespace {

constexpr const char* kTag = "ImMirrorGame";

/*
 * 相机参数. 视场角取原版四套预设共用的 75 度 (camera.lua 第 89 行),
 * 近远裁剪面原版未给出数值, 维持现值.
 */
constexpr float kFovYDegrees = 75.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 320.0f;
constexpr float kEyeHeight = 1.62f;

/* 方块栏长按显示名称的判定阈值, 以及编辑手势判定轻点的最大位移. */
constexpr double kLongPressSeconds = 0.5;
constexpr float kTapMoveLimit = 0.03f;

/* 编辑手势的落点分区: 只在右半屏生效, 左半屏留给移动摇杆. */
constexpr float kEditZoneSplitX = 0.5f;

/* 方块名标签在长按触发后继续显示的秒数. */
constexpr double kNamePlateSeconds = 1.5;

/* 方块栏提示的显示秒数. */
constexpr double kTipSeconds = 1.6;

/* 数量耗尽时的提示文本, 界面点阵字只收录 ASCII, 因此提示用英文. */
constexpr const char* kTipEmptySlot = "EMPTY SLOT";

/*
 * 物品栏槽位与方块栏共享同一张槽位表, 生成表与方块注册表条目等长;
 * 两处不一致时在编译期报错, 避免界面图标、物品栏与数据表三处错位.
 */
static_assert(Inventory::kSlotCount == ui::kHudSlotCount, "物品栏槽位数与方块栏槽位数不一致");
static_assert(gen::kBlockDataCount == static_cast<int32_t>(voxel::block::Count),
              "方块数据表条目数与方块注册表不一致, 请重新运行 scripts/gen_block_data.py");

/* 世界种子, 固定值便于复现地形. */
constexpr uint32_t kWorldSeed = 20260101u;

/* 自动存档间隔. */
constexpr double kAutoSaveSeconds = 20.0;

/*
 * 采集材质族到音效的换算. 方块的材质族在注册表里给出, 音频枚举按同一族名排列,
 * 这里只做一次族名到枚举的翻译. 族为 kNone (水, 基岩) 时返回 kCount, 表示不发声.
 */
audio::SoundId DigSoundOf(voxel::HarvestMaterial material) {
    switch (material) {
        case voxel::HarvestMaterial::kGrass:
            return audio::SoundId::kDigGrass;
        case voxel::HarvestMaterial::kStone:
            return audio::SoundId::kDigStone;
        case voxel::HarvestMaterial::kSand:
            return audio::SoundId::kDigSand;
        case voxel::HarvestMaterial::kWood:
            return audio::SoundId::kDigWood;
        case voxel::HarvestMaterial::kSnow:
            return audio::SoundId::kDigSnow;
        case voxel::HarvestMaterial::kGravel:
            return audio::SoundId::kDigGravel;
        case voxel::HarvestMaterial::kNone:
            break;
    }
    return audio::SoundId::kCount;
}

audio::SoundId PlaceSoundOf(voxel::HarvestMaterial material) {
    switch (material) {
        case voxel::HarvestMaterial::kGrass:
            return audio::SoundId::kPlaceGrass;
        case voxel::HarvestMaterial::kStone:
            return audio::SoundId::kPlaceStone;
        case voxel::HarvestMaterial::kSand:
            return audio::SoundId::kPlaceSand;
        case voxel::HarvestMaterial::kWood:
            return audio::SoundId::kPlaceWood;
        case voxel::HarvestMaterial::kSnow:
            return audio::SoundId::kPlaceSnow;
        case voxel::HarvestMaterial::kGravel:
            return audio::SoundId::kPlaceGravel;
        case voxel::HarvestMaterial::kNone:
            break;
    }
    return audio::SoundId::kCount;
}

/* 按方块材质播放挖掘音或放置音, 没有音效的材质直接跳过. */
void PlayHarvestSound(voxel::BlockId block, bool placement) {
    const voxel::HarvestMaterial material = voxel::BlockRegistry::Instance().Info(block).material;
    const audio::SoundId sound = placement ? PlaceSoundOf(material) : DigSoundOf(material);
    if (sound != audio::SoundId::kCount) {
        audio::AudioEngine::Instance().Play(sound);
    }
}

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

/* 指针 id 到位图的位号, 越界 id 视为永远未按下. */
uint64_t BitOf(int32_t pointerId) {
    if (pointerId < 0 || pointerId >= 64) {
        return 0ull;
    }
    return 1ull << pointerId;
}

/* 脚下是否有实心方块, 用于近似判断玩家是否站在地面上. */
bool GroundBlockSolid(const world::World& world, const core::Vec3& feet) {
    const voxel::BlockId id =
        world.GetBlock(FloorToInt(feet.x), FloorToInt(feet.y - 0.05f), FloorToInt(feet.z));
    return voxel::BlockRegistry::Instance().Info(id).solid;
}

/* 视线前向探测点是否为恰好一格高的台阶: 脚下这一层有方块, 上方两格为空. */
bool StepAheadIsOneBlock(const world::World& world, const core::Vec3& feet, float yaw) {
    const float forwardX = -std::sin(yaw);
    const float forwardZ = -std::cos(yaw);
    const int32_t probeX =
        FloorToInt(feet.x + forwardX * settings::tuning::kAutoJumpProbeDistance);
    const int32_t probeZ =
        FloorToInt(feet.z + forwardZ * settings::tuning::kAutoJumpProbeDistance);
    const int32_t baseY = FloorToInt(feet.y + 0.05f);
    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    if (!registry.Info(world.GetBlock(probeX, baseY, probeZ)).solid) {
        return false;
    }
    if (registry.Info(world.GetBlock(probeX, baseY + 1, probeZ)).solid) {
        return false;
    }
    return !registry.Info(world.GetBlock(probeX, baseY + 2, probeZ)).solid;
}

void MakePerspective(float* out, float fovYDegrees, float aspect, float nearPlane, float farPlane) {
    const float radians = fovYDegrees * 3.14159265358979f / 180.0f;
    const float f = 1.0f / std::tan(radians * 0.5f);
    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }
    out[0] = f / (aspect > 0.0001f ? aspect : 1.0f);
    out[5] = f;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

/* 列主序 4x4 相乘: out = a * b. */
void Multiply4x4(const float* a, const float* b, float* out) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a[k * 4 + row] * b[col * 4 + k];
            }
            out[col * 4 + row] = sum;
        }
    }
}

}  /* namespace */

GameShell::GameShell()
    : world_(std::make_unique<world::World>(kWorldSeed)),
      player_(std::make_unique<player::PlayerController>()),
      renderer_(std::make_unique<render::ChunkRenderer>()) {
    /* 设置单例在构造时已填好默认值, 因此未初始化存储也能取到视距. */
    settings_ = &settings::Settings::Instance();
    viewDistanceChunks_ = settings_->ViewDistanceChunks();
    selectedBlock_ = ui::BlockSlotId(selectedSlot_);

    /* 物品栏按方块栏共享表播种, 开局每种方块 kInitialStack 个. */
    voxel::BlockId slotBlocks[Inventory::kSlotCount];
    for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
        slotBlocks[slot] = ui::BlockSlotId(slot);
    }
    inventory_.Reset(slotBlocks, Inventory::kSlotCount, 2);  /* TEMP-VERIFY */
}

GameShell::~GameShell() = default;

void GameShell::OnSurfaceReady(AAssetManager* assets, const char* dataPath) {
    if (surfaceReady_) {
        return;
    }
    if (!renderer_->Initialize(assets)) {
        IM_LOGE(kTag, "chunk renderer initialize failed");
        return;
    }
    surfaceReady_ = true;
    /* 表面重建后旧的指针快照已经失效, 复位输入状态, 不把退出前的按住带进新画面. */
    ResetInputState();
    PrepareSavePath(dataPath);

    /*
     * 设置文件与存档放在同一个内部存储根目录下. Java 侧可能已经初始化过,
     * 这里重复调用是幂等的; 尚未初始化时由平台给出的目录补上, 启动即完成加载.
     */
    settings_->Initialize(dataPath);
    SyncViewDistance();
    SyncAudioSettings();
    ApplyControlSettings();

    world::World::SaveState state;
    if (!savePath_.empty() && world_->LoadFromFile(savePath_.c_str(), state)) {
        player_->SetPose(core::Vec3{state.playerX, state.playerY, state.playerZ}, state.yaw, state.pitch);
        IM_LOGI(kTag, "world loaded from save, chunks=%d", world_->LoadedChunkCount());
    } else {
        /* 先让中心区块生成地形, 再按地表高度放置玩家, 否则出生点会落在虚空. */
        world_->Update(core::Vec3{8.0f, 0.0f, 8.0f}, viewDistanceChunks_);
        player_->Spawn(*world_, 8.0f, 8.0f);
    }
    world_->Update(player_->Position(), viewDistanceChunks_);
    hud_.SetViewport(width_, height_);
    hud_.Initialize(renderer_->AtlasTexture());
    IM_LOGI(kTag, "game shell ready, chunks=%d fov=%.0f", world_->LoadedChunkCount(),
            static_cast<double>(kFovYDegrees));
}

void GameShell::ResetInputState() {
    lastInput_ = platform::InputSnapshot{};
    previousDownMask_ = 0ull;
    lastProcessedTouchEvents_ = -1;
    jumpPointerId_ = -1;
    jumpHeld_ = false;
    slotPointerId_ = -1;
    slotHoldIndex_ = -1;
    slotHoldSeconds_ = 0.0;
    slotNameShown_ = false;
    editPointerId_ = -1;
    editDownX_ = 0.0f;
    editDownY_ = 0.0f;
    editLastX_ = 0.0f;
    editLastY_ = 0.0f;
    editDownSeconds_ = 0.0;
    tipText_ = nullptr;
    tipRemainSeconds_ = 0.0;
    /* 表面重建会丢弃旧指针, 挖掘进度一并作废, 不带进新画面. */
    player_->CancelMining();
}

void GameShell::PrepareSavePath(const char* dataPath) {
    if (dataPath == nullptr || dataPath[0] == '\0') {
        savePath_.clear();
        return;
    }
    std::string directory = std::string(dataPath) + "/worlds";
    ::mkdir(directory.c_str(), 0700);
    savePath_ = directory + "/default.world";
}

void GameShell::SaveWorld() {
    if (savePath_.empty() || !surfaceReady_) {
        return;
    }
    world::World::SaveState state;
    const core::Vec3 position = player_->Position();
    state.playerX = position.x;
    state.playerY = position.y;
    state.playerZ = position.z;
    state.yaw = player_->Yaw();
    state.pitch = player_->Pitch();
    const bool saved = world_->SaveToFile(savePath_.c_str(), state);
    IM_LOGI(kTag, "world save %s, path=%s", saved ? "ok" : "failed", savePath_.c_str());
}

void GameShell::OnSurfaceLost() {
    if (!surfaceReady_) {
        return;
    }
    SaveWorld();
    hud_.Shutdown();
    renderer_->Shutdown();
    surfaceReady_ = false;
    IM_LOGI(kTag, "game shell surface released");
}

void GameShell::SetQualityFromGrade(const char* grade) {
    qualityGrade_ = (grade != nullptr && grade[0] != '\0') ? grade : "high";
    /*
     * 设备分级只作为视距档位的默认值: 首次运行且设置文件里没有 view_distance 时播种,
     * 用户调整过以后以设置为准, 不再被分级覆盖.
     * 平台侧分级取值为 high / mid / low, 同时兼容 medium 写法, 由设置层解析.
     */
    viewDistanceChunks_ = settings_->SeedViewDistanceFromGrade(qualityGrade_);
    IM_LOGI(kTag, "quality grade=%s view distance=%d", qualityGrade_, viewDistanceChunks_);
}

void GameShell::SetViewportSize(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
    hud_.SetViewport(width, height);
}

void GameShell::Update(double deltaSeconds, const platform::InputSnapshot& input) {
    if (!surfaceReady_) {
        return;
    }
    elapsedSeconds_ += deltaSeconds;
    lastInput_ = input;

    /*
     * 同一份快照可能在一帧里被推进多次 (追帧时固定步长会连跑两步),
     * 累计位移只允许被消费一次, 因此没有新触摸事件时先清零.
     */
    if (lastInput_.touchEvents == lastProcessedTouchEvents_) {
        lastInput_.ClearMotion();
    }
    lastProcessedTouchEvents_ = lastInput_.touchEvents;

    /* 设置界面写入后由主线程低频落盘, 拖滑杆期间不会每帧写文件. */
    settings_->FlushIfDirty();
    SyncViewDistance();
    SyncAudioSettings();
    ApplyControlSettings();

    const core::Vec3 position = player_->Position();
    world_->Update(position, viewDistanceChunks_);

    /*
     * 顺序固定: 先让界面按钮认领指针, 认领过的指针不再参与摇杆与视角分配;
     * 再推进玩家, 最后结算编辑手势. 指针位图在帧末更新, 供下一帧识别新按下.
     */
    UpdateHudButtons(lastInput_, deltaSeconds);
    MaybeRequestAutoJump();
    player_->Update(deltaSeconds, lastInput_, *world_);
    UpdateEditGesture(lastInput_, deltaSeconds);
    UpdateNamePlate(deltaSeconds);
    UpdateTip(deltaSeconds);
    LogStatus(deltaSeconds);
    previousDownMask_ = lastInput_.downMask;

    /* 定期落盘, 避免进程被系统回收时丢失进度. */
    saveAccumulator_ += deltaSeconds;
    if (saveAccumulator_ >= kAutoSaveSeconds) {
        saveAccumulator_ = 0.0;
        SaveWorld();
    }
}

void GameShell::ApplyControlSettings() {
    /*
     * 灵敏度以原版滑杆中位 50 为基准折算: LookFactor 在 50 处为 1,
     * 控制器据此把横向满屏转角从 150 度按比例放大或缩小.
     */
    player::LookTuning look;
    look.sensitivity = player::LookTuning::kReferenceSensitivity * settings_->LookFactor();
    look.invertY = settings_->ReversalY();
    player_->SetLookTuning(look);

    /*
     * 摇杆: 经典模式把锚点固定在底座中心, 摇杆模式锚点跟随按下点;
     * 左手模式把摇杆区换到右半屏. 半径沿用设置层与界面共用的同一常量.
     */
    player::StickTuning stick;
    stick.radius = settings::tuning::kPadRadius;
    stick.fixedAnchor = settings_->ClassicalMoveMode();
    stick.anchorX = settings::tuning::kPadCenterX;
    stick.anchorY = settings::tuning::kPadCenterY;
    stick.leftHanded = settings_->LeftHanded();
    player_->SetStickTuning(stick);
}

void GameShell::UpdateHudButtons(platform::InputSnapshot& input, double deltaSeconds) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;
    const uint64_t released = previousDownMask_ & ~input.downMask;

    /*
     * 认领标记每帧重建: 已被按钮占用的手指在按住期间持续标记,
     * 否则下一帧会被玩家控制器当成空闲指针, 分配给摇杆或视角.
     */
    if (platform::PointerState* held = input.FindMutable(jumpPointerId_)) {
        held->uiClaimed = true;
    }
    if (platform::PointerState* held = input.FindMutable(slotPointerId_)) {
        held->uiClaimed = true;
    }

    /* 新按下的指针先经过界面命中判定, 命中即由界面认领并当场触发动作. */
    if (pressed != 0ull) {
        for (int32_t i = 0; i < platform::kMaxPointers; ++i) {
            platform::PointerState& pointer = input.pointers[i];
            if (!pointer.down || (pressed & BitOf(pointer.id)) == 0ull) {
                continue;
            }
            if (hud_.HitTestJump(pointer.x, pointer.y)) {
                pointer.uiClaimed = true;
                jumpPointerId_ = pointer.id;
                jumpHeld_ = true;
                player_->RequestJump();
                IM_LOGI(kTag, "hud jump pressed pointer=%d at %.3f,%.3f", pointer.id,
                        static_cast<double>(pointer.x), static_cast<double>(pointer.y));
                continue;
            }
            const int32_t slot = hud_.HitTestSlot(pointer.x, pointer.y);
            if (slot >= 0) {
                pointer.uiClaimed = true;
                slotPointerId_ = pointer.id;
                slotHoldIndex_ = slot;
                slotHoldSeconds_ = 0.0;
                slotNameShown_ = false;
                IM_LOGI(kTag, "hud slot pressed slot=%d pointer=%d", slot, pointer.id);
            }
        }
    }

    /* 方块栏长按: 只显示方块名, 选中项保持不变. */
    if (slotPointerId_ >= 0 && slotHoldIndex_ >= 0) {
        slotHoldSeconds_ += deltaSeconds;
        if (!slotNameShown_ && slotHoldSeconds_ >= kLongPressSeconds) {
            slotNameShown_ = true;
            nameSlot_ = slotHoldIndex_;
            nameRemainSeconds_ = kNamePlateSeconds;
            IM_LOGI(kTag, "block name shown, slot=%d name=%s", slotHoldIndex_,
                    ui::BlockSlotName(slotHoldIndex_));
        } else if (slotNameShown_) {
            nameRemainSeconds_ = kNamePlateSeconds;
        }
    }

    /* 方块栏短按才切换选中, 长按抬手不改选中. */
    if (slotPointerId_ >= 0 && (released & BitOf(slotPointerId_)) != 0ull) {
        if (!slotNameShown_ && slotHoldSeconds_ < kLongPressSeconds && slotHoldIndex_ >= 0) {
            SelectSlot(slotHoldIndex_);
        }
        slotPointerId_ = -1;
        slotHoldIndex_ = -1;
        slotHoldSeconds_ = 0.0;
        slotNameShown_ = false;
    }

    if (jumpPointerId_ >= 0 && (released & BitOf(jumpPointerId_)) != 0ull) {
        jumpPointerId_ = -1;
        jumpHeld_ = false;
    }
}

/*
 * 编辑手势. 破坏必须按硬度累积进度, 因此按住与轻点各自绑定一种动作:
 * 按住不松手走进度, 挖满才破坏; 轻点(按住不足轻点窗口且没有位移)放置当前材料.
 * 进度从按下那一刻开始累计, 抬手即清零, 与原版"按住挖掘, 松手作废"的语义一致.
 */
void GameShell::UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;
    const uint64_t released = previousDownMask_ & ~input.downMask;

    /* 只认领一路编辑指针, 且落点必须在右半屏; 界面按钮已认领的指针不参与. */
    if (editPointerId_ < 0 && pressed != 0ull) {
        for (int32_t i = 0; i < platform::kMaxPointers; ++i) {
            const platform::PointerState& pointer = input.pointers[i];
            if (!pointer.down || pointer.uiClaimed) {
                continue;
            }
            if ((pressed & BitOf(pointer.id)) == 0ull || pointer.x <= kEditZoneSplitX) {
                continue;
            }
            editPointerId_ = pointer.id;
            editDownX_ = pointer.x;
            editDownY_ = pointer.y;
            editLastX_ = pointer.x;
            editLastY_ = pointer.y;
            editDownSeconds_ = elapsedSeconds_;
            BeginEditMining();
            break;
        }
    }

    if (editPointerId_ < 0) {
        return;
    }

    const platform::PointerState* pointer = input.Find(editPointerId_);
    if (pointer != nullptr) {
        editLastX_ = pointer->x;
        editLastY_ = pointer->y;
        /* 按住期间逐帧推进进度, 挖满当场破坏. */
        AdvanceEditMining(deltaSeconds);
        return;
    }
    if ((released & BitOf(editPointerId_)) == 0ull) {
        /* 指针既不在按下集合也不在释放集合: 帧内被取消, 结束这次手势. */
        player_->CancelMining();
        editPointerId_ = -1;
        return;
    }

    /* 抬手结算: 只有轻点且没有位移才算放置, 长按只结束挖掘, 进度清零. */
    const double held = elapsedSeconds_ - editDownSeconds_;
    const float movedX = editLastX_ - editDownX_;
    const float movedY = editLastY_ - editDownY_;
    const float moved = std::sqrt(movedX * movedX + movedY * movedY);
    player_->CancelMining();
    if (held < player::mining::kTapSeconds && moved < kTapMoveLimit) {
        /* 轻点放置: 使用方块栏当前选中的材料, 成功放置才扣减数量. */
        if (TryPlaceSelectedBlock()) {
            PlayHarvestSound(selectedBlock_, true);
            /* 放置音按方块材质选择, 日志记下材质与音效编号, 便于实机核对. */
            IM_LOGI(kTag, "place sound material=%d place=%d",
                    static_cast<int32_t>(
                        voxel::BlockRegistry::Instance().Info(selectedBlock_).material),
                    static_cast<int32_t>(PlaceSoundOf(
                        voxel::BlockRegistry::Instance().Info(selectedBlock_).material)));
        }
    }
    editPointerId_ = -1;
}

bool GameShell::BeginEditMining() {
    if (!player_->BeginMining(*world_)) {
        return false;
    }
    const voxel::BlockId block = player_->MiningBlock();
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    /*
     * 起手日志记录硬度与所需秒数, 同一行里的 need 就是 kSecondsPerHardness 的换算结果,
     * 实测时与完成日志的 elapsed 对照即可确认进度确实按硬度累计.
     */
    IM_LOGI(kTag, "mine begin block=%s hardness=%.2f material=%d need=%.2fs dig=%d", info.name,
            static_cast<double>(info.hardness), static_cast<int32_t>(info.material),
            static_cast<double>(player_->MiningDuration()),
            static_cast<int32_t>(DigSoundOf(info.material)));
    return true;
}

void GameShell::AdvanceEditMining(double deltaSeconds) {
    const player::PlayerController::MineTick tick = player_->AdvanceMining(deltaSeconds, *world_);
    /* 进度推进期间按固定节拍播放挖掘音, 音色按方块材质族选择. */
    if (tick.digSound) {
        PlayHarvestSound(tick.block, false);
    }
    if (!tick.destroyed) {
        return;
    }
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(tick.block);
    IM_LOGI(kTag, "mine done block=%s hardness=%.2f need=%.2fs elapsed=%.2fs dig=%d", info.name,
            static_cast<double>(info.hardness), static_cast<double>(tick.duration),
            static_cast<double>(tick.elapsed), static_cast<int32_t>(DigSoundOf(info.material)));
    CollectDrop(tick.block);
    /* 破坏后手指仍按住时接着挖下一格, 与连续挖掘的原版手感一致. */
    BeginEditMining();
}

void GameShell::CollectDrop(voxel::BlockId brokenBlock) {
    const voxel::BlockId drop = static_cast<voxel::BlockId>(gen::DropBlockOf(brokenBlock));
    if (drop == voxel::block::Air) {
        IM_LOGI(kTag, "block has no drop, block=%u", static_cast<uint32_t>(brokenBlock));
        return;
    }
    const int32_t slot = inventory_.SlotOf(drop);
    const char* name = voxel::BlockRegistry::Instance().Info(drop).name;
    const char* sourceName = voxel::BlockRegistry::Instance().Info(brokenBlock).name;
    if (inventory_.Add(drop, 1)) {
        IM_LOGI(kTag, "drop collected from=%s drop=%s slot=%d count=%d", sourceName, name, slot,
                inventory_.Count(slot));
        return;
    }
    /* 槽位堆满或没有对应槽位时丢弃, 只累计数量并写警告, 不影响破坏本身. */
    IM_LOGW(kTag, "inventory full, drop discarded drop=%s slot=%d discarded=%d", name, slot,
            inventory_.Discarded());
}

bool GameShell::TryPlaceSelectedBlock() {
    if (inventory_.Empty(selectedSlot_)) {
        /* 数量为 0 时拒绝放置, 只给提示, 不改动世界与数量. */
        ShowTip(kTipEmptySlot);
        IM_LOGW(kTag, "place rejected, slot=%d block=%s count=0", selectedSlot_,
                ui::BlockSlotName(selectedSlot_));
        return false;
    }
    if (!player_->PlaceBlock(*world_, selectedBlock_)) {
        return false;
    }
    inventory_.Consume(selectedSlot_, 1);
    IM_LOGI(kTag, "block placed slot=%d block=%s remain=%d", selectedSlot_,
            ui::BlockSlotName(selectedSlot_), inventory_.Count(selectedSlot_));
    return true;
}

void GameShell::ShowTip(const char* text) {
    tipText_ = text;
    tipRemainSeconds_ = kTipSeconds;
}

void GameShell::UpdateTip(double deltaSeconds) {
    if (tipRemainSeconds_ <= 0.0) {
        tipText_ = nullptr;
        return;
    }
    tipRemainSeconds_ -= deltaSeconds;
    if (tipRemainSeconds_ <= 0.0) {
        tipRemainSeconds_ = 0.0;
        tipText_ = nullptr;
    }
}

void GameShell::UpdateNamePlate(double deltaSeconds) {
    if (nameRemainSeconds_ <= 0.0) {
        nameSlot_ = -1;
        return;
    }
    nameRemainSeconds_ -= deltaSeconds;
    if (nameRemainSeconds_ <= 0.0) {
        nameRemainSeconds_ = 0.0;
        nameSlot_ = -1;
    }
}

void GameShell::MaybeRequestAutoJump() {
    if (!settings_->AutoJump()) {
        return;
    }
    const platform::StickVisual stick = player_->GetStickVisual();
    if (!stick.active || stick.valueY < settings::tuning::kAutoJumpForwardThreshold) {
        return;
    }
    if (lastAutoJumpSeconds_ >= 0.0 &&
        elapsedSeconds_ - lastAutoJumpSeconds_ < settings::tuning::kAutoJumpIntervalSeconds) {
        return;
    }
    const core::Vec3 feet = player_->Position();
    /* 只有站在实心方块上才补跳跃, 悬空或落水中不注入请求. */
    if (!GroundBlockSolid(*world_, feet)) {
        return;
    }
    /* 只有一格高的台阶才跳跃: 两格以上跳不上去, 交给玩家自己搭方块. */
    if (!StepAheadIsOneBlock(*world_, feet, player_->Yaw())) {
        return;
    }
    lastAutoJumpSeconds_ = elapsedSeconds_;
    player_->RequestJump();
    IM_LOGD(kTag, "auto jump requested, feet=%.2f", static_cast<double>(feet.y));
}

void GameShell::SelectSlot(int32_t slot) {
    if (slot < 0 || slot >= ui::kBlockSlotCount || slot == selectedSlot_) {
        return;
    }
    selectedSlot_ = slot;
    selectedBlock_ = ui::BlockSlotId(slot);
    IM_LOGI(kTag, "block slot %d selected, name=%s id=%u", slot, ui::BlockSlotName(slot),
            static_cast<uint32_t>(selectedBlock_));
}

void GameShell::SyncViewDistance() {
    const int32_t wanted = settings_->ViewDistanceChunks();
    if (wanted == viewDistanceChunks_) {
        return;
    }
    viewDistanceChunks_ = wanted;
    IM_LOGI(kTag, "view distance changed to %d chunk columns", wanted);
}

void GameShell::SyncAudioSettings() {
    const int32_t volumePercent = settings_->Get(settings::Key::kVolume);
    const int32_t soundOpen = settings_->SoundEnabled() ? 1 : 0;
    const int32_t musicOpen = settings_->MusicEnabled() ? 1 : 0;
    if (volumePercent == audioVolumePercent_ && soundOpen == audioSoundOpen_ &&
        musicOpen == audioMusicOpen_) {
        return;
    }
    audioVolumePercent_ = volumePercent;
    audioSoundOpen_ = soundOpen;
    audioMusicOpen_ = musicOpen;

    auto& engine = audio::AudioEngine::Instance();
    engine.SetMasterVolume(settings_->MasterVolume());
    engine.SetSoundEnabled(soundOpen != 0);
    engine.SetMusicEnabled(musicOpen != 0);
    engine.SetVoiceEnabled(settings_->VoiceEnabled());
    IM_LOGI(kTag, "audio setting applied, volume=%d sound=%d music=%d voice=%d", volumePercent,
            soundOpen, musicOpen, settings_->VoiceEnabled() ? 1 : 0);
}

void GameShell::RenderScene() {
    if (!surfaceReady_ || !renderer_->Ready()) {
        return;
    }
    const float aspect = height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
    float projection[16];
    float view[16];
    float viewProjection[16];
    MakePerspective(projection, kFovYDegrees, aspect, kNearPlane, kFarPlane);
    player_->GetViewMatrix(view);
    Multiply4x4(projection, view, viewProjection);

    const core::Vec3 position = player_->Position();
    float camera[3] = {position.x, position.y + kEyeHeight, position.z};

    /*
     * 镜头摇晃. 原版由受击等事件触发, 现阶段没有伤害事件,
     * 先按移动强度做轻微摆动占位, 关闭设置时相机完全静止.
     */
    if (settings_->CameraShake()) {
        const platform::StickVisual motionSource = player_->GetStickVisual();
        const float motion = std::sqrt(motionSource.valueX * motionSource.valueX +
                                       motionSource.valueY * motionSource.valueY);
        if (motion > settings::tuning::kCameraShakeMotionFloor) {
            const float phase = static_cast<float>(elapsedSeconds_) * 6.2831853f *
                                settings::tuning::kCameraShakeFrequency;
            const float amplitude = settings::tuning::kCameraShakeAmplitude * motion;
            camera[0] += std::sin(phase) * amplitude;
            camera[1] += std::sin(phase * 2.0f) * amplitude * 0.5f;
        }
    }

    renderer_->SetViewProjection(viewProjection, camera);
    renderer_->Sync(*world_);
    renderer_->Draw();

    if (settings_->HideUi()) {
        return;
    }

    /*
     * HUD 的摇杆几何直接取控制器回报的视觉状态: 锚点, 摇杆量, 满量程半径三者在
     * 输入与界面之间只有一份来源, 界面不会再和真实输入错位.
     * 经典模式的底座常驻显示, 锚点固定在设置给定的底座中心.
     */
    ui::HudFrame frame;
    frame.selectedSlot = selectedSlot_;
    frame.stick = player_->GetStickVisual();
    frame.jumpPressed = jumpHeld_;
    frame.nameSlot = nameSlot_;
    /* 数量取自物品栏, 与方块栏共享槽位顺序; 提示只在有效期内显示. */
    for (int32_t slot = 0; slot < ui::kHudSlotCount; ++slot) {
        frame.slotCounts[slot] = inventory_.Count(slot);
    }
    frame.tipText = tipText_;
    /*
     * 破坏进度只在按住超过轻点窗口后显示: 轻点放置时进度虽然累计了几帧,
     * 但不会闪出一个进度环; 进度归零时界面层不绘制.
     */
    if (editPointerId_ >= 0 && player_->Mining() &&
        elapsedSeconds_ - editDownSeconds_ >= player::mining::kTapSeconds) {
        frame.mineProgress = player_->MiningProgress();
    }
    if (settings_->ClassicalMoveMode()) {
        frame.stick.active = true;
        frame.stick.anchorX = settings::tuning::kPadCenterX;
        frame.stick.anchorY = settings::tuning::kPadCenterY;
        frame.stick.radius = settings::tuning::kPadRadius;
    }
    hud_.Render(frame);
}

void GameShell::LogStatus(double deltaSeconds) {
    logAccumulator_ += deltaSeconds;
    if (logAccumulator_ < 2.0) {
        return;
    }
    logAccumulator_ = 0.0;
    const core::Vec3 position = player_->Position();
    const platform::StickVisual stick = player_->GetStickVisual();
    /*
     * 状态行同时输出设置生效结果, 用于实机验证:
     * view_distance 与区块数对应, look 为灵敏度换算出的横向满屏转角,
     * yaw 与 pitch 便于用同样手势对比不同灵敏度下的转角,
     * stick 与指针数为多点触控的自检依据.
     */
    IM_LOGI(kTag,
            "world chunks=%d visible=%d drawn=%d triangles=%d pos=%.2f,%.2f,%.2f yaw=%.1f pitch=%.1f "
            "view_distance=%d look=%.1f vol=%d autojump=%d mode=%s pointers=%d stick=%.2f,%.2f",
            world_->LoadedChunkCount(), static_cast<int>(world_->VisibleChunks().size()),
            renderer_->DrawnChunks(), renderer_->DrawnTriangles(), position.x, position.y, position.z,
            player_->Yaw() * 57.29578f, player_->Pitch() * 57.29578f, viewDistanceChunks_,
            settings_->LookDegreesPerScreenWidth(), static_cast<int>(settings_->Get(settings::Key::kVolume)),
            settings_->AutoJump() ? 1 : 0,
            settings_->ClassicalMoveMode() ? "classical" : "rocker",
            lastInput_.pointerCount, static_cast<double>(stick.valueX),
            static_cast<double>(stick.valueY));
}

}  /* namespace immirror::game */
