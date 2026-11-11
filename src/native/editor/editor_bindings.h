#pragma once

#include <string>

#include "script/binding.h"
#include "voxel/voxel_types.h"

namespace immirror::world {
class World;
}

namespace immirror::player {
class PlayerController;
}

namespace immirror::editor {

class Project;

/*
 * 触发器脚本的宿主.
 * 脚本只通过这里读写方块与玩家, 不直接触碰工程与世界对象;
 * 写入方块的路径与编辑器工具一致: 既进工程数据也立刻投影到世界,
 * 因此脚本放的方块会随工程一起落盘.
 */
class TriggerHost final : public script::HostWorld {
public:
    TriggerHost(world::World& world, Project& project, player::PlayerController& player);

    /* HostWorld */
    int32_t GetBlock(int32_t x, int32_t y, int32_t z) override;
    bool SetBlock(int32_t x, int32_t y, int32_t z, int32_t blockId) override;
    bool PlayerPosition(double& x, double& y, double& z) override;
    void Log(const char* message) override;

    /* 传送玩家到指定坐标, 返回是否成功; 供 player_teleport 使用. */
    bool Teleport(double x, double y, double z);

    /* 最近一条脚本日志与累计条数, 界面据此显示运行结果. */
    const std::string& LastMessage() const { return lastMessage_; }
    int32_t MessageCount() const { return messageCount_; }
    void ClearMessages();

private:
    world::World& world_;
    Project& project_;
    player::PlayerController& player_;
    std::string lastMessage_;
    int32_t messageCount_ = 0;
};

/*
 * 注册编辑器侧的脚本白名单: 标准函数, 世界读写, 玩家位置, 以及传送.
 * 传送是编辑器新增的唯一原生函数, 其余全部复用运行时已有的实现.
 */
void RegisterEditorBindings(script::BindingRegistry& registry);

}  /* namespace immirror::editor */
