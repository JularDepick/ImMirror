#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ui/script/lua_vm.h"
#include "ui/script/ui_data_source.h"
#include "ui/widget_tree.h"

namespace immirror::ui::script {

/*
 * 控件宿主: 把控件树暴露给脚本.
 *
 * 脚本里一个控件就是一个带方法的表, 方法绑定到具体控件; 同一个控件多次取到
 * 拿到的是同一张表, 因此脚本里用 == 比较控件身份是可靠的.
 *
 * 提供的全局入口:
 *   getglobal(name) 按名字取控件, 取不到返回空
 *   this            根控件
 * 控件方法: Show, Hide, IsShown, GetName, GetChild, SetText, SetTextColor,
 *           SetSize, SetPoint, SetGray, SetTexture
 */
class WidgetHost {
public:
    /* 载入布局文本; 成功后自动建好控件表与全局绑定. */
    bool Load(const char* layoutText, size_t length);

    /* 按屏幕矩形解算并执行脚本. */
    bool RunScript(const char* source, size_t length, const LayoutRect& screen);

    /* 解算后展开绘制命令. */
    int32_t CollectDrawCommands(std::vector<LayoutDrawCommand>& out);

    WidgetTree& Tree() { return tree_; }
    Interpreter& Vm() { return vm_; }
    const std::string& Error() const { return error_; }

    /*
     * 事件派发: 把事件交给订阅它的控件.
     * 脚本用 this:RegisterEvent("名字") 订阅, 派发时调用 <控件名>_OnEvent(控件, 事件名, ...).
     * 返回成功调用到的处理函数个数.
     */
    int32_t DispatchEvent(const std::string& name, const std::vector<Value>& args);

    /*
     * 绑定数据源: 之后脚本可以读取背包, 玩家状态, 世界信息与进度统计.
     * 数据源由调用方持有, 生命周期必须覆盖宿主.
     */
    void BindDataSource(UiDataSource* source);

    /*
     * 提示出口: 脚本调用 ShowGameTips 时把文案交给这里, 由调用方决定怎么显示.
     * 未设置时只写日志.
     */
    void SetTipSink(std::function<void(const std::string&)> sink) { tipSink_ = std::move(sink); }

    /*
     * 音效出口: 脚本调用管理器播放二维音效时把资源路径交给这里.
     * 未设置时只写日志, 因此宿主断言不依赖音频引擎.
     */
    void SetSoundSink(std::function<void(const std::string&)> sink) {
        soundSink_ = std::move(sink);
    }

    /* 某事件当前有多少个控件订阅, 供统计与调试. */
    int32_t SubscriberCount(const std::string& name) const;

    /* 订阅表总条目数. */
    int32_t SubscriptionCount() const { return subscriptionTotal_; }

private:
    /* 取控件对应的表, 首次访问时创建并缓存. */
    Value TableOf(Widget* widget);

    WidgetTree tree_;
    Interpreter vm_;
    std::string error_;
    /* 控件到表的缓存, 保证同一控件始终是同一张表. */
    std::map<const Widget*, std::shared_ptr<Table>> cache_;
    /* 事件名到订阅控件的列表; 同一控件重复订阅同一事件只记一次. */
    /* 数据源指针, 供管理器方法查询定义; 由 BindDataSource 设置. */
    UiDataSource* dataSource_ = nullptr;
    /* 界面脚本之间共享的键值表: 一个界面写入, 另一个界面读取. */
    std::map<std::string, Value> gameData_;
    /* 脚本提示与音效的出口, 未设置时只写日志. */
    std::function<void(const std::string&)> tipSink_;
    std::function<void(const std::string&)> soundSink_;
    std::map<std::string, std::vector<Widget*>> subscriptions_;
    /* 订阅条目总数, 供统计. */
    int32_t subscriptionTotal_ = 0;
};

}  /* namespace immirror::ui::script */
