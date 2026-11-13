#include "game/game_shell.h"

#include <dirent.h>
#include <sys/stat.h>

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "core/log.h"
#include "core/vec3.h"
#include "entity/entity_tuning.h"
#include "game/crafting/recipe_matcher.h"
#include "game/generated/block_data_gen.h"
#include "game/generated/food_data_gen.h"
#include "game/containers.h"
#include "game/furnace.h"
#include "game/inventory.h"
#include "game/item/inventory_screen.h"
#include "game/item/item_table.h"
#include "player/input_tuning.h"
#include "player/mining_tuning.h"
#include "player/player_controller.h"
#include "player/survival_tuning.h"
#include "platform/input_state.h"
#include "progress/progress_hub.h"
#include "render/chunk_renderer.h"
#include "render/creature_visual.h"
#include "render/particle_system.h"
#include "settings/settings.h"
#include "settings/settings_tuning.h"
#include "ui/hud_renderer.h"
#include "ui/hud_slots.h"
#include "audio/audio_engine.h"
#include "entity/generated/entity_sound_gen.h"
#include "voxel/block_registry.h"
#include "voxel/generated/biome_data_gen.h"
#include "voxel/terrain_generator.h"
#include "world/world.h"

namespace immirror::game {
namespace {

constexpr const char* kTag = "ImMirrorGame";


/*
 * 相机参数. 视场角取四套预设共用的 75 度 (camera.lua 第 89 行),
 * 近远裁剪面未给出数值, 维持现值.
 */
constexpr float kFovYDegrees = 75.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 320.0f;
constexpr float kEyeHeight = 1.62f;

/*
 * 容器方块判定. 方块注册表没有类别字段, 用英文键反查:
 * 熔炉族取类别 furnace 的四个键 (熔炉与三个壁炉, blockdef 同类别),
 * 箱子族取类别 chest 的代表键 "box" (其余柜子等后续接入时再扩表).
 */
const char* const kFurnaceBlockKeys[] = {"smeltingfurnace", "brownfireplace",
                                         "whitefireplace", "blackfireplace"};
const char* const kChestBlockKeys[] = {"box"};

/* 方块 id 是否属于熔炉族. */
bool IsFurnaceBlock(voxel::BlockId block) {
    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    for (const char* key : kFurnaceBlockKeys) {
        if (registry.FindByName(key) == block) {
            return true;
        }
    }
    return false;
}

/* 方块 id 是否属于箱子族. */
bool IsChestBlock(voxel::BlockId block) {
    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    for (const char* key : kChestBlockKeys) {
        if (registry.FindByName(key) == block) {
            return true;
        }
    }
    return false;
}

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
 * 物品栏的快捷栏与底部方块栏共享同一张槽位表, 生成表与方块注册表条目等长;
 * 两处不一致时在编译期报错, 避免界面图标、物品栏与数据表三处错位.
 */
static_assert(Inventory::kHotbarSlots == ui::kHudSlotCount, "物品栏快捷栏格数与方块栏格数不一致");
static_assert(gen::kBlockDataCount == static_cast<int32_t>(voxel::block::Count),
              "方块数据表条目数与方块注册表不一致, 请重新运行 scripts/gen_block_data.py");

/* 世界种子, 固定值便于复现地形. */
constexpr uint32_t kWorldSeed = 20260101u;

/* 自动进食的饥饿阈值: 低于三成时吃, 留出反应余量. */
constexpr float kAutoEatHungerThreshold = 0.3f;
/* 两次自动进食的最小间隔, 避免一口接一口把背包吃空. */
constexpr float kAutoEatCooldownSeconds = 1.5f;

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
        case voxel::HarvestMaterial::kCloth:
            return audio::SoundId::kDigCloth;
        case voxel::HarvestMaterial::kGlass:
            return audio::SoundId::kDigGlass;
        case voxel::HarvestMaterial::kJar:
            return audio::SoundId::kDigJar;
        /* 阶梯没有独立挖掘音, 挖掘按木族出声. */
        case voxel::HarvestMaterial::kLadder:
            return audio::SoundId::kDigWood;
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
        /*
         * 放置音没有布与玻璃的独立族, APK 内也没有 place_cloth/place_glass
         * 音源: 布按挖掘音族出声, 玻璃按石族出声; 罐与阶梯按放置时的材质近亲
         * 折算 (罐归木, 阶梯归木).
         */
        case voxel::HarvestMaterial::kCloth:
            return audio::SoundId::kPlaceCloth;
        case voxel::HarvestMaterial::kGlass:
            return audio::SoundId::kPlaceStone;
        case voxel::HarvestMaterial::kJar:
            return audio::SoundId::kPlaceWood;
        case voxel::HarvestMaterial::kLadder:
            return audio::SoundId::kPlaceWood;
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
  worldTime_(std::make_unique<world::WorldTime>()),
      player_(std::make_unique<player::PlayerController>()),
      renderer_(std::make_unique<render::ChunkRenderer>()) {
    /* 设置单例在构造时已填好默认值, 因此未初始化存储也能取到视距. */
    settings_ = &settings::Settings::Instance();
    viewDistanceChunks_ = settings_->ViewDistanceChunks();
    selectedBlock_ = ui::BlockSlotId(selectedSlot_);

    /*
     * 开局物品: 快捷栏按方块栏共享表每种方块一叠, 背包里再放几样常用材料,
     * 打开背包就能直接试一次合成. 物品表与配方表在首次访问时构造,
     * 这里顺带跑一遍匹配自检, 无序与有序两条路径都在启动时验证一次.
     */
    SeedStartingItems();
    if (!crafting::RecipeMatcher::RunSelfCheck()) {
        IM_LOGE(kTag, "crafting self check failed");
    }
}

/*
 * 自动进食: 饥饿低于阈值时从快捷栏与背包里找一样可食用物品吃掉.
 *
 * 当前没有独立的"使用物品"交互, 这条路径让食物与附加状态先能跑通;
 * 后续接入背包界面的使用按钮后, 这里可以退化为兜底.
 */
uint32_t GameShell::nextRandom() {
    eatRandom_ ^= eatRandom_ << 13;
    eatRandom_ ^= eatRandom_ >> 17;
    eatRandom_ ^= eatRandom_ << 5;
    return eatRandom_;
}

bool GameShell::TryAutoEat() {
    if (player_->HungerRatio() > kAutoEatHungerThreshold || !player_->Alive()) {
        return false;
    }
    for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
        const item::ItemStack stack = inventory_.Slot(slot);
        if (stack.Empty()) {
            continue;
        }
        const gen::FoodEntry* food = gen::FoodOfItem(static_cast<uint16_t>(stack.item));
        if (food == nullptr) {
            continue;
        }
        player_->Eat(static_cast<float>(food->food), static_cast<float>(food->heal));
        /* 附加状态按万分比逐条判定. */
        const uint32_t pairs[3][2] = {{food->buffId1, food->buffOdds1},
                                      {food->buffId2, food->buffOdds2},
                                      {food->buffId3, food->buffOdds3}};
        for (const auto& pair : pairs) {
            if (pair[0] == 0 || pair[1] == 0) {
                continue;
            }
            const uint32_t roll = nextRandom() % 10000u;
            if (roll < pair[1]) {
                player_->ApplyBuff(pair[0]);
                IM_LOGI(kTag, "food buff applied id=%u", pair[0]);
            }
        }
        inventory_.Take(slot, 1);
        IM_LOGI(kTag, "auto eat item=%d slot=%d", static_cast<int32_t>(stack.item), slot);
        return true;
    }
    return false;
}

void GameShell::SeedStartingItems() {
    inventory_.Reset();
    const item::ItemTable& items = item::ItemTable::Instance();
    /*
     * 起步工具先进快捷栏: 第一格放石铲, 第二格放木铲.
     * 铲子对应土与沙, 新建世界的第一件事就是挖土, 拿在手上能直接感受到效率差异.
     */
    const char* startingTools[] = {"StonePickaxe", "StoneShovel"};
    for (const char* key : startingTools) {
        const item::ItemId tool = items.FindByKey(key);
        if (tool != item::kNone) {
            inventory_.Add(tool, 1);
        }
    }
    for (int32_t slot = 0; slot < Inventory::kHotbarSlots; ++slot) {
        const item::ItemId id = items.OfBlock(ui::BlockSlotId(slot));
        if (id != item::kNone) {
            inventory_.Add(id, Inventory::kMaxStack);
        }
    }
    /* 背包起步材料: 果木与木板可以直接做工具, 木棍是多数工具的第二材料. */
    inventory_.Add(items.OfBlock(voxel::block::Planks), 32);
    inventory_.Add(items.OfBlock(voxel::block::Wood), 16);
    /*
     * 建筑构件: 台阶, 楼梯, 栅栏, 玻璃片, 地毯各带一份,
     * 新建世界后可以直接验证这几类非整块形状的摆放与外观.
     */
    const voxel::BlockId buildingParts[] = {396u, 421u, 459u, 527u};
    for (const voxel::BlockId part : buildingParts) {
        const item::ItemId partItem = items.OfBlock(part);
        if (partItem != item::kNone) {
            inventory_.Add(partItem, 16);
        }
    }
    /*
     * 起步食物: 带一份土豆与一份方南瓜, 饥饿见底时自动进食这条路径才跑得起来.
     * 编号取自食物表, 与物品表按原版物品 ID 对齐.
     */
    const item::ItemId startingFood[] = {static_cast<item::ItemId>(733),
                                         static_cast<item::ItemId>(270)};
    for (const item::ItemId food : startingFood) {
        if (food != item::kNone && items.Info(food).key[0] != '\0') {
            inventory_.Add(food, 3);
        }
    }
    const item::ItemId stick = items.FindByKey("WoodBar");
    if (stick != item::kNone) {
        inventory_.Add(stick, 16);
    }
    /* 起步工具的耐久应当已按物品表填满, 这里逐个打印便于核对. */
    for (int32_t slot = 0; slot < Inventory::kHotbarSlots; ++slot) {
        const int32_t durability = inventory_.DurabilityAt(slot);
        if (durability > 0) {
            IM_LOGI(kTag, "starting tool slot=%d item=%d durability=%d", slot,
                    static_cast<int32_t>(inventory_.ItemAt(slot)), durability);
        }
    }
    IM_LOGI(kTag, "starting items seeded, used=%d slots discarded=%d", inventory_.UsedSlots(),
            inventory_.Discarded());
}

void GameShell::RefreshSelectedItem() {
    const item::ItemId id = inventory_.ItemAt(selectedSlot_);
    selectedBlock_ = item::ItemTable::Instance().BlockOf(id);
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
    renderer_->SetViewportSize(width_, height_);
    /* 粒子通道与区块渲染共用同一份图形上下文, 在这里一并初始化. */
    if (!render::particle::Initialize()) {
        IM_LOGW(kTag, "particle system unavailable, block debris disabled");
    }
    surfaceReady_ = true;
    /* 表面重建后旧的指针快照已经失效, 复位输入状态, 不把退出前的按住带进新画面. */
    ResetInputState();
    PrepareSavePath(dataPath);
    /* 大厅可能在本函数之前就下发了世界名, 这里补上切换, 让随后的读档走对文件. */
    if (!pendingWorldName_.empty()) {
        const std::string pending = pendingWorldName_;
        const uint32_t pendingSeed = pendingWorldSeed_;
        pendingWorldName_.clear();
        pendingWorldSeed_ = 0;
        SelectWorld(pending, pendingSeed);
    }

    /*
     * 设置文件与存档放在同一个内部存储根目录下. Java 侧可能已经初始化过,
     * 这里重复调用是幂等的; 尚未初始化时由平台给出的目录补上, 启动即完成加载.
     */
    settings_->Initialize(dataPath);
    SyncViewDistance();
    SyncAudioSettings();
    SyncVisualSettings();
    ApplyControlSettings();

    world::World::SaveState state;
    if (!savePath_.empty() && world_->LoadFromFile(savePath_.c_str(), state)) {
        player_->SetPose(core::Vec3{state.playerX, state.playerY, state.playerZ}, state.yaw, state.pitch);
        /* 容器文件与世界存档配套: 旧世界档没有容器文件时按空容器处理. */
        if (!containerPath_.empty()) {
            LoadContainersFromFile(containerPath_.c_str(), furnaces_, chests_);
        }
        /* 世界时间: v3 早期档没有时间字段, 读出 0 按 0 点处理, 新档带时间. */
        worldTime_->SetTotalSeconds(state.timeOfDaySeconds);
        /* 读档后产出基线随存档重建, 从当前状态重新采样, 不把读档前的差值算进去. */
        smeltBaseline_ = -1;
        IM_LOGI(kTag, "world loaded from save, chunks=%d", world_->LoadedChunkCount());
    } else {
        /* 先让中心区块生成地形, 再按地表高度放置玩家, 否则出生点会落在虚空. */
        world_->Update(core::Vec3{8.0f, 0.0f, 8.0f}, viewDistanceChunks_,
                       static_cast<uint8_t>(treeShape_ < 0 ? 1 : treeShape_));
        player_->Spawn(*world_, 8.0f, 8.0f);
    }
    world_->Update(player_->Position(), viewDistanceChunks_,
                   static_cast<uint8_t>(treeShape_ < 0 ? 1 : treeShape_));
    /*
     * 生物不进存档: 每次进入世界都从生物表重新播种生成,
     * 因此读档与新建世界走同一条复位路径.
     */
    entities_.Reset(kWorldSeed);
    SyncEntitySpawnExtent();
    hud_.SetViewport(width_, height_);
    /*
     * 界面图集独立于方块图集上传, 缺失时返回 0,
     * HUD 内部对 0 有回落处理, 界面不会因为图集缺失而残缺.
     */
    const uint32_t uiAtlasTexture = ui::LoadUiAtlasFromAssets(assets);
    hud_.Initialize(renderer_->AtlasTexture(), uiAtlasTexture);
    /* 生物形象程序与几何在上下文就绪后初始化, 失败只记日志, 生物退回无形象. */
    render::creature::Initialize();
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
    bagPointerId_ = -1;
    bagHeld_ = false;
    /* 表面重建会丢弃旧指针, 挖掘进度与攻击手势一并作废, 不带进新画面. */
    player_->CancelMining();
    attackHeld_ = false;
    editAttacked_ = false;
    attackAccumulator_ = 0.0;
    containerProbeBlock_ = voxel::block::Air;
    containerProbeOpened_ = false;
}

void GameShell::PrepareSavePath(const char* dataPath) {
    if (dataPath == nullptr || dataPath[0] == '\0') {
        savePath_.clear();
        containerPath_.clear();
        return;
    }
    std::string directory = std::string(dataPath) + "/worlds";
    ::mkdir(directory.c_str(), 0700);
    savePath_ = directory + "/default.world";
    SyncContainerPath();
}

/* 容器文件与世界存档同名不同后缀, 两文件始终配套. */
void GameShell::SyncContainerPath() {
    if (savePath_.size() > 6 && savePath_.compare(savePath_.size() - 6, 6, ".world") == 0) {
        containerPath_ = savePath_.substr(0, savePath_.size() - 6) + ".containers";
    } else {
        containerPath_ = savePath_ + ".containers";
    }
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
    /*
     * 世界元数据: 名字取当前存档文件名去后缀 (选择世界时已由文件名确定),
     * 空名回落 "default"; 时间取当前 Unix 秒.
     */
    const size_t nameBegin = savePath_.find_last_of("/\\");
    std::string fileName = nameBegin == std::string::npos ? savePath_ : savePath_.substr(nameBegin + 1);
    if (fileName.size() > 6 && fileName.compare(fileName.size() - 6, 6, ".world") == 0) {
        fileName = fileName.substr(0, fileName.size() - 6);
    }
    /* 存档头里的世界名不带扩展名: 去掉 .world 后缀, 列表展示与再次进入都用干净名字. */
    if (fileName.size() > 6 && fileName.compare(fileName.size() - 6, 6, ".world") == 0) {
        fileName.resize(fileName.size() - 6);
    }
    state.worldName = fileName.empty() ? "default" : fileName;
    state.lastPlayed = static_cast<uint64_t>(std::time(nullptr));
    state.timeOfDaySeconds = worldTime_->TotalSeconds();
    const bool saved = world_->SaveToFile(savePath_.c_str(), state);
    /* 容器状态存独立文件: 世界存档格式归 world 模块, 本期不改动它的版本号. */
    bool containersSaved = false;
    if (!containerPath_.empty()) {
        containersSaved = SaveContainersToFile(containerPath_.c_str(), furnaces_, chests_);
    }
    IM_LOGI(kTag, "world save %s, containers %s, path=%s", saved ? "ok" : "failed",
            containersSaved ? "ok" : "skipped", savePath_.c_str());
}

void GameShell::OnSurfaceLost() {
    if (!surfaceReady_) {
        return;
    }
    /* 表面销毁前释放粒子通道的图形资源, 与区块渲染器同一时机. */
    render::particle::Shutdown();
    SaveWorld();
    /* 面板关闭并把合成网格与携带中的堆叠退回物品栏, 避免退出时丢材料. */
    inventoryScreen_.ReturnAllToInventory(inventory_);
    inventoryScreen_.Reset();
    /* 容器面板同样退回输入格与燃料格, 再统一落盘一次容器状态. */
    CloseContainerPanels();
    /* 生物形象程序先于 HUD 释放, 同一上下文内逆序清理. */
    render::creature::Shutdown();
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
    /* 天空渐变按视口高度归一, 渲染器尚未创建时由 OnSurfaceReady 补一次. */
    if (renderer_) {
        renderer_->SetViewportSize(width, height);
    }
}

void GameShell::Update(double deltaSeconds, const platform::InputSnapshot& input) {
    if (!surfaceReady_) {
        return;
    }
    elapsedSeconds_ += deltaSeconds;
    /* 粒子按帧推进, 与玩家和世界的推进同一时间片. */
    render::particle::Update(deltaSeconds);
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
    SyncVisualSettings();
    ApplyControlSettings();

    /* 世界时间先行推进, 光照与环境音都取本帧的昼夜状态. */
    worldTime_->Tick(static_cast<float>(deltaSeconds));

    /*
     * 天气参数按玩家脚下所在的群系带取: 雪原允许下雪, 沙漠只允许雨,
     * 三档权重直接来自biomedef; 天气本身按现实秒数重抽.
     */
    {
        const voxel::TerrainGenerator generator;
        const core::Vec3 position = player_->Position();
        const int32_t bandIndex = generator.BiomeAt(world_->Seed(), static_cast<int32_t>(position.x),
                                                   static_cast<int32_t>(position.z));
        const voxel::BiomeBand& band = voxel::kBiomeBands[bandIndex];
        weather_.Configure(band.canRain != 0, band.canSnow != 0, band.clearWeight, band.rainWeight,
                           band.thunderWeight);
        weather_.Tick(static_cast<float>(deltaSeconds));
    }
    /*
     * 天光混合因子按小时数取目标值后做 10 秒级平滑, 避免晨昏切换跳变;
     * 渲染侧按 mix(0.27, 1.0, factor) 折算天空亮度, 方块光不受影响.
     */
    const float skyTarget = worldTime_->SkyFactor();
    const float skyBlendRate = static_cast<float>(deltaSeconds) / 10.0f;
    if (skyBlend_ < skyTarget) {
        skyBlend_ = skyBlend_ + skyBlendRate < skyTarget ? skyBlend_ + skyBlendRate : skyTarget;
    } else if (skyBlend_ > skyTarget) {
        skyBlend_ = skyBlend_ - skyBlendRate > skyTarget ? skyBlend_ - skyBlendRate : skyTarget;
    }
    const core::Vec3 position = player_->Position();
    world_->Update(position, viewDistanceChunks_,
                   static_cast<uint8_t>(treeShape_ < 0 ? 1 : treeShape_));

    /*
     * 顺序固定: 先让界面认领指针, 认领过的指针不再参与摇杆与视角分配.
     * 背包面板打开时独占全部指针, 移动, 挖掘与放置整体暂停;
     * 面板关闭时按原顺序推进玩家与编辑手势. 指针位图在帧末更新.
     */
    if (inventoryScreen_.Open()) {
        if (player_->Mining()) {
            player_->CancelMining();
        }
        UpdateInventoryPanel(lastInput_);
    } else if (furnaceScreen_.Open() || chestScreen_.Open()) {
        /* 容器面板与背包面板同一互斥语义: 打开期间独占全部指针. */
        if (player_->Mining()) {
            player_->CancelMining();
        }
        UpdateContainerPanel(lastInput_);
    } else {
        UpdateHudButtons(lastInput_, deltaSeconds);
        MaybeRequestAutoJump();
        /* 饥饿过低时自动进食, 与移动判定同一分支: 面板打开期间不吃. */
        if (eatCooldown_ <= 0.0f && TryAutoEat()) {
            eatCooldown_ = kAutoEatCooldownSeconds;
        }
        if (eatCooldown_ > 0.0f) {
            eatCooldown_ -= static_cast<float>(deltaSeconds);
        }
        player_->Update(deltaSeconds, lastInput_, *world_);
        UpdateEditGesture(lastInput_, deltaSeconds);
    }

    /* 熔炉在世界推进后统一推进, 与是否打开面板无关. */
    const float dtSeconds = static_cast<float>(deltaSeconds);
    for (auto& entry : furnaces_) {
        entry.second.Tick(dtSeconds);
    }
    /*
     * 熔炼事件按产出堆叠总量差值折算: 每完成一件产出 +1, 收取只是转移不重复计.
     * 数量为正的产出格合计与上帧的差值就是本帧新完成的件数, 读档时基线随存档重建.
     */
    int64_t smeltTotal = 0;
    for (const auto& entry : furnaces_) {
        smeltTotal += entry.second.output.count > 0 ? entry.second.output.count : 0;
    }
    if (smeltBaseline_ >= 0 && smeltTotal > smeltBaseline_) {
        const int64_t smelted = smeltTotal - smeltBaseline_;
        progressCounters_.smelted += smelted;
        progress::ProgressHub::Instance().OnSmelt(static_cast<int32_t>(smelted));
    }
    smeltBaseline_ = smeltTotal;
    /* 合成事件: 面板状态机记录的完成件数取走即清零, 并入上报增量与本地成就进度. */
    const int64_t crafted = inventoryScreen_.TakeCraftEventCount();
    if (crafted > 0) {
        progressCounters_.crafts += crafted;
        progress::ProgressHub::Instance().OnCraft(static_cast<int32_t>(crafted));
    }
    RefreshSelectedItem();
    /* 生物在玩家之后推进: 位置与朝向都取本帧最新的玩家状态. */
    UpdateEntities(deltaSeconds);
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

    /* 脚步声: 在地面移动时按脚下方块材质周期播放行走音. */
    if (player_->WalkingOnGround()) {
        walkSoundTimer_ += deltaSeconds;
        if (walkSoundTimer_ >= 0.35) {
            walkSoundTimer_ = 0.0;
            audio::AudioEngine::Instance().Play(WalkSoundForBlock(player_->GroundBlockId()));
        }
    } else {
        walkSoundTimer_ = 0.0;
    }
}

void GameShell::ApplyControlSettings() {
    /*
     * 灵敏度以滑杆中位 50 为基准折算: LookFactor 在 50 处为 1,
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

    /*
     * 视角键 view: 1 主视角 2 背视角 3 正视角 (setting.lua 1666 至 1724 行),
     * 档位直接映射相机模式, 设置变化后下一帧生效, 不需重启.
     */
    const int32_t viewSetting = settings_->Get(settings::Key::kView);
    player::CameraMode mode = player::CameraMode::kFirstPerson;
    if (viewSetting == 2) {
        mode = player::CameraMode::kThirdBack;
    } else if (viewSetting == 3) {
        mode = player::CameraMode::kThirdFront;
    }
    if (mode != player_->GetCameraMode()) {
        player_->SetCameraMode(mode);
    }
}

/*
 * 新世界的种子: 秒级时间戳与世界名哈希混合, 保证同时新建的两个世界也互不相同.
 * 返回 0 会让地形噪声退化成纯常量, 因此 0 一律换成 1.
 */
uint32_t NewWorldSeed(const std::string& name) {
    const uint64_t stamp = static_cast<uint64_t>(std::time(nullptr));
    uint64_t hash = 1469598103934665603ull;
    for (const char c : name) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ull;
    }
    const uint32_t seed = static_cast<uint32_t>((stamp * 2654435761ull) ^ hash);
    return seed == 0u ? 1u : seed;
}

void GameShell::LoadNamedWorld(const std::string& projectName) {
    /* 编辑器导出的存档与世界存档同目录, 文件名为安全化的工程名. */
    if (savePath_.empty()) {
        return;
    }
    const size_t slash = savePath_.find_last_of("/\\");
    const std::string directory = slash == std::string::npos ? savePath_ : savePath_.substr(0, slash);
    std::string safe;
    for (const char c : projectName) {
        safe += (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
    }
    const std::string path = directory + "/" + safe + ".world";
    world::World::SaveState state;
    if (world_->LoadFromFile(path.c_str(), state)) {
        player_->SetPose(core::Vec3{state.playerX, state.playerY, state.playerZ}, state.yaw, state.pitch);
        IM_LOGI(kTag, "playtest world loaded, project=%s path=%s", projectName.c_str(), path.c_str());
    } else {
        IM_LOGW(kTag, "playtest world missing, project=%s path=%s", projectName.c_str(), path.c_str());
    }
}

/*
 * 多世界选择: 把存档路径切换到 worlds 目录下指定名字的文件并立即载入.
 * 名字安全化与世界名回落规则与 LoadNamedWorld 一致; 文件不存在时只切路径,
 * 容器文件同步切换, 后续进入流程由 LoadFromFile 失败自然走进地形生成.
 */
bool GameShell::SelectWorld(const std::string& name, uint32_t seed) {
    if (savePath_.empty()) {
        /*
         * 表面尚未就绪时存档路径还没建立: 记下世界名, 等 OnSurfaceReady
         * 建好路径后重新调用本函数, 否则这次选择会被静默丢弃.
         */
        pendingWorldName_ = name;
        pendingWorldSeed_ = seed;
        IM_LOGI(kTag, "world selection deferred until surface ready, name=%s", name.c_str());
        return false;
    }
    const size_t slash = savePath_.find_last_of("/\\");
    if (slash == std::string::npos) {
        return false;
    }
    const std::string directory = savePath_.substr(0, slash);
    const std::string safe = world::World::SafeFileName(name);
    savePath_ = directory + "/" + safe + ".world";
    SyncContainerPath();

    world::World::SaveState state;
    if (world_->LoadFromFile(savePath_.c_str(), state)) {
        player_->SetPose(core::Vec3{state.playerX, state.playerY, state.playerZ}, state.yaw,
                         state.pitch);
        if (!containerPath_.empty()) {
            LoadContainersFromFile(containerPath_.c_str(), furnaces_, chests_);
        }
        worldTime_->SetTotalSeconds(state.timeOfDaySeconds);
        /* 换世界读档后基线同样重采样. */
        smeltBaseline_ = -1;
        IM_LOGI(kTag, "world selected, name=%s file=%s", safe.c_str(), savePath_.c_str());
        return true;
    }
    /* 新世界: 关掉面板并清空容器, 时间回落正午, 世界本体由生成流程补齐. */
    CloseContainerPanels();
    furnaces_.clear();
    chests_.clear();
    worldTime_->SetTotalSeconds(world::WorldTime::kDayLengthSeconds * 0.5f);
    /*
     * 每个新世界分配独立种子: 沿用上一次的种子会让所有新世界地形完全相同.
     * 调用方给出种子时直接采用, 联机房间据此让各方生成同一份地形.
     */
    world_->SetSeed(seed != 0u ? seed : NewWorldSeed(safe));
    /* 地形换了种子, 原来的脚点已经没有意义, 按原点重新落到地表. */
    player_->Spawn(*world_, 0.0f, 0.0f);
    smeltBaseline_ = -1;
    IM_LOGI(kTag, "world created, name=%s file=%s seed=%u", safe.c_str(), savePath_.c_str(),
            static_cast<unsigned int>(world_->Seed()));
    return false;
}

/*
 * 世界列表: 扫描目录下全部 .world 文件, 逐个读头取元数据.
 * 名字取存档头里的世界名, 旧档 (版本 1) 空名回落文件名.
 * 输出 JSON 数组, 字段顺序固定, 由 Java 侧解析展示.
 */
std::string GameShell::ListWorlds(const std::string& directory) {
    /* 实现下放到世界模块的静态函数: 列表只读文件头, 不需要游戏运行时. */
    return world::ListWorldsJson(directory.c_str());
}

bool GameShell::IsDay() const {
    return worldTime_->IsDay();
}

int32_t GameShell::AmbientContext() const {
    if (world_ == nullptr || player_ == nullptr) {
        return 0;
    }
    /*
     * 相机所在方块优先: 泡在水里或贴着岩浆时环境音必须是水体或岩浆,
     * 此时天气声会让玩家以为在下雨. 否则按天气给雨声或雷声.
     */
    const core::Vec3 camera = player_->CameraPosition();
    const voxel::BlockId inside = world_->GetBlock(static_cast<int32_t>(std::floor(camera.x)),
                                                  static_cast<int32_t>(std::floor(camera.y)),
                                                  static_cast<int32_t>(std::floor(camera.z)));
    const char* name = voxel::BlockRegistry::Instance().Info(inside).name;
    if (name != nullptr) {
        if (std::strstr(name, "lava") != nullptr) {
            return 4;
        }
        if (std::strstr(name, "water") != nullptr || std::strstr(name, "_still") != nullptr) {
            return 3;
        }
    }
    switch (weather_.Kind()) {
        case world::WeatherKind::kRain:
        case world::WeatherKind::kSnow:
            return 1;
        case world::WeatherKind::kThunder:
            return 2;
        case world::WeatherKind::kClear:
        default:
            return 0;
    }
}

std::string GameShell::FormatPlayerPose() const {
    const core::Vec3 position = player_->Position();
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                  R"({"x":%.2f,"y":%.2f,"z":%.2f,"yaw":%.4f,"pitch":%.4f})",
                  static_cast<double>(position.x), static_cast<double>(position.y),
                  static_cast<double>(position.z),
                  static_cast<double>(player_->Yaw()), static_cast<double>(player_->Pitch()));
    return buffer;
}

std::string GameShell::TakeBlockEditsJson() {
    const auto edits = player_->TakeBlockEdits();
    if (edits.empty()) {
        return "[]";
    }
    std::string json = "[";
    for (size_t index = 0; index < edits.size(); ++index) {
        if (index > 0) {
            json += ",";
        }
        char buffer[160];
        std::snprintf(buffer, sizeof(buffer), R"({"x":%d,"y":%d,"z":%d,"block":%u})", edits[index].x,
                      edits[index].y, edits[index].z, static_cast<unsigned>(edits[index].block));
        json += buffer;
    }
    json += "]";
    return json;
}

void GameShell::ApplyRemoteBlockEdits(const std::string& json) {
    /*
     * 简化解析: 手写扫描 JSON 数组, 每条形如 {"x":1,"y":2,"z":3,"block":4}.
     * 远端改动只写世界, 不进本地改动队列, 否则会来回反弹.
     */
    size_t cursor = 0;
    int32_t applied = 0;
    while (true) {
        const size_t open = json.find('{', cursor);
        if (open == std::string::npos) {
            break;
        }
        const size_t close = json.find('}', open);
        if (close == std::string::npos) {
            break;
        }
        const std::string obj = json.substr(open, close - open + 1);
        cursor = close + 1;
        const auto readInt = [&obj](const char* key, int32_t fallback) -> int32_t {
            const size_t pos = obj.find(key);
            if (pos == std::string::npos) {
                return fallback;
            }
            return static_cast<int32_t>(std::strtol(obj.c_str() + pos + std::strlen(key), nullptr, 10));
        };
        const int32_t x = readInt(R"(x":)", INT32_MIN);
        const int32_t y = readInt(R"(y":)", INT32_MIN);
        const int32_t z = readInt(R"(z":)", INT32_MIN);
        const int32_t block = readInt(R"(block":)", -1);
        if (x == INT32_MIN || y == INT32_MIN || z == INT32_MIN || block < 0) {
            continue;
        }
        if (world_->SetBlock(x, y, z, static_cast<voxel::BlockId>(block))) {
            ++applied;
            /* 远端改动同样触发沉降与流动: 别人挖掉沙下或水边的方块时也要有反馈. */
            SettleGravityAt(x, y, z);
            SettleFluidAt(x, y, z);
        }
    }
    if (applied > 0) {
        IM_LOGD(kTag, "remote block edits applied=%d", applied);
    }
}

/*
 * 生物快照导出.
 * 只导出活体: 已被标记移除的不再上报. 字段与房间通道的 creatures 约定一致,
 * kind 取生物表定义索引, 客人侧据此还原同一套体素形象.
 */
std::string GameShell::TakeCreatureSnapshot() const {
    std::string out = "[";
    bool first = true;
    const entity::MonsterTable& table = entity::MonsterTable::Instance();
    for (const auto& creature : entities_.Creatures()) {
        if (creature == nullptr || creature->Removed()) {
            continue;
        }
        const core::Vec3 pos = creature->Position();
        char buffer[192];
        std::snprintf(buffer, sizeof(buffer),
                      "%s{\"id\":%u,\"kind\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,"
                      "\"yaw\":%.3f,\"hp\":%.1f}",
                      first ? "" : ",", static_cast<unsigned>(creature->Id()),
                      table.IndexOfSourceId(creature->Definition().sourceId),
                      static_cast<double>(pos.x), static_cast<double>(pos.y),
                      static_cast<double>(pos.z), static_cast<double>(creature->Yaw()),
                      static_cast<double>(creature->Health()));
        out += buffer;
        first = false;
    }
    out += "]";
    return out;
}

/*
 * 远端生物快照.
 * 手写扫描与远端玩家同一套做法: 逐个对象取字段, 字段缺失按零处理;
 * 解析失败时清空队列, 避免残留上一轮的形象. 死亡与受击不做插值,
 * 由下发的 hp 比例折算一点提亮, 让客人能看出对方在打哪只.
 */
void GameShell::ApplyRemoteCreatures(const std::string& json) {
    remoteCreatures_.clear();
    const size_t begin = json.find('[');
    if (begin == std::string::npos) {
        return;
    }
    size_t cursor = begin + 1;
    while (remoteCreatures_.size() < 64) {
        const size_t open = json.find('{', cursor);
        if (open == std::string::npos) {
            break;
        }
        const size_t close = json.find('}', open);
        if (close == std::string::npos) {
            break;
        }
        const std::string obj = json.substr(open, close - open + 1);
        cursor = close + 1;
        const auto readNumber = [&obj](const char* key, float fallback) -> float {
            const size_t pos = obj.find(key);
            if (pos == std::string::npos) {
                return fallback;
            }
            return std::strtof(obj.c_str() + pos + std::strlen(key), nullptr);
        };
        render::creature::CreatureView view;
        view.x = readNumber("\"x\":", 0.0f);
        view.y = readNumber("\"y\":", 0.0f);
        view.z = readNumber("\"z\":", 0.0f);
        view.yaw = readNumber("\"yaw\":", 0.0f);
        view.defIndex = static_cast<int32_t>(readNumber("\"kind\":", 0.0f));
        view.hurt = 0.0f;
        view.death = 0.0f;
        remoteCreatures_.push_back(view);
    }
    if (static_cast<int32_t>(remoteCreatures_.size()) != lastRemoteCreatureCount_) {
        lastRemoteCreatureCount_ = static_cast<int32_t>(remoteCreatures_.size());
        IM_LOGI(kTag, "remote creatures applied=%d", lastRemoteCreatureCount_);
    }
}

void GameShell::ApplyRemotePlayers(const std::string& json) {
    /* 简化解析: 手写扫描 JSON 数组, 最多 256 个快照. */
    std::string uids[net::RemotePlayerManager::kMaxPlayers];
    float xs[net::RemotePlayerManager::kMaxPlayers];
    float ys[net::RemotePlayerManager::kMaxPlayers];
    float zs[net::RemotePlayerManager::kMaxPlayers];
    float yaws[net::RemotePlayerManager::kMaxPlayers];
    float pitches[net::RemotePlayerManager::kMaxPlayers];
    int32_t count = 0;
    size_t cursor = json.find("\"others\":[");
    if (cursor == std::string::npos) {
        remotePlayers_.ReplaceAll(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
        return;
    }
    cursor += 9;
    while (count < net::RemotePlayerManager::kMaxPlayers) {
        const size_t open = json.find('{', cursor);
        if (open == std::string::npos) {
            break;
        }
        const size_t close = json.find('}', open);
        if (close == std::string::npos) {
            break;
        }
        const std::string obj = json.substr(open, close - open + 1);
        cursor = close + 1;
        /* 提取字段 */
        const size_t uidPos = obj.find("\"uid\":");
        if (uidPos == std::string::npos) {
            continue;
        }
        /* uid 的值是带引号的字符串, 取到下一个引号为止. */
        const size_t uidValueStart = obj.find('"', uidPos + 6);
        const size_t uidValueEnd = uidValueStart == std::string::npos
                                       ? std::string::npos
                                       : obj.find('"', uidValueStart + 1);
        if (uidValueStart == std::string::npos || uidValueEnd == std::string::npos) {
            continue;
        }
        uids[count] = obj.substr(uidValueStart + 1, uidValueEnd - uidValueStart - 1);
        const auto readFloat = [&](const char* key) -> float {
            const size_t pos = obj.find(key);
            if (pos == std::string::npos) {
                return 0.0f;
            }
            /* key 自带引号与冒号, 值紧跟在 key 之后, 不能再多跳字符. */
            const size_t valStart = pos + std::strlen(key);
            return std::strtof(obj.c_str() + valStart, nullptr);
        };
        xs[count] = readFloat("\"x\":");
        ys[count] = readFloat("\"y\":");
        zs[count] = readFloat("\"z\":");
        yaws[count] = readFloat("\"yaw\":");
        pitches[count] = readFloat("\"pitch\":");
        count += 1;
    }
    remotePlayers_.ReplaceAll(uids, xs, ys, zs, yaws, pitches, count);
    /* 远端玩家数每轮打一条: 联机时据此确认房间状态确实下发到了渲染侧. */
    if (count != lastRemoteCount_) {
        lastRemoteCount_ = count;
        IM_LOGI(kTag, "remote players applied=%d", count);
    }
}

/*
 * 按脚下方块的注册表材质族返回对应行走音效, 与WalkSound 的分族一致;
 * 阶梯族走 walk_ladder 音源. 兼容段的方块族保持旧表取值, 未知方块回落石音.
 */
audio::SoundId GameShell::WalkSoundForBlock(voxel::BlockId block) {
    const voxel::HarvestMaterial material =
        voxel::BlockRegistry::Instance().Info(block).material;
    switch (material) {
        case voxel::HarvestMaterial::kGrass: return audio::SoundId::kWalkGrass;
        case voxel::HarvestMaterial::kStone: return audio::SoundId::kWalkStone;
        case voxel::HarvestMaterial::kSand: return audio::SoundId::kWalkSand;
        case voxel::HarvestMaterial::kWood: return audio::SoundId::kWalkWood;
        case voxel::HarvestMaterial::kSnow: return audio::SoundId::kWalkSnow;
        case voxel::HarvestMaterial::kGravel: return audio::SoundId::kWalkGravel;
        case voxel::HarvestMaterial::kCloth: return audio::SoundId::kWalkCloth;
        /* 阶梯行走音走独立族; 玻璃与罐没有行走音, 按近亲材质回落. */
        case voxel::HarvestMaterial::kLadder: return audio::SoundId::kWalkLadder;
        case voxel::HarvestMaterial::kGlass: return audio::SoundId::kWalkStone;
        case voxel::HarvestMaterial::kJar: return audio::SoundId::kWalkWood;
        case voxel::HarvestMaterial::kNone: break;
    }
    return audio::SoundId::kWalkStone;
}

void GameShell::CountProgressEvent(int32_t kind) {
    switch (kind) {
        case 1:
            progressCounters_.breakBlocks += 1;
            break;
        case 2:
            progressCounters_.placeBlocks += 1;
            break;
        case 3:
            progressCounters_.pickedItems += 1;
            break;
        default:
            break;
    }
}

game::GameShell::ProgressCounters GameShell::TakeProgressCounters() {
    const ProgressCounters snapshot = progressCounters_;
    progressCounters_ = ProgressCounters{};
    return snapshot;
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
    if (platform::PointerState* held = input.FindMutable(bagPointerId_)) {
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
            /* 背包按钮: 点一下打开面板, 关闭由面板内的关闭按钮或点面板以外完成. */
            if (hud_.HitTestBag(pointer.x, pointer.y)) {
                pointer.uiClaimed = true;
                bagPointerId_ = pointer.id;
                bagHeld_ = true;
                inventoryScreen_.SetOpen(true, inventory_);
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

    if (bagPointerId_ >= 0 && (released & BitOf(bagPointerId_)) != 0ull) {
        bagPointerId_ = -1;
        bagHeld_ = false;
    }
}

/*
 * 背包面板的指针结算. 面板打开期间所有手指都归面板:
 * 命中面板交给背包界面处理, 点在面板以外则关闭面板并把合成材料退回物品栏;
 * 无论哪种情况都标记认领, 避免同一次点击又落到挖掘或放置上.
 */
void GameShell::UpdateInventoryPanel(platform::InputSnapshot& input) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;

    for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
        platform::PointerState& pointer = input.pointers[index];
        if (pointer.down) {
            pointer.uiClaimed = true;
        }
    }

    if (pressed != 0ull) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            platform::PointerState& pointer = input.pointers[index];
            if (!pointer.down || (pressed & BitOf(pointer.id)) == 0ull) {
                continue;
            }
            const ui::InventoryHit hit = hud_.HitTestInventory(pointer.x, pointer.y);
            if (hit.region == ui::InventoryRegion::kNone) {
                inventoryScreen_.SetOpen(false, inventory_);
                continue;
            }
            inventoryScreen_.SetCarryPosition(pointer.x, pointer.y);
            inventoryScreen_.HandlePress(hit, inventory_);
        }
    }

    /* 携带中的堆叠跟随当前按住的手指, 一眼看出手上拿的是哪一格的东西. */
    if (inventoryScreen_.Carrying()) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            const platform::PointerState& pointer = input.pointers[index];
            if (pointer.down) {
                inventoryScreen_.SetCarryPosition(pointer.x, pointer.y);
                break;
            }
        }
    }
}

/*
 * 容器面板的指针结算, 与 UpdateInventoryPanel 同一结构:
 * 命中交给熔炉或储物箱状态机, 点在面板以外关闭并把携带物退回物品栏.
 */
void GameShell::UpdateContainerPanel(platform::InputSnapshot& input) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;

    for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
        platform::PointerState& pointer = input.pointers[index];
        if (pointer.down) {
            pointer.uiClaimed = true;
        }
    }

    if (pressed != 0ull) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            platform::PointerState& pointer = input.pointers[index];
            if (!pointer.down || (pressed & BitOf(pointer.id)) == 0ull) {
                continue;
            }
            /* 命中判定按当前打开的容器走, 熔炉与储物箱不同时打开. */
            ui::InventoryHit hit;
            if (furnaceScreen_.Open()) {
                hit = hud_.HitTestFurnace(pointer.x, pointer.y);
            } else if (chestScreen_.Open()) {
                hit = hud_.HitTestChest(pointer.x, pointer.y);
            } else {
                continue;
            }
            if (hit.region == ui::InventoryRegion::kNone) {
                if (furnaceScreen_.Open()) {
                    furnaceScreen_.SetOpen(false, inventory_);
                } else {
                    chestScreen_.SetOpen(false, inventory_);
                }
                continue;
            }
            if (furnaceScreen_.Open()) {
                furnaceScreen_.SetCarryPosition(pointer.x, pointer.y);
                furnaceScreen_.HandlePress(hit, inventory_);
            } else {
                chestScreen_.SetCarryPosition(pointer.x, pointer.y);
                chestScreen_.HandlePress(hit, inventory_);
            }
        }
    }

    if (furnaceScreen_.Carrying() || chestScreen_.Carrying()) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            const platform::PointerState& pointer = input.pointers[index];
            if (pointer.down) {
                if (furnaceScreen_.Carrying()) {
                    furnaceScreen_.SetCarryPosition(pointer.x, pointer.y);
                }
                if (chestScreen_.Carrying()) {
                    chestScreen_.SetCarryPosition(pointer.x, pointer.y);
                }
                break;
            }
        }
    }
}

/*
 * 编辑手势按下时的容器探测: 视线命中熔炉或箱子方块即记录目标,
 * 本次手势不再进入挖掘与放置, 由 AdvanceContainerProbe 计时长按开面板.
 */
bool GameShell::ProbeContainerTarget() {
    /* 只读视线探测: 用 ProbeBlock 拿不到坐标, 这里直接向世界做一次 DDA 是玩家层的私有路径,
     * 改为复用玩家探测: 命中的方块坐标经 MiningBlock 不适用 (未开始挖掘),
     * 因此用一次轻量世界查询: 从眼位沿视线逐 0.25 米采样 24 步找首个非空气方块.
     */
    const core::Vec3 eye{player_->Position().x,
                         player_->Position().y + player::PlayerController::kEyeHeight,
                         player_->Position().z};
    const core::Vec3 dir = player_->LookDirection();
    for (int32_t step = 1; step <= 24; ++step) {
        const float travelled = static_cast<float>(step) * 0.25f;
        const core::Vec3 point{eye.x + dir.x * travelled, eye.y + dir.y * travelled,
                               eye.z + dir.z * travelled};
        const int32_t blockX = FloorToInt(point.x);
        const int32_t blockY = FloorToInt(point.y);
        const int32_t blockZ = FloorToInt(point.z);
        const voxel::BlockId block = world_->GetBlock(blockX, blockY, blockZ);
        if (block == voxel::block::Air) {
            continue;
        }
        if (!IsFurnaceBlock(block) && !IsChestBlock(block)) {
            return false;
        }
        containerProbeBlock_ = block;
        containerProbeX_ = blockX;
        containerProbeY_ = blockY;
        containerProbeZ_ = blockZ;
        containerProbeSeconds_ = 0.0;
        containerProbeOpened_ = false;
        return true;
    }
    return false;
}

/* 容器长按推进: 按住超过 kLongPressSeconds 打开对应面板, 抬手或移出轻点范围作废. */
bool GameShell::AdvanceContainerProbe(double deltaSeconds) {
    if (containerProbeOpened_ || containerProbeBlock_ == voxel::block::Air) {
        return false;
    }
    containerProbeSeconds_ += deltaSeconds;
    if (containerProbeSeconds_ < kLongPressSeconds) {
        return true;
    }
    const uint64_t key = ContainerKeyOf(containerProbeX_, containerProbeY_, containerProbeZ_);
    if (IsFurnaceBlock(containerProbeBlock_)) {
        furnaceScreen_.OpenFurnace(furnaces_[key], inventory_);
        IM_LOGI(kTag, "furnace panel opened at %d,%d,%d", containerProbeX_, containerProbeY_,
                containerProbeZ_);
    } else {
        chestScreen_.OpenChest(chests_[key], inventory_);
        IM_LOGI(kTag, "chest panel opened at %d,%d,%d", containerProbeX_, containerProbeY_,
                containerProbeZ_);
    }
    containerProbeOpened_ = true;
    return true;
}

/* 关闭全部容器面板并退回携带物, 世界卸载与表面重建时调用. */
void GameShell::CloseContainerPanels() {
    furnaceScreen_.SetOpen(false, inventory_);
    chestScreen_.SetOpen(false, inventory_);
    furnaceScreen_.Reset();
    chestScreen_.Reset();
}

/*
 * 编辑手势. 破坏必须按硬度累积进度, 因此按住与轻点各自绑定一种动作:
 * 按住不松手走进度, 挖满才破坏; 轻点(按住不足轻点窗口且没有位移)放置当前材料.
 * 进度从按下那一刻开始累计, 抬手即清零, 与"按住挖掘, 松手作废"的语义一致.
 */
void GameShell::UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds) {
    /* 死亡等待期间不响应挖掘与攻击, 避免倒下的瞬间还在改动世界. */
    if (player_->Dying()) {
        if (editPointerId_ >= 0) {
            player_->CancelMining();
            editPointerId_ = -1;
        }
        attackHeld_ = false;
        editAttacked_ = false;
        return;
    }

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
            IM_LOGD(kTag, "edit pointer claimed id=%d at %.3f,%.3f", pointer.id,
                    static_cast<double>(pointer.x), static_cast<double>(pointer.y));
            attackAccumulator_ = 0.0;
            editAttacked_ = false;
            /*
             * 视线先判定容器方块: 命中即进入长按开面板探测, 本次手势不再挖掘;
             * 未命中再判生物与挖掘, 保持原有语义.
             */
            if (ProbeContainerTarget()) {
                break;
            }
            attackHeld_ = TryAttackCreature();
            if (attackHeld_) {
                editAttacked_ = true;
            } else {
                BeginEditMining();
            }
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
        /* 容器探测接管的手势: 计时长按开面板, 不进入攻击与挖掘. */
        if (containerProbeBlock_ != voxel::block::Air) {
            AdvanceContainerProbe(deltaSeconds);
            return;
        }
        if (attackHeld_) {
            /* 按住攻击: 按固定间隔连续出手, 目标消失后回落到挖掘. */
            AdvanceHeldAttack(deltaSeconds);
            return;
        }
        /* 按住期间逐帧推进进度, 挖满当场破坏. */
        AdvanceEditMining(deltaSeconds);
        return;
    }
    if ((released & BitOf(editPointerId_)) == 0ull) {
        /* 指针既不在按下集合也不在释放集合: 帧内被取消, 结束这次手势. */
        player_->CancelMining();
        attackHeld_ = false;
        editAttacked_ = false;
        editPointerId_ = -1;
        containerProbeBlock_ = voxel::block::Air;
        return;
    }

    /* 抬手结算: 只有轻点且没有位移才算放置, 长按只结束挖掘, 进度清零. */
    const double held = elapsedSeconds_ - editDownSeconds_;
    const float movedX = editLastX_ - editDownX_;
    const float movedY = editLastY_ - editDownY_;
    const float moved = std::sqrt(movedX * movedX + movedY * movedY);
    player_->CancelMining();
    const bool attacked = editAttacked_;
    const bool containerOpened = containerProbeOpened_;
    attackHeld_ = false;
    editAttacked_ = false;
    containerProbeBlock_ = voxel::block::Air;
    containerProbeOpened_ = false;
    /* 本次手势出手过就不再放置方块: 攻击与放置互斥, 避免同一次点击两件事都做. */
    if (!attacked && !containerOpened && held < player::mining::kTapSeconds &&
        moved < kTapMoveLimit) {
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

/*
 * 手持工具对目标方块的挖掘速度系数.
 *
 * 规则: 空手为 1.0; 工具族与方块要求的采集工具族一致时按工具效率折算
 * (效率 50 表示比徒手快一倍); 族不一致按三分之一速度, 比徒手还慢,
 * 让玩家有动力换对应工具.
 * 工具等级低于方块要求时再乘十分之一: 硬采得下来, 但慢到不划算,
 * 同时掉落会被 CollectDrop 拦下, 等价于原版的"需要更高级工具".
 */
float GameShell::MiningSpeedAgainst(voxel::BlockId block) const {
    const item::ItemId held = inventory_.ItemAt(selectedSlot_);
    if (held == item::kNone) {
        return 1.0f;
    }
    item::ItemTable::ToolInfo tool;
    if (!item::ItemTable::Instance().ToolOf(held, tool)) {
        return 1.0f;
    }
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    const auto required = static_cast<int32_t>(info.tool);
    const auto harvest = static_cast<int32_t>(tool.harvest);
    /* 徒手可采的方块 (要求 kHand) 任何工具都按效率全额算. */
    const bool familyMatches = harvest == required || required == 0;
    float speed = static_cast<float>(tool.efficiency) / 100.0f;
    if (!familyMatches) {
        speed *= 0.33f;
    }
    /*
     * 等级不足: 只降速, 掉落由 CollectDrop 按同一规则拦下.
     * 只有要求工具族的方块才看等级: 徒手可采的方块 (花草, 羊毛一类)
     * 数据表里也带等级列, 但那一列对手采没有意义.
     */
    const int32_t requiredLevel = static_cast<int32_t>(info.toolLevel);
    if (required > 0 && requiredLevel > 0 && static_cast<int32_t>(tool.level) < requiredLevel) {
        speed *= 0.1f;
    }
    return speed > 0.05f ? speed : 0.05f;
}

bool GameShell::BeginEditMining() {
    if (!player_->BeginMining(*world_)) {
        /* 起手失败要区分原因: 射线没命中, 还是命中的方块不可破坏. */
        const core::Vec3 feet = player_->Position();
        const int32_t fx = static_cast<int32_t>(std::floor(feet.x));
        const int32_t fy = static_cast<int32_t>(std::floor(feet.y));
        const int32_t fz = static_cast<int32_t>(std::floor(feet.z));
        IM_LOGD(kTag,
                "mine begin rejected feet=%.2f,%.2f,%.2f below=%u foot=%u above=%u "
                "yawDeg=%.1f pitchDeg=%.1f",
                static_cast<double>(feet.x), static_cast<double>(feet.y), static_cast<double>(feet.z),
                static_cast<unsigned>(world_->GetBlock(fx, fy - 1, fz)),
                static_cast<unsigned>(world_->GetBlock(fx, fy, fz)),
                static_cast<unsigned>(world_->GetBlock(fx, fy + 1, fz)),
                static_cast<double>(player_->Yaw() * 57.2957795),
                static_cast<double>(player_->Pitch() * 57.2957795));
        return false;
    }
    /*
     * 起手后才能知道目标方块, 而工具是否对口取决于目标方块的采集工具族,
     * 因此速度系数在这里算完再回填时长.
     */
    player_->SetMiningSpeed(MiningSpeedAgainst(player_->MiningBlock()));
    player_->RecomputeMiningDuration();
    const voxel::BlockId block = player_->MiningBlock();
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    /*
     * 起手日志记录硬度与所需秒数, 同一行里的 need 就是 kSecondsPerHardness 的换算结果,
     * 实测时与完成日志的 elapsed 对照即可确认进度确实按硬度累计.
     */
    item::ItemTable::ToolInfo heldTool;
    const item::ItemId heldId = inventory_.ItemAt(selectedSlot_);
    const bool hasHeldTool =
        heldId != item::kNone && item::ItemTable::Instance().ToolOf(heldId, heldTool);
    /* 等级两列一并打出来: 等级不足时速度按十分之一折算, 掉落也会被拦下. */
    IM_LOGI(kTag,
            "mine begin block=%s hardness=%.2f material=%d need=%.2fs dig=%d needLevel=%u heldLevel=%d",
            info.name, static_cast<double>(info.hardness), static_cast<int32_t>(info.material),
            static_cast<double>(player_->MiningDuration()),
            static_cast<int32_t>(DigSoundOf(info.material)),
            static_cast<unsigned>(info.toolLevel),
            hasHeldTool ? static_cast<int32_t>(heldTool.level) : 0);
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
    /* 破坏点撒碎屑: 取方块中心, 颜色按图集格平均色. */
    render::particle::EmitBlockBreak(tick.block, static_cast<float>(tick.blockPos.x) + 0.5f,
                                     static_cast<float>(tick.blockPos.y) + 0.5f,
                                     static_cast<float>(tick.blockPos.z) + 0.5f);
    CollectDrop(tick.block);
    /* 破坏点上方若有重力方块, 失去支撑后落下来; 水边破坏则触发流动. */
    SettleGravityAt(tick.blockPos.x, tick.blockPos.y, tick.blockPos.z);
    SettleFluidAt(tick.blockPos.x, tick.blockPos.y, tick.blockPos.z);
    /*
     * 挖掉一格扣一次手持工具耐久, 耗尽时工具消失并提示.
     * 徒手与方块类手持物不带耐久, 这里会直接返回 false.
     */
    if (inventory_.ConsumeDurability(selectedSlot_)) {
        IM_LOGI(kTag, "tool broken at slot %d", selectedSlot_);
    } else {
        IM_LOGD(kTag, "tool durability left=%d slot=%d", inventory_.DurabilityAt(selectedSlot_),
                selectedSlot_);
    }
    /* 破坏后手指仍按住时接着挖下一格, 与连续挖掘的手感一致. */
    BeginEditMining();
}

/*
 * 生物与战斗. 玩家攻击走视线拾取: 视点沿视线方向的 kPlayerReach 内最近的生物
 * 受一次伤害. 伤害按手持物换算: 空手取拳击伤害, 手持方块按方块硬度加成并封顶,
 * 换算关系集中在 entity_tuning.h, 后续接入武器物品只改那一处.
 */
bool GameShell::TryAttackCreature() {
    const core::Vec3 feet = player_->Position();
    const core::Vec3 eye{feet.x, feet.y + kEyeHeight, feet.z};
    float distance = 0.0f;
    const entity::Entity* target =
        entities_.PickRay(eye, player_->LookDirection(), entity::tuning::kPlayerReach, distance);
    if (target == nullptr) {
        return false;
    }
    /* 手持物取快捷栏当前格: 空手按拳击, 手持方块按该方块的硬度加成. */
    const ItemId heldItem = inventory_.ItemAt(selectedSlot_);
    const bool holding = heldItem != item::kNone;
    const voxel::BlockId heldBlock = item::ItemTable::Instance().BlockOf(heldItem);
    const float hardness = voxel::BlockRegistry::Instance().Info(heldBlock).hardness;
    const float damage = entity::tuning::HeldAttackDamage(holding, hardness);
    const bool killed = entities_.Damage(target->Id(), damage);
    if (killed) {
        /* 致死一击计一次击败事件, 并入上报增量与本地成就进度. */
        progressCounters_.kills += 1;
        progress::ProgressHub::Instance().OnKill(1);
    }
    /* 攻击命中按次扣体力 (tili_action_attack), 未命中不扣. */
    player_->ConsumeStaminaPublic(player::survival::kStaminaAttack);
    IM_LOGI(kTag, "attack creature id=%u damage=%.1f distance=%.2f killed=%d held=%d",
            static_cast<uint32_t>(target->Id()), static_cast<double>(damage),
            static_cast<double>(distance), killed ? 1 : 0, holding ? 1 : 0);
    return true;
}

void GameShell::AdvanceHeldAttack(double deltaSeconds) {
    attackAccumulator_ += deltaSeconds;
    if (attackAccumulator_ < entity::tuning::kAttackHoldInterval) {
        return;
    }
    attackAccumulator_ = 0.0;
    if (TryAttackCreature()) {
        editAttacked_ = true;
        return;
    }
    /* 目标死亡或走出视线: 本次手势回落到挖掘, 按住继续挖的原有手感不变. */
    attackHeld_ = false;
    BeginEditMining();
}

void GameShell::UpdateEntities(double deltaSeconds) {
    entity::PlayerProbe probe;
    probe.feet = player_->Position();
    probe.halfWidth = player::PlayerController::kHalfWidth;
    probe.height = player::PlayerController::kHeight;
    probe.alive = player_->Alive();

    /* 昼夜刷怪口径: 白天只放动物, 夜间才放主动攻击的生物. */
    entities_.SetHostileSpawnAllowed(!worldTime_->IsDay());

    const entity::EntityFrameStats stats = entities_.Update(deltaSeconds, *world_, probe);

    /* 生物音效: 受击与死亡事件按生物种类查表, 路径直接交给音频引擎播放. */
    for (const entity::EntityManager::SoundEvent& event : entities_.TakeSoundEvents()) {
        const char* path = entity::gen::EntitySoundPath(event.sourceId, event.action);
        if (path != nullptr && path[0] != '\0') {
            audio::AudioEngine::Instance().PlayCreatureAsset(path);
        }
    }

    /* 生物伤害汇总后一次性施加, 玩家控制器内部完成免伤窗口与死亡判定.
     * 伤害数值显示 (kDamageSwitch) 暂无消费点: 伤害数字系统尚未实现,
     * 接入飘字后在此按开关决定是否生成数字事件. */
    if (stats.playerDamage > 0.0f) {
        player_->ApplyDamage(stats.playerDamage);
        IM_LOGI(kTag, "player damaged by creature amount=%.1f health=%.1f",
                static_cast<double>(stats.playerDamage),
                static_cast<double>(player_->Health()));
    }
    CollectCreatureDrops();
    if (stats.spawned > 0 || stats.deaths > 0 || stats.despawned > 0) {
        IM_LOGD(kTag, "entities spawned=%d deaths=%d despawned=%d alive=%d awake=%d",
                stats.spawned, stats.deaths, stats.despawned, entities_.Count(),
                entities_.AwakeCount());
    }
}

void GameShell::CollectCreatureDrops() {
    const item::ItemTable& items = item::ItemTable::Instance();
    for (const entity::EntityDrop& drop : entities_.TakeDrops()) {
        /* 生物掉落按方块折算, 再走物品表折算成物品 id, 物品栏只认物品 id. */
        const ItemId itemId = items.OfBlock(drop.block);
        const char* name = voxel::BlockRegistry::Instance().Info(drop.block).name;
        if (itemId == item::kNone) {
            IM_LOGW(kTag, "creature drop has no item, block=%s", name);
            continue;
        }
        const int32_t added = inventory_.Add(itemId, drop.count);
        if (added > 0) {
            IM_LOGI(kTag, "creature drop collected block=%s item=%u count=%d", name,
                    static_cast<uint32_t>(itemId), added);
        }
        if (added < drop.count) {
            IM_LOGW(kTag, "inventory full, creature drop discarded block=%s want=%d added=%d",
                    name, drop.count, added);
        }
    }
}

void GameShell::CollectDrop(voxel::BlockId brokenBlock) {
    /*
     * 工具等级门槛: 等级不足时采下来也不掉落, 与挖掘速度的等级折算同一口径.
     * 判定只读方块表与手持工具, 不影响破坏本身与进度统计.
     */
    const voxel::BlockInfo& broken = voxel::BlockRegistry::Instance().Info(brokenBlock);
    /* 徒手可采的方块不受等级列约束, 只对要求工具族的方块判等级. */
    if (broken.tool != voxel::HarvestTool::kHand && broken.toolLevel > 0) {
        item::ItemTable::ToolInfo held;
        const item::ItemId heldId = inventory_.ItemAt(selectedSlot_);
        const bool hasTool = heldId != item::kNone && item::ItemTable::Instance().ToolOf(heldId, held);
        if (!hasTool || held.level < broken.toolLevel) {
            IM_LOGI(kTag, "drop blocked, need tool level %u block=%s held=%d",
                    static_cast<unsigned>(broken.toolLevel), broken.name,
                    static_cast<int32_t>(heldId));
            return;
        }
    }
    /* 方块掉落规则在物品表里统一折算: 方块 -> 掉落方块 -> 掉落物品, 只此一份. */
    const item::ItemTable& items = item::ItemTable::Instance();
    const item::ItemId dropItem = items.DropOfBlock(brokenBlock);
    if (dropItem == item::kNone) {
        IM_LOGI(kTag, "block has no drop, block=%u", static_cast<uint32_t>(brokenBlock));
        return;
    }
    CountProgressEvent(1);
    const char* sourceName = voxel::BlockRegistry::Instance().Info(brokenBlock).name;
    const char* name = items.Info(dropItem).key;
    const int32_t slot = inventory_.SlotOf(dropItem);
    const int32_t added = inventory_.Add(dropItem, 1);
    if (added == 1) {
        IM_LOGI(kTag, "drop collected from=%s item=%s slot=%d count=%d", sourceName, name, slot,
                inventory_.Count(slot));
        return;
    }
    /* 槽位堆满时丢弃, 只累计数量并写警告, 不影响破坏本身. */
    IM_LOGW(kTag, "inventory full, drop discarded item=%s slot=%d discarded=%d", name, slot,
            inventory_.Discarded());
    ShowTip("BAG FULL");
}

/*
 * 重力沉降.
 * 方块改动后, 从改动点上方一列向上扫: 遇到重力方块且下方为空就把它落到
 * 最近的非空格之上. 单次最多处理 64 格, 避免一次改动引发长链开销;
 * 沉降只搬方块, 不改其它格子的内容, 也不触发掉落.
 */
void GameShell::SettleGravityAt(int32_t x, int32_t y, int32_t z) {
    constexpr int32_t kMaxFall = 64;
    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    int32_t settled = 0;
    const int32_t top = y + 1 + kMaxFall < voxel::kWorldHeight ? y + 1 + kMaxFall : voxel::kWorldHeight;
    for (int32_t columnY = y + 1; columnY < top; ++columnY) {
        const voxel::BlockId id = world_->GetBlock(x, columnY, z);
        if (id == voxel::block::Air) {
            continue;
        }
        const voxel::BlockInfo& info = registry.Info(id);
        /*
         * 花草失去支撑即破坏: 只对非实心的重力方式 3 生效,
         * 避免把浮空的实心方块 (如雪) 误删.
         */
        if (info.gravity == 3 && !info.solid) {
            if (world_->GetBlock(x, columnY - 1, z) == voxel::block::Air) {
                world_->SetBlock(x, columnY, z, voxel::block::Air);
                ++settled;
            }
            continue;
        }
        if (info.gravity != 1) {
            continue;
        }
        int32_t target = columnY;
        while (target > 0 && world_->GetBlock(x, target - 1, z) == voxel::block::Air) {
            target -= 1;
        }
        if (target == columnY) {
            continue;
        }
        world_->SetBlock(x, columnY, z, voxel::block::Air);
        world_->SetBlock(x, target, z, id);
        ++settled;
    }
    if (settled > 0) {
        IM_LOGI(kTag, "gravity settled=%d at %d,%d,%d", settled, x, y, z);
    }
}

/*
 * 流体流动.
 * 以改动点为起点做一次有界广度优先: 空格若与液体相邻就填成同种液体,
 * 优先向下方扩散, 其次水平, 最后向上, 这样水面会先铺平再抬升.
 * 单次最多填 24 格, 半径 3 格, 避免挖开水域时整片灌满.
 */
void GameShell::SettleFluidAt(int32_t x, int32_t y, int32_t z) {
    constexpr int32_t kMaxCells = 24;
    constexpr int32_t kRadius = 3;
    const voxel::BlockRegistry& registry = voxel::BlockRegistry::Instance();
    const auto isFluid = [&registry](voxel::BlockId id) {
        return id != voxel::block::Air && registry.Info(id).gravity == 2;
    };
    /*
     * 蔓延方向: 先向下, 再水平. 不做向上蔓延,
     * 否则水会顺着空格一格一格爬上墙, 观感明显不对.
     */
    constexpr int32_t kOffsets[5][3] = {
        {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1},
    };
    std::vector<voxel::BlockPos> queue;
    queue.push_back(voxel::BlockPos{x, y, z});
    size_t head = 0;
    int32_t filled = 0;
    while (head < queue.size() && filled < kMaxCells) {
        const voxel::BlockPos cell = queue[head];
        ++head;
        if (cell.x < x - kRadius || cell.x > x + kRadius || cell.z < z - kRadius ||
            cell.z > z + kRadius || cell.y < y - kRadius || cell.y > y + kRadius) {
            continue;
        }
        if (cell.y < 0 || cell.y >= voxel::kWorldHeight) {
            continue;
        }
        if (world_->GetBlock(cell.x, cell.y, cell.z) != voxel::block::Air) {
            continue;
        }
        /* 找一格相邻液体作为水源; 找不到就不填, 也不继续扩散. */
        voxel::BlockId source = voxel::block::Air;
        for (const auto& offset : kOffsets) {
            const voxel::BlockId neighbour =
                world_->GetBlock(cell.x + offset[0], cell.y + offset[1], cell.z + offset[2]);
            if (isFluid(neighbour)) {
                source = neighbour;
                break;
            }
        }
        if (source == voxel::block::Air) {
            continue;
        }
        world_->SetBlock(cell.x, cell.y, cell.z, source);
        ++filled;
        for (const auto& offset : kOffsets) {
            queue.push_back(voxel::BlockPos{cell.x + offset[0], cell.y + offset[1],
                                             cell.z + offset[2]});
        }
    }
    if (filled > 0) {
        IM_LOGI(kTag, "fluid spread=%d at %d,%d,%d", filled, x, y, z);
    }
}

bool GameShell::TryPlaceSelectedBlock() {
    const item::ItemTable& items = item::ItemTable::Instance();
    const item::ItemStack stack = inventory_.Slot(selectedSlot_);
    if (stack.Empty()) {
        /* 数量为 0 时拒绝放置, 只给提示, 不改动世界与数量. */
        ShowTip(kTipEmptySlot);
        IM_LOGW(kTag, "place rejected, slot=%d count=0", selectedSlot_);
        return false;
    }
    /*
     * 只有关联方块的物品才能放置: 工具与材料在快捷栏里同样可以选中,
     * 这时不消耗物品, 只给出提示.
     */
    const voxel::BlockId block = items.BlockOf(stack.item);
    if (block == voxel::block::Air) {
        ShowTip("NOT A BLOCK");
        IM_LOGD(kTag, "place rejected, item=%s has no block", items.Info(stack.item).key);
        return false;
    }
    if (!player_->PlaceBlock(*world_, block)) {
        return false;
    }
    inventory_.Consume(selectedSlot_, 1);
    CountProgressEvent(2);
    {
        const voxel::BlockPos placed = player_->LastPlacedPos();
        SettleGravityAt(placed.x, placed.y, placed.z);
        SettleFluidAt(placed.x, placed.y, placed.z);
    }
    IM_LOGI(kTag, "block placed slot=%d item=%s block=%s remain=%d", selectedSlot_,
            items.Info(stack.item).key, voxel::BlockRegistry::Instance().Info(block).name,
            inventory_.Count(selectedSlot_));
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
    RefreshSelectedItem();
    const item::ItemTable& items = item::ItemTable::Instance();
    IM_LOGI(kTag, "hotbar slot %d selected, item=%s block=%u", slot,
            items.Info(inventory_.ItemAt(slot)).key, static_cast<uint32_t>(selectedBlock_));
}

void GameShell::SyncViewDistance() {
    const int32_t wanted = settings_->ViewDistanceChunks();
    if (wanted == viewDistanceChunks_) {
        return;
    }
    viewDistanceChunks_ = wanted;
    SyncEntitySpawnExtent();
    IM_LOGI(kTag, "view distance changed to %d chunk columns", wanted);
}

void GameShell::SyncEntitySpawnExtent() {
    /*
     * 生成半径取视距覆盖的米数并留出一列的余量, 保证采样列已经有区块数据;
     * 具体上下限由实体管理器再裁剪一次.
     */
    const float meters = static_cast<float>(viewDistanceChunks_) *
                         static_cast<float>(voxel::kChunkSizeX);
    entities_.SetSpawnExtent(meters - 6.0f);
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

void GameShell::SyncVisualSettings() {
    const int32_t fogSetting = settings_->Get(settings::Key::kFog);
    const int32_t brightSetting = settings_->Get(settings::Key::kScreenBright);
    const int32_t bloodSetting = settings_->Get(settings::Key::kBloodSwitch);
    const int32_t treeSetting = settings_->Get(settings::Key::kTreeShape);
    if (fogSetting == fogCachedSetting_ && brightSetting == brightCachedSetting_) {
        bloodSwitch_ = bloodSetting;
        treeShape_ = treeSetting;
        return;
    }
    fogCachedSetting_ = fogSetting;
    brightCachedSetting_ = brightSetting;
    /* 雾密度查表, 档位越界时回落到关闭; 亮度按 0.7 到 1.3 线性折算. */
    const int32_t fogLevel = fogSetting >= 0 && fogSetting < settings::tuning::kFogDensityLevelCount
                                 ? fogSetting : 0;
    fogDensity_ = settings::tuning::kFogDensityLevels[fogLevel];
    brightness_ = settings::tuning::kBrightnessAtZero +
                  settings::tuning::kBrightnessSpan * (static_cast<float>(brightSetting) /
                                                       settings::tuning::kBrightnessFull);
    bloodSwitch_ = bloodSetting;
    treeShape_ = treeSetting;
    IM_LOGI(kTag, "visual settings applied, fog=%d density=%.4f bright=%d factor=%.2f",
            fogSetting, static_cast<double>(fogDensity_), brightSetting,
            static_cast<double>(brightness_));
}

namespace {

/* 昼夜口径: 6 点日出, 12 点天顶, 18 点日落, 夜间太阳落到地平线以下. */
constexpr float kPiForSun = 3.14159265358979323846f;

void SunDirectionOf(const world::WorldTime& time, float* out) {
    if (out == nullptr) {
        return;
    }
    const float hour = time.GetHour();
    const float angle = (hour - 6.0f) / 12.0f * kPiForSun;
    /* x 向东, y 向上, z 恒定南偏: 太阳沿东升西落, 影子方向随之改变. */
    out[0] = std::cos(angle) * 0.94f;
    out[1] = std::sin(angle);
    out[2] = 0.34f;
}

}  /* namespace */

void GameShell::SkyColor(float* out) const {
    if (out == nullptr) {
        return;
    }
    /*
     * 昼夜两端的天空色: 白天是亮蓝, 夜间是深蓝黑; 中间按天光因子线性混合.
     * 三个分量都取归一化值, 与渲染后端的 glClearColor 口径一致.
     */
    constexpr float kDayRed = 0.48f;
    constexpr float kDayGreen = 0.70f;
    constexpr float kDayBlue = 0.93f;
    constexpr float kNightRed = 0.04f;
    constexpr float kNightGreen = 0.06f;
    constexpr float kNightBlue = 0.13f;
    const float factor = skyBlend_;
    out[0] = kNightRed + (kDayRed - kNightRed) * factor;
    out[1] = kNightGreen + (kDayGreen - kNightGreen) * factor;
    out[2] = kNightBlue + (kDayBlue - kNightBlue) * factor;
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

    /*
     * 摇晃基准用控制器的当前相机视点: 第一人称即眼位, 第三人称是
     * 后撤后的轨点, 与视图矩阵的平移项共用同一来源, 摇晃不会偏移视点.
     */
    const core::Vec3 cameraBase = player_->CameraPosition();
    float camera[3] = {cameraBase.x, cameraBase.y, cameraBase.z};

    /*
     * 镜头摇晃. 移动强度给一层轻微摆动, 受击时再叠一层快速衰减的抖动,
     * 与血条下降一起构成受伤反馈; 关闭设置时相机完全静止.
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
        const float hurt = player_->HurtSeconds();
        if (hurt > 0.0f) {
            const float strength = hurt / player::PlayerController::kHurtSeconds;
            const float phase = static_cast<float>(elapsedSeconds_) * 57.0f;
            const float amplitude = settings::tuning::kCameraShakeAmplitude * 2.5f * strength;
            camera[0] += std::sin(phase) * amplitude;
            camera[1] += std::cos(phase * 1.7f) * amplitude * 0.7f;
        }
    }

    /* 雾, 亮度与天光因子分别下发: 天光由 shader 的 mix(0.27, 1.0) 承担月夜下限. */
    renderer_->SetAtmosphere(fogDensity_, brightness_, skyBlend_);
    /* 天气: 降水强度压暗天空并叠加雨雪层, 状态变化写一条日志便于实机核对. */
    if (weather_.Kind() != lastWeatherKind_) {
        lastWeatherKind_ = weather_.Kind();
        IM_LOGI(kTag, "weather changed to %s intensity=%.2f", weather_.KindName(),
                static_cast<double>(weather_.Intensity()));
    }
    renderer_->SetWeather(static_cast<int32_t>(weather_.Kind()), weather_.Intensity(),
                          static_cast<float>(elapsedSeconds_));

    /* 天体方向与云层漂移时间: 太阳按世界时间走, 云层按现实秒数缓慢平移. */
    {
        float sunDirection[3] = {0.0f, 1.0f, 0.0f};
        SunDirectionOf(*worldTime_, sunDirection);
        renderer_->SetSkySun(sunDirection, static_cast<float>(elapsedSeconds_));
    }
    renderer_->SetViewProjection(viewProjection, camera);
    renderer_->Sync(*world_);
    renderer_->Draw();

    /* 方块碎屑在区块之后绘制: 它要压在方块表面之上, 又要在生物与界面之下. */
    render::particle::Draw(viewProjection);

    /*
     * 生物的程序化体素形象: 按定义索引折算部件与配色, 受击闪白与死亡倒地
     * 由实体状态直接折算; 远程玩家仍走竖条标记 (那是玩家).
     * 都在区块绘制之后, HUD 之前, 共享投影矩阵; 屏外自动剔除.
     */
    const auto& creatures = entities_.Creatures();
    const int32_t creatureCount = static_cast<int32_t>(creatures.size());
    if (creatureCount > 0 && render::creature::Initialize()) {
        static std::vector<render::creature::CreatureView> views;
        views.clear();
        views.reserve(static_cast<size_t>(creatureCount));
        for (const auto& creature : creatures) {
            if (creature == nullptr || creature->Removed()) {
                continue;
            }
            /* 容量上限与渲染侧一致, 超出部分不提交. */
            if (views.size() >= 32) {
                break;
            }
            const core::Vec3 pos = creature->Position();
            render::creature::CreatureView view;
            view.x = pos.x;
            view.y = pos.y;
            view.z = pos.z;
            view.yaw = creature->Yaw();
            /* 受击闪白: 受击剩余秒数与总时长求比例. */
            view.hurt = creature->HurtSeconds() / entity::tuning::kHurtFlashSeconds;
            /*
             * 死亡倒地: 实体只暴露 Dying 布尔, 渐变进度在总装侧记死亡起点
             * (进程内 map), 按死亡保留时长折算, 越接近回收越躺平.
             */
            if (creature->Dying()) {
                const uint32_t id = static_cast<uint32_t>(creature->Id());
                auto died = deathStartTimes_.find(id);
                if (died == deathStartTimes_.end()) {
                    died = deathStartTimes_.emplace(id, elapsedSeconds_).first;
                }
                view.death = static_cast<float>((elapsedSeconds_ - died->second) /
                                                entity::tuning::kDeathSeconds);
            } else {
                deathStartTimes_.erase(static_cast<uint32_t>(creature->Id()));
            }
            view.defIndex = entity::MonsterTable::Instance().IndexOfSourceId(
                creature->Definition().sourceId);
            views.push_back(view);
        }
        render::creature::Draw(views.data(), static_cast<int32_t>(views.size()), viewProjection);
    }

    /* 远端生物: 与本地生物同一批提交, 位置与朝向来自房主快照. */
    if (!remoteCreatures_.empty() && render::creature::Initialize()) {
        render::creature::Draw(remoteCreatures_.data(),
                               static_cast<int32_t>(remoteCreatures_.size()), viewProjection);
    }

    if (remotePlayers_.Count() > 0) {
        const auto& players = remotePlayers_.Players();
        const int32_t rpCount = static_cast<int32_t>(players.size());
        /*
         * 远端玩家画成与生物同一套体素人形: 比竖条标记更容易辨认距离与朝向.
         * 标记通道保留, 作为头顶的辅助标识, 与形象叠加显示.
         */
        static std::vector<render::creature::PlayerView> playerViews;
        playerViews.clear();
        playerViews.reserve(static_cast<size_t>(rpCount));
        for (int32_t i = 0; i < rpCount; ++i) {
            render::creature::PlayerView view;
            view.x = players[i].x;
            view.y = players[i].y;
            view.z = players[i].z;
            view.yaw = players[i].yaw;
            playerViews.push_back(view);
        }
        render::creature::DrawPlayers(playerViews.data(), rpCount, viewProjection);

        std::vector<float> rxs(rpCount), rys(rpCount), rzs(rpCount);
        for (int32_t i = 0; i < rpCount; ++i) {
            rxs[i] = players[i].x;
            /* 标记抬到头顶, 与形象错开, 不再插进身体里. */
            rys[i] = players[i].y + 1.9f;
            rzs[i] = players[i].z;
        }
        ui::RenderRemoteMarkers(rxs.data(), rys.data(), rzs.data(), rpCount, viewProjection, width_,
                                height_);
    }

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
    /*
     * 血条取真实血量比例; 死亡等待期间为 0, 重生后回满.
     * 血条开关关闭时恒显示满血 (语义: 隐藏掉血状态而非隐藏血条).
     */
    frame.health = bloodSwitch_ != 0 ? player_->HealthRatio() : 1.0f;
    /* 饥饿与体力取控制器的同源比例, 与两条状态条一一对应. */
    frame.hunger = player_->HungerRatio();
    frame.stamina = player_->StaminaRatio();
    /*
     * 快捷栏每格的数量, 图标与首字母都取自物品栏, 与背包面板同源:
     * 关联方块的物品用图集格, 工具与材料用类别色块加首字母占位.
     */
    const item::ItemTable& items = item::ItemTable::Instance();
    for (int32_t slot = 0; slot < ui::kHudSlotCount; ++slot) {
        const item::ItemStack stack = inventory_.Slot(slot);
        frame.slotCounts[slot] = stack.count;
        frame.slotHasItem[slot] = !stack.Empty();
        frame.slotAtlasCell[slot] = stack.Empty() ? -1 : items.AtlasCellOf(stack.item);
        frame.slotInitial[slot] = 0;
        frame.slotTone[slot] = 0;
        if (!stack.Empty() && frame.slotAtlasCell[slot] < 0) {
            frame.slotInitial[slot] = items.Initial(stack.item);
            frame.slotTone[slot] = static_cast<uint8_t>(items.Info(stack.item).category);
        }
        /* 工具带耐久时把剩余量与上限一起交给界面, 由界面在格底画耐久条. */
        frame.slotDurability[slot] = stack.durability;
        frame.slotMaxDurability[slot] = 0;
        if (!stack.Empty()) {
            item::ItemTable::ToolInfo tool;
            if (items.ToolOf(stack.item, tool)) {
                frame.slotMaxDurability[slot] = static_cast<int32_t>(tool.durability);
            }
        }
    }
    /* 长按显示的标签用物品英文键, 空格槽位回落到方块栏共享表. */
    if (nameSlot_ >= 0 && nameSlot_ < ui::kHudSlotCount) {
        const item::ItemStack named = inventory_.Slot(nameSlot_);
        frame.nameText = named.Empty() ? nullptr : items.Info(named.item).key;
    }
    frame.bagPressed = bagHeld_;
    frame.tipText = tipText_;

    /* 背包面板打开时才组装它的绘制数据, 关闭时界面层拿不到面板指针. */
    if (inventoryScreen_.Open()) {
        inventoryScreen_.BuildFrame(inventory_, inventoryFrame_);
        frame.inventory = &inventoryFrame_;
    }
    /* 容器面板与背包互斥, 打开时组装对应帧. */
    if (furnaceScreen_.Open()) {
        furnaceScreen_.BuildFrame(inventory_, furnaceFrame_);
        frame.furnace = &furnaceFrame_;
    } else if (chestScreen_.Open()) {
        chestScreen_.BuildFrame(inventory_, chestFrame_);
        frame.chest = &chestFrame_;
    }
    /*
     * 破坏进度只在按住超过轻点窗口后显示: 轻点放置时进度虽然累计了几帧,
     * 但不会闪出一个进度环; 进度归零时界面层不绘制.
     */
    if (editPointerId_ >= 0 && player_->Mining() &&
        elapsedSeconds_ - editDownSeconds_ >= player::mining::kTapSeconds) {
        frame.mineProgress = player_->MiningProgress();
    /* 罗盘: 朝向直接取玩家控制器, 与视线拾取同一来源. */
    frame.yawRadians = player_->Yaw();
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
            "view_distance=%d look=%.1f vol=%d autojump=%d mode=%s pointers=%d stick=%.2f,%.2f "
            "mobs=%d awake=%d hp=%.0f bag=%d/%d bag_open=%d discard=%d",
            world_->LoadedChunkCount(), static_cast<int>(world_->VisibleChunks().size()),
            renderer_->DrawnChunks(), renderer_->DrawnTriangles(), position.x, position.y, position.z,
            player_->Yaw() * 57.29578f, player_->Pitch() * 57.29578f, viewDistanceChunks_,
            settings_->LookDegreesPerScreenWidth(), static_cast<int>(settings_->Get(settings::Key::kVolume)),
            settings_->AutoJump() ? 1 : 0,
            settings_->ClassicalMoveMode() ? "classical" : "rocker",
            lastInput_.pointerCount, static_cast<double>(stick.valueX),
            static_cast<double>(stick.valueY), entities_.Count(), entities_.AwakeCount(),
            static_cast<double>(player_->Health()), inventory_.UsedSlots(),
            Inventory::kSlotCount, inventoryScreen_.Open() ? 1 : 0, inventory_.Discarded());
}

}  /* namespace immirror::game */