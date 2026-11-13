/* 方块爆炸抗性的数据完整性断言. */
#include <cstdio>
#include <cstring>

#include "game/generated/block_data_gen.h"

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}


int main() {
    int32_t outOfRange = 0;
    int32_t highResist = 0;
    for (int32_t index = 0; index < immirror::game::gen::kRegistryEntryCount; ++index) {
        const int16_t value = immirror::game::gen::kRegistryEntries[index].blastResist;
        if (value < -1) {
            outOfRange += 1;
        }
        if (value >= 10) {
            highResist += 1;
        }
    }
    std::printf("方块条目 %d, 高抗性 %d\n", immirror::game::gen::kRegistryEntryCount,
                highResist);
    check("没有超出约定范围的负值", outOfRange == 0);
    check("存在高抗性方块", highResist > 0);
    /* 分布: 既要有一碰就碎的方块, 也要有炸不动的方块. */
    int32_t fragile = 0;
    for (int32_t index = 0; index < immirror::game::gen::kRegistryEntryCount; ++index) {
        if (immirror::game::gen::kRegistryEntries[index].blastResist == 0) {
            fragile += 1;
        }
    }
    std::printf("低抗性 %d, 高抗性 %d\n", fragile, highResist);
    check("有低抗性方块", fragile > 0);
    /* 两类都要有, 具体比例由数据决定, 不做假设. */
    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
