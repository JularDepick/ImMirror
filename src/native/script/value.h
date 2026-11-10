#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "bytecode.h"
#include "common.h"

namespace immirror::script {

class Table;
struct Closure;
struct StringObject;
struct NativeCall;
struct Value;

/* 值类型标签. 顺序变化会影响序列化之外的所有地方, 不要随意调整 */
enum class ValueType : uint8_t {
    Nil = 0,
    Bool,
    Int,
    Double,
    String,
    Table,
    Closure,
};

const char* ValueTypeName(ValueType type);

/* 驻留后的字符串对象, data 指向区域分配器或常量池中的稳定内存 */
struct StringObject {
    uint32_t hash = 0;
    uint32_t length = 0;
    const char* data = nullptr;
};

/*
 * 原生函数签名. 返回值为结果个数, 负数表示运行时错误码,
 * 具体含义见 sandbox.h 中的 VmError.
 */
using NativeFunction = int32_t (*)(NativeCall& call);

/* 原生调用上下文, 由虚拟机在调用点填充 */
struct NativeCall {
    void* host = nullptr;          /* 宿主世界句柄, 由 Vm::SetHost 提供 */
    const Value* args = nullptr;
    uint32_t argc = 0;
    Value* results = nullptr;
    uint32_t maxResults = 0;
    const char* errorMessage = nullptr;  /* 失败时由 binding 填写 */

    /* 由虚拟机注入的字符串构造函数, 使 binding 无需接触区域分配器 */
    StringObject* (*makeString)(void* context, const char* data, size_t length) = nullptr;
    void* stringContext = nullptr;
};

/* 原生函数返回值约定: 非负为结果个数, 负数为 -VmError 的数值 */
constexpr int32_t kNativeOk = 0;

struct BindingDef;

/* 闭包: 或指向脚本原型, 或指向白名单原生函数, 两者互斥 */
struct Closure {
    enum class Kind : uint8_t {
        Proto = 0,
        Native,
    };
    Kind kind = Kind::Proto;
    const Proto* proto = nullptr;
    uint32_t protoIndex = 0;
    const BindingDef* binding = nullptr;
    const char* name = "";
};

/*
 * 运行时值. 16 字节, 平凡可拷贝,
 * 类型标签决定联合体中哪个成员有效.
 */
struct Value {
    ValueType type;
    union {
        bool boolValue;
        int64_t intValue;
        double doubleValue;
        StringObject* string;
        Table* table;
        Closure* closure;
    };
};

/* 是否可作为调用目标 */
inline bool IsCallable(const Value& value) {
    return value.type == ValueType::Closure && value.closure != nullptr;
}

Value NilValue();
Value MakeBool(bool value);
Value MakeInt(int64_t value);
Value MakeDouble(double value);
Value MakeString(StringObject* value);
Value MakeTable(Table* value);
Value MakeClosure(Closure* value);

/* 真值语义: nil 与 false 为假, 其余为真, 包括 0 与空串 */
bool IsTruthy(const Value& value);

/* 相等语义: 数值跨 int/double 比较, 字符串按内容比较, 表与闭包按身份比较 */
bool ValuesEqual(const Value& left, const Value& right);

/* 面向日志与 tostring 的文本形式 */
std::string ValueToDisplayString(const Value& value);

/* 整数转文本 */
std::string FormatInt(int64_t value);

/* 浮点转文本, 去掉多余尾零 */
std::string FormatDouble(double value);

/* 数值转文本, 非数值返回 false */
bool NumberToText(const Value& value, std::string& out);

/* FNV-1a 32 位散列, 字符串驻留与表键共用 */
uint32_t HashBytes(const char* data, size_t length);

/* 表键. nil 不能作为键, int 与整数值的 double 归一化为 int */
struct TableKey {
    ValueType type = ValueType::Nil;
    union {
        bool boolValue;
        int64_t intValue;
        double doubleValue;
        const void* pointer;
    };

    bool operator==(const TableKey& other) const;
};

struct TableKeyHash {
    size_t operator()(const TableKey& key) const;
};

/* 表内部存储发生变化时的内存统计回调, 返回 false 表示超出配额 */
using TableMemoryHook = bool (*)(void* context, int64_t deltaBytes);

/*
 * 表: 数组部分存放 1 到 N 的连续整数键, 哈希部分存放其余键.
 * 键不能为 nil, 值为 nil 表示删除语义但仍占位.
 */
class Table {
public:
    Table();
    ~Table();

    /* 读取, 键不存在或键非法返回 nil */
    Value Get(const Value& key) const;
    /* 写入, 键为 nil 时忽略 */
    void Set(const Value& key, const Value& value);
    /* 数组部分从 1 开始连续非 nil 的长度 */
    uint32_t Length() const;
    /* 哈希部分条目数, 用于诊断与内存统计 */
    size_t HashCount() const;

    /* 当前内存占用估算, 单位字节 */
    size_t MemorySize() const;

    void SetMemoryHook(TableMemoryHook hook, void* context);

    /* 供 pairs 类扩展使用的遍历接口: 依次返回数组部分与哈希部分 */
    const std::vector<Value>& ArrayPart() const { return array_; }
    const std::unordered_map<TableKey, Value, TableKeyHash>& HashPart() const { return hash_; }

private:
    void NotifyMemory();
    void MigrateFromHash();

    std::vector<Value> array_;
    std::unordered_map<TableKey, Value, TableKeyHash> hash_;
    TableMemoryHook hook_ = nullptr;
    void* hookContext_ = nullptr;
    size_t accounted_ = 0;
};

/*
 * 区域分配器. 所有表、闭包与运行期字符串都从这里分配,
 * 一次执行结束时整体重置, 因此不需要逐个释放.
 */
class Arena {
public:
    Arena();
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    /* 分配原始内存, 失败返回 nullptr */
    void* Allocate(size_t bytes, size_t alignment);

    /* 分配并构造对象, 重置时自动调用析构函数 */
    template <typename T, typename... Args>
    T* New(Args&&... args) {
        const size_t alignment = alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
        void* raw = Allocate(sizeof(ArenaHeader) + sizeof(T), alignment);
        if (raw == nullptr) {
            return nullptr;
        }
        auto* header = static_cast<ArenaHeader*>(raw);
        header->destroy = &DestroyThunk<T>;
        header->next = header_;
        header_ = header;
        void* objectMemory = static_cast<uint8_t*>(raw) + sizeof(ArenaHeader);
        return new (objectMemory) T(static_cast<Args&&>(args)...);
    }

    /* 释放全部块并调用析构, 之后仍可继续分配 */
    void Reset();

    /* 已交给调用方的字节数 */
    size_t UsedBytes() const { return used_; }

    /* 连续分配块的总容量 */
    size_t CapacityBytes() const { return capacity_; }

private:
    struct ArenaHeader {
        void (*destroy)(void*);
        ArenaHeader* next;
    };
    struct Block {
        uint8_t* data;
        size_t size;
        size_t used;
    };

    template <typename T>
    static void DestroyThunk(void* object) {
        static_cast<T*>(object)->~T();
    }

    bool AddBlock(size_t minimumBytes);

    std::vector<Block> blocks_;
    ArenaHeader* header_ = nullptr;
    size_t used_ = 0;
    size_t capacity_ = 0;
};

/* 字符串驻留表, 保证同内容字符串共用同一个 StringObject */
class StringInterner {
public:
    StringObject* Intern(Arena& arena, const char* data, size_t length);
    StringObject* Intern(Arena& arena, const std::string& text);
    /* 只查找不创建 */
    StringObject* Find(const char* data, size_t length) const;
    void Clear();
    size_t Count() const { return table_.size(); }

private:
    std::unordered_map<std::string_view, StringObject*> table_;
};

}  /* namespace immirror::script */
