#include "binding.h"

#include <cmath>
#include <cstring>
#include <string>

namespace immirror::script {

namespace {

int32_t Fail(NativeCall& call, VmError error, const char* message) {
    call.errorMessage = message;
    return -static_cast<int32_t>(error);
}

/* 数值读取: 整数与浮点都接受, 其余类型一律失败 */
bool AsNumber(const Value& value, double& out) {
    if (value.type == ValueType::Int) {
        out = static_cast<double>(value.intValue);
        return true;
    }
    if (value.type == ValueType::Double) {
        out = value.doubleValue;
        return true;
    }
    return false;
}

bool AsInt32(const Value& value, int32_t& out) {
    if (value.type == ValueType::Int) {
        if (value.intValue < -2147483648ll || value.intValue > 2147483647ll) {
            return false;
        }
        out = static_cast<int32_t>(value.intValue);
        return true;
    }
    if (value.type == ValueType::Double) {
        if (!(value.doubleValue >= -2147483648.0 && value.doubleValue <= 2147483647.0)) {
            return false;
        }
        out = static_cast<int32_t>(value.doubleValue);
        return true;
    }
    return false;
}

StringObject* MakeText(NativeCall& call, const std::string& text) {
    if (call.makeString == nullptr) {
        return nullptr;
    }
    return call.makeString(call.stringContext, text.data(), text.size());
}

int32_t WriteString(NativeCall& call, const std::string& text) {
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    StringObject* object = MakeText(call, text);
    if (object == nullptr) {
        return Fail(call, VmError::MemoryQuota, "字符串创建失败, 内存配额可能已满");
    }
    call.results[0] = MakeString(object);
    return 1;
}

/* log(值): 交给宿主日志通道, 没有挂载宿主时静默丢弃 */
int32_t NativeLog(NativeCall& call) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, "log 需要至少一个参数");
    }
    std::string text;
    for (uint32_t i = 0; i < call.argc; ++i) {
        if (i > 0) {
            text += ' ';
        }
        text += ValueToDisplayString(call.args[i]);
    }
    auto* host = static_cast<HostWorld*>(call.host);
    if (host != nullptr) {
        host->Log(text.c_str());
    }
    return kNativeOk;
}

/* type(值): 返回类型名 */
int32_t NativeType(NativeCall& call) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, "type 需要一个参数");
    }
    return WriteString(call, ValueTypeName(call.args[0].type));
}

/* tostring(值): 返回文本形式 */
int32_t NativeToString(NativeCall& call) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, "tostring 需要一个参数");
    }
    return WriteString(call, ValueToDisplayString(call.args[0]));
}

/* tonumber(值): 字符串按十进制与浮点解析, 失败返回 nil */
int32_t NativeToNumber(NativeCall& call) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, "tonumber 需要一个参数");
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    const Value& value = call.args[0];
    if (value.type == ValueType::Int || value.type == ValueType::Double) {
        call.results[0] = value;
        return 1;
    }
    if (value.type == ValueType::String) {
        const std::string text(value.string->data, value.string->length);
        const char* begin = text.c_str();
        char* end = nullptr;
        const long long integer = std::strtoll(begin, &end, 10);
        if (end != nullptr && *end == '\0' && end != begin) {
            call.results[0] = MakeInt(static_cast<int64_t>(integer));
            return 1;
        }
        end = nullptr;
        const double real = std::strtod(begin, &end);
        if (end != nullptr && *end == '\0' && end != begin) {
            call.results[0] = MakeDouble(real);
            return 1;
        }
    }
    call.results[0] = NilValue();
    return 1;
}

/* len(字符串或表): 返回长度 */
int32_t NativeLen(NativeCall& call) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, "len 需要一个参数");
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    const Value& value = call.args[0];
    if (value.type == ValueType::String) {
        call.results[0] = MakeInt(static_cast<int64_t>(value.string->length));
        return 1;
    }
    if (value.type == ValueType::Table) {
        call.results[0] = MakeInt(static_cast<int64_t>(value.table->Length()));
        return 1;
    }
    return Fail(call, VmError::TypeMismatch, "len 只接受字符串或表");
}

/* 数值一元函数: 整数输入且结果精确时保留整数类型 */
template <bool kKeepInteger>
int32_t UnaryMath(NativeCall& call, const char* name, double (*fn)(double)) {
    if (call.argc < 1) {
        return Fail(call, VmError::ArityMismatch, name);
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    const Value& value = call.args[0];
    double input = 0.0;
    if (!AsNumber(value, input)) {
        return Fail(call, VmError::BindingType, "参数必须是数值");
    }
    const double output = fn(input);
    if (kKeepInteger && value.type == ValueType::Int) {
        call.results[0] = MakeInt(static_cast<int64_t>(output));
    } else {
        call.results[0] = MakeDouble(output);
    }
    return 1;
}

int32_t NativeAbs(NativeCall& call) {
    return UnaryMath<true>(call, "abs 需要一个数值参数", std::fabs);
}

int32_t NativeFloor(NativeCall& call) {
    return UnaryMath<true>(call, "floor 需要一个数值参数", std::floor);
}

int32_t NativeCeil(NativeCall& call) {
    return UnaryMath<true>(call, "ceil 需要一个数值参数", std::ceil);
}

int32_t NativeSqrt(NativeCall& call) {
    return UnaryMath<false>(call, "sqrt 需要一个数值参数", std::sqrt);
}

/* min/max: 两个参数, 同为整数时保留整数 */
int32_t NativeMinMax(NativeCall& call, bool wantMax) {
    if (call.argc < 2) {
        return Fail(call, VmError::ArityMismatch, "min/max 需要两个参数");
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    double left = 0.0;
    double right = 0.0;
    if (!AsNumber(call.args[0], left) || !AsNumber(call.args[1], right)) {
        return Fail(call, VmError::BindingType, "min/max 的参数必须是数值");
    }
    if (call.args[0].type == ValueType::Int && call.args[1].type == ValueType::Int) {
        const int64_t a = call.args[0].intValue;
        const int64_t b = call.args[1].intValue;
        call.results[0] = MakeInt(wantMax ? (a > b ? a : b) : (a < b ? a : b));
        return 1;
    }
    call.results[0] = MakeDouble(wantMax ? (left > right ? left : right) : (left < right ? left : right));
    return 1;
}

int32_t NativeMin(NativeCall& call) { return NativeMinMax(call, false); }
int32_t NativeMax(NativeCall& call) { return NativeMinMax(call, true); }

HostWorld* HostOf(NativeCall& call) { return static_cast<HostWorld*>(call.host); }

/* world_get_block(x, y, z) -> 方块 id */
int32_t NativeWorldGetBlock(NativeCall& call) {
    if (call.argc < 3) {
        return Fail(call, VmError::ArityMismatch, "world_get_block 需要三个坐标");
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    if (!AsInt32(call.args[0], x) || !AsInt32(call.args[1], y) || !AsInt32(call.args[2], z)) {
        return Fail(call, VmError::BindingType, "world_get_block 的坐标必须是整数");
    }
    HostWorld* host = HostOf(call);
    if (host == nullptr) {
        return Fail(call, VmError::BindingRejected, "宿主世界未挂载, 无法读取方块");
    }
    call.results[0] = MakeInt(host->GetBlock(x, y, z));
    return 1;
}

/* world_set_block(x, y, z, id) -> 是否成功 */
int32_t NativeWorldSetBlock(NativeCall& call) {
    if (call.argc < 4) {
        return Fail(call, VmError::ArityMismatch, "world_set_block 需要坐标与方块 id");
    }
    if (call.maxResults < 1) {
        return kNativeOk;
    }
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    int32_t block = 0;
    if (!AsInt32(call.args[0], x) || !AsInt32(call.args[1], y) || !AsInt32(call.args[2], z) ||
        !AsInt32(call.args[3], block)) {
        return Fail(call, VmError::BindingType, "world_set_block 的参数必须是整数");
    }
    HostWorld* host = HostOf(call);
    if (host == nullptr) {
        return Fail(call, VmError::BindingRejected, "宿主世界未挂载, 无法写入方块");
    }
    call.results[0] = MakeBool(host->SetBlock(x, y, z, block));
    return 1;
}

/* player_position() -> x, y, z */
int32_t NativePlayerPosition(NativeCall& call) {
    if (call.maxResults < 3) {
        return Fail(call, VmError::ArityMismatch, "player_position 需要三个结果槽位");
    }
    HostWorld* host = HostOf(call);
    if (host == nullptr) {
        return Fail(call, VmError::BindingRejected, "宿主世界未挂载, 无法读取玩家位置");
    }
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!host->PlayerPosition(x, y, z)) {
        return Fail(call, VmError::NativeFailure, "玩家位置不可用");
    }
    call.results[0] = MakeDouble(x);
    call.results[1] = MakeDouble(y);
    call.results[2] = MakeDouble(z);
    return 3;
}

}  /* namespace */

bool BindingRegistry::Register(const BindingDef& def, std::string& error) {
    if (def.name.empty()) {
        error = "binding 名字不能为空";
        return false;
    }
    if (def.function == nullptr) {
        error = "binding " + def.name + " 缺少函数指针";
        return false;
    }
    if (def.maxArgs < def.minArgs) {
        error = "binding " + def.name + " 的最大实参个数小于最小实参个数";
        return false;
    }
    if (def.maxArgs > kMaxNativeArgs) {
        error = "binding " + def.name + " 的实参个数超过上限";
        return false;
    }
    if (def.params.size() != def.minArgs) {
        error = "binding " + def.name + " 的类型描述个数与最小实参个数不一致";
        return false;
    }
    if (index_.find(def.name) != index_.end()) {
        error = "binding " + def.name + " 重复注册";
        return false;
    }
    index_[def.name] = defs_.size();
    defs_.push_back(def);
    return true;
}

bool BindingRegistry::RegisterFunction(const std::string& name, NativeFunction function,
                                       const std::vector<ValueType>& params, uint32_t maxArgs,
                                       const std::string& description, std::string& error) {
    BindingDef def;
    def.name = name;
    def.function = function;
    def.params = params;
    def.minArgs = static_cast<uint32_t>(params.size());
    def.maxArgs = maxArgs < def.minArgs ? def.minArgs : maxArgs;
    def.description = description;
    return Register(def, error);
}

const BindingDef* BindingRegistry::Find(const std::string& name) const {
    const auto it = index_.find(name);
    if (it == index_.end()) {
        return nullptr;
    }
    return &defs_[it->second];
}

void RegisterStandardBindings(BindingRegistry& registry) {
    std::string error;
    registry.RegisterFunction("log", &NativeLog, {}, 4, "输出日志到宿主", error);
    registry.RegisterFunction("type", &NativeType, {}, 1, "返回值的类型名", error);
    registry.RegisterFunction("tostring", &NativeToString, {}, 1, "转换为文本", error);
    registry.RegisterFunction("tonumber", &NativeToNumber, {}, 1, "转换为数值, 失败返回 nil", error);
    registry.RegisterFunction("len", &NativeLen, {}, 1, "字符串或表的长度", error);
    registry.RegisterFunction("abs", &NativeAbs, {ValueType::Double}, 1, "绝对值", error);
    registry.RegisterFunction("floor", &NativeFloor, {ValueType::Double}, 1, "向下取整", error);
    registry.RegisterFunction("ceil", &NativeCeil, {ValueType::Double}, 1, "向上取整", error);
    registry.RegisterFunction("sqrt", &NativeSqrt, {ValueType::Double}, 1, "平方根", error);
    registry.RegisterFunction("min", &NativeMin, {ValueType::Double, ValueType::Double}, 2,
                              "较小值", error);
    registry.RegisterFunction("max", &NativeMax, {ValueType::Double, ValueType::Double}, 2,
                              "较大值", error);
}

void RegisterWorldBindings(BindingRegistry& registry) {
    std::string error;
    registry.RegisterFunction("world_get_block", &NativeWorldGetBlock,
                              {ValueType::Int, ValueType::Int, ValueType::Int}, 3,
                              "读取指定坐标的方块 id", error);
    registry.RegisterFunction("world_set_block", &NativeWorldSetBlock,
                              {ValueType::Int, ValueType::Int, ValueType::Int, ValueType::Int}, 4,
                              "写入指定坐标的方块 id", error);
    registry.RegisterFunction("player_position", &NativePlayerPosition, {}, 0,
                              "返回玩家坐标 x, y, z", error);
}

size_t RegisterDefaultBindings(BindingRegistry& registry) {
    RegisterStandardBindings(registry);
    RegisterWorldBindings(registry);
    return 0;
}

}  /* namespace immirror::script */
