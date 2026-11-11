#include "editor/editor_bindings.h"

#include <cmath>
#include <cstring>

#include "core/log.h"
#include "editor/project_format.h"
#include "player/player_controller.h"
#include "world/world.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditorScript";

/* 取浮点实参, 整数也可以接受. */
bool AsNumber(const script::Value& value, double& out) {
    if (value.type == script::ValueType::Int) {
        out = static_cast<double>(value.intValue);
        return true;
    }
    if (value.type == script::ValueType::Double) {
        out = value.doubleValue;
        return true;
    }
    return false;
}

int32_t Fail(script::NativeCall& call, script::VmError error, const char* message) {
    call.errorMessage = message;
    return -static_cast<int32_t>(error);
}

/*
 * player_teleport(x, y, z) -> 是否成功.
 * 参数是脚底中心的工程坐标, 与 world_get_block 的方块坐标一致.
 */
int32_t NativePlayerTeleport(script::NativeCall& call) {
    if (call.argc < 3) {
        return Fail(call, script::VmError::ArityMismatch, "player_teleport 需要三个坐标");
    }
    if (call.maxResults < 1) {
        return script::kNativeOk;
    }
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!AsNumber(call.args[0], x) || !AsNumber(call.args[1], y) || !AsNumber(call.args[2], z)) {
        return Fail(call, script::VmError::BindingType, "player_teleport 的坐标必须是数值");
    }
    auto* host = static_cast<TriggerHost*>(call.host);
    if (host == nullptr) {
        return Fail(call, script::VmError::BindingRejected, "宿主未挂载, 无法传送玩家");
    }
    call.results[0] = script::MakeBool(host->Teleport(x, y, z));
    return 1;
}

}  /* namespace */

TriggerHost::TriggerHost(world::World& world, Project& project, player::PlayerController& player)
    : world_(world), project_(project), player_(player) {}

int32_t TriggerHost::GetBlock(int32_t x, int32_t y, int32_t z) {
    return static_cast<int32_t>(project_.GetBlock(x, y, z));
}

bool TriggerHost::SetBlock(int32_t x, int32_t y, int32_t z, int32_t blockId) {
    if (blockId < 0 || blockId > 0xFFFF) {
        return false;
    }
    if (y < 0 || y >= voxel::kWorldHeight) {
        return false;
    }
    const bool stored = project_.SetBlock(x, y, z, static_cast<voxel::BlockId>(blockId));
    /* 工程数据是事实来源, 世界只是投影; 区块未加载时写入失败不算错误. */
    world_.SetBlock(x, y, z, static_cast<voxel::BlockId>(blockId));
    return stored;
}

bool TriggerHost::PlayerPosition(double& x, double& y, double& z) {
    const core::Vec3 position = player_.Position();
    x = static_cast<double>(position.x);
    y = static_cast<double>(position.y);
    z = static_cast<double>(position.z);
    return true;
}

void TriggerHost::Log(const char* message) {
    if (message == nullptr) {
        return;
    }
    lastMessage_ = message;
    ++messageCount_;
    IM_LOGI(kTag, "trigger log: %s", message);
}

bool TriggerHost::Teleport(double x, double y, double z) {
    if (y < 0.0 || y > static_cast<double>(voxel::kWorldHeight)) {
        return false;
    }
    player_.SetPose(core::Vec3{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
                    player_.Yaw(), player_.Pitch());
    IM_LOGI(kTag, "player teleported to %.2f, %.2f, %.2f", x, y, z);
    return true;
}

void TriggerHost::ClearMessages() {
    lastMessage_.clear();
    messageCount_ = 0;
}

void RegisterEditorBindings(script::BindingRegistry& registry) {
    /* 先安装运行时已有的全集, 再补编辑器专属函数. */
    script::RegisterStandardBindings(registry);
    script::RegisterWorldBindings(registry);
    std::string error;
    registry.RegisterFunction("player_teleport", &NativePlayerTeleport,
                              {script::ValueType::Double, script::ValueType::Double,
                               script::ValueType::Double},
                              3, "把玩家传送到指定坐标", error);
    if (!error.empty()) {
        IM_LOGW(kTag, "editor binding register failed: %s", error.c_str());
    }
}

}  /* namespace immirror::editor */
