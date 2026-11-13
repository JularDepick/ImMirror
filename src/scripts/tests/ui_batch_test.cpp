/* 真实界面批量跑通: 布局与同名脚本配对, 跑通并统计绘制命令. */
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "ui/script/widget_host.h"

using namespace immirror::ui;
using namespace immirror::ui::script;

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

int main(int argc, char** argv) {
    const std::string layoutDir = argc > 1 ? argv[1] : "../../assets/project/data/xml/ui/mobile";
    const std::string scriptDir = argc > 2 ? argv[2] : "../../assets/project/ui/mobile";
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    /* 脚本目录是镜像树, 按文件名建索引, 才能与布局配对. */
    std::map<std::string, std::string> scriptIndex;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
            continue;
        }
        const std::string stem = entry.path().stem().string();
        /* 同名脚本优先取层级最浅的那个. */
        if (scriptIndex.find(stem) == scriptIndex.end()) {
            scriptIndex[stem] = entry.path().string();
        }
    }

    int layouts = 0;
    int loaded = 0;
    int withScript = 0;
    int ranOk = 0;
    int commands = 0;
    int producedNothing = 0;
    int onLoadCalled = 0;
    int withSubscriptions = 0;
    int subscriptions = 0;
    std::vector<std::string> onLoadFailures;
    std::vector<std::string> loadFailures;
    std::vector<std::string> runFailures;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(layoutDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".xml") {
            continue;
        }
        layouts += 1;
        std::string layout;
        if (!ReadFile(entry.path().string(), layout)) {
            continue;
        }
        WidgetHost host;
        if (!host.Load(layout.data(), layout.size())) {
            /* 没有帧的 XML 是资源清单一类, 不算失败. */
            if (host.Error() != "没有帧节点") {
                loadFailures.push_back(entry.path().filename().string() + " :: " + host.Error());
            }
            continue;
        }
        loaded += 1;

        /* 同名脚本: 布局在 data/xml/ui/mobile, 脚本在 ui/mobile, 目录层级不同. */
        const std::string stem = entry.path().stem().string();
        std::string script;
        const auto found = scriptIndex.find(stem);
        const bool hasScript = found != scriptIndex.end() && ReadFile(found->second, script);
        if (hasScript) {
            withScript += 1;
        }
        if (!host.RunScript(hasScript ? script.data() : "", hasScript ? script.size() : 0, screen)) {
            runFailures.push_back(stem + " :: " + host.Error());
            continue;
        }
        ranOk += 1;
        /*
         * 界面脚本的主体是事件处理函数, 单纯加载不产生可见效果.
         * 这里额外调用 *_OnLoad 入口, 让初始化逻辑真正跑一遍.
         */
        if (hasScript) {
            bool calledOnLoad = false;
            for (const std::string& name : host.Vm().GlobalFunctionNames()) {
                if (name.size() <= 7 || name.compare(name.size() - 7, 7, "_OnLoad") != 0) {
                    continue;
                }
                host.Vm().CallGlobal(name, {});
                calledOnLoad = true;
                break;
            }
            if (calledOnLoad) {
                onLoadCalled += 1;
                /* 订阅数反映脚本对事件系统的依赖程度. */
                if (host.SubscriptionCount() > 0) {
                    withSubscriptions += 1;
                    subscriptions += host.SubscriptionCount();
                }
                if (!host.Vm().Error().empty()) {
                    onLoadFailures.push_back(stem + " :: " + host.Vm().Error());
                }
            }
        }
        std::vector<LayoutDrawCommand> out;
        const int32_t count = host.CollectDrawCommands(out);
        commands += count;
        if (count == 0) {
            producedNothing += 1;
        }
    }

    std::printf("布局 %d, 载入成功 %d, 有同名脚本 %d, 跑通 %d\n", layouts, loaded, withScript,
                ranOk);
    std::printf("绘制命令合计 %d, 零命令布局 %d\n", commands, producedNothing);
    std::printf("调用入口函数 %d 个, 入口执行失败 %d 个\n", onLoadCalled,
                static_cast<int>(onLoadFailures.size()));
    std::printf("订阅事件的界面 %d 个, 订阅条目合计 %d\n", withSubscriptions, subscriptions);
    for (size_t index = 0; index < onLoadFailures.size() && index < 8; ++index) {
        std::printf("  入口失败 %s\n", onLoadFailures[index].c_str());
    }
    for (size_t index = 0; index < loadFailures.size() && index < 6; ++index) {
        std::printf("  载入失败 %s\n", loadFailures[index].c_str());
    }
    for (size_t index = 0; index < runFailures.size() && index < 8; ++index) {
        std::printf("  执行失败 %s\n", runFailures[index].c_str());
    }
    return 0;
}
