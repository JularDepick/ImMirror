#include "editor/editor_shell.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "core/log.h"
#include "editor/editor_bindings.h"
#include "editor/editor_session.h"
#include "player/input_tuning.h"
#include "player/mining_tuning.h"
#include "player/player_controller.h"
#include "render/chunk_renderer.h"
#include "settings/settings.h"
#include "settings/settings_tuning.h"
#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditor";

/*
 * 相机参数与游戏总装保持一致: 同样的视场角与裁剪面, 编辑里看到的构图
 * 与试玩时完全一致, 不需要另调一套手感.
 */
constexpr float kFovYDegrees = 75.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 320.0f;

/* 编辑手势的落点分区: 右半屏用于编辑与视角, 左半屏留给移动摇杆. */
constexpr float kEditZoneSplitX = 0.5f;

/* 轻点判定: 按住时长与累计位移上限, 与游戏的轻点窗口取同一常量. */
constexpr float kTapMoveLimit = 0.03f;

/* 提示文本的显示秒数. */
constexpr double kHintSeconds = 2.4f;

/* 状态文本与面板文本的最大长度. */
constexpr int32_t kStatusTextLength = 96;
constexpr int32_t kValueTextLength = 32;

/* 面板行的编辑语义, 与界面按钮的绘制顺序无关, 只描述这一行改的是什么. */
enum RowKind {
    kRowNone = 0,
    kRowTriggerIndex = 1,
    kRowEnabled = 2,
    kRowEvent = 3,
    kRowCenter = 4,
    kRowSizeX = 5,
    kRowSizeY = 6,
    kRowSizeZ = 7,
    kRowBrokenBlock = 8,
    kRowInterval = 9,
    kRowActionType = 10,
    kRowActionValue = 11,
    kRowActionCount = 12,
    kRowSelectBlock = 13,
};

/* 日志动作可选的文本: 原生界面没有软键盘, 文本在这里循环, 精确编辑走工程 JSON. */
const char* const kLogPresets[] = {
    "trigger fired", "welcome", "region entered", "block broken", "teleport",
};
constexpr int32_t kLogPresetCount = 5;

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

/* 指针 id 到位图位号, 越界 id 视为永远未按下. */
uint64_t BitOf(int32_t pointerId) {
    if (pointerId < 0 || pointerId >= 64) {
        return 0ull;
    }
    return 1ull << pointerId;
}

bool IsAir(voxel::BlockId id) {
    return id == voxel::block::Air;
}

void MakePerspective(float* out, float fovYDegrees, float aspect, float nearPlane, float farPlane) {
    const float radians = fovYDegrees * 3.14159265358979f / 180.0f;
    const float f = 1.0f / std::tan(radians * 0.5f);
    for (int32_t index = 0; index < 16; ++index) {
        out[index] = 0.0f;
    }
    out[0] = f / (aspect > 0.0001f ? aspect : 1.0f);
    out[5] = f;
    out[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    out[11] = -1.0f;
    out[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

/* 列主序 4x4 相乘: out = a * b, 与游戏总装同一实现. */
void Multiply4x4(const float* a, const float* b, float* out) {
    for (int32_t col = 0; col < 4; ++col) {
        for (int32_t row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int32_t k = 0; k < 4; ++k) {
                sum += a[k * 4 + row] * b[col * 4 + k];
            }
            out[col * 4 + row] = sum;
        }
    }
}

/* 取工程目录的最后一段作为标题里的工程标识, 目录名只含安全字符. */
std::string DirectoryStem(const std::string& directory) {
    if (directory.empty()) {
        return std::string("project");
    }
    const size_t slash = directory.find_last_of('/');
    return slash == std::string::npos ? directory : directory.substr(slash + 1);
}

}  /* namespace */

EditorShell::EditorShell() = default;

EditorShell::~EditorShell() = default;

bool EditorShell::Attach(const EditorContext& context, const std::string& projectDirectory,
                         std::string& error) {
    if (context.world == nullptr || context.player == nullptr || context.renderer == nullptr) {
        error = "编辑器缺少世界, 玩家或渲染器";
        return false;
    }
    context_ = context;
    if (!project_.OpenOrCreate(projectDirectory, "未命名工程", limits::kDefaultSeed, error)) {
        return false;
    }

    /* 世界种子跟随工程: 工程只存与地形不同的方块, 生成规则必须一致. */
    context_.world->SetSeed(project_.Seed());
    viewDistanceChunks_ = std::clamp(project_.Meta().viewDistance, 2, 6);
    if (context_.viewDistanceChunks > 0) {
        viewDistanceChunks_ = std::clamp(context_.viewDistanceChunks, 2, 6);
    }

    host_ = std::make_unique<TriggerHost>(*context_.world, project_, *context_.player);
    runtime_ = std::make_unique<TriggerRuntime>(project_, *host_);
    runtime_->Sync();

    selectedPalette_ =
        PaletteIndexOf(static_cast<voxel::BlockId>(project_.Meta().selectedBlock));
    if (selectedPalette_ < 0) {
        selectedPalette_ = 0;
    }

    RespawnAtProjectSpawn();
    context_.world->Update(context_.player->Position(), viewDistanceChunks_);
    project_.ProjectToWorld(*context_.world, limits::kProjectionBudgetPerFrame);

    hud_.SetViewport(width_, height_);
    if (!hud_.Initialize(context_.renderer->AtlasTexture())) {
        error = "编辑器界面初始化失败";
        return false;
    }
    ApplyControlSettings();
    active_ = true;
    BuildFrame();
    IM_LOGI(kTag, "editor attached, dir=%s seed=%u chunks=%d triggers=%d",
            project_.Directory().c_str(), project_.Seed(), project_.ChunkCount(),
            static_cast<int32_t>(project_.Triggers().size()));
    return true;
}

void EditorShell::Detach() {
    if (!active_) {
        return;
    }
    SaveNow();
    hud_.Shutdown();
    runtime_.reset();
    host_.reset();
    active_ = false;
    IM_LOGI(kTag, "editor detached");
}

void EditorShell::SetViewport(int32_t width, int32_t height) {
    width_ = width;
    height_ = height;
    hud_.SetViewport(width, height);
}

bool EditorShell::SaveNow() {
    if (!active_) {
        return false;
    }
    std::string error;
    /* 选中方块与视距随工程保存, 下次进入保持一致. */
    project_.MutableMeta().selectedBlock = static_cast<int32_t>(PaletteBlock(selectedPalette_));
    project_.MutableMeta().viewDistance = viewDistanceChunks_;
    if (!project_.Save(error)) {
        lastError_ = error;
        IM_LOGE(kTag, "project save failed: %s", error.c_str());
        ShowHint("SAVE FAILED");
        return false;
    }
    ShowHint("PROJECT SAVED");
    return true;
}

void EditorShell::ApplyControlSettings() {
    if (context_.settings == nullptr) {
        return;
    }
    player::LookTuning look;
    look.sensitivity = player::LookTuning::kReferenceSensitivity * context_.settings->LookFactor();
    look.invertY = context_.settings->ReversalY();
    context_.player->SetLookTuning(look);

    player::StickTuning stick;
    stick.radius = settings::tuning::kPadRadius;
    stick.fixedAnchor = context_.settings->ClassicalMoveMode();
    stick.anchorX = settings::tuning::kPadCenterX;
    stick.anchorY = settings::tuning::kPadCenterY;
    stick.leftHanded = context_.settings->LeftHanded();
    context_.player->SetStickTuning(stick);
}

void EditorShell::RespawnAtProjectSpawn() {
    const ProjectMeta& meta = project_.Meta();
    context_.player->Spawn(*context_.world, meta.spawnX, meta.spawnZ);
    context_.player->SetPose(core::Vec3{meta.spawnX, context_.player->Position().y, meta.spawnZ},
                             meta.yaw, meta.pitch);
}

core::Vec3 EditorShell::EyePosition() const {
    const core::Vec3 feet = context_.player->Position();
    /* 视点高度与游戏控制器一致 (1.62 格). */
    return core::Vec3{feet.x, feet.y + 1.62f, feet.z};
}

core::Vec3 EditorShell::LookDirection() const {
    return context_.player->LookDirection();
}

bool EditorShell::Pick(const world::World& world, const core::Vec3& origin,
                       const core::Vec3& direction, float maxDistance, voxel::BlockPos& block,
                       voxel::BlockPos& face) const {
    block = voxel::BlockPos{};
    face = voxel::BlockPos{};
    int32_t x = FloorToInt(origin.x);
    int32_t y = FloorToInt(origin.y);
    int32_t z = FloorToInt(origin.z);
    /* 视点在实心方块内部时放弃拾取, 与游戏一致. */
    if (voxel::BlockRegistry::Instance().Info(world.GetBlock(x, y, z)).solid) {
        return false;
    }
    const int32_t stepX = direction.x > 0.0f ? 1 : (direction.x < 0.0f ? -1 : 0);
    const int32_t stepY = direction.y > 0.0f ? 1 : (direction.y < 0.0f ? -1 : 0);
    const int32_t stepZ = direction.z > 0.0f ? 1 : (direction.z < 0.0f ? -1 : 0);
    constexpr float kInfinity = 1.0e30f;
    const float deltaX = stepX != 0 ? 1.0f / std::fabs(direction.x) : kInfinity;
    const float deltaY = stepY != 0 ? 1.0f / std::fabs(direction.y) : kInfinity;
    const float deltaZ = stepZ != 0 ? 1.0f / std::fabs(direction.z) : kInfinity;
    float nextX = stepX > 0 ? (static_cast<float>(x + 1) - origin.x) * deltaX
                            : (stepX < 0 ? (origin.x - static_cast<float>(x)) * deltaX : kInfinity);
    float nextY = stepY > 0 ? (static_cast<float>(y + 1) - origin.y) * deltaY
                            : (stepY < 0 ? (origin.y - static_cast<float>(y)) * deltaY : kInfinity);
    float nextZ = stepZ > 0 ? (static_cast<float>(z + 1) - origin.z) * deltaZ
                            : (stepZ < 0 ? (origin.z - static_cast<float>(z)) * deltaZ : kInfinity);
    for (int32_t step = 0; step < limits::kRaySteps; ++step) {
        float distance = 0.0f;
        voxel::BlockPos stepFace{};
        if (nextX <= nextY && nextX <= nextZ) {
            x += stepX;
            distance = nextX;
            nextX += deltaX;
            stepFace = {-stepX, 0, 0};
        } else if (nextY <= nextZ) {
            y += stepY;
            distance = nextY;
            nextY += deltaY;
            stepFace = {0, -stepY, 0};
        } else {
            z += stepZ;
            distance = nextZ;
            nextZ += deltaZ;
            stepFace = {0, 0, -stepZ};
        }
        if (distance > maxDistance) {
            return false;
        }
        /*
         * 编辑器命中任意非空气方块: 水与花草不是实心方块,
         * 但作者需要能选中它们并删除, 因此这里比游戏的拾取放宽一档.
         */
        const voxel::BlockId hit = world.GetBlock(x, y, z);
        if (!IsAir(hit)) {
            block = voxel::BlockPos{x, y, z};
            face = stepFace;
            return true;
        }
    }
    return false;
}

bool EditorShell::WriteBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id) {
    if (y < 0 || y >= voxel::kWorldHeight) {
        return false;
    }
    const bool stored = project_.SetBlock(x, y, z, id);
    context_.world->SetBlock(x, y, z, id);
    return stored;
}

void EditorShell::UndoApply(void* user, int32_t x, int32_t y, int32_t z, voxel::BlockId id) {
    auto* shell = static_cast<EditorShell*>(user);
    if (shell != nullptr) {
        shell->WriteBlock(x, y, z, id);
    }
}

bool EditorShell::ApplySingle(voxel::BlockPos target, voxel::BlockId id, const char* label) {
    if (target.y < 0 || target.y >= voxel::kWorldHeight) {
        return false;
    }
    const voxel::BlockId before = project_.GetBlock(target.x, target.y, target.z);
    if (before == id) {
        return false;
    }
    WriteBlock(target.x, target.y, target.z, id);
    EditBatch batch;
    batch.label = label;
    batch.edits.push_back(BlockEdit{target.x, target.y, target.z, before, id});
    undo_.Push(std::move(batch));
    return true;
}

bool EditorShell::ApplyFill(const voxel::BlockPos& first, const voxel::BlockPos& second,
                            voxel::BlockId id) {
    const int32_t minX = std::min(first.x, second.x);
    const int32_t maxX = std::max(first.x, second.x);
    const int32_t minY = std::max(0, std::min(first.y, second.y));
    const int32_t maxY = std::min(voxel::kWorldHeight - 1, std::max(first.y, second.y));
    const int32_t minZ = std::min(first.z, second.z);
    const int32_t maxZ = std::max(first.z, second.z);
    const int64_t volume = static_cast<int64_t>(maxX - minX + 1) * static_cast<int64_t>(maxY - minY + 1) *
                           static_cast<int64_t>(maxZ - minZ + 1);
    if (volume <= 0 || volume > limits::kMaxFillBlocks) {
        ShowHint("FILL TOO BIG (%lld)", static_cast<long long>(volume));
        return false;
    }
    EditBatch batch;
    batch.label = "fill";
    batch.edits.reserve(static_cast<size_t>(volume));
    for (int32_t y = minY; y <= maxY; ++y) {
        for (int32_t z = minZ; z <= maxZ; ++z) {
            for (int32_t x = minX; x <= maxX; ++x) {
                const voxel::BlockId before = project_.GetBlock(x, y, z);
                if (before == id) {
                    continue;
                }
                batch.edits.push_back(BlockEdit{x, y, z, before, id});
            }
        }
    }
    if (batch.edits.empty()) {
        ShowHint("FILL CHANGED NOTHING");
        return false;
    }
    for (const BlockEdit& edit : batch.edits) {
        WriteBlock(edit.x, edit.y, edit.z, edit.after);
    }
    const int32_t count = static_cast<int32_t>(batch.edits.size());
    undo_.Push(std::move(batch));
    ShowHint("FILLED %d BLOCKS", count);
    return true;
}

void EditorShell::SetPlaytest(bool enabled) {
    if (enabled) {
        /* 试玩前先落盘, 保证工程数据与画面一致. */
        SaveNow();
        mode_ = EditorMode::kPlaytest;
        RespawnAtProjectSpawn();
        runtime_->Reset();
        runtime_->Sync();
        /* 首帧只记录区域状态, 不立刻触发, 从出生点开始才算真正进入. */
        runtime_->Tick(0.0, context_.player->Position());
        ShowHint("PLAYTEST MODE");
    } else {
        mode_ = EditorMode::kEdit;
        context_.player->CancelMining();
        editPointerId_ = -1;
        fillHasFirst_ = false;
        runtime_->Reset();
        runtime_->Sync();
        ShowHint("BACK TO EDIT");
    }
    IM_LOGI(kTag, "editor mode=%s", mode_ == EditorMode::kPlaytest ? "playtest" : "edit");
}

bool EditorShell::Undo() {
    if (!undo_.Undo(this, &EditorShell::UndoApply)) {
        ShowHint("NOTHING TO UNDO");
        return false;
    }
    ShowHint("UNDO");
    return true;
}

bool EditorShell::Redo() {
    if (!undo_.Redo(this, &EditorShell::UndoApply)) {
        ShowHint("NOTHING TO REDO");
        return false;
    }
    ShowHint("REDO");
    return true;
}

bool EditorShell::SetTool(int32_t tool) {
    if (tool < 0 || tool > 2) {
        return false;
    }
    tool_ = static_cast<EditTool>(tool);
    fillHasFirst_ = false;
    return true;
}

bool EditorShell::SetPaletteIndex(int32_t index) {
    if (index < 0 || index >= PaletteCount()) {
        return false;
    }
    selectedPalette_ = index;
    return true;
}

bool EditorShell::ExportWorld(const std::string& path, std::string& error) {
    if (!active_) {
        error = "编辑器未启动";
        return false;
    }
    return project_.ExportWorld(path, error);
}

int32_t EditorShell::AddTrigger() {
    Trigger* trigger = project_.AddTrigger();
    if (trigger == nullptr) {
        ShowHint("TRIGGER LIMIT REACHED");
        return -1;
    }
    activeTriggerId_ = trigger->id;
    panelVisible_ = true;
    runtime_->Sync();
    ShowHint("TRIGGER %d ADDED", activeTriggerId_);
    return activeTriggerId_;
}

bool EditorShell::DeleteTrigger(int32_t id) {
    if (!project_.RemoveTrigger(id)) {
        ShowHint("NO TRIGGER SELECTED");
        return false;
    }
    if (activeTriggerId_ == id) {
        activeTriggerId_ = -1;
    }
    runtime_->Sync();
    ShowHint("TRIGGER DELETED");
    return true;
}

bool EditorShell::TargetBlock(voxel::BlockPos& out) const {
    if (!hasTarget_) {
        return false;
    }
    out = targetBlock_;
    return true;
}

Trigger* EditorShell::ActiveTrigger() {
    if (activeTriggerId_ < 0) {
        return nullptr;
    }
    return project_.FindTrigger(activeTriggerId_);
}

void EditorShell::TouchTrigger(Trigger& trigger) {
    trigger.RebuildScript();
    project_.MarkDirty();
    runtime_->Sync();
}

std::string EditorShell::DescribeState() const {
    /* Java 侧工程列表与调试日志使用的一行摘要, 分隔符固定. */
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                  "dir=%s chunks=%d edits=%lld triggers=%d undo=%d mode=%s", project_.Directory().c_str(),
                  project_.ChunkCount(), static_cast<long long>(project_.ChangedBlockCount()),
                  static_cast<int32_t>(project_.Triggers().size()), undo_.UndoDepth(),
                  mode_ == EditorMode::kPlaytest ? "playtest" : "edit");
    return buffer;
}

void EditorShell::ShowHint(const char* format, ...) {
    char buffer[kStatusTextLength];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::snprintf(hint_, sizeof(hint_), "%s", buffer);
    hintRemainSeconds_ = kHintSeconds;
}

void EditorShell::TickHint(double deltaSeconds) {
    if (hintRemainSeconds_ <= 0.0) {
        hint_[0] = '\0';
        return;
    }
    hintRemainSeconds_ -= deltaSeconds;
    if (hintRemainSeconds_ <= 0.0) {
        hintRemainSeconds_ = 0.0;
        hint_[0] = '\0';
    }
}

void EditorShell::PushRow(const char* label, const char* value, bool steppable, int32_t kind,
                          int32_t extra) {
    if (rowCount_ >= limits::kPanelRows) {
        return;
    }
    EditorRowState& row = rows_[rowCount_];
    row.label = label;
    std::snprintf(row.value, sizeof(row.value), "%s", value);
    row.steppable = steppable;
    rowKind_[rowCount_] = kind;
    rowExtra_[rowCount_] = extra;
    ++rowCount_;
}

void EditorShell::PushRowInt(const char* label, int32_t value, bool steppable, int32_t kind,
                             int32_t extra) {
    char text[kValueTextLength];
    std::snprintf(text, sizeof(text), "%d", value);
    PushRow(label, text, steppable, kind, extra);
}

void EditorShell::PushRowFloat(const char* label, float value, bool steppable, int32_t kind,
                               int32_t extra) {
    char text[kValueTextLength];
    std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(value));
    PushRow(label, text, steppable, kind, extra);
}

void EditorShell::FillButtons() {
    /* 按钮顺序固定, 试玩时只保留存档, 撤销, 重做与退出, 其余隐藏. */
    int32_t count = 0;
    const auto add = [&](EditorButton id, const char* label, bool active) {
        if (count >= static_cast<int32_t>(sizeof(buttons_) / sizeof(buttons_[0]))) {
            return;
        }
        buttons_[count].id = id;
        buttons_[count].label = label;
        buttons_[count].active = active;
        ++count;
    };
    add(EditorButton::kSave, "SAVE", false);
    add(EditorButton::kUndo, "UNDO", false);
    add(EditorButton::kRedo, "REDO", false);
    if (mode_ == EditorMode::kEdit) {
        const char* toolLabel = tool_ == EditTool::kPlace
                                    ? "MODE PLACE"
                                    : (tool_ == EditTool::kRemove ? "MODE REMOVE" : "MODE FILL");
        add(EditorButton::kFill, toolLabel, tool_ != EditTool::kPlace);
        add(EditorButton::kPanel, "PROPS", panelVisible_);
        add(EditorButton::kAddTrigger, "TRIG+", false);
        add(EditorButton::kTestTrigger, "TRIG RUN", false);
        add(EditorButton::kDeleteTrigger, "TRIG DEL", false);
        add(EditorButton::kPlay, "PLAY", false);
    } else {
        add(EditorButton::kPlay, "STOP", true);
    }
    add(EditorButton::kBack, "EXIT", false);
    buttonCount_ = count;
}

void EditorShell::BuildStatusLines() {
    char line[kStatusTextLength];
    int32_t index = 0;
    std::snprintf(title_, sizeof(title_), "EDITOR %s  %s",
                  DirectoryStem(project_.Directory()).c_str(),
                  mode_ == EditorMode::kPlaytest ? "PLAYTEST" : "EDIT");

    std::snprintf(line, sizeof(line), "TOOL %s  PALETTE %s",
                  tool_ == EditTool::kPlace ? "PLACE" : (tool_ == EditTool::kRemove ? "REMOVE" : "FILL"),
                  PaletteKey(selectedPalette_));
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    const core::Vec3 feet = context_.player->Position();
    std::snprintf(line, sizeof(line), "POS %.1f %.1f %.1f YAW %.0f", static_cast<double>(feet.x),
                  static_cast<double>(feet.y), static_cast<double>(feet.z),
                  static_cast<double>(context_.player->Yaw() * 57.29578f));
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    if (hasTarget_) {
        const voxel::BlockId id =
            context_.world->GetBlock(targetBlock_.x, targetBlock_.y, targetBlock_.z);
        const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(id);
        std::snprintf(line, sizeof(line), "TARGET %d %d %d %s h=%.1f", targetBlock_.x,
                      targetBlock_.y, targetBlock_.z, info.name,
                      static_cast<double>(info.hardness));
    } else {
        std::snprintf(line, sizeof(line), "TARGET none");
    }
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    std::snprintf(line, sizeof(line), "CHUNKS %d EDITS %lld", project_.ChunkCount(),
                  static_cast<long long>(project_.ChangedBlockCount()));
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    std::snprintf(line, sizeof(line), "UNDO %d REDO %d  TRIGGERS %d FIRES %d",
                  undo_.UndoDepth(), undo_.RedoDepth(),
                  static_cast<int32_t>(project_.Triggers().size()),
                  runtime_ != nullptr ? runtime_->TotalFires() : 0);
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    if (fillHasFirst_) {
        std::snprintf(line, sizeof(line), "FILL A %d %d %d  PICK B", fillFirst_.x, fillFirst_.y,
                      fillFirst_.z);
    } else {
        std::snprintf(line, sizeof(line), "FILL idle  SEED %u VIEW %d", project_.Seed(),
                      viewDistanceChunks_);
    }
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    if (runtime_ != nullptr && !runtime_->LastError().empty()) {
        std::snprintf(line, sizeof(line), "SCRIPT ERR %s", runtime_->LastError().c_str());
    } else if (runtime_ != nullptr && !runtime_->LastLabel().empty()) {
        std::snprintf(line, sizeof(line), "LAST TRIGGER %s", runtime_->LastLabel().c_str());
    } else if (!lastError_.empty()) {
        std::snprintf(line, sizeof(line), "ERR %s", lastError_.c_str());
    } else {
        std::snprintf(line, sizeof(line), "READY");
    }
    std::snprintf(statusLines_[index++], kStatusTextLength, "%s", line);

    frame_.statusLineCount = index;
    for (int32_t i = 0; i < index; ++i) {
        statusPointers_[i] = statusLines_[i];
    }
    for (int32_t i = 0; i < limits::kStatusLines; ++i) {
        frame_.statusLines[i] = statusPointers_[i];
    }
}

void EditorShell::BuildPanelRows() {
    rowCount_ = 0;
    Trigger* trigger = ActiveTrigger();
    const bool triggerPanel = panelVisible_ && trigger != nullptr;

    if (triggerPanel) {
        std::snprintf(panelTitle_, sizeof(panelTitle_), "TRIGGER %d/%d", trigger->id,
                      static_cast<int32_t>(project_.Triggers().size()));
        /* 第一行用于在已有触发器之间切换, 不需要先退出面板. */
        char indexText[kValueTextLength];
        std::snprintf(indexText, sizeof(indexText), "%d", trigger->id);
        PushRow("SELECT", indexText, true, kRowTriggerIndex, 0);
        PushRow("ENABLED", trigger->enabled ? "ON" : "OFF", true, kRowEnabled, 0);
        PushRow("EVENT", TriggerEventNameEn(trigger->eventType), true, kRowEvent, 0);
        switch (trigger->eventType) {
            case TriggerEventType::kEnterRegion: {
                char center[kValueTextLength];
                std::snprintf(center, sizeof(center), "%d,%d,%d", trigger->region.cx,
                              trigger->region.cy, trigger->region.cz);
                PushRow("CENTER", center, true, kRowCenter, 0);
                PushRowInt("SIZE X", trigger->region.sx, true, kRowSizeX, 0);
                PushRowInt("SIZE Y", trigger->region.sy, true, kRowSizeY, 0);
                PushRowInt("SIZE Z", trigger->region.sz, true, kRowSizeZ, 0);
                break;
            }
            case TriggerEventType::kBlockBroken: {
                const int32_t paletteIndex = PaletteIndexOf(trigger->brokenBlock);
                PushRow("BLOCK", trigger->brokenBlock == 0 ? "ANY" : PaletteKey(paletteIndex),
                        true, kRowBrokenBlock, 0);
                break;
            }
            case TriggerEventType::kTimer:
                PushRowFloat("INTERVAL", trigger->intervalSeconds, true, kRowInterval, 0);
                break;
        }

        /* 动作逐条列出: 类型与参数各占一行, 最多显示两条以保证面板不溢出. */
        const int32_t actionCount = static_cast<int32_t>(trigger->actions.size());
        PushRowInt("ACTIONS", actionCount, true, kRowActionCount, 0);
        for (int32_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
            if (rowCount_ + 2 > limits::kPanelRows) {
                break;
            }
            const TriggerAction& action = trigger->actions[static_cast<size_t>(actionIndex)];
            char label[16];
            std::snprintf(label, sizeof(label), "ACT %d", actionIndex);
            PushRow(label, TriggerActionNameEn(action.type), true, kRowActionType, actionIndex);
            char valueLabel[16];
            std::snprintf(valueLabel, sizeof(valueLabel), "ARG %d", actionIndex);
            std::string description = action.DescribeAscii();
            if (description.size() > static_cast<size_t>(kValueTextLength - 1)) {
                description.resize(static_cast<size_t>(kValueTextLength - 1));
            }
            PushRow(valueLabel, description.c_str(), true, kRowActionValue, actionIndex);
        }
        return;
    }

    /* 信息面板: 显示拾取目标与工程规模, 同时提供方块选择行. */
    std::snprintf(panelTitle_, sizeof(panelTitle_), "PROPERTIES");
    if (hasTarget_) {
        const voxel::BlockId id =
            context_.world->GetBlock(targetBlock_.x, targetBlock_.y, targetBlock_.z);
        const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(id);
        PushRow("TARGET", info.name, false, kRowNone, 0);
        PushRowFloat("HARDNESS", info.hardness, false, kRowNone, 0);
        PushRowInt("TOOL", static_cast<int32_t>(info.tool), false, kRowNone, 0);
    } else {
        PushRow("TARGET", "none", false, kRowNone, 0);
    }
    PushRow("SEL BLOCK", PaletteKey(selectedPalette_), true, kRowSelectBlock, 0);
    PushRowInt("SEED", static_cast<int32_t>(project_.Seed()), false, kRowNone, 0);
    PushRowInt("CHUNKS", project_.ChunkCount(), false, kRowNone, 0);
    PushRowInt("EDITS", static_cast<int32_t>(project_.ChangedBlockCount()), false, kRowNone, 0);
    PushRowInt("TRIGGERS", static_cast<int32_t>(project_.Triggers().size()), false, kRowNone, 0);
    PushRowInt("UNDO DEPTH", undo_.UndoDepth(), false, kRowNone, 0);
    PushRowFloat("VIEW DIST", static_cast<float>(viewDistanceChunks_), false, kRowNone, 0);
}

void EditorShell::BuildFrame() {
    FillButtons();
    BuildStatusLines();
    BuildPanelRows();

    frame_.mode = mode_;
    frame_.title = title_;
    frame_.buttons = buttons_;
    frame_.buttonCount = buttonCount_;
    frame_.panelVisible = panelVisible_;
    frame_.panelTitle = panelTitle_;
    frame_.rows = rows_;
    frame_.rowCount = rowCount_;
    frame_.highlightedRow = highlightedRow_;
    frame_.selectedPalette = mode_ == EditorMode::kEdit ? selectedPalette_ : -1;
    frame_.hint = hint_[0] != '\0' ? hint_ : nullptr;
    frame_.drawTarget = hasTarget_;
    frame_.targetMin = targetBlock_;
    frame_.targetMax = targetBlock_;
    frame_.drawFill = fillHasFirst_;
    frame_.fillMin = voxel::BlockPos{std::min(fillFirst_.x, targetBlock_.x),
                                     std::min(fillFirst_.y, targetBlock_.y),
                                     std::min(fillFirst_.z, targetBlock_.z)};
    frame_.fillMax = voxel::BlockPos{std::max(fillFirst_.x, targetBlock_.x),
                                     std::max(fillFirst_.y, targetBlock_.y),
                                     std::max(fillFirst_.z, targetBlock_.z)};
}

void EditorShell::OnButton(EditorButton button) {
    switch (button) {
        case EditorButton::kSave:
            SaveNow();
            break;
        case EditorButton::kUndo:
            Undo();
            break;
        case EditorButton::kRedo:
            Redo();
            break;
        case EditorButton::kFill: {
            const int32_t next = (static_cast<int32_t>(tool_) + 1) % 3;
            tool_ = static_cast<EditTool>(next);
            fillHasFirst_ = false;
            ShowHint("TOOL %s", tool_ == EditTool::kPlace
                                    ? "PLACE"
                                    : (tool_ == EditTool::kRemove ? "REMOVE" : "FILL"));
            break;
        }
        case EditorButton::kPanel:
            panelVisible_ = !panelVisible_;
            if (panelVisible_ && ActiveTrigger() == nullptr && !project_.Triggers().empty()) {
                activeTriggerId_ = project_.Triggers().front().id;
            }
            break;
        case EditorButton::kPlay:
            SetPlaytest(!Playtest());
            break;
        case EditorButton::kBack:
            SaveNow();
            exitRequested_ = true;
            RequestExitToJava();
            break;
        case EditorButton::kAddTrigger:
            AddTrigger();
            break;
        case EditorButton::kTestTrigger:
            if (activeTriggerId_ < 0) {
                ShowHint("NO TRIGGER SELECTED");
                break;
            }
            runtime_->Sync();
            ShowHint(runtime_->FireOnce(activeTriggerId_) ? "TRIGGER RUN" : "TRIGGER FAILED");
            break;
        case EditorButton::kDeleteTrigger:
            DeleteTrigger(activeTriggerId_);
            break;
        case EditorButton::kNone:
            break;
    }
}

void EditorShell::OnPalette(int32_t index) {
    if (!SetPaletteIndex(index)) {
        return;
    }
    ShowHint("BLOCK %s", PaletteKey(selectedPalette_));
}

void EditorShell::OnRowDelta(int32_t row, int32_t delta) {
    if (row < 0 || row >= rowCount_) {
        return;
    }
    /* 行被点中但没有加减号时, 视为加号一次, 便于大按钮操作. */
    if (delta == 0) {
        delta = 1;
    }
    const int32_t kind = rowKind_[row];
    const int32_t extra = rowExtra_[row];
    highlightedRow_ = row;

    /* 信息面板的方块选择行. */
    if (kind == kRowSelectBlock) {
        SetPaletteIndex(PaletteWrap(selectedPalette_ + delta));
        return;
    }

    Trigger* trigger = ActiveTrigger();
    if (trigger == nullptr) {
        return;
    }

    switch (kind) {
        case kRowTriggerIndex: {
            const std::vector<Trigger>& triggers = project_.Triggers();
            if (triggers.empty()) {
                break;
            }
            int32_t index = 0;
            for (int32_t probe = 0; probe < static_cast<int32_t>(triggers.size()); ++probe) {
                if (triggers[static_cast<size_t>(probe)].id == trigger->id) {
                    index = probe;
                    break;
                }
            }
            index = (index + (delta > 0 ? 1 : -1) + static_cast<int32_t>(triggers.size())) %
                    static_cast<int32_t>(triggers.size());
            activeTriggerId_ = triggers[static_cast<size_t>(index)].id;
            break;
        }
        case kRowEnabled:
            trigger->enabled = !trigger->enabled;
            project_.MarkDirty();
            break;
        case kRowEvent: {
            const int32_t current = static_cast<int32_t>(trigger->eventType);
            const int32_t next = (current + (delta > 0 ? 1 : -1) + kEventTypeCount) % kEventTypeCount;
            trigger->eventType = TriggerEventAt(next);
            /* 切换事件时把默认参数摆到作者当前看得到的位置, 少一次手动输入. */
            if (trigger->eventType == TriggerEventType::kEnterRegion) {
                trigger->region = Region{};
                if (hasTarget_) {
                    trigger->region.cx = targetBlock_.x;
                    trigger->region.cy = targetBlock_.y;
                    trigger->region.cz = targetBlock_.z;
                }
                trigger->region.sx = 8;
                trigger->region.sy = 6;
                trigger->region.sz = 8;
            }
            project_.MarkDirty();
            break;
        }
        case kRowCenter:
            /* 加号取当前对准方块, 减号取玩家脚下: 两种常用落点各一键. */
            if (delta > 0 && hasTarget_) {
                trigger->region.cx = targetBlock_.x;
                trigger->region.cy = targetBlock_.y;
                trigger->region.cz = targetBlock_.z;
            } else if (delta < 0) {
                const core::Vec3 feet = context_.player->Position();
                trigger->region.cx = FloorToInt(feet.x);
                trigger->region.cy = FloorToInt(feet.y);
                trigger->region.cz = FloorToInt(feet.z);
            }
            project_.MarkDirty();
            break;
        case kRowSizeX:
            trigger->region.sx = std::max(1, trigger->region.sx + delta);
            project_.MarkDirty();
            break;
        case kRowSizeY:
            trigger->region.sy = std::max(1, trigger->region.sy + delta);
            project_.MarkDirty();
            break;
        case kRowSizeZ:
            trigger->region.sz = std::max(1, trigger->region.sz + delta);
            project_.MarkDirty();
            break;
        case kRowBrokenBlock: {
            /* ANY -> 调色板首项 -> ... -> 末项 -> ANY 循环. */
            const int32_t paletteIndex = PaletteIndexOf(trigger->brokenBlock);
            int32_t next = paletteIndex + delta;
            if (next < 0) {
                trigger->brokenBlock = 0;
            } else if (next >= PaletteCount()) {
                trigger->brokenBlock = 0;
            } else {
                trigger->brokenBlock = PaletteBlock(next);
            }
            project_.MarkDirty();
            break;
        }
        case kRowInterval: {
            const float step = delta > 0 ? 0.5f : -0.5f;
            trigger->intervalSeconds = std::clamp(trigger->intervalSeconds + step, 0.5f, 60.0f);
            project_.MarkDirty();
            break;
        }
        case kRowActionCount: {
            if (delta > 0) {
                if (static_cast<int32_t>(trigger->actions.size()) >= limits::kMaxActions) {
                    ShowHint("ACTION LIMIT");
                    break;
                }
                TriggerAction action;
                action.type = TriggerActionType::kLog;
                action.itemId = TriggerActionItemId(action.type);
                action.text = "trigger fired";
                trigger->actions.push_back(action);
            } else if (!trigger->actions.empty()) {
                trigger->actions.pop_back();
            }
            TouchTrigger(*trigger);
            break;
        }
        case kRowActionType: {
            if (extra < 0 || extra >= static_cast<int32_t>(trigger->actions.size())) {
                break;
            }
            TriggerAction& action = trigger->actions[static_cast<size_t>(extra)];
            const int32_t current = static_cast<int32_t>(action.type);
            const int32_t next =
                (current + (delta > 0 ? 1 : -1) + kActionTypeCount) % kActionTypeCount;
            action.type = TriggerActionAt(next);
            action.itemId = TriggerActionItemId(action.type);
            /* 换类型时把参数摆到当前上下文: 放置取对准方块, 传送取脚底. */
            const core::Vec3 feet = context_.player->Position();
            if (action.type == TriggerActionType::kSetBlock) {
                action.x = hasTarget_ ? targetBlock_.x : FloorToInt(feet.x);
                action.y = hasTarget_ ? targetBlock_.y : FloorToInt(feet.y) + 1;
                action.z = hasTarget_ ? targetBlock_.z : FloorToInt(feet.z);
                action.block = static_cast<int32_t>(PaletteBlock(selectedPalette_));
            } else if (action.type == TriggerActionType::kTeleport) {
                action.fx = feet.x;
                action.fy = feet.y + 1.0f;
                action.fz = feet.z;
            } else {
                action.text = kLogPresets[0];
            }
            TouchTrigger(*trigger);
            break;
        }
        case kRowActionValue: {
            if (extra < 0 || extra >= static_cast<int32_t>(trigger->actions.size())) {
                break;
            }
            TriggerAction& action = trigger->actions[static_cast<size_t>(extra)];
            switch (action.type) {
                case TriggerActionType::kLog: {
                    int32_t preset = 0;
                    for (int32_t index = 0; index < kLogPresetCount; ++index) {
                        if (action.text == kLogPresets[index]) {
                            preset = index;
                            break;
                        }
                    }
                    preset = (preset + (delta > 0 ? 1 : -1) + kLogPresetCount) % kLogPresetCount;
                    action.text = kLogPresets[preset];
                    break;
                }
                case TriggerActionType::kSetBlock: {
                    const int32_t paletteIndex = PaletteIndexOf(
                        static_cast<voxel::BlockId>(action.block));
                    const int32_t next =
                        PaletteWrap(paletteIndex < 0 ? 0 : paletteIndex + delta);
                    action.block = static_cast<int32_t>(PaletteBlock(next));
                    if (hasTarget_) {
                        action.x = targetBlock_.x;
                        action.y = targetBlock_.y;
                        action.z = targetBlock_.z;
                    }
                    break;
                }
                case TriggerActionType::kTeleport: {
                    const core::Vec3 feet = context_.player->Position();
                    action.fx = feet.x;
                    action.fy = feet.y + 1.0f;
                    action.fz = feet.z;
                    break;
                }
            }
            TouchTrigger(*trigger);
            break;
        }
        default:
            break;
    }
}

void EditorShell::HandleUiPointers(platform::InputSnapshot& input) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;
    if (pressed == 0ull) {
        return;
    }
    for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
        platform::PointerState& pointer = input.pointers[index];
        if (!pointer.down || pointer.uiClaimed) {
            continue;
        }
        if ((pressed & BitOf(pointer.id)) == 0ull) {
            continue;
        }
        const EditorHit hit = hud_.HitTest(frame_, pointer.x, pointer.y);
        if (hit.button != EditorButton::kNone) {
            pointer.uiClaimed = true;
            OnButton(hit.button);
            continue;
        }
        if (hit.row >= 0) {
            pointer.uiClaimed = true;
            OnRowDelta(hit.row, hit.rowDelta);
            continue;
        }
        if (hit.paletteIndex >= 0) {
            pointer.uiClaimed = true;
            OnPalette(hit.paletteIndex);
        }
    }
}

void EditorShell::UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds) {
    (void)deltaSeconds;
    const uint64_t pressed = input.downMask & ~previousDownMask_;
    const uint64_t released = previousDownMask_ & ~input.downMask;

    if (editPointerId_ < 0 && pressed != 0ull) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            const platform::PointerState& pointer = input.pointers[index];
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
        return;
    }
    const bool releasedNow = (released & BitOf(editPointerId_)) != 0ull;
    const int32_t pointerId = editPointerId_;
    editPointerId_ = -1;
    if (!releasedNow) {
        /* 指针在帧内被取消, 本次手势作废. */
        return;
    }
    const double held = elapsedSeconds_ - editDownSeconds_;
    const float movedX = editLastX_ - editDownX_;
    const float movedY = editLastY_ - editDownY_;
    const float moved = std::sqrt(movedX * movedX + movedY * movedY);
    if (held >= player::mining::kTapSeconds || moved >= kTapMoveLimit) {
        /* 拖动只用于转视角, 不结算编辑动作. */
        return;
    }
    IM_LOGD(kTag, "editor tap pointer=%d tool=%d", pointerId, static_cast<int32_t>(tool_));
    if (tool_ == EditTool::kFill) {
        if (!hasTarget_) {
            ShowHint("AIM AT A BLOCK");
            return;
        }
        if (!fillHasFirst_) {
            fillFirst_ = targetBlock_;
            fillHasFirst_ = true;
            ShowHint("CORNER A %d %d %d", fillFirst_.x, fillFirst_.y, fillFirst_.z);
            return;
        }
        const voxel::BlockPos second = targetBlock_;
        fillHasFirst_ = false;
        ApplyFill(fillFirst_, second, PaletteBlock(selectedPalette_));
        return;
    }
    if (!hasTarget_) {
        ShowHint("AIM AT A BLOCK");
        return;
    }
    if (tool_ == EditTool::kRemove) {
        if (ApplySingle(targetBlock_, voxel::block::Air, "remove")) {
            ShowHint("REMOVED");
        }
        return;
    }
    /* 放置沿命中面的外法线向外一格, 与游戏内的放置规则一致. */
    const voxel::BlockPos placed{targetBlock_.x + targetFace_.x, targetBlock_.y + targetFace_.y,
                                 targetBlock_.z + targetFace_.z};
    if (ApplySingle(placed, PaletteBlock(selectedPalette_), "place")) {
        ShowHint("PLACED %s", PaletteKey(selectedPalette_));
    }
}

void EditorShell::UpdatePlaytestGesture(platform::InputSnapshot& input, double deltaSeconds) {
    const uint64_t pressed = input.downMask & ~previousDownMask_;
    const uint64_t released = previousDownMask_ & ~input.downMask;

    if (editPointerId_ < 0 && pressed != 0ull) {
        for (int32_t index = 0; index < platform::kMaxPointers; ++index) {
            const platform::PointerState& pointer = input.pointers[index];
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
            /* 记住这次挖掘的目标格, 破坏后按同一格写回工程数据. */
            mineTarget_ = targetBlock_;
            context_.player->BeginMining(*context_.world);
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
        const player::PlayerController::MineTick tick =
            context_.player->AdvanceMining(deltaSeconds, *context_.world);
        if (tick.destroyed) {
            /* 控制器只改世界, 工程数据在这里补齐, 触发器同时收到破坏事件. */
            if (IsAir(context_.world->GetBlock(mineTarget_.x, mineTarget_.y, mineTarget_.z))) {
                ApplySingle(mineTarget_, voxel::block::Air, "mine");
            }
            runtime_->NotifyBlockBroken(tick.block, mineTarget_.x, mineTarget_.y, mineTarget_.z);
        }
        return;
    }
    const bool releasedNow = (released & BitOf(editPointerId_)) != 0ull;
    editPointerId_ = -1;
    context_.player->CancelMining();
    if (!releasedNow) {
        return;
    }
    const double held = elapsedSeconds_ - editDownSeconds_;
    const float movedX = editLastX_ - editDownX_;
    const float movedY = editLastY_ - editDownY_;
    const float moved = std::sqrt(movedX * movedX + movedY * movedY);
    if (held >= player::mining::kTapSeconds || moved >= kTapMoveLimit) {
        return;
    }
    if (!hasTarget_) {
        return;
    }
    const voxel::BlockPos placed{targetBlock_.x + targetFace_.x, targetBlock_.y + targetFace_.y,
                                 targetBlock_.z + targetFace_.z};
    if (IsAir(context_.world->GetBlock(placed.x, placed.y, placed.z))) {
        ApplySingle(placed, PaletteBlock(selectedPalette_), "place");
    }
}

void EditorShell::Update(double deltaSeconds, platform::InputSnapshot& input) {
    if (!active_) {
        return;
    }
    elapsedSeconds_ += deltaSeconds;
    /*
     * 同一份快照可能在一帧里被推进多次, 累计位移只允许被消费一次,
     * 没有新触摸事件时先清零, 与游戏总装的处理一致.
     */
    if (input.touchEvents == lastProcessedTouchEvents_) {
        input.ClearMotion();
    }
    lastProcessedTouchEvents_ = input.touchEvents;

    ApplyControlSettings();
    lastFeet_ = context_.player->Position();
    context_.world->Update(lastFeet_, viewDistanceChunks_);
    project_.ProjectToWorld(*context_.world, limits::kProjectionBudgetPerFrame);

    /* 拾取在输入之前算一次, 界面与手势共用同一份目标. */
    voxel::BlockPos block{};
    voxel::BlockPos face{};
    hasTarget_ = Pick(*context_.world, EyePosition(), LookDirection(), limits::kRayDistance, block,
                      face);
    targetBlock_ = block;
    targetFace_ = face;

    BuildFrame();
    HandleUiPointers(input);
    context_.player->Update(deltaSeconds, input, *context_.world);

    if (mode_ == EditorMode::kPlaytest) {
        UpdatePlaytestGesture(input, deltaSeconds);
        runtime_->Tick(deltaSeconds, context_.player->Position());
    } else {
        UpdateEditGesture(input, deltaSeconds);
    }

    TickHint(deltaSeconds);
    previousDownMask_ = input.downMask;
    /* 手势可能改了方块与触发器, 收尾再取一次目标与界面数据. */
    hasTarget_ = Pick(*context_.world, EyePosition(), LookDirection(), limits::kRayDistance, block,
                      face);
    targetBlock_ = block;
    targetFace_ = face;
    BuildFrame();
}

void EditorShell::RenderScene() {
    if (!active_ || context_.renderer == nullptr || context_.world == nullptr) {
        return;
    }
    const float aspect =
        height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
    float projection[16];
    float view[16];
    MakePerspective(projection, kFovYDegrees, aspect, kNearPlane, kFarPlane);
    context_.player->GetViewMatrix(view);
    Multiply4x4(projection, view, viewProjection_);

    const core::Vec3 position = context_.player->Position();
    float camera[3] = {position.x, position.y + 1.62f, position.z};
    context_.renderer->SetViewProjection(viewProjection_, camera);
    context_.renderer->Sync(*context_.world);
    context_.renderer->Draw();

    /* 覆盖层使用的世界框线依赖本帧的视投影矩阵. */
    frame_.viewProjection = viewProjection_;
    hud_.Render(frame_);
}

}  /* namespace immirror::editor */
