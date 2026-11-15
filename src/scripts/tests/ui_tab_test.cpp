/* 页签切换断言: 调用页签处理函数后, 可见面板集合必须改变. */
#include <cstdio>
#include <filesystem>
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

static bool ReadFile(const std::string& path, std::string& out) {
    FILE* file = std::fopen(path.c_str(), "rb");
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

/* 收集当前可见命令里带指定前缀的元素名. */
static std::vector<std::string> Names(WidgetHost& host, const char* prefix) {
    std::vector<std::string> names;
    std::vector<LayoutDrawCommand> commands;
    host.CollectDrawCommands(commands);
    for (const LayoutDrawCommand& command : commands) {
        if (command.name == nullptr) {
            continue;
        }
        const std::string name(command.name);
        /* 页签按钮本身也叫 GameSetFrameControlBtn, 不算面板内容. */
        if (name.rfind(prefix, 0) == 0 && name.find("Btn") == std::string::npos) {
            names.push_back(name);
        }
    }
    return names;
}

int main(int argc, char** argv) {
    const std::string layoutDir = argc > 1 ? argv[1] : "../../assets/project/data/xml/ui/mobile";
    const std::string scriptDir = argc > 2 ? argv[2] : "../../assets/project/ui/mobile";

    LayoutTemplates templates;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(layoutDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".xml") {
            continue;
        }
        std::string body;
        if (!ReadFile(entry.path().string(), body)) {
            continue;
        }
        LayoutTemplates part;
        if (ParseTemplates(body.data(), body.size(), part)) {
            templates.insert(part.begin(), part.end());
        }
    }
    /* 模板可能跨文件继承, 合并后再展开一次继承链. */
    ResolveTemplateInheritance(templates);

    std::string layout;
    check("读入设置布局", ReadFile(layoutDir + "/setting.xml", layout));
    std::string script;
    check("读入设置脚本", ReadFile(scriptDir + "/setting.lua", script));

    LayoutDocument templateDocument;
    {
        std::string templateText;
        if (ReadFile(layoutDir + "/uiconfig/uitemplate.xml", templateText)) {
            templateDocument.SetTemplates(&templates);
            templateDocument.Parse(templateText.data(), templateText.size());
        }
    }

    WidgetHost host;
    host.SetTemplates(&templates);
    host.SetTemplateDocument(&templateDocument);
    check("载入布局", host.Load(layout.data(), layout.size()));
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};
    check("执行脚本", host.RunScript(script.data(), script.size(), screen));

    Widget* frame = host.Tree().Find("GameSetFrame");
    check("找到设置主帧", frame != nullptr);
    if (frame == nullptr) {
        return 1;
    }
    host.Tree().Show(frame);
    host.Vm().CallGlobal("GameSetFrame_OnShow", {});
    host.Tree().Resolve(screen);

    Widget* base = host.Tree().Find("GameSetFrameBase");
    Widget* control = host.Tree().Find("GameSetFrameControl");
    std::printf("诊断: Base 存在=%d shown=%d | Control 存在=%d shown=%d\n", base != nullptr,
                base != nullptr && base->shown, control != nullptr, control != nullptr && control->shown);

    const std::vector<std::string> baseBefore = Names(host, "GameSetFrameBase");
    check("OnShow 后基础页签可见", !baseBefore.empty());
    std::printf("默认页签命令 %zu 条\n", baseBefore.size());
    const std::vector<std::string> controlBefore = Names(host, "GameSetFrameControl");
    std::printf("切换前控制页签命令 %zu 条\n", controlBefore.size());
    check("控制页签默认隐藏", controlBefore.empty());

    /* 点"控制"页签(id=2). */
    host.Vm().CallGlobal("SetFrameTabBtnTemplate_OnClick", {Value::Number(2.0)});
    host.Tree().Resolve(screen);
    const std::vector<std::string> controlAfter = Names(host, "GameSetFrameControl");
    std::printf("切换后控制页签命令 %zu 条\n", controlAfter.size());
    check("切换后控制页签有内容", !controlAfter.empty());
    check("页签切换改变了可见集合", controlAfter.size() != controlBefore.size());

    /*
     * 再切到"高级"页签(id=3): 页签互斥, 控制页签必须收起.
     * id=0 在原版里只做按钮状态初始化, 不切页面, 因此不能用它验证互斥.
     */
    host.Vm().CallGlobal("SetFrameTabBtnTemplate_OnClick", {Value::Number(3.0)});
    host.Tree().Resolve(screen);
    const std::vector<std::string> controlBack = Names(host, "GameSetFrameControl");
    std::printf("切到高级后控制页签命令 %zu 条\n", controlBack.size());
    check("切到高级后控制页签收起", controlBack.size() < controlAfter.size());
    return failures == 0 ? 0 : 1;
}
