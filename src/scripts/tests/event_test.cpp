/* 事件系统: 订阅与派发. */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/script/widget_host.h"

using namespace immirror::ui;
using namespace immirror::ui::script;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static bool ReadFile(const char* path, std::string& out) {
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    char buffer[8192];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        out.append(buffer, got);
    }
    std::fclose(file);
    return true;
}

int main(int argc, char** argv) {
    const char* layoutPath =
        argc > 1 ? argv[1] : "../../assets/project/data/xml/ui/mobile/account.xml";
    std::string layout;
    if (!ReadFile(layoutPath, layout)) {
        std::printf("FAIL 读不到布局 %s\n", layoutPath);
        return 1;
    }
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    WidgetHost host;
    check("载入布局", host.Load(layout.data(), layout.size()));

    /* 脚本订阅事件, 并定义处理函数把控件隐藏掉. */
    const char* script =
        "function DropdownBox_OnEvent(self, event) "
        "  if event == \"GE_HIDE\" then self:Hide() end "
        "end "
        "this:RegisterEvent(\"GE_HIDE\") "
        "this:RegisterEvent(\"GE_HIDE\") ";
    check("执行订阅脚本", host.RunScript(script, std::strlen(script), screen));
    check("重复订阅只记一次", host.SubscriberCount("GE_HIDE") == 1);
    check("订阅总数正确", host.SubscriptionCount() == 1);

    /* 未订阅的事件不派发. */
    check("未订阅事件返回零", host.DispatchEvent("GE_OTHER", {}) == 0);

    /* 派发后控件被隐藏, 绘制命令减少. */
    std::vector<LayoutDrawCommand> before;
    const int32_t beforeCount = host.CollectDrawCommands(before);
    check("派发命中处理函数", host.DispatchEvent("GE_HIDE", {}) == 1);
    std::vector<LayoutDrawCommand> after;
    const int32_t afterCount = host.CollectDrawCommands(after);
    std::printf("派发前 %d 条, 派发后 %d 条\n", beforeCount, afterCount);
    check("处理函数改动了界面", afterCount < beforeCount);

    /* 取消订阅后不再派发. */
    const char* unsubscribe = "this:UnregisterEvent(\"GE_HIDE\")";
    check("执行退订脚本", host.RunScript(unsubscribe, std::strlen(unsubscribe), screen));
    check("退订后无订阅", host.SubscriberCount("GE_HIDE") == 0);
    check("退订后不派发", host.DispatchEvent("GE_HIDE", {}) == 0);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
