/* 控件宿主绑定: 脚本通过控件表操作控件. */
#include <cmath>
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

    /* 基线: 不跑脚本时的绘制命令数. */
    WidgetHost host;
    check("载入布局", host.Load(layout.data(), layout.size()));
    host.RunScript("", 0, screen);
    std::vector<LayoutDrawCommand> base;
    const int32_t baseCount = host.CollectDrawCommands(base);
    std::printf("基线绘制命令 %d 条\n", baseCount);
    check("基线有命令", baseCount > 0);

    /* 脚本按名字取控件并隐藏, 命令数必须减少. */
    WidgetHost hiding;
    check("载入布局(隐藏用例)", hiding.Load(layout.data(), layout.size()));
    const char* hideScript = "local w = getglobal(\"DropdownBox\") if w ~= nil then w:Hide() end";
    check("执行隐藏脚本", hiding.RunScript(hideScript, std::strlen(hideScript), screen));
    std::vector<LayoutDrawCommand> hidden;
    const int32_t hiddenCount = hiding.CollectDrawCommands(hidden);
    std::printf("隐藏后绘制命令 %d 条\n", hiddenCount);
    check("隐藏后命令减少", hiddenCount < baseCount);

    /* 取不到的名字返回空, 脚本判空后不报错. */
    WidgetHost missing;
    missing.Load(layout.data(), layout.size());
    const char* missingScript =
        "local w = getglobal(\"NoSuchWidget\") if w == nil then Missing = 1 else Missing = 0 end";
    check("取不存在的控件", missing.RunScript(missingScript, std::strlen(missingScript), screen));
    check("判空成立", missing.Vm().GetGlobal("Missing").number == 1.0);

    /* 通过 this 取子控件并设文本. */
    WidgetHost textHost;
    textHost.Load(layout.data(), layout.size());
    const char* textScript =
        "local c = getglobal(\"DropdownBoxNickname\") if c ~= nil then c:SetText(\"HELLO\") end";
    check("执行设文本脚本", textHost.RunScript(textScript, std::strlen(textScript), screen));
    /* 直接读回控件状态: 文本是否真的写到了控件上. */
    Widget* label = textHost.Tree().Find("DropdownBoxNickname");
    check("找到文本控件", label != nullptr);
    check("文本写入控件", label != nullptr && label->text == "HELLO");

    /* 同一控件两次取到必须是同一张表. */
    WidgetHost identity;
    identity.Load(layout.data(), layout.size());
    const char* idScript =
        "local a = getglobal(\"DropdownBox\") local b = getglobal(\"DropdownBox\") "
        "Same = (a == b) and 1 or 0";
    check("执行身份脚本", identity.RunScript(idScript, std::strlen(idScript), screen));
    check("控件身份稳定", identity.Vm().GetGlobal("Same").number == 1.0);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
