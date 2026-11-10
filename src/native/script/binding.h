#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "sandbox.h"
#include "value.h"

namespace immirror::script {

/*
 * 一条原生函数的白名单定义. params 描述每个实参的期望类型,
 * 长度内为强类型校验, 超出部分按 maxArgs 允许但只做存在性校验.
 */
struct BindingDef {
    std::string name;
    NativeFunction function = nullptr;
    uint32_t minArgs = 0;
    uint32_t maxArgs = 0;
    std::vector<ValueType> params;
    std::string description;
};

/*
 * 白名单注册表. 只有注册过的名字会进入脚本全局环境,
 * 注册阶段就完成签名合法性检查, 运行期只做参数校验.
 */
class BindingRegistry {
public:
    /* 注册, 重名或签名非法时返回 false 并填写 error */
    bool Register(const BindingDef& def, std::string& error);

    /* 便捷重载, description 可为空字符串 */
    bool RegisterFunction(const std::string& name, NativeFunction function,
                          const std::vector<ValueType>& params, uint32_t maxArgs,
                          const std::string& description, std::string& error);

    const BindingDef* Find(const std::string& name) const;
    const std::vector<BindingDef>& All() const { return defs_; }
    size_t Count() const { return defs_.size(); }

private:
    std::vector<BindingDef> defs_;
    std::unordered_map<std::string, size_t> index_;
};

/*
 * 宿主世界接口. 引擎侧实现该接口后传给虚拟机,
 * binding 只依赖这个抽象, 不直接触碰体素与玩家模块.
 */
class HostWorld {
public:
    virtual ~HostWorld() = default;
    virtual int32_t GetBlock(int32_t x, int32_t y, int32_t z) = 0;
    virtual bool SetBlock(int32_t x, int32_t y, int32_t z, int32_t blockId) = 0;
    virtual bool PlayerPosition(double& x, double& y, double& z) = 0;
    virtual void Log(const char* message) = 0;
};

/* 注册内置函数: 类型判断, 数值与字符串工具 */
void RegisterStandardBindings(BindingRegistry& registry);

/* 注册世界相关白名单函数: world_get_block 等 */
void RegisterWorldBindings(BindingRegistry& registry);

/* 一次性注册默认全集, 返回注册失败的名字数量 */
size_t RegisterDefaultBindings(BindingRegistry& registry);

/* 单次原生调用允许的最大实参个数, 与虚拟机的结果缓冲一致 */
constexpr uint32_t kMaxNativeArgs = 8;

/* 单次原生调用允许的最大结果个数 */
constexpr uint32_t kMaxNativeResults = 8;

}  /* namespace immirror::script */
