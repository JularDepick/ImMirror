#include "ui/layout_xml.h"

#include "ui/generated/ui_font_index_gen.h"

#include <string>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <set>

namespace immirror::ui {
namespace {

/* 解析期暂存的锚点: 名字还是文本, 待全部帧建好后再解析成指针. */
struct RawAnchor {
    std::string point;
    std::string relativePoint;
    std::string relativeTo;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
};

/* 一个元素的属性表, 只保留解析用得到的几项. */
struct Attributes {
    std::string name;
    std::string point;
    std::string relativePoint;
    std::string relativeTo;
    std::string x;
    std::string y;
    std::string r;
    /* 通用数值属性: <Angle value="90"/> 这类标签用它. */
    std::string value;
    std::string g;
    std::string b;
    std::string a;
    std::string file;
    std::string level;
    /* hidden="true": 布局载入时默认不显示, 由脚本按需打开. */
    std::string hidden;
    std::string inherits;
    std::string uvname;
    /* 文本控件的内容与模板标记. */
    std::string text;
    std::string isVirtual;
    /* 文本控件的字体名, 例如 BlackFont20, 数字部分即字号. */
    std::string fonttype;
    /* 元素上的 id 属性. */
    std::string id;
    /* 模板常用 abs_x/abs_y 直接写尺寸. */
    std::string absX;
    std::string absY;
    /* 文本对齐与行距. */
    std::string justifyH;
    std::string justifyV;
    std::string lineInterval;
    /* 拉伸边缘: 带此属性的纹理铺满父帧. */
    std::string stretchEdge;
    /* 元素颜色: 八位十六进制, 依次为透明度与红绿蓝. */
    std::string color;
    /* 贴图翻转方式. */
    std::string uvType;
    /* 同图层内的绘制优先级. */
    std::string frameLevel;
    /* 流式布局样式与间距. */
    std::string layoutStyle;
    std::string spaceX;
    std::string spaceY;
    /* 混合模式. */
    std::string alphaMode;
    /* 文字阴影色与文字色. */
    std::string shadowColor;
    std::string textColor;
    /* 平台特化标记: pc / mobile, 移动端只取 mobile 那份. */
    std::string platform;
    /* 跨主帧层级名: BACKGROUND / MEDIUM / HIGH / FULLSCREEN / TOOLTIP. */
    std::string frameStrata;
    /* 元素透明度: 布局里多写作 0 到 1 的小数, 也有 0 到 255 的写法. */
    std::string alpha;
    /* 多行文本标记. */
    std::string multiLine;
    /* 混合尺寸写法: <Dimension rel_x="0.8" abs_y="49"/>, 宽高各取相对或绝对. */
    std::string relX;
    std::string relY;
    /* 文字样式: border 表示描边. */
    std::string fontStyle;
    /* 元素标签名与九宫/三片参数: 只在模板解析里用到. */
    std::string tagName;
    std::string drawType;
    std::string ox;
    std::string oy;
    std::string tilew;
    std::string tileh;
    /* 直接写在元素上的宽高 (像素): 一部分元素用这两个属性代替 Size 子元素. */
    std::string w;
    std::string h;
    /* size="宽,高" 的紧凑写法. */
    std::string size;
};

/* 图层名转枚举; 未知名按 artwork 处理. */
LayerLevel ParseLayerLevel(const std::string& level) {
    if (level == "background") {
        return LayerLevel::kBackground;
    }
    if (level == "border") {
        return LayerLevel::kBorders;
    }
    if (level == "artwork") {
        return LayerLevel::kArtwork;
    }
    if (level == "overlay") {
        return LayerLevel::kOverlay;
    }
    if (level == "highlight") {
        return LayerLevel::kHighlight;
    }
    return LayerLevel::kArtwork;
}

/* 结构性元素: 只承载几何与层级, 自身不是帧. */
/* 标签名大小写不敏感: 布局里同时存在 Size/size、Texture/texture、Angle/angle 这类写法. */
bool SameTag(const std::string& left, const char* right) {
    if (left.size() != std::strlen(right)) {
        return false;
    }
    for (size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }
    return true;
}

bool IsStructural(const std::string& tag) {
    /*
     * size 是小写的 Size, 布局里有零星写法; HighlightTexture 与 DisabledTexture
     * 是按钮的悬停态与禁用态贴图, 它们不建帧, 不登记的话会被当成普通帧画成空壳.
     */
    static const char* const kTags[] = {"Ui",      "Size",   "size",         "AbsDimension", "Anchors",
                                        "Anchor",  "Offset", "Layers",       "Layer",
                                        "Color",   "Scripts", "Script",      "Accel",
                                        "Highlight", "Shadow", "NormalTexture", "PushedTexture",
                                        "DisabledTexture", "HighlightTexture", "CheckedTexture",
                                        "RelDimension", "Dimension", "Height", "height", "top",
                                        "bottom", "LinkColor", "ShadowColor", "extratextcolor",
                                        "Icon", "Tile", "TileSize", "AbsValue", "Cursor", "PictureTexture",
                                        "configPath", "popPictureTex", "normal", "link", "Height",
                                        "Topleft", "Topright", "Bottomleft", "Bottomright", "Bottom",
                                        "OffSet", "Font", "OnClick",
                                        "Frames", "Center", "NineSquare", "Left", "Right", "Angle",
                                        "UVAnimation", "SlidingPlane", "SafeEditBox"};
    for (const char* known : kTags) {
        if (SameTag(tag, known)) {
            return true;
        }
    }
    return false;
}

bool IsSpace(char value) {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

/* 把 $parent 展开成给定的父名, 可能出现多次. */
std::string ExpandName(const std::string& raw, const std::string& parentName) {
    std::string out;
    size_t index = 0;
    while (index < raw.size()) {
        if (raw.compare(index, 7, "$parent") == 0) {
            out += parentName;
            index += 7;
            continue;
        }
        out.push_back(raw[index]);
        index += 1;
    }
    return out;
}

float ToFloat(const std::string& text) {
    if (text.empty()) {
        return 0.0f;
    }
    return static_cast<float>(std::atof(text.c_str()));
}

/* 从 <Tag a="1" b="2"> 里取属性; 只认解析需要的几个键. */
Attributes ParseAttributes(const std::string& body) {
    Attributes attributes;
    size_t index = 0;
    while (index < body.size()) {
        while (index < body.size() && IsSpace(body[index])) {
            ++index;
        }
        const size_t keyStart = index;
        while (index < body.size() && body[index] != '=' && !IsSpace(body[index])) {
            ++index;
        }
        const std::string key = body.substr(keyStart, index - keyStart);
        if (index >= body.size() || body[index] != '=') {
            continue;
        }
        ++index;
        if (index >= body.size() || body[index] != '"') {
            continue;
        }
        const size_t valueStart = ++index;
        while (index < body.size() && body[index] != '"') {
            ++index;
        }
        const std::string value = body.substr(valueStart, index - valueStart);
        if (index < body.size()) {
            ++index;
        }
        if (key == "name") {
            attributes.name = value;
        } else if (key == "point") {
            attributes.point = value;
        } else if (key == "relativePoint") {
            attributes.relativePoint = value;
        } else if (key == "stretchedge") {
            attributes.stretchEdge = value;
        } else if (key == "color") {
            attributes.color = value;
        } else if (key == "UVType") {
            attributes.uvType = value;
        } else if (key == "framelevel") {
            attributes.frameLevel = value;
        } else if (key == "alphamode") {
            attributes.alphaMode = value;
        } else if (key == "layoutstyle") {
            attributes.layoutStyle = value;
        } else if (key == "spacex") {
            attributes.spaceX = value;
        } else if (key == "spacey") {
            attributes.spaceY = value;
        } else if (key == "shadowcolor") {
            attributes.shadowColor = value;
        } else if (key == "textcolor") {
            attributes.textColor = value;
        } else if (key == "platform") {
            attributes.platform = value;
        } else if (key == "frameStrata") {
            attributes.frameStrata = value;
        } else if (key == "alpha") {
            attributes.alpha = value;
        } else if (key == "multiLine") {
            attributes.multiLine = value;
        } else if (key == "autowrap") {
            /*
             * 自动换行: 布局里用 autowrap="true" 表达"按容器宽度折行",
             * 与 multiLine 是同一件事. 不认它会让长文案单行铺开被容器截断.
             */
            attributes.multiLine = value;
        } else if (key == "fontStyle") {
            attributes.fontStyle = value;
        } else if (key == "rel_x") {
            attributes.relX = value;
        } else if (key == "rel_y") {
            attributes.relY = value;
        } else if (key == "relativeTo") {
            attributes.relativeTo = value;
        } else if (key == "x") {
            attributes.x = value;
        } else if (key == "y") {
            attributes.y = value;
        } else if (key == "value") {
            attributes.value = value;
        } else if (key == "r") {
            attributes.r = value;
        } else if (key == "g") {
            attributes.g = value;
        } else if (key == "b") {
            attributes.b = value;
        } else if (key == "a") {
            attributes.a = value;
        } else if (key == "file") {
            attributes.file = value;
        } else if (key == "level") {
            attributes.level = value;
        } else if (key == "hidden") {
            attributes.hidden = value;
        } else if (key == "inherits") {
            attributes.inherits = value;
        } else if (key == "uvname") {
            attributes.uvname = value;
        } else if (key == "text") {
            attributes.text = value;
        } else if (key == "virtual") {
            attributes.isVirtual = value;
        } else if (key == "fonttype") {
            attributes.fonttype = value;
        } else if (key == "id") {
            attributes.id = value;
        } else if (key == "abs_x") {
            attributes.absX = value;
        } else if (key == "abs_y") {
            attributes.absY = value;
        } else if (key == "justifyH") {
            attributes.justifyH = value;
        } else if (key == "justifyV") {
            attributes.justifyV = value;
        } else if (key == "lineInterval") {
            attributes.lineInterval = value;
        } else if (key == "DrawType") {
            attributes.drawType = value;
        } else if (key == "ox") {
            attributes.ox = value;
        } else if (key == "oy") {
            attributes.oy = value;
        } else if (key == "tilew") {
            attributes.tilew = value;
        } else if (key == "tileh") {
            attributes.tileh = value;
        } else if (key == "w") {
            attributes.w = value;
        } else if (key == "h") {
            attributes.h = value;
        } else if (key == "size") {
            attributes.size = value;
        }
    }
    return attributes;
}

}  /* namespace */

bool ParseAnchorPoint(const std::string& text, AnchorPoint& out) {
    if (text == "topleft") {
        out = AnchorPoint::kTopLeft;
    } else if (text == "top") {
        out = AnchorPoint::kTop;
    } else if (text == "topright") {
        out = AnchorPoint::kTopRight;
    } else if (text == "left") {
        out = AnchorPoint::kLeft;
    } else if (text == "center") {
        out = AnchorPoint::kCenter;
    } else if (text == "right") {
        out = AnchorPoint::kRight;
    } else if (text == "bottomleft") {
        out = AnchorPoint::kBottomLeft;
    } else if (text == "bottom") {
        out = AnchorPoint::kBottom;
    } else if (text == "bottomright") {
        out = AnchorPoint::kBottomRight;
    } else {
        return false;
    }
    return true;
}

LayoutFrame* LayoutDocument::Find(const char* name) const {
    if (name == nullptr) {
        return nullptr;
    }
    for (const std::unique_ptr<LayoutFrame>& frame : pool_) {
        if (frame != nullptr && frame->name != nullptr && std::strcmp(frame->name, name) == 0) {
            return frame.get();
        }
    }
    return nullptr;
}

LayoutFrame* LayoutDocument::CreateFrame(const std::string& name) {
    names_.push_back(name);
    pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
    LayoutFrame* frame = pool_.back().get();
    frame->name = names_.back().c_str();
    return frame;
}

bool LayoutDocument::Parse(const char* text, size_t length) {
    names_.clear();
    textures_.clear();
    texts_.clear();
    inherits_.clear();
    snippets_.clear();
    pool_.clear();
    root_ = nullptr;
    error_.clear();
    if (text == nullptr || length == 0) {
        error_ = "空文档";
        return false;
    }

    /*
     * 帧栈决定父子关系; 配对栈记录每个未闭合标签是否真的建了帧.
     * 弹栈必须与压栈严格对应: 无名的容器标签不建帧, 若也去弹帧栈,
     * 就会把父帧甚至合成根弹掉, 后续建帧时读到空栈.
     */
    std::vector<LayoutFrame*> frameStack;
    std::vector<std::pair<std::string, bool>> openTags;
    /* 与 openTags 平行的平台标记: 布局里同一元素会写 platform="pc" 与 platform="mobile" 两份. */
    std::vector<std::string> openPlatforms;
    std::vector<LayerLevel> layerStack;
    std::vector<std::pair<LayoutFrame*, RawAnchor>> pending;
    std::string current;

    /*
     * 合成根: 布局里可以平铺多个顶层帧, 需要一个统一父节点把它们收在一起,
     * 否则除第一个之外的同级顶层帧会被丢掉.
     */
    names_.push_back(std::string());
    pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
    root_ = pool_.back().get();
    root_->name = names_.back().c_str();
    frameStack.push_back(root_);

    size_t index = 0;

    /*
     * 报错带上行号: 布局文件动辄上千行, 只说"标签未闭合"几乎无法定位.
     * 行号按当前扫描位置数换行符得到, 不需要额外维护计数.
     */
    const auto failAt = [this, text, length](size_t offset, const char* reason) {
        int32_t line = 1;
        const size_t limit = offset < length ? offset : length;
        for (size_t position = 0; position < limit; ++position) {
            if (text[position] == '\n') {
                ++line;
            }
        }
        error_ = "第 " + std::to_string(line) + " 行: " + reason;
        return false;
    };

    /* 正在收集脚本片段: 遇到脚本事件开标签后, 标签之间的文本即脚本. */
    bool capturingClick = false;
    /* 正在收集的是哪个事件: OnClick / OnLoad / OnShow / OnHide. */
    std::string capturingEvent;
    while (index < length) {
        if (text[index] != '<') {
            const size_t runStart = index;
            while (index < length && text[index] != '<') {
                ++index;
            }
            if (capturingClick && !frameStack.empty()) {
                size_t begin = runStart;
                size_t end = index;
                while (begin < end && (text[begin] == ' ' || text[begin] == '\t' ||
                                       text[begin] == '\r' || text[begin] == '\n')) {
                    ++begin;
                }
                while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' ||
                                       text[end - 1] == '\r' || text[end - 1] == '\n')) {
                    --end;
                }
                if (end > begin) {
                    snippets_.push_back(std::string(text + begin, end - begin));
                    /*
                     * 按事件类型写到对应字段. 生命周期事件之前完全没有收集,
                     * 导致界面初始化脚本在真实运行路径上永远不会被触发.
                     */
                    LayoutFrame* owner = frameStack.back();
                    const char* snippet = snippets_.back().c_str();
                    if (SameTag(capturingEvent, "OnClick")) {
                        owner->onClick = snippet;
                    } else if (SameTag(capturingEvent, "OnLoad")) {
                        owner->onLoad = snippet;
                    } else if (SameTag(capturingEvent, "OnShow")) {
                        owner->onShow = snippet;
                    } else if (SameTag(capturingEvent, "OnHide")) {
                        owner->onHide = snippet;
                    } else {
                        /* 其余事件统一进事件表, 键是去掉 On 前缀的小写名. */
                        if (owner->events == nullptr) {
                            owner->events = new std::map<std::string, const char*>();
                        }
                        std::string key = capturingEvent.substr(2);
                        for (char& ch : key) {
                            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                        }
                        (*owner->events)[key] = snippet;
                    }
                }
            }
            capturingClick = false;
            continue;
        }
        /* 注释与处理指令整段跳过. */
        if (index + 3 < length && text[index + 1] == '!' && text[index + 2] == '-' &&
            text[index + 3] == '-') {
            const size_t end = std::string(text + index, length - index).find("-->");
            if (end == std::string::npos) {
                return failAt(index, "注释未闭合");
            }
            index += end + 3;
            continue;
        }
        if (index + 1 < length && text[index + 1] == '?') {
            const size_t end = std::string(text + index, length - index).find("?>");
            if (end == std::string::npos) {
                return failAt(index, "处理指令未闭合");
            }
            index += end + 2;
            continue;
        }
        const size_t close = std::string(text + index, length - index).find('>');
        if (close == std::string::npos) {
            return failAt(index, "标签未闭合");
        }
        const std::string body = std::string(text + index + 1, close - 1);
        index += close + 1;

        if (!body.empty() && body[0] == '/') {
            /* 结束标签: 弹栈. */
            const std::string tag = body.substr(1);
            if (tag == "Layer" && !layerStack.empty()) {
                layerStack.pop_back();
            }
            if (!openTags.empty()) {
                /* 只有压栈时真的建过帧的标签才弹帧栈. */
                if (openTags.back().second && !frameStack.empty()) {
                    frameStack.pop_back();
                }
                openTags.pop_back();
                if (!openPlatforms.empty()) {
                    openPlatforms.pop_back();
                }
            }
            continue;
        }

        const bool selfClosing = !body.empty() && body[body.size() - 1] == '/';
        std::string head = selfClosing ? body.substr(0, body.size() - 1) : body;
        size_t headEnd = 0;
        while (headEnd < head.size() && !IsSpace(head[headEnd])) {
            ++headEnd;
        }
        const std::string tag = head.substr(0, headEnd);
        const Attributes attributes = ParseAttributes(head.substr(headEnd));

        /* 脚本片段容器: 只收文本, 不建帧. */
        /*
         * 事件标签也有大小写变体, 布局里出现过 onClick / Onclick / onload.
         * 点击与三个生命周期事件都要收集, 否则界面初始化脚本永远不会被触发.
         */
        /*
         * 所有 On 开头的事件标签都收集: 除了点击与生命周期, 还有 OnEvent / OnUpdate /
         * OnMouseDown / OnEnter / OnValueChanged 等, 合计四百余处.
         */
        if (tag.size() > 2 && (tag[0] == 'O' || tag[0] == 'o') && (tag[1] == 'n' || tag[1] == 'N')) {
            capturingClick = !selfClosing;
            capturingEvent = tag;
            if (!selfClosing) {
                openTags.push_back(std::make_pair(tag, false));
                openPlatforms.push_back(attributes.platform);
            }
            continue;
        }
        /*
         * 平台特化: 同一份布局给 pc 与 mobile 各写一份 Size/Offset, 移动端只取 mobile.
         * 写成两份时后写的会覆盖先写的, 顺序不保证, 因此必须按平台名过滤.
         */
        /* 平台名大小写混用, 布局里也写作 PC. */
        const bool pcOnly = SameTag(attributes.platform, "pc");

        /*
         * 旋转角: <Angle value="90"/> 属于结构性标签, 必须在跳过结构性标签之前处理,
         * 否则永远读不到. 只支持直角, 非直角无法用交换 uv 表达.
         */
        /*
         * 三片切片: <Center><Left w=.. /><Center w=.. /><Right w=.. /></Center>.
         * 左右固定宽度, 中间拉伸. 不解析的话内边距为 0, 会退化成整体拉伸,
         * 圆角按钮与长条底板的边角就被拉变形了.
         */
        /*
         * 纵向切片: <Height><top h=.. /><bottom h=.. /></Height>, 上下固定高度中间拉伸.
         * 与横向的 <Center><Left/><Right/></Center> 对称, 常见于竖直长条底板.
         */
        if ((SameTag(tag, "top") || SameTag(tag, "bottom")) && !frameStack.empty()) {
            const float height = ToFloat(attributes.h);
            if (height > 0.0f) {
                LayoutFrame* sliced = frameStack.back();
                if (SameTag(tag, "top")) {
                    sliced->insetTop = height;
                } else {
                    sliced->insetBottom = height;
                }
            }
        }
        if ((tag == "Left" || tag == "Right") && !frameStack.empty()) {
            const float width = ToFloat(attributes.w);
            if (width > 0.0f) {
                LayoutFrame* sliced = frameStack.back();
                /*
                 * 九宫的内边距优先级更高: 它已经用 <NineSquare> 给过四边,
                 * 不能再被三片的两边覆盖, 否则九宫会退化成三片.
                 */
                const bool alreadySliced = sliced->insetLeft > 0.0f || sliced->insetRight > 0.0f ||
                                           sliced->insetTop > 0.0f || sliced->insetBottom > 0.0f;
                if (!alreadySliced) {
                    if (tag == "Left") {
                        sliced->insetLeft = width;
                    } else {
                        sliced->insetRight = width;
                    }
                }
            }
        }
        if (tag == "Angle" && !frameStack.empty()) {
            /* 角度归一化到 0 到 359: 布局里也写 -90 与 -180 这类负值. */
            int32_t angle = static_cast<int32_t>(ToFloat(attributes.value)) % 360;
            if (angle < 0) {
                angle += 360;
            }
            LayoutFrame* rotated = frameStack.back();
            if (angle == 90) {
                rotated->rotateQuarter = 1;
            } else if (angle == 180) {
                rotated->rotateQuarter = 2;
            } else if (angle == 270) {
                rotated->rotateQuarter = 3;
            }
        }
        if (IsStructural(tag)) {
            if (tag == "Anchor" && !frameStack.empty()) {
                RawAnchor anchor;
                anchor.point = attributes.point;
                anchor.relativePoint = attributes.relativePoint;
                anchor.relativeTo = attributes.relativeTo;
                pending.push_back(std::make_pair(frameStack.back(), anchor));
            } else if (tag == "AbsDimension") {
                /* 归属看最近的 Anchor 或 Size: 前者是偏移, 后者是尺寸. */
                std::string owner;
                for (size_t i = openTags.size(); i > 0; --i) {
                    if (openTags[i - 1].first == "Anchor" || openTags[i - 1].first == "Size") {
                        owner = openTags[i - 1].first;
                        break;
                    }
                }
                /* 归属标签若标了 pc 平台, 移动端跳过这一份. */
                bool ownerPcOnly = false;
                for (size_t i = openTags.size(); i > 0; --i) {
                    if (openTags[i - 1].first == "Anchor" || openTags[i - 1].first == "Size") {
                        ownerPcOnly = i <= openPlatforms.size() && openPlatforms[i - 1] == "pc";
                        break;
                    }
                }
                if (ownerPcOnly) {
                    continue;
                }
                if (owner == "Anchor" && !pending.empty()) {
                    pending.back().second.offsetX = ToFloat(attributes.x);
                    pending.back().second.offsetY = ToFloat(attributes.y);
                } else if (owner == "Size" && !frameStack.empty()) {
                    frameStack.back()->width = ToFloat(attributes.x);
                    frameStack.back()->height = ToFloat(attributes.y);
                }
            } else if (tag == "NineSquare" && !frameStack.empty() && !pcOnly) {
                /*
                 * 元素自身也可以直接写九宫参数(不只是模板里写).
                 * 边角为 0 时按 tilew/tileh 平铺, 否则按九宫拉伸.
                 */
                LayoutFrame* frame = frameStack.back();
                frame->insetLeft = ToFloat(attributes.ox);
                frame->insetRight = ToFloat(attributes.ox);
                frame->insetTop = ToFloat(attributes.oy);
                frame->insetBottom = ToFloat(attributes.oy);
                frame->tileWidth = ToFloat(attributes.tilew);
                frame->tileHeight = ToFloat(attributes.tileh);
            } else if (tag == "Dimension" && !frameStack.empty() && !pcOnly) {

                /*
                 * 混合尺寸: 宽高可以一个用绝对值一个用相对比例.
                 * 归属看最近的 Size; 没有 Size 时按帧自身尺寸处理.
                 */
                if (!attributes.absX.empty()) {
                    frameStack.back()->width = ToFloat(attributes.absX);
                }
                if (!attributes.absY.empty()) {
                    frameStack.back()->height = ToFloat(attributes.absY);
                }
                if (!attributes.relX.empty()) {
                    frameStack.back()->relWidth = ToFloat(attributes.relX);
                }
                if (!attributes.relY.empty()) {
                    frameStack.back()->relHeight = ToFloat(attributes.relY);
                }
            } else if (tag == "RelDimension" && !frameStack.empty() && !pcOnly) {
                /* 相对尺寸按父帧比例, 解算时再乘上去. */
                frameStack.back()->relWidth = ToFloat(attributes.x);
                frameStack.back()->relHeight = ToFloat(attributes.y);
            } else if ((tag == "Color" || tag == "color" || tag == "textcolor" ||
                        tag == "cursorcolor") &&
                       !frameStack.empty()) {
                /*
                 * 颜色标签有大小写两种写法, 布局里都出现过:
                 * <Color r g b /> 与 <color r g b /> 设元素颜色,
                 * <textcolor r g b /> 设文字颜色, 两者在帧上共用同一组通道.
                 */
                /* 颜色取最近一帧; 分量按 0 到 255 给出, 转成 0 到 1. */
                LayoutFrame* painted = frameStack.back();
                painted->hasColor = true;
                painted->red = ToFloat(attributes.r) / 255.0f;
                painted->green = ToFloat(attributes.g) / 255.0f;
                painted->blue = ToFloat(attributes.b) / 255.0f;
                painted->alpha = attributes.a.empty() ? 1.0f : ToFloat(attributes.a) / 255.0f;
            } else if (tag == "CheckedTexture" && !frameStack.empty()) {
                if (!attributes.file.empty()) {
                    textures_.push_back(attributes.file);
                    frameStack.back()->checkedTexture = textures_.back().c_str();
                }
                if (!attributes.uvname.empty()) {
                    sprites_.push_back(attributes.uvname);
                    frameStack.back()->checkedSprite = sprites_.back().c_str();
                }
            } else if (tag == "NormalTexture" || tag == "PushedTexture" ||
                       tag == "DisabledTexture") {
                /*
                 * 三态贴图是按钮的子元素, 不是独立帧: 属性挂到父帧上.
                 * DisabledTexture 暂与普通态同样处理, 置灰由颜色压暗表达.
                 */
                if (!frameStack.empty()) {
                    LayoutFrame* button = frameStack.back();
                    const bool pushed = tag == "PushedTexture";
                    if (!attributes.file.empty()) {
                        textures_.push_back(attributes.file);
                        (pushed ? button->pushedTexture : button->normalTexture) =
                            textures_.back().c_str();
                    }
                    if (!attributes.uvname.empty()) {
                        sprites_.push_back(attributes.uvname);
                        (pushed ? button->pushedSprite : button->normalSprite) =
                            sprites_.back().c_str();
                    }
                }
            } else if (tag == "Layer") {
                layerStack.push_back(ParseLayerLevel(attributes.level));
            }
            if (!selfClosing) {
                openTags.push_back(std::make_pair(tag, false));
            }
            continue;
        }

        /* 带名字的元素才成为帧; 无名的按普通容器处理, 不进入帧树. */
        if (attributes.name.empty()) {
            if (!selfClosing) {
                openTags.push_back(std::make_pair(tag, false));
            }
            continue;
        }

        const std::string parentName = frameStack.empty() ? std::string() : frameStack.back()->name;
        names_.push_back(ExpandName(attributes.name, parentName));
        pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
        LayoutFrame* frame = pool_.back().get();
        frame->name = names_.back().c_str();
        /* 所在图层决定绘制顺序; 贴图路径来自元素自身的 file 属性. */
        frame->layer = layerStack.empty() ? LayerLevel::kArtwork : layerStack.back();
        /* hidden="true" 的帧载入时不显示, 与脚本里的 Show/Hide 配合. */
        frame->hidden = attributes.hidden == "true";
        /*
         * virtual="true" 的帧只是模板, 自身不绘制, 只被 Clone 复制到别处.
         * 文本控件把文字内容一并存下来, 绘制阶段据此产出文字图元.
         */
        frame->isVirtual = attributes.isVirtual == "true";
        /* 记下标签名, 供核对控件类型是否与布局一致. */
        tags_.push_back(tag);
        frame->tag = tags_.back().c_str();
        frame->stretchEdge = attributes.stretchEdge == "true";
        frame->multiLine = attributes.multiLine == "true";
        /*
         * 元素透明度: 小数写法直接取, 大于 1 的按 0 到 255 折算.
         * 不读它会让半透明蒙版与淡出文字变成完全不透明.
         */
        if (!attributes.alpha.empty()) {
            const float raw = ToFloat(attributes.alpha);
            frame->alpha = raw > 1.0f ? raw / 255.0f : raw;
        }
        /* 跨主帧层级: 布局里写作 BACKGROUND / MEDIUM / HIGH / FULLSCREEN / TOOLTIP. */
        if (attributes.frameStrata == "BACKGROUND") {
            frame->strata = 0;
        } else if (attributes.frameStrata == "HIGH" || attributes.frameStrata == "High") {
            frame->strata = 2;
        } else if (attributes.frameStrata == "FULLSCREEN") {
            frame->strata = 3;
        } else if (attributes.frameStrata == "TOOLTIP") {
            frame->strata = 4;
        }
        frame->outline = attributes.fontStyle == "border";
        /* 加粗: bolder 与 embolden 都表示同一件事. */
        frame->bold = SameTag(attributes.fontStyle, "bolder") ||
                      SameTag(attributes.fontStyle, "embolden");
        /* 加粗: bolder 与 embolden 都表示同一件事. */
        frame->bold = SameTag(attributes.fontStyle, "bolder") ||
                      SameTag(attributes.fontStyle, "embolden");
        /*
         * 混合模式: add 表示加法混合(发光类效果), alphablend 是默认普通混合.
         * 不读它会让发光元素按普通混合画, 亮度明显偏暗.
         */
        if (SameTag(attributes.alphaMode, "add")) {
            frame->additive = true;
        }
        /* 加法混合: 叠加类元素靠它出光效. */
        frame->additive = attributes.alphaMode == "add";
        /* 流式布局: 容器按样式把子元素依次排开. */
        if (!attributes.layoutStyle.empty()) {
            layoutStyles_.push_back(attributes.layoutStyle);
            frame->layoutStyle = layoutStyles_.back().c_str();
            frame->spaceX = ToFloat(attributes.spaceX);
            frame->spaceY = ToFloat(attributes.spaceY);
        }
        /* 同图层内的绘制优先级: 数值大的后画, 压在上面. */
        if (!attributes.frameLevel.empty()) {
            frame->frameLevel = std::atoi(attributes.frameLevel.c_str());
        }
        /* 文字阴影色: 八位十六进制, 省略透明度时按不透明处理. */
        {
            const std::string& raw = attributes.shadowColor;
            if (raw.size() >= 6) {
                const std::string tail = raw.substr(raw.size() - 6);
                const uint32_t packed =
                    static_cast<uint32_t>(std::strtoul(tail.c_str(), nullptr, 16));
                frame->shadowRed = static_cast<float>((packed >> 16) & 0xFFu) / 255.0f;
                frame->shadowGreen = static_cast<float>((packed >> 8) & 0xFFu) / 255.0f;
                frame->shadowBlue = static_cast<float>(packed & 0xFFu) / 255.0f;
            }
        }
        /* 文字颜色: 允许带 # 前缀. */
        {
            std::string raw = attributes.textColor;
            if (!raw.empty() && raw[0] == '#') {
                raw = raw.substr(1);
            }
            if (raw.size() == 6) {
                const uint32_t packed =
                    static_cast<uint32_t>(std::strtoul(raw.c_str(), nullptr, 16));
                frame->hasColor = true;
                frame->alpha = 1.0f;
                frame->red = static_cast<float>((packed >> 16) & 0xFFu) / 255.0f;
                frame->green = static_cast<float>((packed >> 8) & 0xFFu) / 255.0f;
                frame->blue = static_cast<float>(packed & 0xFFu) / 255.0f;
            }
        }
        /*
         * 贴图翻转: 布局用它复用同一张贴图的四个朝向.
         * 水平/垂直镜像与旋转半周都只是交换 uv 两端, 旋转直角需要换轴, 暂不处理.
         */
        if (attributes.uvType == "mirroeu") {
            frame->flipU = true;
        } else if (attributes.uvType == "mirroev") {
            frame->flipV = true;
        } else if (attributes.uvType == "turn180") {
            frame->flipU = true;
            frame->flipV = true;
        } else if (attributes.uvType == "turn90") {
            frame->rotateQuarter = 1;
        } else if (attributes.uvType == "turn270") {
            frame->rotateQuarter = 3;
        }
        /*
         * 元素颜色: 布局里写作 color="AARRGGBB", 用于给贴图染色与画纯色块.
         * 蒙版、遮罩这类元素只靠它显色, 不解析就会画成纯白.
         */
        if (attributes.color.size() == 8) {
            const uint32_t packed = static_cast<uint32_t>(
                std::strtoul(attributes.color.c_str(), nullptr, 16));
            frame->hasColor = true;
            frame->alpha = static_cast<float>((packed >> 24) & 0xFFu) / 255.0f;
            frame->red = static_cast<float>((packed >> 16) & 0xFFu) / 255.0f;
            frame->green = static_cast<float>((packed >> 8) & 0xFFu) / 255.0f;
            frame->blue = static_cast<float>(packed & 0xFFu) / 255.0f;
        } else if (attributes.color.size() == 6) {
            /* 六位写法省略透明度, 按不透明处理. */
            const uint32_t packed = static_cast<uint32_t>(
                std::strtoul(attributes.color.c_str(), nullptr, 16));
            frame->hasColor = true;
            frame->alpha = 1.0f;
            frame->red = static_cast<float>((packed >> 16) & 0xFFu) / 255.0f;
            frame->green = static_cast<float>((packed >> 8) & 0xFFu) / 255.0f;
            frame->blue = static_cast<float>(packed & 0xFFu) / 255.0f;
        }
        /* 滑动容器: 子元素只在自身矩形内可见. */
        frame->clipChildren = tag == "SlidingFrame";
        /*
         * abs_x/abs_y 在元素上写作尺寸: 大量元素继承"只有贴图没有尺寸"的九宫模板,
         * 尺寸靠实例上的这两个属性给出. 显式 Size 仍然优先, 因此这里只在缺尺寸时补.
         */
        if (!attributes.absX.empty() || !attributes.absY.empty()) {
            const float absWidth = ToFloat(attributes.absX);
            const float absHeight = ToFloat(attributes.absY);
            if (absWidth > 0.0f) {
                frame->width = absWidth;
            }
            if (absHeight > 0.0f) {
                frame->height = absHeight;
            }
        }
        if (!attributes.inherits.empty()) {
            inherits_.push_back(attributes.inherits);
            frame->inherits = inherits_.back().c_str();
        }
        /* 布局 id: 页签按钮与列表行靠它区分身份. */
        if (!attributes.id.empty()) {
            frame->clientId = static_cast<int32_t>(std::atoi(attributes.id.c_str()));
        }
        if (!attributes.text.empty()) {
            texts_.push_back(attributes.text);
            frame->text = texts_.back().c_str();
        }
        /* 文本对齐与行距: 只对文本控件有意义, 其余元素置默认值. */
        /*
         * 对齐取值大小写混用: 布局里同时有 center / CENTER / Center / Left / LEFT,
         * 还有数字写法. 按大小写不敏感比较, 否则大写的那些全部退回默认左对齐.
         */
        if (SameTag(attributes.justifyH, "center") || attributes.justifyH == "1") {
            frame->justifyH = 1;
        } else if (SameTag(attributes.justifyH, "right") || attributes.justifyH == "2") {
            frame->justifyH = 2;
        }
        /* 垂直对齐同样大小写混用, 另有 middel 这类笔误, 一并按不敏感比较. */
        if (SameTag(attributes.justifyV, "middle") || SameTag(attributes.justifyV, "middel") ||
            attributes.justifyV == "1") {
            frame->justifyV = 1;
        } else if (SameTag(attributes.justifyV, "bottom") || attributes.justifyV == "2") {
            frame->justifyV = 2;
        }
        frame->lineInterval = ToFloat(attributes.lineInterval);
        /* 字号: fonttype 里的数字部分, 例如 BlackFont20 得 20. */
        if (!attributes.fonttype.empty()) {
            size_t digits = attributes.fonttype.size();
            while (digits > 0 && (attributes.fonttype[digits - 1] < '0' || attributes.fonttype[digits - 1] > '9')) {
                digits -= 1;
            }
            size_t start = digits;
            while (start > 0 && attributes.fonttype[start - 1] >= '0' && attributes.fonttype[start - 1] <= '9') {
                start -= 1;
            }
            if (digits > start) {
                /*
             * 字号优先查字体表: 像 BlackFont(16) / MinimumBlackFont(10) 这类名字里没有数字,
             * 只从名字提数字会落到默认值, 字号就不对了.
             */
            int32_t resolved = 0;
            for (int32_t index = 0; index < gen::kUiFontCount; ++index) {
                const gen::UiFontEntry& entry = gen::kUiFonts[index];
                if (entry.name != nullptr && attributes.fonttype == entry.name) {
                    resolved = entry.height;
                    break;
                }
            }
            frame->fontSize = resolved > 0
                                  ? static_cast<float>(resolved)
                                  : ToFloat(attributes.fonttype.substr(start, digits - start));
            }
        }
        /*
         * 模板继承: 元素用 inherits 指向模板时, 外观信息从模板取.
         * 元素自身给了贴图就不覆盖, 这样模板可以被局部改写.
         */
        if (!attributes.inherits.empty() && templates_ != nullptr) {
            const auto found = templates_->find(attributes.inherits);
            if (found != templates_->end()) {
                const LayoutTemplate& entry = found->second;
                if (!entry.texture.empty()) {
                    textures_.push_back(entry.texture);
                    frame->texture = textures_.back().c_str();
                }
                if (!entry.sprite.empty()) {
                    sprites_.push_back(entry.sprite);
                    frame->sprite = sprites_.back().c_str();
                }
                /* 点击脚本: 实例没写 OnClick 时继承模板的. */
                if (frame->onClick == nullptr && !entry.onClick.empty()) {
                    snippets_.push_back(entry.onClick);
                    frame->onClick = snippets_.back().c_str();
                }
                /* 九宫切片参数随模板一起带下来. */
                frame->tileOffsetX = entry.tileOffsetX;
                frame->tileOffsetY = entry.tileOffsetY;
                /* 按钮三态: 元素自己没写时继承模板的. */
                if (!entry.normalTexture.empty()) {
                    textures_.push_back(entry.normalTexture);
                    frame->normalTexture = textures_.back().c_str();
                }
                if (!entry.normalSprite.empty()) {
                    sprites_.push_back(entry.normalSprite);
                    frame->normalSprite = sprites_.back().c_str();
                }
                if (!entry.pushedTexture.empty()) {
                    textures_.push_back(entry.pushedTexture);
                    frame->pushedTexture = textures_.back().c_str();
                }
                if (!entry.pushedSprite.empty()) {
                    sprites_.push_back(entry.pushedSprite);
                    frame->pushedSprite = sprites_.back().c_str();
                }
                if (!entry.checkedTexture.empty()) {
                    textures_.push_back(entry.checkedTexture);
                    frame->checkedTexture = textures_.back().c_str();
                }
                if (!entry.checkedSprite.empty()) {
                    sprites_.push_back(entry.checkedSprite);
                    frame->checkedSprite = sprites_.back().c_str();
                }
                frame->tileWidth = entry.tileWidth;
                frame->tileHeight = entry.tileHeight;
                frame->insetLeft = entry.insetLeft;
                frame->insetRight = entry.insetRight;
                frame->insetTop = entry.insetTop;
                frame->insetBottom = entry.insetBottom;
                /* 尺寸同样只在元素没给的时候才取模板的值. */
                if (frame->width <= 0.0f && entry.width > 0.0f) {
                    frame->width = entry.width;
                }
                if (frame->height <= 0.0f && entry.height > 0.0f) {
                    frame->height = entry.height;
                }
            }
        }
        /*
         * 宽高有两种写法: Size 子元素里的 AbsDimension, 或元素上的 w/h 属性.
         * 这里先按属性给值; 若后面读到 Size 子元素, 会覆盖成子元素的值.
         */
        if (!attributes.w.empty() && !attributes.h.empty()) {
            frame->width = ToFloat(attributes.w);
            frame->height = ToFloat(attributes.h);
        }
        /* 紧凑写法 size="宽,高". */
        if (!attributes.size.empty()) {
            const size_t comma = attributes.size.find(',');
            if (comma != std::string::npos) {
                frame->width = ToFloat(attributes.size.substr(0, comma));
                frame->height = ToFloat(attributes.size.substr(comma + 1));
            }
        }
        if (!attributes.file.empty()) {
            textures_.push_back(attributes.file);
            frame->texture = textures_.back().c_str();
        }
        /*
         * 元素自身声明的精灵名必须一并落库: 图集页里一格对应一个精灵,
         * 只记页面不记精灵, 渲染时只能按整页画, 界面贴图就全错位.
         */
        if (!attributes.uvname.empty()) {
            sprites_.push_back(attributes.uvname);
            frame->sprite = sprites_.back().c_str();
            /*
             * 直接绘制元素按精灵名反查模板的切片参数:
             * 布局里大量 Texture 不写 inherits, 但用的是同一个精灵, 观感应当一致.
             */
            const auto found = bySprite_.find(attributes.uvname);
            if (found != bySprite_.end() && frame->insetLeft <= 0.0f && frame->insetRight <= 0.0f &&
                frame->insetTop <= 0.0f && frame->insetBottom <= 0.0f) {
                frame->tileOffsetX = found->second->tileOffsetX;
                frame->tileOffsetY = found->second->tileOffsetY;
                frame->tileWidth = found->second->tileWidth;
                frame->tileHeight = found->second->tileHeight;
                frame->insetLeft = found->second->insetLeft;
                frame->insetRight = found->second->insetRight;
                frame->insetTop = found->second->insetTop;
                frame->insetBottom = found->second->insetBottom;
            }
        }
        if (!frameStack.empty()) {
            frameStack.back()->AddChild(frame);
        }
        if (!selfClosing) {
            openTags.push_back(std::make_pair(tag, true));
            frameStack.push_back(frame);
        }
    }

    if (root_ == nullptr || root_->children.empty()) {
        /* 只有合成根说明这份 XML 不是布局, 例如资源清单或按键表. */
        error_ = "没有帧节点";
        return false;
    }

    /* 锚点二段解析: 此时全部帧已就位, relativeTo 可以跨层级引用. */
    for (const std::pair<LayoutFrame*, RawAnchor>& entry : pending) {
        LayoutFrame* frame = entry.first;
        const RawAnchor& raw = entry.second;
        LayoutAnchor anchor;
        if (!ParseAnchorPoint(raw.point, anchor.point)) {
            anchor.point = AnchorPoint::kTopLeft;
        }
        if (!ParseAnchorPoint(raw.relativePoint, anchor.relativePoint)) {
            anchor.relativePoint = AnchorPoint::kTopLeft;
        }
        const std::string parentName =
            frame->parent == nullptr ? std::string() : std::string(frame->parent->name);
        const std::string target = ExpandName(raw.relativeTo, parentName);
        anchor.relativeTo = target.empty() ? nullptr : Find(target.c_str());
        /* 本文件没有就去参照文档找: 界面常用游戏内 HUD 与其他界面的帧做锚点参照. */
        if (anchor.relativeTo == nullptr && !target.empty()) {
            for (const LayoutDocument* reference : referenceDocuments_) {
                anchor.relativeTo = reference->Find(target.c_str());
                if (anchor.relativeTo != nullptr) {
                    break;
                }
            }
        }
        if (!target.empty()) {
            /* 记下参照物名字: 解算时据此区分"没写参照物"与"写了但找不到". */
            anchorNames_.push_back(target);
            anchor.relativeName = anchorNames_.back().c_str();
        }
        anchor.offsetX = raw.offsetX;
        anchor.offsetY = raw.offsetY;
        frame->anchors.push_back(anchor);
    }

    /*
     * 模板实例化: 继承模板的元素要把模板的整棵子树克隆进来.
     * 原版里模板是 virtual 帧, 自身不绘制, 实例才绘制它的子元素.
     */
    {
        std::vector<LayoutFrame*> all;
        std::function<void(LayoutFrame*)> collect = [&](LayoutFrame* frame) {
            all.push_back(frame);
            for (LayoutFrame* child : frame->children) {
                if (child != nullptr) {
                    collect(child);
                }
            }
        };
        if (root_ != nullptr) {
            collect(root_);
        }
        std::unordered_map<std::string, LayoutFrame*> byName;
        for (LayoutFrame* frame : all) {
            if (frame->name != nullptr && frame->name[0] != '\0') {
                byName.emplace(frame->name, frame);
            }
        }

        std::unordered_map<const LayoutFrame*, LayoutFrame*> mapping;
        /* 模板里的名字在解析阶段已按模板名展开, 克隆时要把前缀换成实例名. */
        std::string templatePrefix;
        std::string instanceName;
        std::function<LayoutFrame*(const LayoutFrame&, LayoutFrame*, const std::string&)> clone =
            [&](const LayoutFrame& source, LayoutFrame* parent, const std::string& parentName) {
                std::string name = ExpandName(source.name, parentName);
                if (!templatePrefix.empty() && !instanceName.empty() &&
                    name.compare(0, templatePrefix.size(), templatePrefix) == 0) {
                    name = instanceName + name.substr(templatePrefix.size());
                }
                names_.push_back(name);
                pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
                LayoutFrame* copy = pool_.back().get();
                copy->name = names_.back().c_str();
                copy->width = source.width;
                copy->height = source.height;
                copy->relWidth = source.relWidth;
                copy->relHeight = source.relHeight;
                copy->hasColor = source.hasColor;
                copy->red = source.red;
                copy->green = source.green;
                copy->blue = source.blue;
                copy->alpha = source.alpha;
                copy->texture = source.texture;
                copy->sprite = source.sprite;
                copy->text = source.text;
                copy->fontSize = source.fontSize;
                copy->justifyH = source.justifyH;
                copy->justifyV = source.justifyV;
                copy->lineInterval = source.lineInterval;
                copy->normalTexture = source.normalTexture;
                copy->normalSprite = source.normalSprite;
                copy->pushedTexture = source.pushedTexture;
                copy->pushedSprite = source.pushedSprite;
                copy->tileOffsetX = source.tileOffsetX;
                copy->tileOffsetY = source.tileOffsetY;
                copy->tileWidth = source.tileWidth;
                copy->tileHeight = source.tileHeight;
                copy->insetLeft = source.insetLeft;
                copy->insetRight = source.insetRight;
                copy->insetTop = source.insetTop;
                copy->insetBottom = source.insetBottom;
                copy->clientId = source.clientId;
                copy->onClick = source.onClick;
                copy->hidden = source.hidden;
                copy->layer = source.layer;
                copy->isVirtual = false;
                mapping[&source] = copy;
                parent->AddChild(copy);
                for (const LayoutFrame* child : source.children) {
                    if (child != nullptr) {
                        clone(*child, copy, copy->name);
                    }
                }
                return copy;
            };

        /*
         * 模板继承模板时也要展开: 设置屏的页签模板继承通用按钮模板, 若跳过模板帧,
         * 通用模板的子元素就传不到最终实例上, 链式继承会断在中间.
         * 模板帧自身不绘制(绘制阶段直接跳过 virtual), 克隆出的子元素挂在实例下正常绘制.
         */
        std::vector<LayoutFrame*> targets;
        for (LayoutFrame* frame : all) {
            if (frame->inherits != nullptr && frame->inherits[0] != '\0') {
                targets.push_back(frame);
            }
        }
        /*
         * 收集整条模板继承链(自身在前, 祖先在后), 允许跨到外部模板文档.
         * 子元素要按整条链合并: 子模板只写自己新增的部分, 其余来自祖先模板.
         */
        const auto collectSources = [&](const char* name) -> std::vector<const LayoutFrame*> {
            std::vector<const LayoutFrame*> out;
            if (name == nullptr || name[0] == '\0') {
                return out;
            }
            const LayoutFrame* current = nullptr;
            const auto local = byName.find(name);
            if (local != byName.end()) {
                current = local->second;
            } else if (templateDocument_ != nullptr) {
                current = templateDocument_->Find(name);
            }
            for (int32_t depth = 0; depth < 8 && current != nullptr; ++depth) {
                out.push_back(current);
                if (current->inherits == nullptr || current->inherits[0] == '\0') {
                    break;
                }
                const LayoutFrame* next = nullptr;
                const auto again = byName.find(current->inherits);
                if (again != byName.end()) {
                    next = again->second;
                } else if (templateDocument_ != nullptr) {
                    next = templateDocument_->Find(current->inherits);
                }
                if (next == current) {
                    break;
                }
                current = next;
            }
            return out;
        };

        for (LayoutFrame* frame : targets) {
            const std::vector<const LayoutFrame*> sources = collectSources(frame->inherits);
            if (sources.empty()) {
                continue;
            }
            mapping.clear();
            instanceName = frame->name == nullptr ? std::string() : frame->name;
            /* 从最远的祖先开始克隆, 子模板的同名子元素后写并覆盖. */
            std::unordered_map<std::string, LayoutFrame*> cloned;
            for (auto it = sources.rbegin(); it != sources.rend(); ++it) {
                const LayoutFrame* source = *it;
                if (source == frame) {
                    continue;
                }
                templatePrefix = source->name == nullptr ? std::string() : source->name;
                for (const LayoutFrame* child : source->children) {
                    if (child == nullptr || child->name == nullptr) {
                        continue;
                    }
                    if (cloned.find(child->name) != cloned.end()) {
                        continue;
                    }
                    LayoutFrame* copy = clone(*child, frame, frame->name);
                    if (copy != nullptr && copy->name != nullptr) {
                        cloned.emplace(copy->name, copy);
                    }
                }
            }
            /*
             * 克隆去重: 实例自己已经声明了同名子元素时, 保留实例自己的, 丢掉克隆出来的.
             * 否则模板克隆与布局声明会各画一份, 画面上同一元素叠两层.
             */
            {
                std::set<std::string> seen;
                std::vector<LayoutFrame*> kept;
                for (LayoutFrame* child : frame->children) {
                    if (child == nullptr || child->name == nullptr) {
                        kept.push_back(child);
                        continue;
                    }
                    if (!seen.insert(child->name).second) {
                        continue;
                    }
                    kept.push_back(child);
                }
                frame->children = kept;
            }
            /*
             * 克隆帧的锚点: 指向模板内部的改指对应克隆;
             * 指向模板自身($parent)的改指实例帧, 否则退化成父帧, 子元素会全部叠在一起.
             */
            std::set<const LayoutFrame*> templateFrames;
            for (const LayoutFrame* source : sources) {
                templateFrames.insert(source);
            }
            for (const auto& pair : mapping) {
                LayoutFrame* copy = pair.second;
                copy->anchors.clear();
                for (const LayoutAnchor& anchor : pair.first->anchors) {
                    LayoutAnchor adjusted = anchor;
                    const auto hit = mapping.find(anchor.relativeTo);
                    if (hit != mapping.end()) {
                        adjusted.relativeTo = hit->second;
                    } else if (anchor.relativeTo != nullptr &&
                               templateFrames.count(anchor.relativeTo) != 0) {
                        adjusted.relativeTo = frame;
                    }
                    copy->anchors.push_back(adjusted);
                }
            }
        }
    }
    return true;
}

void LayoutDocument::SetTemplates(const LayoutTemplates* templates) {
    templates_ = templates;
    bySprite_.clear();
    if (templates == nullptr) {
        return;
    }
    /*
     * 同一精灵可能被多个模板引用, 取第一个带切片参数的:
     * 直接写 file+uvname 的元素没有 inherits, 只能按精灵名反查切片方式.
     */
    for (const auto& entry : *templates) {
        const LayoutTemplate& item = entry.second;
        if (item.sprite.empty()) {
            continue;
        }
        const bool sliced = item.insetLeft > 0.0f || item.insetRight > 0.0f ||
                            item.insetTop > 0.0f || item.insetBottom > 0.0f;
        if (!sliced) {
            continue;
        }
        bySprite_.emplace(item.sprite, &item);
    }
}

bool ParseTemplates(const char* text, size_t length, LayoutTemplates& out) {
    if (text == nullptr || length == 0) {
        return false;
    }
    out.clear();
    std::string currentName;
    /* 是否处在模板的 Size 区段内. */
    bool inSize = false;
    /* 是否正在收集模板的 OnClick 片段, 以及片段正文的起点. */
    bool capturingClick = false;
    size_t clickTextStart = 0;
    const std::string body(text, length);
    size_t cursor = 0;
    while (cursor < body.size()) {
        const size_t start = body.find('<', cursor);
        if (start == std::string::npos) {
            break;
        }
        const size_t end = body.find('>', start);
        if (end == std::string::npos) {
            break;
        }
        cursor = end + 1;
        const std::string head = body.substr(start + 1, end - start - 1);
        /*
         * 脚本片段: 从开标签之后到当前 '<' 之间的文本.
         * 此处 cursor 已越过闭标签, 因此正文结束位置必须用 start, 不能用 cursor.
         */
        if (capturingClick) {
            size_t begin = clickTextStart;
            size_t finish = start;
            while (begin < finish && (body[begin] == ' ' || body[begin] == '\t' ||
                                      body[begin] == '\r' || body[begin] == '\n')) {
                ++begin;
            }
            while (finish > begin && (body[finish - 1] == ' ' || body[finish - 1] == '\t' ||
                                      body[finish - 1] == '\r' || body[finish - 1] == '\n')) {
                --finish;
            }
            if (finish > begin && out.count(currentName) != 0) {
                out[currentName].onClick = body.substr(begin, finish - begin);
            }
            capturingClick = false;
        }
        Attributes attributes = ParseAttributes(head);
        const size_t tagEnd = head.find_first_of(" \t\r\n");
        attributes.tagName = tagEnd == std::string::npos ? head : head.substr(0, tagEnd);
        /* 结束标签: 关掉 Size 区段与脚本收集. */
        if (!head.empty() && head[0] == '/') {
            if (head.compare(1, 4, "Size") == 0) {
                inSize = false;
            }
            if (head.compare(1, 7, "OnClick") == 0) {
                capturingClick = false;
            }
            continue;
        }
        /* OnClick 开标签: 正文从标签之后开始, 到下一个 '<' 结束. */
        if (attributes.tagName == "OnClick") {
            capturingClick = !currentName.empty();
            clickTextStart = cursor;
            continue;
        }
        /*
         * 按钮三态贴图虽然带 name, 但它是当前模板的外观而不是新模板:
         * <NormalTexture> / <PushedTexture> / <CheckedTexture> 直接写到当前模板上.
         */
        if (!attributes.name.empty() && !currentName.empty() && out.count(currentName) != 0 &&
            (attributes.tagName == "NormalTexture" || attributes.tagName == "PushedTexture" ||
             attributes.tagName == "CheckedTexture")) {
            LayoutTemplate& owner = out[currentName];
            const bool pushed = attributes.tagName == "PushedTexture";
            const bool checked = attributes.tagName == "CheckedTexture";
            std::string& targetTexture = checked ? owner.checkedTexture
                                                 : (pushed ? owner.pushedTexture
                                                           : owner.normalTexture);
            std::string& targetSprite = checked ? owner.checkedSprite
                                                : (pushed ? owner.pushedSprite
                                                          : owner.normalSprite);
            if (!attributes.file.empty()) {
                targetTexture = attributes.file;
            }
            if (!attributes.uvname.empty()) {
                targetSprite = attributes.uvname;
            }
            continue;
        }
        /*
         * 只关心带名字的元素; 容器模板没有精灵名, 但有尺寸, 同样要登记.
         */
        if (attributes.name.empty()) {
            /* 子元素: 归属最近一个带名字的模板. */
            if (!currentName.empty() && out.count(currentName) != 0) {
                LayoutTemplate& entry = out[currentName];
                if (attributes.tagName == "Size") {
                    inSize = true;
                } else if (attributes.tagName == "AbsDimension" && inSize) {
                    /* 模板尺寸写在 Size 子元素里, 继承者据此拿到宽高. */
                    entry.width = ToFloat(attributes.x);
                    entry.height = ToFloat(attributes.y);
                } else if (attributes.tagName == "NineSquare") {
                    entry.insetLeft = ToFloat(attributes.ox);
                    entry.insetRight = ToFloat(attributes.ox);
                    entry.insetTop = ToFloat(attributes.oy);
                    entry.insetBottom = ToFloat(attributes.oy);
                    entry.tileWidth = ToFloat(attributes.tilew);
                    entry.tileHeight = ToFloat(attributes.tileh);
                } else if (attributes.tagName == "NormalTexture" ||
                           attributes.tagName == "PushedTexture" ||
                           attributes.tagName == "CheckedTexture") {
                    /* 按钮三态贴图: 记在模板上, 实例继承后才有外观. */
                    const bool pushed = attributes.tagName == "PushedTexture";
                    const bool checked = attributes.tagName == "CheckedTexture";
                    std::string& targetTexture =
                        checked ? entry.checkedTexture
                                : (pushed ? entry.pushedTexture : entry.normalTexture);
                    std::string& targetSprite =
                        checked ? entry.checkedSprite
                                : (pushed ? entry.pushedSprite : entry.normalSprite);
                    if (!attributes.file.empty()) {
                        targetTexture = attributes.file;
                    }
                    if (!attributes.uvname.empty()) {
                        targetSprite = attributes.uvname;
                    }
                } else if (attributes.tagName == "OffSet") {
                    /* 平铺偏移: 九宫参数里写作 OffSet(大写 S). */
                    entry.tileOffsetX = ToFloat(attributes.x);
                    entry.tileOffsetY = ToFloat(attributes.y);
                } else if (attributes.tagName == "Left") {
                    entry.insetLeft = ToFloat(attributes.w);
                } else if (attributes.tagName == "Right") {
                    entry.insetRight = ToFloat(attributes.w);
                } else if (attributes.tagName == "Top") {
                    entry.insetTop = ToFloat(attributes.h);
                } else if (attributes.tagName == "Bottom") {
                    entry.insetBottom = ToFloat(attributes.h);
                } else if (attributes.tagName == "Topleft") {
                    entry.insetLeft = ToFloat(attributes.w);
                    entry.insetTop = ToFloat(attributes.h);
                } else if (attributes.tagName == "Topright") {
                    entry.insetRight = ToFloat(attributes.w);
                    entry.insetTop = ToFloat(attributes.h);
                } else if (attributes.tagName == "Bottomleft") {
                    entry.insetLeft = ToFloat(attributes.w);
                    entry.insetBottom = ToFloat(attributes.h);
                } else if (attributes.tagName == "Bottomright") {
                    entry.insetRight = ToFloat(attributes.w);
                    entry.insetBottom = ToFloat(attributes.h);
                } else if (attributes.tagName == "Bottom") {
                    entry.insetBottom = ToFloat(attributes.h);
                }
            }
            continue;
        }
        LayoutTemplate entry;
        entry.texture = attributes.file;
        entry.sprite = attributes.uvname;
        entry.inherits = attributes.inherits;
        /* 模板直接写了宽高时一并收下; 子元素里的 Size 在下面按区段收. */
        entry.width = ToFloat(attributes.w);
        entry.height = ToFloat(attributes.h);
        /* abs_x/abs_y 是模板里另一种写尺寸的方式. */
        if (entry.width <= 0.0f) {
            entry.width = ToFloat(attributes.absX);
        }
        if (entry.height <= 0.0f) {
            entry.height = ToFloat(attributes.absY);
        }
        inSize = false;
        /* 拉伸方式: ninesquare 为九宫, center 为三片, 其余按整体拉伸. */
        if (attributes.drawType == "ninesquare" || attributes.drawType == "ninesquare_center") {
            /* ninesquare_center 是九宫的另一写法: 中心块居中, 四边拉伸. */
            entry.drawType = 2;
        } else if (attributes.drawType == "center") {
            entry.drawType = 1;
        }
        out[attributes.name] = entry;
        /* 子元素(NineSquare / Center 的 Left 与 Right)决定切片尺寸. */
        currentName = attributes.name;
    }
    ResolveTemplateInheritance(out);
    return !out.empty();
}

void ResolveTemplateInheritance(LayoutTemplates& templates) {
    if (templates.empty()) {
        return;
    }
    /* 逐层展开: 每轮把所有能解析的继承关系补上, 最多 8 轮. */
    for (int32_t pass = 0; pass < 8; ++pass) {
        bool changed = false;
        for (auto& item : templates) {
            LayoutTemplate& child = item.second;
            if (child.inherits.empty()) {
                continue;
            }
            const auto found = templates.find(child.inherits);
            if (found == templates.end() || found->first == item.first) {
                continue;
            }
            const LayoutTemplate& parent = found->second;
            if (child.texture.empty() && !parent.texture.empty()) {
                child.texture = parent.texture;
                changed = true;
            }
            if (child.sprite.empty() && !parent.sprite.empty()) {
                child.sprite = parent.sprite;
                changed = true;
            }
            if (child.width <= 0.0f && parent.width > 0.0f) {
                child.width = parent.width;
                changed = true;
            }
            if (child.height <= 0.0f && parent.height > 0.0f) {
                child.height = parent.height;
                changed = true;
            }
            if (child.drawType == 0 && parent.drawType != 0) {
                child.drawType = parent.drawType;
                child.insetLeft = parent.insetLeft;
                child.insetRight = parent.insetRight;
                child.insetTop = parent.insetTop;
                child.insetBottom = parent.insetBottom;
                child.tileWidth = parent.tileWidth;
                child.tileHeight = parent.tileHeight;
                changed = true;
            }
            if (child.onClick.empty() && !parent.onClick.empty()) {
                child.onClick = parent.onClick;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
}

}  /* namespace immirror::ui */
