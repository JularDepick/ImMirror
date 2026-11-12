#include "game/containers.h"

#include <cstdio>
#include <cstring>

namespace immirror::game {
namespace {

/*
 * 容器存档格式: 魔数与版本头, 熔炉节, 箱子节, 全部小端.
 * 版本 2: 首个容器版本; 世界存档 kSaveVersion 为 1, 容器节从 2 起记,
 * 读世界存档版本 1 的旧档时容器文件不存在, 按空容器处理, 天然向后兼容.
 */
constexpr char kContainerMagic[4] = {'I', 'M', 'C', 'F'};
constexpr uint32_t kContainerVersion = 2;

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

bool SaveContainersToFile(const char* path, const FurnaceMap& furnaces, const ChestMap& chests) {
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

    std::fclose(file);
    return true;
}

bool LoadContainersFromFile(const char* path, FurnaceMap& furnaces, ChestMap& chests) {
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
    if (std::fread(&version, sizeof(version), 1, file) != 1 || version != kContainerVersion) {
        /* 未知版本按空容器处理, 保持向后兼容, 不阻断进世界. */
        std::fclose(file);
        return true;
    }
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
        for (int32_t index = 0; index < ChestState::kSlots; ++index) {
            if (!ReadStack(file, chest.MutableSlot(index))) {
                std::fclose(file);
                return false;
            }
        }
        chests.emplace(key, chest);
    }

    std::fclose(file);
    return true;
}

}  /* namespace immirror::game */
