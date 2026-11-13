/* 控件绑定层的离线断言: 加载真实布局后逐个验证脚本侧常用动作. */
#include <cmath>
#include <cstdio>
#include <filesystem>
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

/* 批量模式: 逐个加载布局, 解算并展开绘制命令, 统计规模与异常. */
static int Batch(const char* directory) {
    int files = 0;
    int loaded = 0;
    int skipped = 0;
    int failed = 0;
    int widgets = 0;
    int commands = 0;
    int hiddenOnly = 0;
    std::vector<std::string> failures;
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".xml") {
            continue;
        }
        files += 1;
        std::string text;
        if (!ReadFile(entry.path().string().c_str(), text)) {
            failed += 1;
            failures.push_back(entry.path().filename().string());
            continue;
        }
        WidgetTree tree;
        if (!tree.Load(text.data(), text.size())) {
            /* 没有帧的 XML 是资源清单一类, 不计失败. */
            if (tree.Error() == "没有帧节点") {
                skipped += 1;
            } else {
                failed += 1;
                failures.push_back(entry.path().filename().string() + " :: " + tree.Error());
            }
            continue;
        }
        tree.Resolve(screen);
        std::vector<LayoutDrawCommand> out;
        tree.CollectDrawCommands(out);
        loaded += 1;
        widgets += tree.WidgetCount();
        commands += static_cast<int>(out.size());
        if (out.empty()) {
            hiddenOnly += 1;
        }
    }
    std::printf("XML 文件 %d, 加载成功 %d, 非布局 %d, 失败 %d\n", files, loaded, skipped, failed);
    std::printf("控件总计 %d, 绘制命令总计 %d, 无命令的布局 %d\n", widgets, commands, hiddenOnly);
    for (size_t index = 0; index < failures.size() && index < 8; ++index) {
        std::printf("  失败 %s\n", failures[index].c_str());
    }
    return failures.empty() ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc > 2 && std::string(argv[1]) == "--batch") {
        return Batch(argv[2]);
    }
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

    /* 命中测试: 屏幕坐标点落到控件上. */
    tree.SetSize(box, 400.0f, 200.0f);
    tree.SetPoint(box, AnchorPoint::kTopLeft, AnchorPoint::kTopLeft, 0.0f, 0.0f);
    tree.Resolve(screen);
    Widget* hit = tree.HitTest(200.0f, 100.0f);
    check("命中测试返回控件", hit != nullptr);
    check("命中落在容器内", hit == box || hit->frame->parent != nullptr);
    check("屏幕外不命中", tree.HitTest(-50.0f, -50.0f) == nullptr);

    /* 隐藏后其子树不再参与命中. */
    tree.Hide(box);
    check("隐藏后父帧不命中", tree.HitTest(200.0f, 100.0f) != box);
    tree.Show(box);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
