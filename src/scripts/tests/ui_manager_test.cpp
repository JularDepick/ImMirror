/* 管理器单例断言: 界面脚本最常调用的 ClientMgr 方法. */
#include <cstdio>
#include <string>

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

int main() {
    WidgetHost host;
    const std::string layout =
        "<Ui><Frame name=\"Root\"><Size><AbsDimension x=\"100\" y=\"100\"/></Size></Frame></Ui>";
    check("载入布局", host.Load(layout.data(), layout.size()));

    const std::string script =
        "local m = GetInst(\"ClientMgr\") "
        "R_NET = m:getNetworkState() "
        "R_VER = m:getVersionParamInt() "
        "R_PC = m:isPC() "
        "R_ID = m:getDeviceID() "
        "m:setGameData(\"k\", 7) "
        "R_DATA = m:getGameData(\"k\") "
        "R_HAS = m:hasGameData(\"k\") "
        "R_APPLY = m:appalyGameSetData() ";
    check("执行脚本", host.RunScript(script.data(), script.size(), LayoutRect{0.0f, 0.0f, 100.0f, 100.0f}));

    const Value net = host.Vm().GetGlobal("R_NET");
    const Value ver = host.Vm().GetGlobal("R_VER");
    const Value pc = host.Vm().GetGlobal("R_PC");
    const Value id = host.Vm().GetGlobal("R_ID");
    const Value data = host.Vm().GetGlobal("R_DATA");
    const Value has = host.Vm().GetGlobal("R_HAS");
    const Value apply = host.Vm().GetGlobal("R_APPLY");

    check("网络状态为数字", net.kind == Value::Kind::kNumber && net.number == 1.0);
    check("版本参数为零", ver.kind == Value::Kind::kNumber && ver.number == 0.0);
    check("isPC 为假", pc.kind == Value::Kind::kBool && !pc.boolean);
    check("设备号非空", id.kind == Value::Kind::kString && !id.text.empty());
    check("共享数据可读回", data.kind == Value::Kind::kNumber && data.number == 7.0);
    check("hasGameData 为真", has.kind == Value::Kind::kBool && has.boolean);
    check("应用设置为真", apply.kind == Value::Kind::kBool && apply.boolean);
    check("脚本无错误", host.Vm().Error().empty());
    return failures == 0 ? 0 : 1;
}
