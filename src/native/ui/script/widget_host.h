#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ui/script/lua_vm.h"
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

private:
    /* 取控件对应的表, 首次访问时创建并缓存. */
    Value TableOf(Widget* widget);

    WidgetTree tree_;
    Interpreter vm_;
    std::string error_;
    /* 控件到表的缓存, 保证同一控件始终是同一张表. */
    std::map<const Widget*, std::shared_ptr<Table>> cache_;
};

}  /* namespace immirror::ui::script */
