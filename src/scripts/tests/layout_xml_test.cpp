/* 布局解析的端到端断言: 读一份真实布局, 解析成帧树并解算锚点. */
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_frame.h"
#include "ui/layout_xml.h"

using namespace immirror::ui;

static int failures = 0;

static void check(const char* label, bool ok) {
    if (ok) {
        std::printf("ok   %s\n", label);
    } else {
        std::printf("FAIL %s\n", label);
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

/* 批量模式: 遍历目录下全部布局, 统计解析成功率与帧总数. */
static int Batch(const char* directory) {
    int files = 0;
    int parsed = 0;
    int skipped = 0;
    int frames = 0;
    std::vector<std::string> failed;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".xml") {
            continue;
        }
        files += 1;
        std::string text;
        if (!ReadFile(entry.path().string().c_str(), text)) {
            failed.push_back(entry.path().filename().string());
            continue;
        }
        LayoutDocument document;
        if (!document.Parse(text.data(), text.size())) {
            /* 没有帧的 XML 是资源清单或按键表一类, 不是布局, 单独计数. */
            if (document.Error() == "没有帧节点") {
                skipped += 1;
            } else {
                failed.push_back(entry.path().string() + " :: " + document.Error());
            }
            continue;
        }
        parsed += 1;
        frames += document.FrameCount();
    }
    std::printf("XML 文件 %d, 含帧布局 %d, 非布局 %d, 解析失败 %d, 帧总计 %d\n", files, parsed,
                skipped, static_cast<int>(failed.size()), frames);
    for (size_t index = 0; index < failed.size() && index < 10; ++index) {
        std::printf("  失败 %s\n", failed[index].c_str());
    }
    return failed.empty() ? 0 : 1;
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
    std::printf("布局字节数 %zu\n", text.size());

    LayoutDocument document;
    if (!document.Parse(text.data(), text.size())) {
        std::printf("FAIL 解析失败: %s\n", document.Error().c_str());
        return 1;
    }
    std::printf("帧数 %d\n", document.FrameCount());
    check("帧数足够多", document.FrameCount() > 20);

    LayoutFrame* box = document.Find("DropdownBox");
    check("找到 DropdownBox", box != nullptr);
    if (box == nullptr) {
        return 1;
    }
    near("DropdownBox.width", box->width, 266.0f);
    near("DropdownBox.height", box->height, 68.0f);

    /* 名字里的 $parent 必须展开成父帧名字. */
    LayoutFrame* nickname = document.Find("DropdownBoxNickname");
    check("找到展开后的子帧名", nickname != nullptr);
    if (nickname == nullptr) {
        return 1;
    }
    near("Nickname.width", nickname->width, 80.0f);
    check("子帧有锚点", !nickname->anchors.empty());
    check("锚点指向父帧", !nickname->anchors.empty() && nickname->anchors[0].relativeTo == box);

    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};
    ResolveLayout(*box, screen);
    near("Nickname.x", nickname->rect.x, 15.0f);
    near("Nickname.y", nickname->rect.y, 10.0f);
    check("父子帧均完成解算", nickname->rect.width > 0.0f);

    /* 绘制命令: 帧树展开成矩形序列. */
    std::vector<LayoutDrawCommand> commands;
    const int32_t count = CollectDrawCommands(*box, commands);
    std::printf("绘制命令 %d 条\n", count);
    check("生成了绘制命令", count > 0);
    /*
     * 布局里大量帧只给几何, 贴图与文字由脚本在运行时设置,
     * 因此能直接画出来的帧只占少数. 这个比例是判断"纯数据驱动能走多远"的依据.
     */
    std::printf("帧总数 %d, 可直接绘制 %d (%.1f%%)\n", document.FrameCount(), count,
                document.FrameCount() > 0
                    ? 100.0 * static_cast<double>(count) / static_cast<double>(document.FrameCount())
                    : 0.0);
    bool allSized = true;
    bool allWithinRoot = true;
    for (const LayoutDrawCommand& command : commands) {
        if (command.rect.width <= 0.0f || command.rect.height <= 0.0f) {
            allSized = false;
        }
        /* 子帧允许溢出父帧, 但整体必须落在屏幕矩形内. */
        if (command.rect.x < -1.0f || command.rect.y < -1.0f ||
            command.rect.x + command.rect.width > 1281.0f ||
            command.rect.y + command.rect.height > 721.0f) {
            allWithinRoot = false;
        }
    }
    check("每条命令都有正尺寸", allSized);
    check("命令整体落在屏幕内", allWithinRoot);

    /* 有颜色的帧必须把颜色带进命令. */
    int32_t colored = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (command.red < 0.999f || command.green < 0.999f || command.blue < 0.999f) {
            colored += 1;
        }
    }
    std::printf("带非白颜色的命令 %d 条\n", colored);
    check("颜色被解析进命令", colored > 0);

    /* 解析失败要带行号, 否则上千行的布局文件无法定位. */
    {
        LayoutDocument broken;
        const char* text = "<Root>\n<Frame name=\"A\">\n</Frame>\n<!-- unclosed\n";
        check("未闭合注释被拒", !broken.Parse(text, std::string(text).size()));
        check("错误信息带行号", broken.Error().find("行") != std::string::npos);
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
