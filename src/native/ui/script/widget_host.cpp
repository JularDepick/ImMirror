#include "ui/script/widget_host.h"

#include <cstring>

namespace immirror::ui::script {
namespace {

AnchorPoint AnchorFromName(const std::string& name, AnchorPoint fallback) {
    if (name == "topleft") {
        return AnchorPoint::kTopLeft;
    }
    if (name == "top") {
        return AnchorPoint::kTop;
    }
    if (name == "topright") {
        return AnchorPoint::kTopRight;
    }
    if (name == "left") {
        return AnchorPoint::kLeft;
    }
    if (name == "center") {
        return AnchorPoint::kCenter;
    }
    if (name == "right") {
        return AnchorPoint::kRight;
    }
    if (name == "bottomleft") {
        return AnchorPoint::kBottomLeft;
    }
    if (name == "bottom") {
        return AnchorPoint::kBottom;
    }
    if (name == "bottomright") {
        return AnchorPoint::kBottomRight;
    }
    return fallback;
}

/*
 * 冒号形式的方法调用会把控件自身作为第一个实参传进来, 而绑定已经捕获了控件,
 * 因此这里统一跳过第一个实参, 后面的才是脚本真正写的参数.
 */
const std::vector<Value> kNoArgs;

const std::vector<Value>& RealArgs(const std::vector<Value>& args) {
    return args;
}

double NumberAt(const std::vector<Value>& args, size_t index, double fallback) {
    /* index 以"脚本实参"计, 因此实际取 args[index + 1]. */
    const size_t actual = index + 1;
    return actual < args.size() && args[actual].kind == Value::Kind::kNumber ? args[actual].number
                                                                            : fallback;
}

const Value& ArgAt(const std::vector<Value>& args, size_t index) {
    static const Value kEmpty;
    const size_t actual = index + 1;
    return actual < args.size() ? args[actual] : kEmpty;
}

}  /* namespace */

bool WidgetHost::Load(const char* layoutText, size_t length) {
    cache_.clear();
    if (!tree_.Load(layoutText, length)) {
        error_ = tree_.Error();
        return false;
    }
    /* 根控件: 布局里第一个有名字的控件, 作为脚本的 this. */
    Widget* root = nullptr;
    for (int32_t index = 0; index < tree_.WidgetCount(); ++index) {
        Widget* candidate = tree_.WidgetAt(index);
        if (candidate != nullptr && candidate->frame != nullptr && candidate->frame->name != nullptr &&
            candidate->frame->name[0] != '\0') {
            root = candidate;
            break;
        }
    }
    if (root == nullptr) {
        error_ = "布局里没有可用作根的名字控件";
        return false;
    }
    vm_.SetGlobal("this", TableOf(root));

    /* getglobal: 按名字取控件, 取不到返回空, 与脚本里的判空写法一致. */
    vm_.RegisterNative("getglobal", [this](const std::vector<Value>& args) {
        if (args.empty() || args[0].kind != Value::Kind::kString) {
            return std::vector<Value>{Value()};
        }
        Widget* widget = tree_.Find(args[0].text.c_str());
        return std::vector<Value>{widget == nullptr ? Value() : TableOf(widget)};
    });
    return true;
}

Value WidgetHost::TableOf(Widget* widget) {
    if (widget == nullptr) {
        return Value();
    }
    const auto found = cache_.find(widget);
    if (found != cache_.end()) {
        return Value::Of(found->second);
    }
    std::shared_ptr<Table> table(new Table());
    cache_[widget] = table;

    /* 方法一律用原生闭包实现, 捕获控件指针; 控件由控件树持有, 生命周期覆盖脚本. */
    const auto bind = [table](const char* name, Closure::NativeFn fn) {
        std::shared_ptr<Closure> closure(new Closure());
        closure->native = std::move(fn);
        table->Set(Value::Str(name), Value::Of(closure));
    };

    bind("Show", [this, widget](const std::vector<Value>&) {
        tree_.Show(widget);
        return std::vector<Value>();
    });
    bind("Hide", [this, widget](const std::vector<Value>&) {
        tree_.Hide(widget);
        return std::vector<Value>();
    });
    bind("IsShown", [this, widget](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(tree_.IsShown(widget))};
    });
    bind("GetName", [widget](const std::vector<Value>&) {
        const char* name = widget->frame != nullptr ? widget->frame->name : nullptr;
        return std::vector<Value>{Value::Str(name == nullptr ? "" : name)};
    });
    bind("GetChild", [this, widget](const std::vector<Value>& args) {
        const Value& name = ArgAt(args, 0);
        if (name.kind != Value::Kind::kString || widget->frame == nullptr) {
            return std::vector<Value>{Value()};
        }
        /* 子控件名 = 父名 + 子名, 与布局里 $parent 的展开规则一致. */
        const std::string childName = std::string(widget->frame->name) + name.text;
        Widget* child = tree_.Find(childName.c_str());
        return std::vector<Value>{child == nullptr ? Value() : TableOf(child)};
    });
    bind("SetText", [this, widget](const std::vector<Value>& args) {
        tree_.SetText(widget, ArgAt(args, 0).ToText().c_str());
        return std::vector<Value>();
    });
    bind("SetTextColor", [this, widget](const std::vector<Value>& args) {
        tree_.SetTextColor(widget, static_cast<float>(NumberAt(args, 0, 1.0)),
                           static_cast<float>(NumberAt(args, 1, 1.0)),
                           static_cast<float>(NumberAt(args, 2, 1.0)));
        return std::vector<Value>();
    });
    bind("SetSize", [this, widget](const std::vector<Value>& args) {
        tree_.SetSize(widget, static_cast<float>(NumberAt(args, 0, 0.0)),
                      static_cast<float>(NumberAt(args, 1, 0.0)));
        return std::vector<Value>();
    });
    bind("SetPoint", [this, widget](const std::vector<Value>& args) {
        const std::string point = ArgAt(args, 0).ToText();
        const std::string relativePoint = ArgAt(args, 1).ToText();
        tree_.SetPoint(widget, AnchorFromName(point, AnchorPoint::kTopLeft),
                       AnchorFromName(relativePoint, AnchorPoint::kTopLeft),
                       static_cast<float>(NumberAt(args, 2, 0.0)),
                       static_cast<float>(NumberAt(args, 3, 0.0)));
        return std::vector<Value>();
    });
    bind("SetGray", [this, widget](const std::vector<Value>& args) {
        const bool grayed = ArgAt(args, 0).Truthy();
        tree_.SetGray(widget, grayed);
        return std::vector<Value>();
    });
    bind("SetTexture", [this, widget](const std::vector<Value>& args) {
        tree_.SetTexture(widget, ArgAt(args, 0).ToText().c_str());
        return std::vector<Value>();
    });
    /* RegisterEvent: 事件系统尚未接入, 注册即忽略, 但不报错以免脚本中断. */
    bind("RegisterEvent", [](const std::vector<Value>&) { return std::vector<Value>(); });
    return Value::Of(table);
}

bool WidgetHost::RunScript(const char* source, size_t length, const LayoutRect& screen) {
    if (!vm_.Run(source, length)) {
        error_ = vm_.Error();
        return false;
    }
    /* 脚本改过尺寸与锚点后再解算, 因此解算必须排在执行之后. */
    tree_.Resolve(screen);
    return true;
}

int32_t WidgetHost::CollectDrawCommands(std::vector<LayoutDrawCommand>& out) {
    return tree_.CollectDrawCommands(out);
}

}  /* namespace immirror::ui::script */
