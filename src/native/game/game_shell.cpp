#include "game/game_shell.h"

#include <cmath>
#include <cstdio>

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

/* 视距: 以区块列为单位, 4 表示 9x9 列. */
constexpr int32_t kViewDistanceChunks = 4;

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

void GameShell::OnSurfaceReady(AAssetManager* assets) {
    if (surfaceReady_) {
        return;
    }
    if (!renderer_->Initialize(assets)) {
        IM_LOGE(kTag, "chunk renderer initialize failed");
        return;
    }
    surfaceReady_ = true;
    /* 先让中心区块生成地形, 再按地表高度放置玩家, 否则出生点会落在虚空. */
    world_->Update(core::Vec3{8.0f, 0.0f, 8.0f}, kViewDistanceChunks);
    player_->Spawn(*world_, 8.0f, 8.0f);
    world_->Update(player_->Position(), kViewDistanceChunks);
    IM_LOGI(kTag, "game shell ready, chunks=%d", world_->LoadedChunkCount());
}

void GameShell::OnSurfaceLost() {
    if (!surfaceReady_) {
        return;
    }
    renderer_->Shutdown();
    surfaceReady_ = false;
    IM_LOGI(kTag, "game shell surface released");
}

void GameShell::SetViewportSize(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
}

void GameShell::Update(double deltaSeconds, const platform::InputSnapshot& input) {
    if (!surfaceReady_) {
        return;
    }
    elapsedSeconds_ += deltaSeconds;
    const core::Vec3 position = player_->Position();
    world_->Update(position, kViewDistanceChunks);
    player_->Update(deltaSeconds, input, *world_);
    HandleTapActions(input);
    LogStatus(deltaSeconds);
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
        /* 编辑动作只在右半屏生效, 左半屏留给移动摇杆与跳跃, 避免手势互相干扰. */
        const bool editZone = pointerDownX_ > 0.5f;
        if (editZone && moved < kTapMoveLimit) {
            if (held < kLongPressSeconds) {
                if (player_->BreakBlock(*world_)) {
                    IM_LOGI(kTag, "block broken");
                }
            } else {
                /* 长按放置: 以草方块为默认材料, 后续接入物品栏再改. */
                if (player_->PlaceBlock(*world_, voxel::block::Grass)) {
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
