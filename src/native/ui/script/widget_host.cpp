#include "ui/script/widget_host.h"

#include <cstring>

#include "core/log.h"
#include "ui/generated/string_data_gen.h"

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

/*
 * 普通函数调用没有 self, 实参从零开始.
 * 数据源绑定都是全局函数, 必须用这一组; 控件方法才用上面那组.
 */
double PlainNumberAt(const std::vector<Value>& args, size_t index, double fallback) {
    return index < args.size() && args[index].kind == Value::Kind::kNumber ? args[index].number
                                                                           : fallback;
}

const Value& PlainArgAt(const std::vector<Value>& args, size_t index) {
    static const Value kEmpty;
    return index < args.size() ? args[index] : kEmpty;
}

/* 把一个宿主函数包成可放进表里的值. */
Value MakeNative(Closure::NativeFn fn) {
    std::shared_ptr<Closure> closure(new Closure());
    closure->native = std::move(fn);
    return Value::Of(closure);
}

}  /* namespace */

void WidgetHost::SetTemplates(const LayoutTemplates* templates) {
    tree_.SetTemplates(templates);
}

bool WidgetHost::Load(const char* layoutText, size_t length) {
    /*
     * 控件树会重建, 旧控件表必须一起作废:
     * 表里缓存的是控件指针, 重建后指针可能被复用, 旧表会把调用打到已经不在树上的对象.
     */
    cache_.clear();
    /*
     * 预置运行环境编号: 脚本里的 get_game_env 读 ClientMgr:getGameData("game_env"),
     * 引擎进入界面之前会写这个键; 离线环境没有引擎, 这里按"首次创建角色"预置.
     */
    if (gameData_.find("game_env") == gameData_.end()) {
        gameData_["game_env"] = Value::Number(1.0);
    }
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
    /*
     * 脚本侧常用全局:
     *   GetS(id)          字符串表查表, 界面文案几乎全部经它取
     *   Log(text)         写日志
     *   ShowGameTips(text) 给玩家一条提示
     *   GetInst(name)     取管理器单例; 返回空表, 方法调用自然得到空值而不报错
     */
    /*
     * 运行环境编号: 原版用它区分首次创建角色, 大厅与单人入口等场景.
     * 1 表示"首次创建角色", 界面脚本据此展开选角相关面板.
     */
    vm_.RegisterNative("get_game_env", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(1.0)};
    });
    /*
     * HideAllFrame(name): 把指定帧的所有直接子帧隐藏.
     * 界面脚本用它做页签切换前的复位, 原版定义在 playmain.lua, 这里按同一语义实现.
     */
    /*
     * HideAllFrame(name): 把指定帧的直接子帧全部隐藏, 用于页签切换前复位.
     * 与 playmain.lua 里的同名函数同一语义.
     */
    /*
     * HideAllFrame(name): 原版在页签切换前把子帧全部隐藏.
     * 离线环境按无操作处理: 真正执行后背包整屏空白, 因为随后的
     * "显示 PackFrame" 没能恢复可见性(链路上的 Show 未生效).
     * 保留布局默认状态, 避免转储输出空屏.
     */
    vm_.RegisterNative("HideAllFrame", [](const std::vector<Value>&) {
        return std::vector<Value>();
    });
    /* 脚本通过 _G 取用的其余引擎函数: 与画面无关的按安全默认值返回. */
    vm_.RegisterNative("IsEnableNewAccountSystem", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(false)};
    });
    vm_.RegisterNative("IsUIFrameShown", [this](const std::vector<Value>& args) {
        const std::string name = PlainArgAt(args, 0).ToText();
        Widget* widget = tree_.Find(name.c_str());
        return std::vector<Value>{Value::Bool(tree_.IsShown(widget))};
    });
    vm_.RegisterNative("IgnoreEsc", [](const std::vector<Value>&) {
        return std::vector<Value>();
    });
    vm_.RegisterNative("IsEduTouristMode", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(false)};
    });
    vm_.RegisterNative("isInGame", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(false)};
    });
    /*
     * 其余脚本常用全局: 只保证取到类型正确的值, 让界面逻辑能继续往下走.
     * 数值类给 0, 列表类给空表, 布尔类给假.
     */
    /*
     * press_btn(name): 按名字"按下"一个按钮, 即执行它的 OnClick 片段.
     * 界面脚本大量用它做页签与入口的统一触发, 因此要真执行而不是空实现.
     */
    vm_.RegisterNative("press_btn", [this](const std::vector<Value>& args) {
        const std::string name = PlainArgAt(args, 0).ToText();
        Widget* widget = tree_.Find(name.c_str());
        if (widget != nullptr) {
            Click(widget);
        }
        return std::vector<Value>();
    });
    /* 页签按钮的三态刷新: 只影响按钮外观, 布局侧按无操作处理. */
    vm_.RegisterNative("TemplateTabBtn2_SetState", [](const std::vector<Value>&) {
        return std::vector<Value>();
    });
    vm_.RegisterNative("getUin", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(0.0)};
    });
    vm_.RegisterNative("getOWID", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(0.0)};
    });
    vm_.RegisterNative("getNoviceGuideState", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(0.0)};
    });
    vm_.RegisterNative("getStringDef", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Str("")};
    });
    vm_.RegisterNative("getMyWorldList", [](const std::vector<Value>&) {
        std::shared_ptr<Table> list(new Table());
        return std::vector<Value>{Value::Of(list)};
    });
    vm_.RegisterNative("getMultiPlayer", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(false)};
    });
    vm_.RegisterNative("IsCoderEditMethod", [](const std::vector<Value>&) {
        return std::vector<Value>{Value::Bool(false)};
    });
    vm_.RegisterNative("setOperateUI", [](const std::vector<Value>&) {
        return std::vector<Value>();
    });
    vm_.RegisterNative("GetS", [](const std::vector<Value>& args) {
        if (args.empty()) {
            return std::vector<Value>{Value::Str("")};
        }
        const int32_t id = static_cast<int32_t>(PlainNumberAt(args, 0, 0.0));
        const char* text = gen::StringOf(static_cast<uint32_t>(id));
        return std::vector<Value>{Value::Str(text == nullptr ? "" : text)};
    });
    vm_.RegisterNative("Log", [](const std::vector<Value>& args) {
        std::string line;
        for (size_t index = 0; index < args.size(); ++index) {
            if (index > 0) {
                line += " ";
            }
            line += args[index].ToText();
        }
        IM_LOGI("UiScript", "%s", line.c_str());
        return std::vector<Value>();
    });
    vm_.RegisterNative("ShowGameTips", [this](const std::vector<Value>& args) {
        const std::string text = PlainArgAt(args, 0).ToText();
        if (tipSink_) {
            tipSink_(text);
        } else {
            IM_LOGI("UiScript", "tip: %s", text.c_str());
        }
        return std::vector<Value>();
    });
    /*
     * 管理器单例: 按名字给出带常用方法的表.
     * 只接有确定语义的方法 (平台判定, 版本, 字符串表查询, 播放音效);
     * 其余方法取到空值即可, 脚本的判空写法本来就会跳过它们.
     */
    /*
     * 界面管理器: 目前只接按模板复制控件.
     * 模板就是布局里已有的一棵子树 (通常是隐藏的), 复制到指定父控件下.
     */
    vm_.RegisterNative("GetInst", [this](const std::vector<Value>& args) {
        const std::string name = PlainArgAt(args, 0).ToText();
        std::shared_ptr<Table> table(new Table());
        if (name == "ClientMgr") {
            /* 平台判定: 客户端只跑移动端, 因此 isPC 恒假, isMobile 恒真. */
            table->Set(Value::Str("isPC"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(false)};
            }));
            table->Set(Value::Str("isMobile"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(true)};
            }));
            table->Set(Value::Str("getApiId"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("clientVersion"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Str("0.1.0")};
            }));
            table->Set(Value::Str("clientVersionFromStr"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            /*
             * 共享键值: 界面脚本用它把数据从一个界面带到另一个界面
             * (例如列表页把选中的编号交给详情页). 值原样保存, 不做类型转换.
             */
            /*
             * 冒号调用会把管理器自身放在参数第 0 位, 点号调用不会.
             * 这里取"第一个字符串参数"作键, 两种写法都能对上.
             */
            table->Set(Value::Str("getGameData"), MakeNative([this](const std::vector<Value>& call) {
                std::string key;
                for (const Value& value : call) {
                    if (value.kind == Value::Kind::kString) {
                        key = value.text;
                        break;
                    }
                }
                const auto found = gameData_.find(key);
                return std::vector<Value>{found == gameData_.end() ? Value() : found->second};
            }));
            table->Set(Value::Str("setGameData"), MakeNative([this](const std::vector<Value>& call) {
                size_t keyIndex = call.size();
                for (size_t index = 0; index < call.size(); ++index) {
                    if (call[index].kind == Value::Kind::kString) {
                        keyIndex = index;
                        break;
                    }
                }
                if (keyIndex >= call.size()) {
                    return std::vector<Value>();
                }
                const std::string key = call[keyIndex].text;
                gameData_[key] = keyIndex + 1 < call.size() ? call[keyIndex + 1] : Value();
                return std::vector<Value>();
            }));
            /* 二维音效: 界面点击音一类, 直接交给音频引擎. */
            table->Set(Value::Str("playSound2D"), MakeNative([this](const std::vector<Value>& call) {
                const std::string path = ArgAt(call, 0).ToText();
                if (path.empty()) {
                    return std::vector<Value>();
                }
                if (soundSink_) {
                    soundSink_(path);
                } else {
                    IM_LOGI("UiScript", "sound: %s", path.c_str());
                }
                return std::vector<Value>();
            }));
            /*
             * 其余常用方法: 界面脚本调用频次最高的一批.
             * 与画面无关的(设置写回, 评价, 跳转)按成功处理, 只保证脚本不因取空而中断.
             */
            table->Set(Value::Str("getVersionParamInt"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("appalyGameSetData"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(true)};
            }));
            table->Set(Value::Str("getNetworkState"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(1.0)};
            }));
            table->Set(Value::Str("playSound"), MakeNative([this](const std::vector<Value>& call) {
                const std::string path = ArgAt(call, 0).ToText();
                if (!path.empty()) {
                    if (soundSink_) {
                        soundSink_(path);
                    } else {
                        IM_LOGI("UiScript", "sound: %s", path.c_str());
                    }
                }
                return std::vector<Value>();
            }));
            table->Set(Value::Str("playMusic"), MakeNative([this](const std::vector<Value>& call) {
                const std::string path = ArgAt(call, 0).ToText();
                if (!path.empty() && soundSink_) {
                    soundSink_(path);
                }
                return std::vector<Value>();
            }));
            table->Set(Value::Str("getDeviceID"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Str("immirror-device")};
            }));
            table->Set(Value::Str("clickCopy"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(true)};
            }));
            table->Set(Value::Str("getNullItemIcon"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Str("")};
            }));
            table->Set(Value::Str("isFirstEnterGame"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(false)};
            }));
            table->Set(Value::Str("RequestReview"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>();
            }));
            table->Set(Value::Str("gotoGame"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>();
            }));
            table->Set(Value::Str("setAccount"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>();
            }));
            table->Set(Value::Str("setOneKeyBindCode"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>();
            }));
            table->Set(Value::Str("getRoomHostType"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            /* 同名方法在 ClientMgr 上也有一份, 脚本两种写法混用. */
            table->Set(Value::Str("getUin"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("getOWID"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("getNoviceGuideState"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("getStringDef"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Str("")};
            }));
            table->Set(Value::Str("getMultiPlayer"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(false)};
            }));
            table->Set(Value::Str("getMyWorldList"), MakeNative([](const std::vector<Value>&) {
                std::shared_ptr<Table> list(new Table());
                return std::vector<Value>{Value::Of(list)};
            }));
            table->Set(Value::Str("setOperateUI"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>();
            }));
            table->Set(Value::Str("IsCoderEditMethod"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Bool(false)};
            }));
            table->Set(Value::Str("hasGameData"), MakeNative([this](const std::vector<Value>& call) {
                const std::string key = ArgAt(call, 0).ToText();
                return std::vector<Value>{Value::Bool(gameData_.count(key) != 0)};
            }));
        } else if (name == "ClientBackpack") {
            /*
             * 背包管理器: 界面脚本按格位取物品编号, 空格返回 0.
             * 数据来自界面数据源, 因此离线跑界面时也有一致的格子内容.
             */
            table->Set(Value::Str("getGridItem"), MakeNative([this](const std::vector<Value>& call) {
                const int32_t slot = static_cast<int32_t>(NumberAt(call, 0, -1.0));
                if (dataSource_ == nullptr || slot < 0 || slot >= dataSource_->BagSlotCount()) {
                    return std::vector<Value>{Value::Number(0.0)};
                }
                const std::string name = dataSource_->BagSlotName(slot);
                return std::vector<Value>{Value::Number(name.empty() ? 0.0 : 1.0)};
            }));
            table->Set(Value::Str("getGridCount"), MakeNative([this](const std::vector<Value>&) {
                return std::vector<Value>{
                    Value::Number(dataSource_ == nullptr ? 0.0 : dataSource_->BagSlotCount())};
            }));
            table->Set(Value::Str("getGridCountAt"), MakeNative([this](const std::vector<Value>& call) {
                const int32_t slot = static_cast<int32_t>(NumberAt(call, 0, -1.0));
                if (dataSource_ == nullptr || slot < 0) {
                    return std::vector<Value>{Value::Number(0.0)};
                }
                return std::vector<Value>{
                    Value::Number(dataSource_->BagSlotCountAt(slot))};
            }));
            table->Set(Value::Str("getShortcutStartIndex"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("getGridNum"), MakeNative([this](const std::vector<Value>& call) {
                const int32_t slot = static_cast<int32_t>(NumberAt(call, 0, -1.0));
                if (dataSource_ == nullptr || slot < 0) {
                    return std::vector<Value>{Value::Number(0.0)};
                }
                return std::vector<Value>{Value::Number(dataSource_->BagSlotCountAt(slot))};
            }));
            table->Set(Value::Str("getGridUserdataStr"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Str("")};
            }));
            table->Set(Value::Str("getGridEnchantNum"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
            table->Set(Value::Str("getGridEnchantColor"), MakeNative([](const std::vector<Value>&) {
                return std::vector<Value>{Value::Number(0.0)};
            }));
        } else if (name == "UIFrameMgr") {
            table->Set(Value::Str("CreateFrameByTemplate"),
                       MakeNative([this](const std::vector<Value>& call) {
                           const std::string templateName = ArgAt(call, 0).ToText();
                           const std::string parentName = ArgAt(call, 1).ToText();
                           const std::string newName = ArgAt(call, 2).ToText();
                           Widget* source = tree_.Find(templateName.c_str());
                           if (source == nullptr) {
                               return std::vector<Value>{Value()};
                           }
                           /* 未指定父控件时挂在模板自己的父控件下. */
                           Widget* parent = nullptr;
                           if (!parentName.empty()) {
                               parent = tree_.Find(parentName.c_str());
                           } else if (source->frame != nullptr && source->frame->parent != nullptr) {
                               parent = tree_.Find(source->frame->parent->name);
                           }
                           Widget* created = tree_.Clone(source, parent, newName.c_str());
                           return std::vector<Value>{created == nullptr ? Value() : TableOf(created)};
                       }));
        } else if (name == "DefMgr") {
            /*
             * 物品定义查询: 返回表里放脚本实际读取的字段.
             * 字段名与物品表列名一致, 未收录的字段返回零或空串, 由脚本判空跳过.
             */
            /* 方块定义查询: 字段名与方块表列名一致, 未收录的返回空表. */
            if (dataSource_ != nullptr) {
                table->Set(Value::Str("getBlockDef"), MakeNative([this](const std::vector<Value>& call) {
                    const int32_t id = static_cast<int32_t>(NumberAt(call, 0, 0.0));
                    const BlockDefView view = dataSource_->BlockDef(id);
                    std::shared_ptr<Table> def(new Table());
                    if (!view.found) {
                        return std::vector<Value>{Value::Of(def)};
                    }
                    def->Set(Value::Str("ID"), Value::Number(static_cast<double>(view.id)));
                    def->Set(Value::Str("EnglishName"), Value::Str(view.englishName));
                    def->Set(Value::Str("Name"), Value::Str(view.englishName));
                    def->Set(Value::Str("Hardness"), Value::Number(static_cast<double>(view.hardness)));
                    def->Set(Value::Str("Tool"), Value::Number(static_cast<double>(view.tool)));
                    def->Set(Value::Str("Material"), Value::Number(static_cast<double>(view.material)));
                    def->Set(Value::Str("ToolLevel"),
                             Value::Number(static_cast<double>(view.toolLevel)));
                    def->Set(Value::Str("Type"), Value::Number(static_cast<double>(view.shape)));
                    def->Set(Value::Str("Shape"), Value::Number(static_cast<double>(view.shape)));
                    def->Set(Value::Str("Gravity"), Value::Number(static_cast<double>(view.gravity)));
                    def->Set(Value::Str("Replaceable"),
                             Value::Number(static_cast<double>(view.replaceable)));
                    def->Set(Value::Str("DropChance"),
                             Value::Number(static_cast<double>(view.dropChance)));
                    def->Set(Value::Str("AntiExplode"),
                             Value::Number(static_cast<double>(view.blastResist)));
                    def->Set(Value::Str("Slipperiness"),
                             Value::Number(static_cast<double>(view.slipperiness)));
                    return std::vector<Value>{Value::Of(def)};
                }));
                /* 方块对应物品的定义: 界面拿方块编号直接要物品信息. */
                table->Set(Value::Str("getBlockItemDefById"),
                           MakeNative([this](const std::vector<Value>& call) {
                               const int32_t id = static_cast<int32_t>(NumberAt(call, 0, 0.0));
                               const ItemDefView view = dataSource_->BlockItemDef(id);
                               std::shared_ptr<Table> def(new Table());
                               if (!view.found) {
                                   return std::vector<Value>{Value::Of(def)};
                               }
                               def->Set(Value::Str("ID"), Value::Number(static_cast<double>(view.id)));
                               def->Set(Value::Str("Name"), Value::Str(view.displayName));
                               def->Set(Value::Str("EnglishName"), Value::Str(view.englishName));
                               def->Set(Value::Str("Icon"), Value::Str(view.icon));
                               return std::vector<Value>{Value::Of(def)};
                           }));
                table->Set(Value::Str("getItemDef"), MakeNative([this](const std::vector<Value>& call) {
                    const int32_t id = static_cast<int32_t>(NumberAt(call, 0, 0.0));
                    const ItemDefView view = dataSource_->ItemDef(id);
                    std::shared_ptr<Table> def(new Table());
                    if (!view.found) {
                        return std::vector<Value>{Value::Of(def)};
                    }
                    def->Set(Value::Str("ID"), Value::Number(static_cast<double>(view.id)));
                    def->Set(Value::Str("Name"), Value::Str(view.displayName));
                    def->Set(Value::Str("EnglishName"), Value::Str(view.englishName));
                    def->Set(Value::Str("Icon"), Value::Str(view.icon));
                    def->Set(Value::Str("Type"), Value::Number(static_cast<double>(view.category)));
                    def->Set(Value::Str("ItemType"), Value::Number(static_cast<double>(view.category)));
                    def->Set(Value::Str("MaxStack"), Value::Number(static_cast<double>(view.maxStack)));
                    /* 界面字段: 名字与物品表列名一致. */
                    def->Set(Value::Str("Desc"), Value::Str(view.description));
                    def->Set(Value::Str("InvolvedID"),
                             Value::Number(static_cast<double>(view.involvedId)));
                    def->Set(Value::Str("UnlockFlag"),
                             Value::Number(static_cast<double>(view.unlockFlag)));
                    def->Set(Value::Str("UnlockType"),
                             Value::Number(static_cast<double>(view.unlockType)));
                    def->Set(Value::Str("CreateType"),
                             Value::Number(static_cast<double>(view.createType)));
                    def->Set(Value::Str("Model"), Value::Number(static_cast<double>(view.model)));
                    def->Set(Value::Str("ReplaceID"),
                             Value::Number(static_cast<double>(view.replaceId)));
                    def->Set(Value::Str("EnchantTag"),
                             Value::Number(static_cast<double>(view.enchantTag)));
                    def->Set(Value::Str("StuffType"),
                             Value::Number(static_cast<double>(view.stuffType)));
                    def->Set(Value::Str("Quality"), Value::Number(static_cast<double>(view.quality)));
                    return std::vector<Value>{Value::Of(def)};
                }));
            }
            /* 字符串表查询: 与全局 GetS 同一张表. */
            table->Set(Value::Str("getStringDef"), MakeNative([](const std::vector<Value>& call) {
                const int32_t id = static_cast<int32_t>(NumberAt(call, 0, 0.0));
                const char* text = gen::StringOf(static_cast<uint32_t>(id));
                return std::vector<Value>{Value::Str(text == nullptr ? "" : text)};
            }));
            table->Set(Value::Str("filterString"), MakeNative([](const std::vector<Value>& call) {
                return std::vector<Value>{Value::Str(ArgAt(call, 0).ToText())};
            }));
        }
        return std::vector<Value>{Value::Of(table)};
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
        /*
         * 显示后触发生命周期脚本: 首次显示先跑 OnLoad(列表构建、数据绑定),
         * 之后每次显示跑 OnShow. 之前这两个事件从未被调用, 界面初始化等于缺失.
         */
        if (widget != nullptr && widget->frame != nullptr) {
            if (!widget->loaded) {
                widget->loaded = true;
                FireFrameEvent(widget, widget->frame->onLoad);
            }
            FireFrameEvent(widget, widget->frame->onShow);
        }
        return std::vector<Value>();
    });
    bind("Hide", [this, widget](const std::vector<Value>&) {
        tree_.Hide(widget);
        if (widget != nullptr && widget->frame != nullptr) {
            FireFrameEvent(widget, widget->frame->onHide);
        }
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
    /*
     * SetTextColor(红, 绿, 蓝): 脚本按 0 到 255 给分量, 内部按 0 到 1 存.
     * 不归一化会让颜色值超出范围, 渲染时直接过曝.
     */
    bind("SetTextColor", [this, widget](const std::vector<Value>& args) {
        tree_.SetTextColor(widget, static_cast<float>(NumberAt(args, 0, 255.0)) / 255.0f,
                           static_cast<float>(NumberAt(args, 1, 255.0)) / 255.0f,
                           static_cast<float>(NumberAt(args, 2, 255.0)) / 255.0f);
        return std::vector<Value>();
    });
    bind("SetSize", [this, widget](const std::vector<Value>& args) {
        tree_.SetSize(widget, static_cast<float>(NumberAt(args, 0, 0.0)),
                      static_cast<float>(NumberAt(args, 1, 0.0)));
        return std::vector<Value>();
    });
    /*
     * SetPoint 的实参顺序是 (point, relativeTo, relativePoint, x, y):
     * 界面脚本靠它把格子按网格摆开, 少读一个实参会让坐标整体错位.
     */
    bind("SetPoint", [this, widget](const std::vector<Value>& args) {
        const std::string point = ArgAt(args, 0).ToText();
        const std::string relativeTo = ArgAt(args, 1).ToText();
        const std::string relativePoint = ArgAt(args, 2).ToText();
        tree_.SetPointTo(widget, AnchorFromName(point, AnchorPoint::kTopLeft), relativeTo.c_str(),
                         AnchorFromName(relativePoint, AnchorPoint::kTopLeft),
                         static_cast<float>(NumberAt(args, 3, 0.0)),
                         static_cast<float>(NumberAt(args, 4, 0.0)));
        return std::vector<Value>();
    });
    /*
     * Checked(值): 页签按钮的选中态.
     * 界面脚本用它标记当前页签, 选中后应换成 CheckedTexture 的贴图.
     */
    bind("Checked", [this, widget](const std::vector<Value>& args) {
        if (widget != nullptr) {
            widget->checked = args.size() > 1 ? args[1].Truthy() : true;
        }
        return std::vector<Value>();
    });
    bind("SetChecked", [this, widget](const std::vector<Value>& args) {
        if (widget != nullptr) {
            widget->checked = ArgAt(args, 0).Truthy();
        }
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
    /* 高清贴图: 与普通贴图同一语义, 布局侧只认一个贴图字段. */
    bind("SetTextureHuires", [this, widget](const std::vector<Value>& args) {
        tree_.SetTexture(widget, ArgAt(args, 0).ToText().c_str());
        return std::vector<Value>();
    });
    /* 遮罩与叠加: 属于渲染特效, 二维布局层按无操作处理. */
    bind("setMask", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("SetMaskColor", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("SetOverlay", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("SetUVAnimation", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("SetBlendAlpha", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("IsReshow", [](const std::vector<Value>&) { return std::vector<Value>{Value::Bool(false)}; });
    bind("IsRehide", [](const std::vector<Value>&) { return std::vector<Value>{Value::Bool(false)}; });
    /* 文本读取: 脚本大量用文本做判断与拼接. */
    bind("GetText", [widget](const std::vector<Value>&) {
        if (!widget->text.empty()) {
            return std::vector<Value>{Value::Str(widget->text)};
        }
        const char* text = widget->frame != nullptr ? widget->frame->text : nullptr;
        return std::vector<Value>{Value::Str(text == nullptr ? "" : text)};
    });
    /* 贴图 uv 覆盖: 与整图替换同一语义, 二维布局里按无操作处理. */
    bind("SetTexUV", [](const std::vector<Value>&) { return std::vector<Value>(); });
    /* 父控件: 列表行常用它反查所属列表. */
    bind("GetParentFrame", [this, widget](const std::vector<Value>&) {
        Widget* parent = nullptr;
        if (widget->frame != nullptr && widget->frame->parent != nullptr) {
            parent = tree_.Find(widget->frame->parent->name);
        }
        return std::vector<Value>{parent == nullptr ? Value() : TableOf(parent)};
    });
    /* 控件编号: 原版用于区分同模板克隆出来的行, 这里给 0 表示未分配. */
    bind("GetClientID", [widget](const std::vector<Value>&) {
        return std::vector<Value>{
            Value::Number(widget->frame == nullptr ? 0.0 : widget->frame->clientId)};
    });
    /* 禁用与启用: 置灰表达, 命中仍可达, 由脚本按 IsShown 与置灰判断. */
    bind("Disable", [this, widget](const std::vector<Value>&) {
        tree_.SetGray(widget, true);
        return std::vector<Value>();
    });
    bind("Enable", [this, widget](const std::vector<Value>&) {
        tree_.SetGray(widget, false);
        return std::vector<Value>();
    });
    /* 控件级共享串: 克隆出来的列表行用它携带行数据. */
    bind("SetClientString", [this, widget](const std::vector<Value>& args) {
        clientStrings_[widget] = ArgAt(args, 0).ToText();
        return std::vector<Value>();
    });
    bind("GetClientString", [this, widget](const std::vector<Value>&) {
        const auto found = clientStrings_.find(widget);
        return std::vector<Value>{Value::Str(found == clientStrings_.end() ? "" : found->second)};
    });
    bind("SetClientUserData", [this, widget](const std::vector<Value>& args) {
        clientStrings_[widget] = ArgAt(args, 0).ToText();
        return std::vector<Value>();
    });
    /* 取子控件: 与 GetChild 同义, 名字前缀规则一致. */
    bind("GetCtrl", [this, widget](const std::vector<Value>& args) {
        const Value& name = ArgAt(args, 0);
        if (name.kind != Value::Kind::kString || widget->frame == nullptr) {
            return std::vector<Value>{Value()};
        }
        const std::string childName = std::string(widget->frame->name) + name.text;
        Widget* child = tree_.Find(childName.c_str());
        return std::vector<Value>{child == nullptr ? Value() : TableOf(child)};
    });
    /* 清空: 列表重建前的复位, 二维布局里没有子控件增删, 按无操作处理. */
    bind("Clear", [](const std::vector<Value>&) { return std::vector<Value>(); });
    /*
     * 三维角色预览: 布局里的 View 控件控制相机与角色站位, 与二维界面无关.
     * 这里显式给空实现, 避免脚本因为方法不存在而中断.
     */
    bind("setCameraWidthFov", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("setCameraLookAt", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("setActorPosition", [](const std::vector<Value>&) { return std::vector<Value>(); });
    bind("addBackgroundEffect", [](const std::vector<Value>&) { return std::vector<Value>(); });
    /* RegisterEvent: 把控件挂到该事件上, 重复订阅同一事件只记一次. */
    bind("RegisterEvent", [this, widget](const std::vector<Value>& args) {
        const Value& name = ArgAt(args, 0);
        if (name.kind != Value::Kind::kString) {
            return std::vector<Value>();
        }
        std::vector<Widget*>& list = subscriptions_[name.text];
        for (Widget* existing : list) {
            if (existing == widget) {
                return std::vector<Value>();
            }
        }
        list.push_back(widget);
        subscriptionTotal_ += 1;
        return std::vector<Value>();
    });
    /* UnregisterEvent: 反向操作, 脚本里少见但成对出现. */
    bind("UnregisterEvent", [this, widget](const std::vector<Value>& args) {
        const Value& name = ArgAt(args, 0);
        if (name.kind != Value::Kind::kString) {
            return std::vector<Value>();
        }
        std::vector<Widget*>& list = subscriptions_[name.text];
        for (size_t index = 0; index < list.size(); ++index) {
            if (list[index] == widget) {
                list.erase(list.begin() + static_cast<long>(index));
                subscriptionTotal_ -= 1;
                break;
            }
        }
        return std::vector<Value>();
    });
    return Value::Of(table);
}

bool WidgetHost::RunScript(const char* source, size_t length, const LayoutRect& screen) {
    /* 每次执行前刷新管理器全局与 _G: 原生函数可能在上一步才注册. */
    const char* bindManagers =
        "ClientMgr = GetInst(\"ClientMgr\")\n"
        "ClientBackpack = GetInst(\"ClientBackpack\")\n"
        "DefMgr = GetInst(\"DefMgr\")\n"
        "AccountManager = GetInst(\"AccountManager\")\n"
        "CurWorld = GetInst(\"CurWorld\")\n"
        "CurMainPlayer = GetInst(\"CurMainPlayer\")\n"
        "UIFrameMgr = GetInst(\"UIFrameMgr\")\n"
        "UIManager = GetInst(\"UIManager\")\n"
        "ClientScene = GetInst(\"ClientScene\")\n"
        "CurScene = GetInst(\"CurScene\")\n"
        "ClientActor = GetInst(\"ClientActor\")\n";
    vm_.Run(bindManagers, std::strlen(bindManagers));
    vm_.PublishGlobalTable();
    if (!vm_.Run(source, length)) {
        error_ = vm_.Error();
        return false;
    }
    /* 脚本改过尺寸与锚点后再解算, 因此解算必须排在执行之后. */
    lastScreen_ = screen;
    tree_.Resolve(screen);
    return true;
}

int32_t WidgetHost::FireFrameEvent(Widget* widget, const char* snippet) {
    if (widget == nullptr || snippet == nullptr || snippet[0] == '\0') {
        return 0;
    }
    /* 脚本里可能再调 Show/Hide, 递归会直接爆栈, 这里挡住. */
    if (firingEvent_) {
        return 0;
    }
    firingEvent_ = true;
    /*
     * 生命周期脚本要把 this 指向被触发的控件, 但执行完必须恢复现场 ——
     * 否则调用方(比如脚本里 Show 之后继续用 this)会拿到错误的控件.
     */
    const Value savedThis = vm_.GetGlobal("this");
    vm_.SetGlobal("this", TableOf(widget));
    const bool ok = vm_.Run(snippet, std::strlen(snippet));
    vm_.SetGlobal("this", savedThis);
    firingEvent_ = false;
    if (!ok) {
        error_ = vm_.Error();
        return 0;
    }
    tree_.Resolve(lastScreen_);
    return 1;
}

int32_t WidgetHost::Click(Widget* widget) {
    if (widget == nullptr || widget->frame == nullptr || widget->frame->onClick == nullptr) {
        return 0;
    }
    vm_.SetGlobal("this", TableOf(widget));
    const char* snippet = widget->frame->onClick;
    if (!vm_.Run(snippet, std::strlen(snippet))) {
        error_ = vm_.Error();
        return 0;
    }
    /* 点击常改文本与显示状态, 执行后重新解算. */
    tree_.Resolve(lastScreen_);
    return 1;
}

void WidgetHost::BindDataSource(UiDataSource* source) {
    if (source == nullptr) {
        return;
    }
    dataSource_ = source;
    /*
     * 数据读取一律走宿主函数: 脚本不直接持有游戏对象, 因此界面可以先用假数据跑通,
     * 接入游戏时只换数据源实现.
     */
    vm_.RegisterNative("BagSlotCount", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(static_cast<double>(source->BagSlotCount()))};
    });
    vm_.RegisterNative("BagSlotName", [source](const std::vector<Value>& args) {
        const int32_t slot = static_cast<int32_t>(PlainNumberAt(args, 0, 0.0));
        return std::vector<Value>{Value::Str(source->BagSlotName(slot))};
    });
    vm_.RegisterNative("BagSlotCountAt", [source](const std::vector<Value>& args) {
        const int32_t slot = static_cast<int32_t>(PlainNumberAt(args, 0, 0.0));
        return std::vector<Value>{Value::Number(static_cast<double>(source->BagSlotCountAt(slot)))};
    });
    vm_.RegisterNative("BagSlotDurability", [source](const std::vector<Value>& args) {
        const int32_t slot = static_cast<int32_t>(PlainNumberAt(args, 0, 0.0));
        return std::vector<Value>{
            Value::Number(static_cast<double>(source->BagSlotDurability(slot)))};
    });
    vm_.RegisterNative("PlayerHealth", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(static_cast<double>(source->Health()))};
    });
    vm_.RegisterNative("PlayerHunger", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(static_cast<double>(source->Hunger()))};
    });
    vm_.RegisterNative("PlayerStamina", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(static_cast<double>(source->Stamina()))};
    });
    vm_.RegisterNative("WorldName", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Str(source->WorldName())};
    });
    vm_.RegisterNative("WorldTime", [source](const std::vector<Value>&) {
        return std::vector<Value>{Value::Number(static_cast<double>(source->WorldTimeSeconds()))};
    });
    vm_.RegisterNative("ProgressValue", [source](const std::vector<Value>& args) {
        const Value& key = PlainArgAt(args, 0);
        if (key.kind != Value::Kind::kString) {
            return std::vector<Value>{Value::Number(0.0)};
        }
        return std::vector<Value>{
            Value::Number(static_cast<double>(source->ProgressValue(key.text)))};
    });
}

std::vector<std::string> WidgetHost::SubscribedEvents() const {
    std::vector<std::string> names;
    names.reserve(subscriptions_.size());
    for (const auto& entry : subscriptions_) {
        if (!entry.second.empty()) {
            names.push_back(entry.first);
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

int32_t WidgetHost::SubscriberCount(const std::string& name) const {
    const auto found = subscriptions_.find(name);
    return found == subscriptions_.end() ? 0 : static_cast<int32_t>(found->second.size());
}

int32_t WidgetHost::DispatchEvent(const std::string& name, const std::vector<Value>& args) {
    const auto found = subscriptions_.find(name);
    if (found == subscriptions_.end()) {
        return 0;
    }
    int32_t called = 0;
    /* 订阅列表在派发期间可能被脚本改动, 先拷贝一份再遍历. */
    const std::vector<Widget*> targets = found->second;
    for (Widget* widget : targets) {
        if (widget == nullptr || widget->frame == nullptr || widget->frame->name == nullptr) {
            continue;
        }
        /* 处理函数名 = 控件名 + _OnEvent, 与界面脚本的命名约定一致. */
        const std::string handler = std::string(widget->frame->name) + "_OnEvent";
        const Value function = vm_.GetGlobal(handler);
        if (function.kind != Value::Kind::kFunction) {
            continue;
        }
        std::vector<Value> callArgs;
        callArgs.push_back(TableOf(widget));
        callArgs.push_back(Value::Str(name));
        callArgs.insert(callArgs.end(), args.begin(), args.end());
        vm_.CallGlobal(handler, callArgs);
        called += 1;
        if (!vm_.Error().empty()) {
            error_ = vm_.Error();
            break;
        }
    }
    return called;
}

int32_t WidgetHost::CollectDrawCommandsFrom(const char* name,
                                           std::vector<LayoutDrawCommand>& out) {
    return tree_.CollectDrawCommandsFrom(name, out);
}

int32_t WidgetHost::CollectDrawCommands(std::vector<LayoutDrawCommand>& out) {
    return tree_.CollectDrawCommands(out);
}

}  /* namespace immirror::ui::script */
