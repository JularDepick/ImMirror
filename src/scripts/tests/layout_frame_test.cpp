/* 锚点求解器的宿主断言: 用 g++ 编译运行, 不依赖 Android 与 GL. */
#include <cmath>
#include <cstdio>

#include "ui/layout_frame.h"

using namespace immirror::ui;

static int failures = 0;

static void near(const char* label, float got, float want) {
    if (std::fabs(got - want) > 0.01f) {
        std::printf("FAIL %-28s got %.2f want %.2f\n", label, got, want);
        failures += 1;
    } else {
        std::printf("ok   %-28s %.2f\n", label, got);
    }
}

int main() {
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    /* 根帧铺满屏幕. */
    LayoutFrame root;
    root.name = "root";

    /* 子帧贴左上角, 偏移 20,30. */
    LayoutFrame a;
    a.name = "a";
    a.width = 200.0f;
    a.height = 100.0f;
    a.anchors.push_back(LayoutAnchor{AnchorPoint::kTopLeft, nullptr, AnchorPoint::kTopLeft, 20.0f, 30.0f});
    root.AddChild(&a);

    /* 孙帧以兄弟帧的右下角为基准: 靠右一步. */
    LayoutFrame b;
    b.name = "b";
    b.width = 100.0f;
    b.height = 50.0f;
    b.anchors.push_back(LayoutAnchor{AnchorPoint::kTopLeft, &a, AnchorPoint::kBottomRight, 10.0f, 5.0f});
    root.AddChild(&b);

    /* 居中帧: 自己的中心对根的中心. */
    LayoutFrame c;
    c.name = "c";
    c.width = 400.0f;
    c.height = 200.0f;
    c.anchors.push_back(LayoutAnchor{AnchorPoint::kCenter, nullptr, AnchorPoint::kCenter, 0.0f, 0.0f});
    root.AddChild(&c);

    /* 贴底: 自己的底边对根的底边, 上移 40. */
    LayoutFrame d;
    d.name = "d";
    d.width = 300.0f;
    d.height = 60.0f;
    d.anchors.push_back(LayoutAnchor{AnchorPoint::kBottom, nullptr, AnchorPoint::kBottom, 0.0f, -40.0f});
    root.AddChild(&d);

    ResolveLayout(root, screen);

    near("root.width", root.rect.width, 1280.0f);
    near("a.x", a.rect.x, 20.0f);
    near("a.y", a.rect.y, 30.0f);
    near("b.x", b.rect.x, 20.0f + 200.0f + 10.0f);
    near("b.y", b.rect.y, 30.0f + 100.0f + 5.0f);
    near("c.x", c.rect.x, (1280.0f - 400.0f) * 0.5f);
    near("c.y", c.rect.y, (720.0f - 200.0f) * 0.5f);
    /* 只有底边锚点时, 横向按自身中心对齐父帧中心, 因此 x 是居中的. */
    near("d.x", d.rect.x, (1280.0f - 300.0f) * 0.5f);
    near("d.y", d.rect.y, 720.0f - 60.0f - 40.0f);

    /*
     * 无锚点帧贴父帧左上角. 帧树新增节点后必须整体再解算一次,
     * 局部追加不会自动获得坐标.
     */
    LayoutFrame e;
    e.name = "e";
    e.width = 10.0f;
    e.height = 10.0f;
    c.AddChild(&e);
    ResolveLayout(root, screen);
    near("e.x", e.rect.x, c.rect.x);
    near("e.y", e.rect.y, c.rect.y);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
