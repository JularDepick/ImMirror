/*
 * 屏幕转储: 载入布局, 跑同名界面脚本, 调用 *_OnLoad 入口, 打开指定主帧, 再输出可见绘制命令.
 * 用法: ui_screen_dump <布局.xml> <脚本目录> <模板目录> [--frame 名字]... [--list]
 */
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "ui/layout_render.h"
#include "ui/script/widget_host.h"

using namespace immirror::ui;
using namespace immirror::ui::script;

/* 假数据源: 离线跑界面脚本时提供确定的游戏状态, 让列表克隆分支能走到. */
class StubDataSource : public UiDataSource {
public:
    BlockDefView BlockDef(int32_t id) const override {
        BlockDefView view;
        view.found = true;
        view.id = id;
        view.englishName = "stone";
        view.hardness = 1.5f;
        view.tool = 1;
        view.material = 1;
        view.toolLevel = 1;
        view.shape = 0;
        view.gravity = 0;
        view.replaceable = 0;
        view.dropChance = 10000;
        view.blastResist = 15;
        view.slipperiness = 1.0f;
        return view;
    }
    ItemDefView BlockItemDef(int32_t id) const override {
        ItemDefView view = ItemDef(id);
        view.found = true;
        return view;
    }
    ItemDefView ItemDef(int32_t id) const override {
        ItemDefView view;
        view.found = true;
        view.id = id;
        view.displayName = "Stone";
        view.englishName = "stone";
        view.icon = "stone";
        view.category = 1;
        view.maxStack = 64;
        view.quality = 1;
        return view;
    }
    int32_t BagSlotCount() const override { return 36; }
    std::string BagSlotName(int32_t slot) const override {
        return slot >= 0 && slot < 8 ? std::string("stone") : std::string();
    }
    int32_t BagSlotCountAt(int32_t slot) const override {
        return slot >= 0 && slot < 8 ? 64 : 0;
    }
    int32_t BagSlotDurability(int32_t slot) const override {
        return slot >= 0 && slot < 4 ? 100 : 0;
    }
    float Health() const override { return 0.85f; }
    float Hunger() const override { return 0.70f; }
    float Stamina() const override { return 0.60f; }
    std::string WorldName() const override { return std::string("homegarden"); }
    float WorldTimeSeconds() const override { return 8.0f * 3600.0f; }
    int64_t ProgressValue(const std::string& key) const override {
        (void)key;
        return 0;
    }
};

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

static std::string Escape(const char* text) {
    std::string out;
    if (text == nullptr) {
        return out;
    }
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == '\t') {
            out += "\\t";
        } else if (*p == '\n') {
            out += "\\n";
        } else {
            out += *p;
        }
    }
    return out;
}

static const char* LayerName(LayerLevel level) {
    switch (level) {
        case LayerLevel::kBackground: return "background";
        case LayerLevel::kBorders: return "borders";
        case LayerLevel::kArtwork: return "artwork";
        case LayerLevel::kOverlay: return "overlay";
        case LayerLevel::kHighlight: return "highlight";
    }
    return "?";
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "用法: ui_screen_dump <布局.xml> <脚本目录> <模板目录> [--frame 名字]... [--list]\n");
        return 2;
    }
    const std::string layoutPath = argv[1];
    const std::string scriptDir = argv[2];
    const std::string templateDir = argv[3];
    std::vector<std::string> showFrames;
    /* 共享脚本: 界面脚本依赖的公共函数(如 UpdateItemIconCount)在这里定义. */
    std::vector<std::string> prelude;
    /* 按索引调用的刷新函数: 例如背包逐格刷新. */
    std::string callName;
    int32_t callTimes = 0;
    /* 名字检索: 只列出名字里含该片段的控件, 用于核对 `$parent` 展开后的实际名字. */
    std::string nameFilter;
    /* 点击冒烟: 模拟点击指定元素, 验证它的 OnClick 脚本能跑通. */
    std::string clickName;
    /* 批量点击时最多尝试的个数. */
    int32_t clickCount = 0;
    /* 打开主帧时是否走脚本路径(触发 OnLoad/OnShow). */
    bool useEvents = false;
    /* 只统计该帧子树: 用于核对子界面, 主帧仍打开提供参照物. */
    std::string onlyName;
    /* 没指定名字时点第一个带 OnClick 的可见元素. */
    bool clickFirst = false;
    /* 显示状态打点: 追踪某个控件在各阶段的 shown 值. */
    std::string traceName;
    bool listOnly = false;
    for (int i = 4; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frame") == 0 && i + 1 < argc) {
            showFrames.push_back(argv[++i]);
        } else if (std::strcmp(argv[i], "--prelude") == 0 && i + 1 < argc) {
            prelude.push_back(argv[++i]);
        } else if (std::strcmp(argv[i], "--call") == 0 && i + 1 < argc) {
            callName = argv[++i];
        } else if (std::strcmp(argv[i], "--times") == 0 && i + 1 < argc) {
            callTimes = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--click") == 0 && i + 1 < argc) {
            clickName = argv[++i];
        } else if (std::strcmp(argv[i], "--only") == 0 && i + 1 < argc) {
            onlyName = argv[++i];
        } else if (std::strcmp(argv[i], "--use-events") == 0) {
            useEvents = true;
        } else if (std::strcmp(argv[i], "--click-all") == 0 && i + 1 < argc) {
            clickFirst = true;
            clickCount = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--click-first") == 0) {
            clickFirst = true;
        } else if (std::strcmp(argv[i], "--trace") == 0 && i + 1 < argc) {
            traceName = argv[++i];
        } else if (std::strcmp(argv[i], "--names") == 0 && i + 1 < argc) {
            nameFilter = argv[++i];
        } else if (std::strcmp(argv[i], "--list") == 0) {
            listOnly = true;
        }
    }
    /* 没有 Size 的元素按图集精灵的原始像素补齐尺寸. */
    SetSpriteSizeResolver(&UiSpriteSize);
    SetTextureSizeResolver(&UiPageSize);
    ResetMissingAnchorReferenceCount();
    (void)0;
    const LayoutRect screen{0.0f, 0.0f, 1280.0f, 720.0f};

    LayoutTemplates templates;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(templateDir)) {
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
    if (!ReadFile(layoutPath, layout)) {
        std::fprintf(stderr, "读不到布局: %s\n", layoutPath);
        return 2;
    }

    StubDataSource data;
    /* 模板文件解析成帧树: 跨文件的模板靠它提供子树. */
    LayoutDocument templateDocument;
    {
        std::string templateText;
        if (ReadFile(templateDir + "/uiconfig/uitemplate.xml", templateText)) {
            /* 模板文档自己也要按模板表展开继承, 否则克隆出来的子元素没有贴图与尺寸. */
            templateDocument.SetTemplates(&templates);
            templateDocument.Parse(templateText.data(), templateText.size());
        }
    }

    /*
     * 参照文档: 游戏内 HUD 布局. 界面里常用 PlayShortcut 这类 HUD 帧做锚点参照,
     * 单独加载本屏布局时那些帧不存在, 锚点会退化为父帧, 整棵子树位置偏离.
     */
    /*
     * 参照文档: 游戏内 HUD 与常被引用的界面布局.
     * 不载入全部布局: 同名帧(如通用的 Bkg2)在别的界面里坐标不同,
     * 全量载入会把锚点指到错误位置, 反而让本屏元素跑到画布外.
     */
    std::vector<std::unique_ptr<LayoutDocument>> referenceDocuments;
    std::vector<std::string> referenceNames = {"playmain.xml", "setting.xml", "messagebox.xml"};
    for (const std::string& name : referenceNames) {
        std::string referenceText;
        if (!ReadFile(templateDir + "/" + name, referenceText) || referenceText.empty()) {
            continue;
        }
        std::unique_ptr<LayoutDocument> document(new LayoutDocument());
        document->SetTemplates(&templates);
        document->SetTemplateDocument(&templateDocument);
        if (!document->Parse(referenceText.data(), referenceText.size())) {
            continue;
        }
        ResolveLayout(*document->Root(), LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f});
        referenceDocuments.push_back(std::move(document));
    }

    std::fprintf(stderr, "参照文档 %d 个\n", static_cast<int>(referenceDocuments.size()));

    WidgetHost host;
    host.SetTemplates(&templates);
    host.SetTemplateDocument(&templateDocument);
    for (const std::unique_ptr<LayoutDocument>& document : referenceDocuments) {
        host.AddReferenceDocument(document.get());
    }
    /* 参照文档解算时的缺失不计入本屏, 从这里重新起算. */
    ResetMissingAnchorReferenceCount();
    host.BindDataSource(&data);
    if (!host.Load(layout.data(), layout.size())) {
        std::fprintf(stderr, "布局载入失败: %s\n", host.Error().c_str());
        return 1;
    }

    const std::string stem = std::filesystem::path(layoutPath).stem().string();
    std::string scriptPath;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") {
            continue;
        }
        if (entry.path().stem().string() == stem) {
            scriptPath = entry.path().string();
            break;
        }
    }
    /* 脚本错误: 静默中断会让命令数变少而不报警, 必须单独统计. */
    std::vector<std::string> scriptErrors;
    const auto noteError = [&]() {
        const std::string& message = host.Vm().Error();
        if (!message.empty() &&
            std::find(scriptErrors.begin(), scriptErrors.end(), message) == scriptErrors.end()) {
            scriptErrors.push_back(message);
        }
    };

    const auto trace = [&](const char* label) {
        if (traceName.empty()) {
            return;
        }
        Widget* widget = host.Tree().Find(traceName.c_str());
        std::fprintf(stderr, "  [trace] %-14s %s shown=%d\n", label, traceName.c_str(),
                     widget == nullptr ? -1 : (widget->shown ? 1 : 0));
    };
    trace("载入后");

    /* 先跑共享脚本, 再跑本屏脚本: 与引擎的加载顺序一致. */
    for (const std::string& name : prelude) {
        std::string shared;
        if (ReadFile(scriptDir + "/" + name, shared)) {
            host.RunScript(shared.data(), shared.size(), screen);
            noteError();
        }
    }

    std::string script;
    const bool hasScript = !scriptPath.empty() && ReadFile(scriptPath, script);
    if (!host.RunScript(hasScript ? script.data() : "", hasScript ? script.size() : 0, screen)) {
        std::fprintf(stderr, "脚本执行失败: %s\n", host.Error().c_str());
        return 1;
    }

    /* 入口: 逐个调用脚本里声明的 *_OnLoad, 顺序按名字排序, 保证可复现. */
    int onLoadCalls = 0;
    if (hasScript) {
        std::vector<std::string> entries;
        for (const std::string& name : host.Vm().GlobalFunctionNames()) {
            if (name.size() > 7 && name.compare(name.size() - 7, 7, "_OnLoad") == 0) {
                entries.push_back(name);
            }
        }
        std::sort(entries.begin(), entries.end());
        if (std::getenv("DUMP_TRACE_ONLOAD") != nullptr) {
            std::string joined;
            for (const std::string& name : entries) {
                joined += name;
                joined += " ";
            }
            std::fprintf(stderr, "OnLoad 清单(%d): %s\n", static_cast<int>(entries.size()),
                         joined.c_str());
        }
        for (const std::string& name : entries) {
            host.Vm().CallGlobal(name, {});
            onLoadCalls += 1;
            noteError();
        }
    }

    /*
     * 原版的界面靠事件驱动: 载入后游戏会先派发一批初始事件(世界列表变化, 网络状态等),
     * 界面在这些事件里把列表克隆出来并显示. 这里按已订阅的事件补跑一轮,
     * 让离线转储尽量贴近原版的初始状态.
     */
    int dispatched = 0;
    for (int round = 0; round < 2; ++round) {
        const std::vector<std::string> events = host.SubscribedEvents();
        if (events.empty()) {
            break;
        }
        for (const std::string& event : events) {
            host.DispatchEvent(event, {});
            dispatched += 1;
            noteError();
        }
    }

    trace("刷新后");
    if (!scriptErrors.empty()) {
        std::fprintf(stderr, "脚本错误 %zu 处: %s\n", scriptErrors.size(), scriptErrors[0].c_str());
    }

    if (false) {
        int32_t hits = 0;
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (widget == nullptr || widget->frame == nullptr || widget->frame->name == nullptr) {
                continue;
            }
            const std::string name(widget->frame->name);
            if (name.find(nameFilter) == std::string::npos) {
                continue;
            }
            hits += 1;
            std::printf("%s | shown=%d | %.0fx%.0f | 贴图=%s | 精灵=%s\n", name.c_str(),
                        widget->shown ? 1 : 0, widget->frame->rect.width,
                        widget->frame->rect.height,
                        widget->frame->texture == nullptr ? "-" : widget->frame->texture,
                        widget->frame->sprite == nullptr ? "-" : widget->frame->sprite);
        }
        std::fprintf(stderr, "名字含 [%s] 的控件 %d 个\n", nameFilter.c_str(), hits);
        return 0;
    }

    if (listOnly) {
        for (const std::string& name : host.Vm().GlobalFunctionNames()) {
            std::printf("%s\n", name.c_str());
        }
        std::fprintf(stderr, "全局函数 %zu 个, OnLoad 调用 %d 次\n",
                     host.Vm().GlobalFunctionNames().size(), onLoadCalls);
        return 0;
    }

    int shown = 0;
    int onShowCalls = 0;
    const std::vector<std::string> globals = host.Vm().GlobalFunctionNames();
    const auto hasGlobal = [&globals](const std::string& name) {
        return std::find(globals.begin(), globals.end(), name) != globals.end();
    };
    for (const std::string& name : showFrames) {
        Widget* widget = host.Tree().Find(name.c_str());
        if (widget == nullptr) {
            std::fprintf(stderr, "找不到主帧: %s\n", name.c_str());
            continue;
        }
        /*
         * 两种打开方式: 直接改可见标志(快, 不触发脚本), 或走脚本的 Show
         * (会触发 OnLoad/OnShow, 更接近真实运行路径).
         */
        if (useEvents) {
            const std::string script = "local w = getglobal(\"" + name + "\") if w ~= nil then w:Show() end";
            if (!host.RunScript(script.c_str(), script.size(), LayoutRect{0.0f, 0.0f, 1280.0f, 720.0f})) {
                noteError();
            }
        } else {
            host.Tree().Show(widget);
        }
        trace("显示主帧后");
        shown += 1;
        /*
         * 原版在帧显示后调用 <帧名>_OnShow: 页签选中, 列表填充多在这里发生.
         * 只跑 OnLoad 会漏掉整块面板, 因此显示后补一次.
         */
        /* 走事件链时 Show 已经触发过 OnShow, 不能再手动补一次. */
        const std::string onShow = useEvents ? std::string() : (name + "_OnShow");
        if (hasGlobal(onShow)) {
            host.Vm().CallGlobal(onShow, {});
            onShowCalls += 1;
            noteError();
        }
    }
    /* 按索引调用刷新函数(例如背包逐格刷新). */
    int32_t calls = 0;
    if (!callName.empty() && callTimes > 0) {
        for (int32_t index = 1; index <= callTimes; ++index) {
            host.Vm().CallGlobal(callName, {Value::Number(static_cast<double>(index))});
            calls += 1;
            noteError();
        }
        host.Tree().Resolve(screen);
    }

    /* 点击冒烟占位: 实际执行放在收集命令之后, 避免关闭类按钮影响绘制统计. */
    if (false) {
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (widget == nullptr || widget->frame == nullptr || !widget->shown) {
                continue;
            }
            if (widget->frame->onClick != nullptr && widget->frame->name != nullptr) {
                clickName = widget->frame->name;
                break;
            }
        }
    }
    /* OnShow 里可能新订阅事件, 再派发一轮. */
    for (const std::string& event : host.SubscribedEvents()) {
        host.DispatchEvent(event, {});
        noteError();
        if (!traceName.empty()) {
            Widget* widget = host.Tree().Find(traceName.c_str());
            std::fprintf(stderr, "  [trace] 事件 %-20s shown=%d\n", event.c_str(),
                         widget == nullptr ? -1 : (widget->shown ? 1 : 0));
        }
    }

    /* 应绘制名单: 统计阶段收集, 命令收集后与实际绘制对比. */
    std::map<std::string, int32_t> expectedDraws;

    /* 主帧子树统计: 有多少帧, 多少可绘制, 多少当前可见. */
    {
        std::map<const LayoutFrame*, bool> shownByFrame;
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (widget != nullptr) {
                shownByFrame[widget->frame] = widget->shown;
            }
        }
        for (const std::string& name : showFrames) {
            Widget* root = host.Tree().Find(name.c_str());
            if (root == nullptr) {
                continue;
            }
            int total = 0;
            int drawable = 0;
            int visible = 0;
            int zeroSize = 0;
            int eligible = 0;
            /* 可见性沿祖先传递: 祖先隐藏时子树整棵不绘制. */
            std::function<void(const LayoutFrame*, bool)> walk = [&](const LayoutFrame* frame,
                                                                     bool ancestorsShown) {
                total += 1;
                const auto found = shownByFrame.find(frame);
                const bool selfShown = found != shownByFrame.end() ? found->second : true;
                const bool shown = ancestorsShown && selfShown;
                const bool isDrawable = frame->hasColor || frame->texture != nullptr ||
                                        (frame->text != nullptr && frame->text[0] != '\0');
                if (isDrawable) {
                    drawable += 1;
                }
                if (frame->rect.width <= 0.0f || frame->rect.height <= 0.0f) {
                    zeroSize += 1;
                }
                if (shown) {
                    visible += 1;
                }
                if (isDrawable && shown && frame->rect.width > 0.0f && frame->rect.height > 0.0f) {
                    eligible += 1;
                    if (frame->name != nullptr) {
                        expectedDraws[frame->name] += 1;
                    }
                }
                for (const LayoutFrame* child : frame->children) {
                    walk(child, shown);
                }
            };
            walk(root->frame, true);
            std::fprintf(stderr, "  子树 %s: 帧 %d, 可绘制 %d, 可见 %d, 零尺寸 %d, 应输出 %d\n",
                         name.c_str(), total, drawable, visible, zeroSize, eligible);
            /* 零尺寸帧按来源分类, 便于逐类补规则. */
            int noAnchor = 0;
            int spriteMissing = 0;
            int anchorUnpaired = 0;
            int other = 0;
            std::function<void(const LayoutFrame*)> classify = [&](const LayoutFrame* frame) {
                const bool zero = frame->rect.width <= 0.0f || frame->rect.height <= 0.0f;
                if (zero) {
                    if (frame->anchors.empty()) {
                        noAnchor += 1;
                    } else if (frame->sprite != nullptr && frame->sprite[0] != '\0') {
                        float sw = 0.0f;
                        float sh = 0.0f;
                        if (!UiSpriteSize(frame->sprite, &sw, &sh)) {
                            spriteMissing += 1;
                        } else {
                            anchorUnpaired += 1;
                        }
                    } else {
                        anchorUnpaired += 1;
                    }
                }
                for (const LayoutFrame* child : frame->children) {
                    classify(child);
                }
            };
            classify(root->frame);
            /* 样例: 零尺寸且可绘制的帧, 用来判断缺的是精灵名还是尺寸规则. */
            int samples = 0;
            std::function<void(const LayoutFrame*)> showSample = [&](const LayoutFrame* frame) {
                if (samples >= 6) {
                    return;
                }
                const bool zero = frame->rect.width <= 0.0f || frame->rect.height <= 0.0f;
                const bool isDrawable = frame->hasColor || frame->texture != nullptr ||
                                        (frame->text != nullptr && frame->text[0] != '\0');
                if (zero && isDrawable) {
                    samples += 1;
                    std::fprintf(stderr, "    样例 %s: 锚点 %zu, 贴图 %s, 精灵 %s\n", frame->name,
                                 frame->anchors.size(),
                                 frame->texture == nullptr ? "-" : frame->texture,
                                 frame->sprite == nullptr ? "-" : frame->sprite);
                }
                for (const LayoutFrame* child : frame->children) {
                    showSample(child);
                }
            };
            showSample(root->frame);
            std::fprintf(stderr, "    零尺寸来源: 无锚点 %d, 精灵未命中 %d, 锚点不成对 %d\n",
                         noAnchor, spriteMissing, anchorUnpaired);
        }
    }

    /* 名字检索放在显示主帧之后, 这样看到的 shown 状态与绘制阶段一致. */
    if (!nameFilter.empty()) {
        int32_t hits = 0;
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (widget == nullptr || widget->frame == nullptr || widget->frame->name == nullptr) {
                continue;
            }
            const std::string name(widget->frame->name);
            if (name.find(nameFilter) == std::string::npos) {
                continue;
            }
            hits += 1;
            std::printf("%s | shown=%d | %.0fx%.0f | 贴图=%s | 精灵=%s\n", name.c_str(),
                        widget->shown ? 1 : 0, widget->frame->rect.width,
                        widget->frame->rect.height,
                        widget->frame->texture == nullptr ? "-" : widget->frame->texture,
                        widget->frame->sprite == nullptr ? "-" : widget->frame->sprite);
        }
        std::fprintf(stderr, "名字含 [%s] 的控件 %d 个\n", nameFilter.c_str(), hits);
        return 0;
    }

    std::vector<LayoutDrawCommand> commands;
    /*
     * 收集前统一解算一次: 脚本里的 SetPoint / SetSize 只写进控件覆盖,
     * 不解算就不会反映到帧矩形上, 格子这类靠脚本排布的元素会全部叠在原位.
     */
    host.Tree().Resolve(screen);
    const int32_t count = onlyName.empty()
                              ? host.CollectDrawCommands(commands)
                              : host.CollectDrawCommandsFrom(onlyName.c_str(), commands);

    /*
     * 点击冒烟: 在绘制统计之后执行.
     * 关闭类按钮会把主帧隐藏, 放在统计之前会让该屏命令数归零.
     */
    if (clickFirst) {
        /* 点击前 N 个带脚本的可见元素, 统计执行成功与失败. */
        const int32_t limit = clickCount > 0 ? clickCount : 1;
        int32_t tried = 0;
        int32_t handled = 0;
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (tried >= limit) {
                break;
            }
            if (widget == nullptr || widget->frame == nullptr || !widget->shown) {
                continue;
            }
            if (widget->frame->onClick == nullptr || widget->frame->name == nullptr) {
                continue;
            }
            tried += 1;
            const int32_t ran = host.Click(widget.get());
            if (ran > 0) {
                handled += 1;
            }
            noteError();
        }
        std::fprintf(stderr, "点击冒烟: 尝试 %d 个, 执行成功 %d 个, 错误=[%s]\n",
                     tried, handled, host.Vm().Error().c_str());
        clickName.clear();
    }
    if (!clickName.empty()) {
        Widget* target = host.Tree().Find(clickName.c_str());
        if (target == nullptr) {
            std::fprintf(stderr, "点击冒烟 %s: 控件未找到\n", clickName.c_str());
        } else if (target->frame == nullptr || target->frame->onClick == nullptr) {
            std::fprintf(stderr, "点击冒烟 %s: 没有 OnClick 脚本\n", clickName.c_str());
        } else {
            const int32_t handled = host.Click(target);
            std::fprintf(stderr, "点击冒烟 %s: 处理函数 %d 个, 错误=[%s]\n", clickName.c_str(),
                         handled, host.Vm().Error().c_str());
            noteError();
        }
    }

    /*
     * 覆盖核对: 应绘制的元素是否都产出了命令.
     * 差异说明元素被判定可见却画不出来, 是内容完整性的直接指标.
     */
    if (!expectedDraws.empty()) {
        std::map<std::string, int32_t> drawn;
        for (const LayoutDrawCommand& command : commands) {
            if (command.name != nullptr) {
                drawn[command.name] += 1;
            }
        }
        std::string missing;
        int32_t missingCount = 0;
        for (const auto& entry : expectedDraws) {
            const auto hit = drawn.find(entry.first);
            const int32_t have = hit == drawn.end() ? 0 : hit->second;
            if (have < entry.second) {
                missingCount += entry.second - have;
                if (missing.empty()) {
                    missing = entry.first;
                }
            }
        }
        if (missingCount > 0) {
            std::fprintf(stderr, "提示 应绘制未绘制 %d 个, 首个 %s\n", missingCount, missing.c_str());
        }
    }

    /*
     * 顶点统计: 文字描边会额外生成四组深色文字, 命令数不变但四元组数增加.
     * 用它与命令数一起核对渲染链路是否真的把描边画出来.
     */
    /* 交互覆盖: 带点击脚本的元素与按钮形态的元素分开统计. */
    int32_t withClickScript = 0;
    int32_t withButtonTexture = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->onClick != nullptr) {
            withClickScript += 1;
        }
        if (frame->normalTexture != nullptr || frame->pushedTexture != nullptr) {
            withButtonTexture += 1;
        }
    }

    /* 九宫切片覆盖: 带内边距的贴图元素按九宫绘制, 数量与普通贴图分开统计. */
    /*
     * 图层顺序不作为指标: 绘制阶段已按图层稳定排序, 布局里子元素的声明顺序
     * 与图层不一致属正常写法, 统计它只会产生噪声. 试过两种口径(全局单调、
     * 同父帧相邻)都不反映真实问题, 已移除.
     */
    const int32_t layerJumps = 0;

    /* 零面积精灵: 命中图集但宽高为 0, 画出来是空白. */
    int32_t zeroSprite = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (command.sprite == nullptr || command.sprite[0] == '\0') {
            continue;
        }
        float spriteWidth = 0.0f;
        float spriteHeight = 0.0f;
        if (UiSpriteSize(command.sprite, &spriteWidth, &spriteHeight) &&
            (spriteWidth <= 0.0f || spriteHeight <= 0.0f)) {
            zeroSprite += 1;
        }
    }

    /*
     * 模板克隆完整性: 实例的子元素数不应少于模板定义的子元素数.
     * 少了说明克隆没拿到全部子元素.
     */
    /*
     * 结构自洽: 每个帧必须出现在它声明的父帧的子帧列表里.
     * 不在说明父子关系断裂, 位置与层级都会算错.
     */
    /*
     * 死按钮: 有普通态帖图但既无点击脚本也无其他帖图与子元素.
     * 这类元素点不了, 通常是类型或外观没解析到.
     */
    /*
     * 类型不符: 布局里写 <Button>, 实例上却没点击脚本也没任何帖图.
     * 说明按钮类型或外观没解析到.
     */
    /*
     * 精灵尺寸不作为指标: 大量元素用精灵做平铺与拉伸,
     * 尺寸本就不等于精灵原始尺寸(大厅一屏就有 952 处).
     * 要区分就得逐个判断是否应该拉伸, 代价太高.
     */
    const int32_t spriteSizeMismatch = 0;

    int32_t typeMismatch = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->tag == nullptr || std::string(frame->tag) != "Button") {
            continue;
        }
        if (frame->onClick == nullptr && frame->normalTexture == nullptr &&
            frame->pushedTexture == nullptr && frame->checkedTexture == nullptr &&
            frame->texture == nullptr && frame->children.empty()) {
            /* 全屏透明按钮通常是拖拽交互区(如大厅的视角旋转), 没有 OnClick 属正常. */
            const bool fullScreenArea = frame->rect.width >= 1200.0f && frame->rect.height >= 600.0f;
            if (!fullScreenArea) {
                typeMismatch += 1;
            }
        }
    }

    int32_t deadButton = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->normalTexture == nullptr) {
            continue;
        }
        if (frame->onClick == nullptr && frame->children.empty() && frame->texture == nullptr &&
            frame->pushedTexture == nullptr && frame->checkedTexture == nullptr) {
            deadButton += 1;
        }
    }

    int32_t brokenParent = 0;
    {
        std::set<const LayoutFrame*> reachable;
        std::function<void(const LayoutFrame*)> walkTree = [&](const LayoutFrame* node) {
            if (node == nullptr || !reachable.insert(node).second) {
                return;
            }
            for (const LayoutFrame* child : node->children) {
                if (child != nullptr) {
                    if (child->parent != node) {
                        brokenParent += 1;
                    }
                    walkTree(child);
                }
            }
        };
        for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
            if (widget != nullptr && widget->frame != nullptr && widget->frame->parent == nullptr) {
                walkTree(widget->frame);
            }
        }
    }

    /*
     * 尺寸与模板声明不符: 实例自己没写尺寸、却与模板声明的尺寸不一致.
     * 尺寸没继承到会让元素被拉伸或缩小.
     */
    int32_t sizeMismatch = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->inherits == nullptr || frame->inherits[0] == 0) {
            continue;
        }
        const LayoutFrame* templateFrame = templateDocument.Find(frame->inherits);
        if (templateFrame == nullptr) {
            continue;
        }
        if (templateFrame->width <= 0.0f || templateFrame->height <= 0.0f) {
            continue;
        }
        if (frame->width <= 0.0f && frame->height <= 0.0f && frame->relWidth <= 0.0f &&
            frame->relHeight <= 0.0f) {
            if (frame->rect.width <= 0.0f || frame->rect.height <= 0.0f) {
                sizeMismatch += 1;
            }
        }
    }

    /*
     * 零尺寸有内容: 有贴图或文字, 但宽或高为 0, 会被跳过不画.
     */
    int32_t zeroSizedContent = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        const bool hasLook = frame->texture != nullptr || frame->normalTexture != nullptr ||
                             frame->checkedTexture != nullptr ||
                             (frame->text != nullptr && frame->text[0] != 0);
        if (!hasLook) {
            continue;
        }
        if (frame->rect.width <= 0.0f || frame->rect.height <= 0.0f) {
            zeroSizedContent += 1;
        }
    }

    int32_t shortClone = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->inherits == nullptr || frame->inherits[0] == 0) {
            continue;
        }
        const LayoutFrame* templateFrame = templateDocument.Find(frame->inherits);
        if (templateFrame == nullptr || templateFrame->children.empty()) {
            continue;
        }
        if (frame->children.size() < templateFrame->children.size()) {
            shortClone += 1;
        }
    }

    /*
     * 贴图链路断裂: 布局里给了贴图但绘制命令里没有贴图.
     * 常见于按下态选了空的按下贴图, 或字段没传到命令上.
     */
    int32_t brokenTexture = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (command.texture != nullptr && command.texture[0] != 0) {
            continue;
        }
        if (command.name == nullptr) {
            continue;
        }
        Widget* widget = host.Tree().Find(command.name);
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->texture != nullptr || frame->normalTexture != nullptr ||
            frame->pushedTexture != nullptr || frame->checkedTexture != nullptr) {
            brokenTexture += 1;
        }
    }

    /*
     * 隐形可点击不作为指标: 祖先隐藏时元素自身仍标记为显示,
     * 单看自身会误报, 而看是否产出命令又会把未打开的面板全算进去.
     * 外观丢失已由“空壳继承”与“贴图断”两项覆盖.
     */
    const int32_t invisibleClickable = 0;

    /*
     * 模板克隆完整性: 写了 inherits 却一个子元素都没有、自己也没有贴图的实例,
     * 说明模板子树没克隆进来, 画出来只剩空壳.
     */
    int32_t emptyInherits = 0;
    for (const std::unique_ptr<Widget>& widget : host.Tree().Widgets()) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        const LayoutFrame* frame = widget->frame;
        if (frame->inherits != nullptr && frame->inherits[0] != '\0' && frame->children.empty() &&
            frame->texture == nullptr && frame->normalTexture == nullptr) {
            emptyInherits += 1;
        }
    }

    /* 颜色合法性: 通道值必须落在 0 到 1 之间, 越界说明解析出错. */
    int32_t badColor = 0;
    for (const LayoutDrawCommand& command : commands) {
        const float channels[4] = {command.red, command.green, command.blue, command.alpha};
        for (const float value : channels) {
            if (value < 0.0f || value > 1.0f) {
                badColor += 1;
                break;
            }
        }
    }

    int32_t sliced = 0;
    int32_t tiled = 0;
    for (const LayoutDrawCommand& command : commands) {
        if (command.insetLeft > 0.0f || command.insetRight > 0.0f || command.insetTop > 0.0f ||
            command.insetBottom > 0.0f) {
            sliced += 1;
        } else if (command.tileWidth > 0.0f && command.tileHeight > 0.0f &&
                   (command.rect.width > command.tileWidth ||
                    command.rect.height > command.tileHeight)) {
            tiled += 1;
        }
    }

    LayoutRenderStats renderStats;
    {
        std::vector<float> vertices(static_cast<size_t>(200000) * kLayoutFloatsPerVertex, 0.0f);
        EmitLayoutCommands(commands, 1280.0f, 720.0f, vertices.data(), 200000, &renderStats);
    }
    /* 缺失参照物: 锚点写了 relativeTo 但离线树里找不到, 位置会退化为父帧. */
    const int32_t missingRefs = MissingAnchorReferenceCount();
    if (missingRefs > 0) {
        std::string names;
        for (const std::string& name : MissingAnchorReferenceNames()) {
            if (!names.empty()) {
                names += " ";
            }
            names += name;
        }
        std::fprintf(stderr, "提示 锚点参照物缺失 %d 处(位置退化为父帧, 名字 %d 个): %s\n",
                     missingRefs, static_cast<int>(MissingAnchorReferenceNames().size()),
                     names.c_str());
    }
    int withTexture = 0;
    int withSprite = 0;
    int spriteResolved = 0;
    int withText = 0;
    int alignedText = 0;
    std::printf("idx\tlayer\tname\tx\ty\tw\th\ttexture\tsprite\tresolved\ttext\tjh\tjv\tline\tr\tg\tb\ta\n");
    for (size_t index = 0; index < commands.size(); ++index) {
        const LayoutDrawCommand& command = commands[index];
        const bool hasTexture = command.texture != nullptr && command.texture[0] != '\0';
        const bool hasSprite = command.sprite != nullptr && command.sprite[0] != '\0';
        bool resolved = false;
        if (hasSprite) {
            float u0 = 0.0f;
            float v0 = 0.0f;
            float u1 = 0.0f;
            float v1 = 0.0f;
            resolved = UiSpriteRectUv(command.sprite, &u0, &v0, &u1, &v1);
        }
        withTexture += hasTexture ? 1 : 0;
        withSprite += hasSprite ? 1 : 0;
        spriteResolved += resolved ? 1 : 0;
        const bool hasText = command.text != nullptr && command.text[0] != '\0';
        withText += hasText ? 1 : 0;
        if (hasText && (command.justifyH != 0 || command.justifyV != 0)) {
            alignedText += 1;
        }
        std::printf("%zu\t%s\t%s\t%.1f\t%.1f\t%.1f\t%.1f\t%s\t%s\t%d\t%s\t%d\t%d\t%.1f\t%.2f\t%.2f\t%.2f\t%.2f\n",
                    index, LayerName(command.layer), Escape(command.name).c_str(), command.rect.x,
                    command.rect.y, command.rect.width, command.rect.height,
                    Escape(command.texture).c_str(), Escape(command.sprite).c_str(), resolved ? 1 : 0,
                    Escape(command.text).c_str(), command.justifyH, command.justifyV,
                    command.lineInterval, command.red, command.green, command.blue, command.alpha);
    }
    std::fprintf(stderr,
                 "布局 %s | 脚本 %s | 共享 %zu | OnLoad %d | OnShow %d | 刷新 %d | 事件派发 %d | 打开主帧 %d | 可见命令 %d | 带贴图 %d | 带精灵 %d | 精灵命中 %d | 带文字 %d(非默认对齐 %d) | 点击脚本 %d | 按钮 %d | 九宫 %d | 平铺 %d | 裁字 %d | 九宫退化 %d | 坏色 %d | 零面积 %d | 空壳继承 %d | 贴图断 %d | 克隆残缺 %d | 零尺有物 %d | 尺寸不符 %d | 父子断 %d | 死按钮 %d | 类型不符 %d | 精灵尺 %d | 四元组 %d | 顶点 %d\n",
                 stem.c_str(), hasScript ? "有" : "无", prelude.size(), onLoadCalls, onShowCalls, calls,
                 dispatched, shown, count, withTexture, withSprite, spriteResolved, withText,
                 alignedText, withClickScript, withButtonTexture, sliced, tiled,
                 renderStats.clippedChars, renderStats.sliceFallback, badColor, zeroSprite,
                 emptyInherits, brokenTexture, shortClone, zeroSizedContent, sizeMismatch, brokenParent, deadButton, typeMismatch, spriteSizeMismatch, renderStats.quads,
                 renderStats.vertices);
    return 0;
}
