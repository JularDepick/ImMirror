/* 食物附带状态效果的数据完整性断言. */
#include <cstdio>

#include "game/generated/food_data_gen.h"

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    int32_t withBuff = 0;
    int32_t badOdds = 0;
    int32_t badId = 0;
    for (int32_t index = 0; index < immirror::game::gen::kFoodEntryCount; ++index) {
        const immirror::game::gen::FoodEntry& entry = immirror::game::gen::kFoodEntries[index];
        const uint32_t ids[3] = {entry.buffId1, entry.buffId2, entry.buffId3};
        const uint16_t odds[3] = {entry.buffOdds1, entry.buffOdds2, entry.buffOdds3};
        bool has = false;
        for (int32_t slot = 0; slot < 3; ++slot) {
            if (ids[slot] == 0) {
                /* 没有效果编号时概率必须为零, 否则会去挂一个不存在的效果. */
                if (odds[slot] != 0) {
                    badId += 1;
                }
                continue;
            }
            has = true;
            if (odds[slot] == 0 || odds[slot] > 10000) {
                badOdds += 1;
            }
        }
        if (has) {
            withBuff += 1;
        }
    }
    std::printf("食物条目 %d, 带状态效果 %d\n", immirror::game::gen::kFoodEntryCount, withBuff);
    /* 客户端可消费的食物条目里, 带效果的应当占多数. */
    check("有食物附带效果", withBuff > 20);
    check("概率都在有效区间", badOdds == 0);
    check("空编号不带概率", badId == 0);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
