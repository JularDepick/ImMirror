#include "game/furnace.h"

#include "game/generated/furnace_data_gen.h"

namespace immirror::game {
namespace {

/* 生成表所在命名空间的别名, 与 block_registry.cpp 的处理一致. */
namespace gen = immirror::game::gen;

}  /* namespace */

const FurnaceTable& FurnaceTable::Instance() {
    static const FurnaceTable table;
    return table;
}

int32_t FurnaceTable::Count() const {
    return gen::kFurnaceEntryCount;
}

int32_t FurnaceTable::FindByMaterial(ItemId item) const {
    if (item == item::kNone) {
        return -1;
    }
    for (int32_t index = 0; index < gen::kFurnaceEntryCount; ++index) {
        if (gen::kFurnaceEntries[index].materialItem == item) {
            return index;
        }
    }
    return -1;
}

ItemId FurnaceTable::ResultOf(int32_t recipeIndex) const {
    if (recipeIndex < 0 || recipeIndex >= gen::kFurnaceEntryCount) {
        return item::kNone;
    }
    return static_cast<ItemId>(gen::kFurnaceEntries[recipeIndex].resultItem);
}

int32_t FurnaceTable::CombustionOf(ItemId item) const {
    if (item == item::kNone) {
        return 0;
    }
    for (int32_t index = 0; index < gen::kFuelEntryCount; ++index) {
        if (gen::kFuelEntries[index].item == item) {
            return gen::kFuelEntries[index].combustion;
        }
    }
    return 0;
}

uint64_t ContainerKeyOf(int32_t x, int32_t y, int32_t z) {
    /* 21 位坐标编码, 覆盖世界高度 256 与 int 范围的水平坐标. */
    return (static_cast<uint64_t>(static_cast<uint32_t>(x) & 0x1FFFFFu) << 42) |
           (static_cast<uint64_t>(static_cast<uint32_t>(y) & 0x1FFFFFu) << 21) |
           static_cast<uint64_t>(static_cast<uint32_t>(z) & 0x1FFFFFu);
}

bool FurnaceState::Smeltable() const {
    if (input.Empty()) {
        return false;
    }
    const int32_t recipe = FurnaceTable::Instance().FindByMaterial(input.item);
    if (recipe < 0) {
        return false;
    }
    const ItemId result = FurnaceTable::Instance().ResultOf(recipe);
    if (result == item::kNone) {
        return false;
    }
    if (!output.Empty() && (output.item != result || output.count >= 64)) {
        /* 产出格被别的物品占用或已堆满: 暂停熔炼, 等待收取. */
        return false;
    }
    return true;
}

void FurnaceState::Tick(float dt) {
    if (fuelTicks > 0.0f) {
        fuelTicks -= dt;
        if (fuelTicks <= 0.0f) {
            /* 当前格燃料烧尽: 扣一格, 仍有剩余则接着烧下一格. */
            fuelTicks = 0.0f;
            fuelBurnTotal = 0.0f;
            if (!fuel.Empty()) {
                fuel.count -= 1;
                if (fuel.count <= 0) {
                    fuel.Clear();
                } else {
                    const int32_t combustion = FurnaceTable::Instance().CombustionOf(fuel.item);
                    if (combustion > 0) {
                        fuelBurnTotal = static_cast<float>(combustion) * kFuelSecondsPerUnit;
                        fuelTicks = fuelBurnTotal;
                    }
                }
            }
        }
    } else if (!fuel.Empty() && Smeltable()) {
        /* 空闲且可熔炼: 点燃下一格燃料. */
        const int32_t combustion = FurnaceTable::Instance().CombustionOf(fuel.item);
        if (combustion > 0) {
            fuelBurnTotal = static_cast<float>(combustion) * kFuelSecondsPerUnit;
            fuelTicks = fuelBurnTotal;
        }
    }

    if (fuelTicks <= 0.0f || !Smeltable()) {
        progressSeconds = 0.0f;
        progress = 0.0f;
        return;
    }

    /* 累计秒数判定出货, 浮点逐帧累加不会卡在临界值下方. */
    progressSeconds += dt;
    if (progressSeconds < kSmeltSeconds) {
        progress = progressSeconds / kSmeltSeconds;
        return;
    }
    progressSeconds -= kSmeltSeconds;

    /* 一件熔炼完成: 产出并入输出格并扣一件输入. */
    const int32_t recipe = FurnaceTable::Instance().FindByMaterial(input.item);
    const ItemId resultItem = FurnaceTable::Instance().ResultOf(recipe);
    if (output.Empty()) {
        output = item::MakeStack(resultItem, 1);
    } else {
        output.count += 1;
    }
    input.count -= 1;
    if (input.count <= 0) {
        input.Clear();
    }
    progress = progressSeconds / kSmeltSeconds;
}

}  /* namespace immirror::game */
