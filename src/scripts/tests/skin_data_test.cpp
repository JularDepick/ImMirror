/* 角色皮肤表的数据完整性断言. */
#include <cstdio>
#include <cstring>

#include "game/generated/skin_data_gen.h"

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    int32_t withName = 0;
    int32_t withModel = 0;
    bool ordered = true;
    for (int32_t index = 0; index < immirror::game::gen::kSkinEntryCount; ++index) {
        const immirror::game::gen::SkinEntry& entry = immirror::game::gen::kSkinEntries[index];
        if (entry.name != nullptr && entry.name[0] != '\0') {
            withName += 1;
        }
        if (entry.model != 0) {
            withModel += 1;
        }
        if (index > 0 &&
            immirror::game::gen::kSkinEntries[index - 1].id >= entry.id) {
            ordered = false;
        }
    }
    std::printf("皮肤 %d, 有名字 %d, 有模型 %d\n", immirror::game::gen::kSkinEntryCount,
                withName, withModel);
    check("皮肤表非空", immirror::game::gen::kSkinEntryCount > 50);
    check("每条都有名字", withName == immirror::game::gen::kSkinEntryCount);
    check("按编号升序", ordered);
    check("查表能取到第一条",
          immirror::game::gen::SkinOf(immirror::game::gen::kSkinEntries[0].id) != nullptr);
    check("查不到时返回空", immirror::game::gen::SkinOf(99999u) == nullptr);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
