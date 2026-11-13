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

    /* 管理器方法: 平台判定, 版本, 字符串查询与音效出口. */
    std::string lastSound;
    host.SetSoundSink([&lastSound](const std::string& path) { lastSound = path; });
    const char* managerScript =
        "local mgr = GetInst(\"ClientMgr\") "
        "IsPC = mgr:isPC() and 1 or 0 "
        "IsMobile = mgr:isMobile() and 1 or 0 "
        "Version = mgr:clientVersion() "
        "mgr:playSound2D(\"sounds/ui_click.ogg\") "
        "local def = GetInst(\"DefMgr\") "
        "Str = def:getStringDef(1) "
        "Other = GetInst(\"NoSuchMgr\") "
        "OtherOk = (Other ~= nil) and 1 or 0 ";
    check("执行管理器脚本", host.RunScript(managerScript, std::strlen(managerScript), screen));
    check("平台判定为移动端", host.Vm().GetGlobal("IsPC").number == 0.0 &&
                                   host.Vm().GetGlobal("IsMobile").number == 1.0);
    check("版本号非空", host.Vm().GetGlobal("Version").text.size() > 0);
    check("音效出口收到路径", lastSound == "sounds/ui_click.ogg");
    check("字符串查询可用", host.Vm().GetGlobal("Str").text.size() > 0);
    check("未知管理器也非空", host.Vm().GetGlobal("OtherOk").number == 1.0);

    /* 共享键值: 一个界面写入, 另一个界面能读到同一个值. */
    const char* dataScript =
        "local mgr = GetInst(\"ClientMgr\") "
        "mgr:setGameData(\"picked\", 42) "
        "Back = mgr:getGameData(\"picked\") "
        "Nothing = mgr:getGameData(\"absent\") "
        "AbsentOk = (Nothing == nil) and 1 or 0 ";
    check("执行共享数据脚本", host.RunScript(dataScript, std::strlen(dataScript), screen));
    check("共享数据能读回", host.Vm().GetGlobal("Back").number == 42.0);
    check("未写入的键为空", host.Vm().GetGlobal("AbsentOk").number == 1.0);

    /* 按模板复制控件: 新控件出现在树里, 模板本身不变. */
    const int32_t before = host.Tree().WidgetCount();
    const char* cloneScript =
        "local mgr = GetInst(\"UIFrameMgr\") "
        "local made = mgr:CreateFrameByTemplate(\"DropdownBox\", \"\", \"CopyA\") "
        "MadeOk = (made ~= nil) and 1 or 0 "
        "local again = mgr:CreateFrameByTemplate(\"DropdownBox\", \"\", \"CopyA\") "
        "DuplicateOk = (again == nil) and 1 or 0 "
        "local missing = mgr:CreateFrameByTemplate(\"NoSuchWidget\", \"\", \"CopyB\") "
        "MissingOk = (missing == nil) and 1 or 0 ";
    check("执行复制脚本", host.RunScript(cloneScript, std::strlen(cloneScript), screen));
    check("复制成功", host.Vm().GetGlobal("MadeOk").number == 1.0);
    check("重名被拒", host.Vm().GetGlobal("DuplicateOk").number == 1.0);
    check("模板不存在返回空", host.Vm().GetGlobal("MissingOk").number == 1.0);
    check("控件数增加", host.Tree().WidgetCount() > before);
    check("新控件可按名字取到", host.Tree().Find("CopyA") != nullptr);
    check("模板仍在", host.Tree().Find("DropdownBox") != nullptr);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
