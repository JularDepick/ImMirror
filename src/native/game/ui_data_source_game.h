#pragma once

#include <cstdint>
#include <string>

#include "ui/script/ui_data_source.h"

namespace immirror::world {
class World;
class WorldTime;
}  /* namespace immirror::world */

namespace immirror::player {
class PlayerController;
}  /* namespace player */

namespace immirror::game {

class Inventory;

/*
 * 游戏侧的数据源: 把背包, 玩家, 世界与进度接到界面脚本上.
 *
 * 只做读取与换算, 不改任何游戏状态; 界面因此不会因为脚本出错而影响存档.
 */
class GameUiDataSource : public ui::script::UiDataSource {
public:
    GameUiDataSource(const Inventory& inventory, const player::PlayerController& player,
                     const world::World& world, const world::WorldTime& worldTime);

    ui::script::ItemDefView ItemDef(int32_t id) const override;

    int32_t BagSlotCount() const override;
    std::string BagSlotName(int32_t slot) const override;
    int32_t BagSlotCountAt(int32_t slot) const override;
    int32_t BagSlotDurability(int32_t slot) const override;

    float Health() const override;
    float Hunger() const override;
    float Stamina() const override;

    std::string WorldName() const override;
    float WorldTimeSeconds() const override;

    int64_t ProgressValue(const std::string& key) const override;

    /* 世界名由总装在载入时写入. */
    void SetWorldName(const std::string& name) { worldName_ = name; }

private:
    const Inventory& inventory_;
    const player::PlayerController& player_;
    const world::World& world_;
    const world::WorldTime& worldTime_;
    std::string worldName_;
};

}  /* namespace immirror::game */
