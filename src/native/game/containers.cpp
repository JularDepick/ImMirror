#include "game/containers.h"

#include <cstdio>
#include <cstring>

namespace immirror::game {
namespace {

/*
 * 容器存档格式: 魔数与版本头, 熔炉节, 箱子节, 全部小端.
 * 版本 2: 首个容器版本, 箱子 27 格.
 * 版本 3: 箱子扩到 60 格; 读 v2 时按 27 格还原旧档数据.
 * 版本 4: 末尾追加物品栏节 (45 格), 每格多记耐久与附魔; 读 v2 与 v3 时不读该节,
 * 物品栏保持初始状态, 旧档照样能进世界.
 * 版本 5: 追加四个护甲部位, 只记物品编号.
 * 版本 6: 护甲部位改记整份堆叠, 附魔与耐久一并保留.
 */
constexpr char kContainerMagic[4] = {'I', 'M', 'C', 'F'};
constexpr uint32_t kContainerVersion = 6;
constexpr uint32_t kLegacyChestSlots = 27;

/* 物品栏单格的落盘形式: 数量之外还要记耐久与附魔, 否则读档会丢工具状态. */
struct InventoryRecord {
    uint16_t item;
    int32_t count;
    int16_t durability;
    uint16_t enchantId;
    uint8_t enchantLevel;
    uint8_t reserved;
};

/* 单格堆叠的落盘形式: 物品 id 与数量. */
struct StackRecord {
    uint16_t item;
    int32_t count;
};

bool WriteStack(FILE* file, const ItemStack& stack) {
    StackRecord record{stack.item, stack.count};
    return std::fwrite(&record, sizeof(record), 1, file) == 1;
}

bool ReadStack(FILE* file, ItemStack& stack) {
    StackRecord record{};
    if (std::fread(&record, sizeof(record), 1, file) != 1) {
        return false;
    }
    if (record.item == 0 || record.count <= 0) {
        stack.Clear();
    } else {
        stack.item = record.item;
        stack.count = record.count;
    }
    return true;
}

}  /* namespace */

const ItemStack& ChestState::Slot(int32_t index) const {
    return slots_[ClampIndex(index)];
}

ItemStack& ChestState::MutableSlot(int32_t index) {
    return slots_[ClampIndex(index)];
}

int32_t ChestState::ClampIndex(int32_t index) {
    if (index < 0) {
        return 0;
    }
    return index >= kSlots ? kSlots - 1 : index;
}

namespace {

/*
 * 物品栏单格读写. 空格写成全 0, 读出时按空处理, 与箱子堆叠同一口径.
 */
bool WriteInventoryStack(FILE* file, const ItemStack& stack) {
    InventoryRecord record{stack.item, stack.count, stack.durability, stack.enchantId,
                           stack.enchantLevel, 0};
    if (stack.Empty()) {
        record = InventoryRecord{0, 0, -1, 0, 0, 0};
    }
    return std::fwrite(&record, sizeof(record), 1, file) == 1;
}

bool ReadInventoryStack(FILE* file, ItemStack& stack) {
    InventoryRecord record{};
    if (std::fread(&record, sizeof(record), 1, file) != 1) {
        return false;
    }
    stack.Clear();
    if (record.item == 0 || record.count <= 0) {
        return true;
    }
    stack.item = record.item;
    stack.count = record.count;
    stack.durability = record.durability;
    stack.enchantId = record.enchantId;
    stack.enchantLevel = record.enchantLevel;
    return true;
}

}  /* namespace */

bool SaveContainersToFile(const char* path, const FurnaceMap& furnaces, const ChestMap& chests,
                          const Inventory& inventory, const ArmorSet& armor) {
    if (path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    char magic[4];
    std::memcpy(magic, kContainerMagic, sizeof(magic));
    uint32_t version = kContainerVersion;
    uint32_t furnaceCount = static_cast<uint32_t>(furnaces.size());
    uint32_t chestCount = static_cast<uint32_t>(chests.size());
    if (std::fwrite(magic, sizeof(magic), 1, file) != 1 ||
        std::fwrite(&version, sizeof(version), 1, file) != 1 ||
        std::fwrite(&furnaceCount, sizeof(furnaceCount), 1, file) != 1 ||
        std::fwrite(&chestCount, sizeof(chestCount), 1, file) != 1) {
        std::fclose(file);
        return false;
    }

    for (const auto& entry : furnaces) {
        const uint64_t key = entry.first;
        const FurnaceState& state = entry.second;
        if (std::fwrite(&key, sizeof(key), 1, file) != 1 ||
            !WriteStack(file, state.input) || !WriteStack(file, state.fuel) ||
            !WriteStack(file, state.output) ||
            std::fwrite(&state.progress, sizeof(state.progress), 1, file) != 1 ||
            std::fwrite(&state.progressSeconds, sizeof(state.progressSeconds), 1, file) != 1 ||
            std::fwrite(&state.fuelTicks, sizeof(state.fuelTicks), 1, file) != 1 ||
            std::fwrite(&state.fuelBurnTotal, sizeof(state.fuelBurnTotal), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
    }

    for (const auto& entry : chests) {
        const uint64_t key = entry.first;
        const ChestState& chest = entry.second;
        if (std::fwrite(&key, sizeof(key), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        for (int32_t index = 0; index < ChestState::kSlots; ++index) {
            if (!WriteStack(file, chest.Slot(index))) {
                std::fclose(file);
                return false;
            }
        }
    }

    /* 物品栏节: 固定 45 格, 逐格写物品 id, 数量, 耐久与附魔. */
    for (int32_t index = 0; index < Inventory::kSlotCount; ++index) {
        if (!WriteInventoryStack(file, inventory.Slot(index))) {
            std::fclose(file);
            return false;
        }
    }

    /*
     * 护甲四个部位: 记物品编号, 数量, 耐久与附魔.
     * 与物品栏同一口径, 否则穿上身的附魔装备读档后会变回白板.
     */
    for (int32_t slot = 0; slot < ArmorSet::kSlotCount; ++slot) {
        if (!WriteInventoryStack(file, armor.WornStack(slot))) {
            std::fclose(file);
            return false;
        }
    }

    std::fclose(file);
    return true;
}

bool LoadContainersFromFile(const char* path, FurnaceMap& furnaces, ChestMap& chests,
                           Inventory& inventory, ArmorSet& armor) {
    if (path == nullptr) {
        return false;
    }
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        /* 文件不存在是首次进入世界的正常情况, 不算错误. */
        return true;
    }
    char magic[4];
    uint32_t version = 0;
    uint32_t furnaceCount = 0;
    uint32_t chestCount = 0;
    if (std::fread(magic, sizeof(magic), 1, file) != 1 ||
        std::memcmp(magic, kContainerMagic, sizeof(magic)) != 0) {
        std::fclose(file);
        return false;
    }
    if (std::fread(&version, sizeof(version), 1, file) != 1 ||
        (version < 2 || version > kContainerVersion)) {
        /* 未知版本按空容器处理, 保持向后兼容, 不阻断进世界. */
        std::fclose(file);
        return true;
    }
    const int32_t chestSlots = version == 2 ? kLegacyChestSlots : ChestState::kSlots;
    if (std::fread(&furnaceCount, sizeof(furnaceCount), 1, file) != 1 ||
        std::fread(&chestCount, sizeof(chestCount), 1, file) != 1) {
        std::fclose(file);
        return false;
    }

    furnaces.clear();
    chests.clear();
    for (uint32_t i = 0; i < furnaceCount; ++i) {
        uint64_t key = 0;
        FurnaceState state;
        if (std::fread(&key, sizeof(key), 1, file) != 1 ||
            !ReadStack(file, state.input) || !ReadStack(file, state.fuel) ||
            !ReadStack(file, state.output) ||
            std::fread(&state.progress, sizeof(state.progress), 1, file) != 1 ||
            std::fread(&state.progressSeconds, sizeof(state.progressSeconds), 1, file) != 1 ||
            std::fread(&state.fuelTicks, sizeof(state.fuelTicks), 1, file) != 1 ||
            std::fread(&state.fuelBurnTotal, sizeof(state.fuelBurnTotal), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        furnaces.emplace(key, state);
    }
    for (uint32_t i = 0; i < chestCount; ++i) {
        uint64_t key = 0;
        ChestState chest;
        if (std::fread(&key, sizeof(key), 1, file) != 1) {
            std::fclose(file);
            return false;
        }
        for (int32_t index = 0; index < chestSlots; ++index) {
            if (!ReadStack(file, chest.MutableSlot(index))) {
                std::fclose(file);
                return false;
            }
        }
        chests.emplace(key, chest);
    }

    /*
     * 物品栏节只在版本 4 起存在; 更早的文件读到这里已经到末尾,
     * 此时保留调用方的物品栏初值, 不算错误.
     */
    if (version >= 4) {
        for (int32_t index = 0; index < Inventory::kSlotCount; ++index) {
            ItemStack stack;
            if (!ReadInventoryStack(file, stack)) {
                std::fclose(file);
                return false;
            }
            inventory.SetSlot(index, stack);
        }
    }

    /*
     * 护甲部位在版本 5 起存在. 版本 5 只记物品编号, 版本 6 起改记整份堆叠;
     * 读更早的文件时保留调用方给的初值.
     */
    if (version >= 6) {
        for (int32_t slot = 0; slot < ArmorSet::kSlotCount; ++slot) {
            ItemStack stack;
            if (!ReadInventoryStack(file, stack)) {
                std::fclose(file);
                return false;
            }
            if (!stack.Empty()) {
                armor.Equip(stack);
            }
        }
    } else if (version == 5) {
        for (int32_t slot = 0; slot < ArmorSet::kSlotCount; ++slot) {
            uint16_t worn = 0;
            if (std::fread(&worn, sizeof(worn), 1, file) != 1) {
                std::fclose(file);
                return false;
            }
            if (worn != 0) {
                armor.Equip(static_cast<ItemId>(worn));
            }
        }
    }

    std::fclose(file);
    return true;
}

}  /* namespace immirror::game */
