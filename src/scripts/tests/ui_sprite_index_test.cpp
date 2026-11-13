/* 界面精灵索引断言: 条目存在, 已知名字能查到矩形, 未知名字查不到. */
#include <cstdio>
#include <cstring>

#include "ui/generated/ui_sprite_index_gen.h"
#include "ui/layout_render.h"

using namespace immirror::ui;

namespace {

int failures = 0;

void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

}  /* namespace */

int main() {
    check("索引条目大于零", gen::kUiSpriteRectCount > 0);

    /* 索引按名字排序, 二分查找的前提是顺序正确. */
    bool sorted = true;
    for (int32_t index = 1; index < gen::kUiSpriteRectCount; ++index) {
        if (std::strcmp(gen::kUiSpriteRects[index - 1].name, gen::kUiSpriteRects[index].name) > 0) {
            sorted = false;
            break;
        }
    }
    check("索引按名字有序", sorted);

    /* 归一化矩形必须落在 0 到 1 之间, 否则取到图外. */
    bool inRange = true;
    for (int32_t index = 0; index < gen::kUiSpriteRectCount; ++index) {
        const gen::UiSpriteRect& rect = gen::kUiSpriteRects[index];
        if (rect.u0 < 0.0f || rect.v0 < 0.0f || rect.u1 > 1.0f || rect.v1 > 1.0f ||
            rect.u1 <= rect.u0 || rect.v1 <= rect.v0) {
            inRange = false;
            break;
        }
    }
    check("矩形都在图内", inRange);

    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    const char* known = gen::kUiSpriteRects[0].name;
    check("按名字能查到", UiSpriteRectUv(known, &u0, &v0, &u1, &v1));
    check("查到后矩形有效", u1 > u0 && v1 > v0);
    check("未知名字查不到", !UiSpriteRectUv("no-such-sprite-name", &u0, &v0, &u1, &v1));
    check("空名字查不到", !UiSpriteRectUv("", &u0, &v0, &u1, &v1));

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
