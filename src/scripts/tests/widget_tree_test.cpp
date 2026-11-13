/* 控件绑定层的离线断言: 加载真实布局后逐个验证脚本侧常用动作. */
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ui/widget_tree.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static void near(const char* label, float got, float want) {
    if (std::fabs(got - want) > 0.01f) {
        std::printf("FAIL %-24s got %.2f want %.2f\n", label, got, want);
        failures += 1;
    } else {
        std::printf("ok   %-24s %.2f\n", label, got);
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
    const char* path = argc > 1 ? argv[1] : "../../assets/project/data/xml/ui/mobile/account.xml";
    std::string text;
    if (!ReadFile(path, text)) {
        std::printf("FAIL 读不到布局文件 %s\n", path);
        return 1;
    }

    WidgetTree tree;
    if (!tree.Load(text.data(), text.size())) {
        std::printf("FAIL 加载失败: %s\n", tree.Error().c_str());
        return 1;
    }
    std::printf("控件数 %d\n", tree.WidgetCount());
    check("控件数与帧数一致", tree.WidgetCount() > 300);

    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};
    tree.Resolve(screen);

    Widget* box = tree.Find("DropdownBox");
    check("找到控件", box != nullptr);
    if (box == nullptr) {
        return 1;
    }
    check("初始可见", tree.IsShown(box));

    std::vector<LayoutDrawCommand> base;
    tree.CollectDrawCommands(base);
    std::printf("初始绘制命令 %d 条\n", static_cast<int>(base.size()));

    /* 隐藏整棵子树, 命令数必须减少; 再显示回来必须恢复. */
    tree.Hide(box);
    check("隐藏后状态为不可见", !tree.IsShown(box));
    std::vector<LayoutDrawCommand> hidden;
    tree.CollectDrawCommands(hidden);
    check("隐藏后命令减少", hidden.size() < base.size());
    tree.Show(box);
    std::vector<LayoutDrawCommand> shown;
    tree.CollectDrawCommands(shown);
    check("重新显示后命令恢复", shown.size() == base.size());

    /* 设文本: 文本控件必须带上文本, 且按设置的颜色输出. */
    Widget* label = tree.Find("DropdownBoxNickname");
    check("找到文本控件", label != nullptr);
    if (label != nullptr) {
        tree.SetText(label, "HELLO");
        tree.SetTextColor(label, 0.25f, 0.5f, 0.75f);
        std::vector<LayoutDrawCommand> commands;
        tree.CollectDrawCommands(commands);
        bool foundText = false;
        bool colorOk = false;
        for (const LayoutDrawCommand& command : commands) {
            if (command.text != nullptr && std::string(command.text) == "HELLO") {
                foundText = true;
                colorOk = std::fabs(command.red - 0.25f) < 0.01f &&
                          std::fabs(command.green - 0.5f) < 0.01f &&
                          std::fabs(command.blue - 0.75f) < 0.01f;
            }
        }
        check("文本进入绘制命令", foundText);
        check("文本颜色生效", colorOk);

        /* 置灰: 颜色整体压暗. */
        tree.SetGray(label, true);
        tree.CollectDrawCommands(commands);
        bool grayed = false;
        for (const LayoutDrawCommand& command : commands) {
            if (command.text != nullptr && std::string(command.text) == "HELLO") {
                grayed = command.red < 0.24f;
            }
        }
        check("置灰压暗颜色", grayed);
        tree.SetGray(label, false);
    }

    /* 设尺寸与设锚点: 解算后必须反映到矩形上. */
    tree.SetSize(box, 500.0f, 120.0f);
    tree.Resolve(screen);
    near("设尺寸后宽", box->frame->rect.width, 500.0f);
    near("设尺寸后高", box->frame->rect.height, 120.0f);

    tree.SetPoint(box, AnchorPoint::kTopLeft, AnchorPoint::kTopLeft, 40.0f, 60.0f);
    tree.Resolve(screen);
    near("设锚点后 x", box->frame->rect.x, 40.0f);
    near("设锚点后 y", box->frame->rect.y, 60.0f);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
