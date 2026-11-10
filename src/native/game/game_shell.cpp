#include "game/game_shell.h"

#include <sys/stat.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "core/vec3.h"
#include "player/player_controller.h"
#include "voxel/block_registry.h"
#include "platform/input_state.h"
#include "render/chunk_renderer.h"
#include "world/world.h"

namespace immirror::game {
namespace {

constexpr const char* kTag = "ImMirrorGame";

/* 视距: 以区块列为单位, 按设备分级在 2 到 4 之间取值. */
constexpr int32_t kViewDistanceHigh = 4;
constexpr int32_t kViewDistanceMedium = 3;
constexpr int32_t kViewDistanceLow = 2;

/* 相机参数. */
constexpr float kFovYDegrees = 70.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 320.0f;
constexpr float kEyeHeight = 1.62f;

/* 短按破坏与长按放置的判定阈值. */
constexpr double kLongPressSeconds = 0.5;
constexpr float kTapMoveLimit = 0.03f;

/* 世界种子, 固定值便于复现地形. */
constexpr uint32_t kWorldSeed = 20260101u;

/* 自动存档间隔. */
constexpr double kAutoSaveSeconds = 20.0;

/* 底部方块栏依次对应的方块, 与界面层图集格顺序一致. */
const voxel::BlockId kSlotBlocks[ui::HudRenderer::kSlotCount] = {
    voxel::block::Grass, voxel::block::Dirt,      voxel::block::Stone, voxel::block::Sand,
    voxel::block::Wood,  voxel::block::Leaves,    voxel::block::Brick, voxel::block::Cobblestone,
    voxel::block::Snow,
};

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
      renderer_(std::make_unique<render::ChunkRenderer>()) {}

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
    PrepareSavePath(dataPath);

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
    IM_LOGI(kTag, "game shell ready, chunks=%d", world_->LoadedChunkCount());
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
    /* 平台侧分级取值为 high / mid / low, 同时兼容 medium 写法. */
    if (std::strcmp(qualityGrade_, "low") == 0) {
        viewDistanceChunks_ = kViewDistanceLow;
    } else if (std::strcmp(qualityGrade_, "mid") == 0 || std::strcmp(qualityGrade_, "medium") == 0) {
        viewDistanceChunks_ = kViewDistanceMedium;
    } else {
        viewDistanceChunks_ = kViewDistanceHigh;
    }
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
    const core::Vec3 position = player_->Position();
    world_->Update(position, viewDistanceChunks_);
    player_->Update(deltaSeconds, input, *world_);
    HandleTapActions(input);
    LogStatus(deltaSeconds);

    /* 定期落盘, 避免进程被系统回收时丢失进度. */
    saveAccumulator_ += deltaSeconds;
    if (saveAccumulator_ >= kAutoSaveSeconds) {
        saveAccumulator_ = 0.0;
        SaveWorld();
    }
}

void GameShell::HandleTapActions(const platform::InputSnapshot& input) {
    const bool down = input.pointerDown;
    if (down && !previousPointerDown_) {
        pointerDownSeconds_ = elapsedSeconds_;
        pointerDownX_ = input.pointerX;
        pointerDownY_ = input.pointerY;
    } else if (!down && previousPointerDown_) {
        const double held = elapsedSeconds_ - pointerDownSeconds_;
        const float dx = input.pointerX - pointerDownX_;
        const float dy = input.pointerY - pointerDownY_;
        const float moved = std::sqrt(dx * dx + dy * dy);
        /* 方块栏优先: 落在栏位内即切换当前方块, 不触发破坏或放置. */
        const int32_t slot = moved < kTapMoveLimit ? hud_.HitTestSlot(pointerDownX_, pointerDownY_) : -1;
        if (slot >= 0) {
            selectedSlot_ = slot;
            selectedBlock_ = kSlotBlocks[slot];
            IM_LOGI(kTag, "block slot %d selected, id=%u", slot, static_cast<uint32_t>(selectedBlock_));
            previousPointerDown_ = input.pointerDown;
            return;
        }
        /* 编辑动作只在右半屏生效, 左半屏留给移动摇杆与跳跃, 避免手势互相干扰. */
        const bool editZone = pointerDownX_ > 0.5f;
        if (editZone && moved < kTapMoveLimit) {
            if (held < kLongPressSeconds) {
                if (player_->BreakBlock(*world_)) {
                    audio::AudioEngine::Instance().Play(audio::SoundId::kDigGrass);
                    IM_LOGI(kTag, "block broken");
                }
            } else {
                /* 长按放置: 使用方块栏当前选中的材料. */
                if (player_->PlaceBlock(*world_, selectedBlock_)) {
                    audio::AudioEngine::Instance().Play(audio::SoundId::kPlaceGrass);
                    IM_LOGI(kTag, "block placed");
                }
            }
        }
    }
    previousPointerDown_ = down;
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
    const float camera[3] = {position.x, position.y + kEyeHeight, position.z};
    renderer_->SetViewProjection(viewProjection, camera);
    renderer_->Sync(*world_);
    renderer_->Draw();

    /* 摇杆只在左半屏按下时显示, 量值由触点相对摇杆中心的偏移近似. */
    float stickX = 0.0f;
    float stickY = 0.0f;
    const bool stickActive = lastInput_.pointerDown && lastInput_.pointerX < 0.5f;
    if (stickActive) {
        stickX = (lastInput_.pointerX - 0.26f) / 0.14f;
        stickY = (0.58f - lastInput_.pointerY) / 0.14f;
        stickX = stickX < -1.0f ? -1.0f : (stickX > 1.0f ? 1.0f : stickX);
        stickY = stickY < -1.0f ? -1.0f : (stickY > 1.0f ? 1.0f : stickY);
    }
    hud_.Render(stickX, stickY, stickActive, selectedSlot_);
}

void GameShell::LogStatus(double deltaSeconds) {
    logAccumulator_ += deltaSeconds;
    if (logAccumulator_ < 2.0) {
        return;
    }
    logAccumulator_ = 0.0;
    const core::Vec3 position = player_->Position();
    IM_LOGI(kTag, "world chunks=%d visible=%d drawn=%d triangles=%d pos=%.2f,%.2f,%.2f",
            world_->LoadedChunkCount(), static_cast<int>(world_->VisibleChunks().size()),
            renderer_->DrawnChunks(), renderer_->DrawnTriangles(), position.x, position.y, position.z);
}

}  /* namespace immirror::game */