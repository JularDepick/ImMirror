#include "game/ui_data_source_game.h"

#include "game/inventory.h"
#include "game/item/item_table.h"
#include "player/player_controller.h"
#include "progress/progress_hub.h"
#include "world/world.h"
#include "world/world_time.h"

namespace immirror::game {

GameUiDataSource::GameUiDataSource(const Inventory& inventory,
                                   const player::PlayerController& player,
                                   const world::World& world,
                                   const world::WorldTime& worldTime)
    : inventory_(inventory), player_(player), world_(world), worldTime_(worldTime) {}

int32_t GameUiDataSource::BagSlotCount() const {
    return Inventory::kSlotCount;
}

std::string GameUiDataSource::BagSlotName(int32_t slot) const {
    const item::ItemStack& stack = inventory_.Slot(slot);
    if (stack.Empty()) {
        return std::string();
    }
    /* 界面标签用英文键: 点阵字没有中文字形. */
    const char* key = item::ItemTable::Instance().Info(stack.item).key;
    return key == nullptr ? std::string() : std::string(key);
}

int32_t GameUiDataSource::BagSlotCountAt(int32_t slot) const {
    const item::ItemStack& stack = inventory_.Slot(slot);
    return stack.Empty() ? 0 : stack.count;
}

int32_t GameUiDataSource::BagSlotDurability(int32_t slot) const {
    const item::ItemStack& stack = inventory_.Slot(slot);
    if (stack.Empty() || stack.durability < 0) {
        return -1;
    }
    return stack.durability;
}

float GameUiDataSource::Health() const {
    return player_.HealthRatio();
}

float GameUiDataSource::Hunger() const {
    return player_.HungerRatio();
}

float GameUiDataSource::Stamina() const {
    return player_.StaminaRatio();
}

std::string GameUiDataSource::WorldName() const {
    return worldName_;
}

float GameUiDataSource::WorldTimeSeconds() const {
    return worldTime_.TotalSeconds();
}

int64_t GameUiDataSource::ProgressValue(const std::string& key) const {
    /* 进度统计按名字取累计值; 未收录的键返回零, 与界面侧的判空写法配合. */
    return progress::ProgressHub::Instance().StatTotal(key.c_str());
}

}  /* namespace immirror::game */
