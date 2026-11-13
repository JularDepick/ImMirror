/* 界面全局函数: 字符串表, 日志, 提示与管理器单例. */
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
        std::printf("FAIL 读不到布局\n");
        return 1;
    }
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    WidgetHost host;
    check("载入布局", host.Load(layout.data(), layout.size()));

    /* 提示出口: 脚本调用后能收到文案. */
    std::string lastTip;
    host.SetTipSink([&lastTip](const std::string& text) { lastTip = text; });

    const char* script =
        "S1 = GetS(1) "
        "SNone = GetS(99999999) "
        "Inst = GetInst(\"ClientMgr\") "
        "InstOk = (Inst ~= nil) and 1 or 0 "
        "Log(\"hello\") "
        "ShowGameTips(\"TIP-OK\") ";
    check("执行全局脚本", host.RunScript(script, std::strlen(script), screen));

    check("字符串表取到文案", host.Vm().GetGlobal("S1").text.size() > 0);
    check("未收录编号返回空串", host.Vm().GetGlobal("SNone").text.empty());
    check("管理器单例非空", host.Vm().GetGlobal("InstOk").number == 1.0);
    check("提示出口收到文案", lastTip == "TIP-OK");

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
