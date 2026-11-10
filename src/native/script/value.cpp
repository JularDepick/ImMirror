#include "value.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

namespace immirror::script {

namespace {

/* 区域分配器的块大小, 单块越大分配越快, 但重置前的峰值占用也越高 */
const size_t kArenaBlockSize = 64u * 1024u;

/* 哈希表每个条目的估算开销, 用于内存配额统计 */
const size_t kHashEntryEstimate = 64;

const size_t kPointerSize = sizeof(void*);

size_t AlignUp(size_t value, size_t alignment) {
    if (alignment == 0) {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

/* 把 double 归一化为 int 键, 无法精确表示时返回 false */
bool NormalizeDoubleKey(double value, int64_t& out) {
    if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0)) {
        return false;
    }
    if (value != std::floor(value)) {
        return false;
    }
    out = static_cast<int64_t>(value);
    return true;
}

}  /* namespace */

const char* ValueTypeName(ValueType type) {
    switch (type) {
        case ValueType::Nil: return "nil";
        case ValueType::Bool: return "bool";
        case ValueType::Int: return "int";
        case ValueType::Double: return "double";
        case ValueType::String: return "string";
        case ValueType::Table: return "table";
        case ValueType::Closure: return "closure";
    }
    return "?";
}

Value NilValue() {
    Value value;
    value.type = ValueType::Nil;
    value.intValue = 0;
    return value;
}

Value MakeBool(bool input) {
    Value value;
    value.type = ValueType::Bool;
    value.boolValue = input;
    return value;
}

Value MakeInt(int64_t input) {
    Value value;
    value.type = ValueType::Int;
    value.intValue = input;
    return value;
}

Value MakeDouble(double input) {
    Value value;
    value.type = ValueType::Double;
    value.doubleValue = input;
    return value;
}

Value MakeString(StringObject* input) {
    Value value;
    value.type = ValueType::String;
    value.string = input;
    return value;
}

Value MakeTable(Table* input) {
    Value value;
    value.type = ValueType::Table;
    value.table = input;
    return value;
}

Value MakeClosure(Closure* input) {
    Value value;
    value.type = ValueType::Closure;
    value.closure = input;
    return value;
}

bool IsTruthy(const Value& value) {
    if (value.type == ValueType::Nil) {
        return false;
    }
    if (value.type == ValueType::Bool) {
        return value.boolValue;
    }
    return true;
}

std::string FormatInt(int64_t value) { return std::to_string(value); }

std::string FormatDouble(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%.14g", value);
    return buffer;
}

bool NumberToText(const Value& value, std::string& out) {
    if (value.type == ValueType::Int) {
        out = FormatInt(value.intValue);
        return true;
    }
    if (value.type == ValueType::Double) {
        out = FormatDouble(value.doubleValue);
        return true;
    }
    return false;
}

bool ValuesEqual(const Value& left, const Value& right) {
    if (left.type == right.type) {
        switch (left.type) {
            case ValueType::Nil:
                return true;
            case ValueType::Bool:
                return left.boolValue == right.boolValue;
            case ValueType::Int:
                return left.intValue == right.intValue;
            case ValueType::Double:
                return left.doubleValue == right.doubleValue;
            case ValueType::String:
                if (left.string == right.string) {
                    return true;
                }
                if (left.string->length != right.string->length) {
                    return false;
                }
                return std::memcmp(left.string->data, right.string->data, left.string->length) == 0;
            case ValueType::Table:
                return left.table == right.table;
            case ValueType::Closure:
                return left.closure == right.closure;
        }
        return false;
    }
    if (left.type == ValueType::Int && right.type == ValueType::Double) {
        int64_t converted = 0;
        return NormalizeDoubleKey(right.doubleValue, converted) && converted == left.intValue;
    }
    if (left.type == ValueType::Double && right.type == ValueType::Int) {
        int64_t converted = 0;
        return NormalizeDoubleKey(left.doubleValue, converted) && converted == right.intValue;
    }
    return false;
}

std::string ValueToDisplayString(const Value& value) {
    switch (value.type) {
        case ValueType::Nil:
            return "nil";
        case ValueType::Bool:
            return value.boolValue ? "true" : "false";
        case ValueType::Int:
            return FormatInt(value.intValue);
        case ValueType::Double:
            return FormatDouble(value.doubleValue);
        case ValueType::String:
            return std::string(value.string->data, value.string->length);
        case ValueType::Table: {
            char buffer[48];
            std::snprintf(buffer, sizeof(buffer), "table: %p", static_cast<const void*>(value.table));
            return buffer;
        }
        case ValueType::Closure: {
            const char* name = value.closure != nullptr ? value.closure->name : "?";
            return std::string("function: ") + name;
        }
    }
    return "?";
}

uint32_t HashBytes(const char* data, size_t length) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; ++i) {
        hash ^= static_cast<uint32_t>(static_cast<unsigned char>(data[i]));
        hash *= 16777619u;
    }
    return hash;
}

bool TableKey::operator==(const TableKey& other) const {
    if (type != other.type) {
        return false;
    }
    switch (type) {
        case ValueType::Bool:
            return boolValue == other.boolValue;
        case ValueType::Int:
            return intValue == other.intValue;
        case ValueType::Double:
            return doubleValue == other.doubleValue;
        case ValueType::String:
        case ValueType::Table:
        case ValueType::Closure:
            return pointer == other.pointer;
        case ValueType::Nil:
            return true;
    }
    return false;
}

size_t TableKeyHash::operator()(const TableKey& key) const {
    uint64_t h = 1469598103934665603ull;
    h ^= static_cast<uint64_t>(key.type) * 0x9E3779B97F4A7C15ull;
    switch (key.type) {
        case ValueType::Bool:
            h ^= key.boolValue ? 0x51u : 0x17u;
            break;
        case ValueType::Int:
            h ^= static_cast<uint64_t>(key.intValue) * 0x9E3779B97F4A7C15ull;
            break;
        case ValueType::Double: {
            uint64_t bits = 0;
            std::memcpy(&bits, &key.doubleValue, sizeof(bits));
            h ^= bits * 0x9E3779B97F4A7C15ull;
            break;
        }
        case ValueType::String: {
            const auto* text = static_cast<const StringObject*>(key.pointer);
            h ^= static_cast<uint64_t>(text->hash) << 1;
            break;
        }
        default:
            h ^= reinterpret_cast<uintptr_t>(key.pointer) * 0x9E3779B97F4A7C15ull;
            break;
    }
    h ^= h >> 29;
    h *= 0xBF58476D1CE4E5B9ull;
    h ^= h >> 32;
    return static_cast<size_t>(h);
}

Table::Table() = default;

Table::~Table() = default;

void Table::SetMemoryHook(TableMemoryHook hook, void* context) {
    hook_ = hook;
    hookContext_ = context;
    accounted_ = MemorySize();
}

void Table::Reserve(uint32_t arrayCount, uint32_t hashCount) {
    if (arrayCount > 0) {
        array_.reserve(arrayCount);
    }
    if (hashCount > 0) {
        hash_.reserve(hashCount);
    }
    NotifyMemory();
}

size_t Table::MemorySize() const {
    return sizeof(Table) + array_.capacity() * sizeof(Value) + hash_.size() * kHashEntryEstimate;
}

void Table::NotifyMemory() {
    if (hook_ == nullptr) {
        return;
    }
    const size_t current = MemorySize();
    if (current == accounted_) {
        return;
    }
    hook_(hookContext_, static_cast<int64_t>(current) - static_cast<int64_t>(accounted_));
    accounted_ = current;
}

Value Table::Get(const Value& key) const {
    if (key.type == ValueType::Nil) {
        return NilValue();
    }
    if (key.type == ValueType::Int) {
        const int64_t index = key.intValue;
        if (index >= 1 && static_cast<size_t>(index) <= array_.size()) {
            return array_[static_cast<size_t>(index) - 1];
        }
        TableKey normalized;
        normalized.type = ValueType::Int;
        normalized.intValue = index;
        const auto it = hash_.find(normalized);
        return it == hash_.end() ? NilValue() : it->second;
    }
    if (key.type == ValueType::Double) {
        int64_t converted = 0;
        if (NormalizeDoubleKey(key.doubleValue, converted)) {
            Value intKey;
            intKey.type = ValueType::Int;
            intKey.intValue = converted;
            return Get(intKey);
        }
        TableKey normalized;
        normalized.type = ValueType::Double;
        normalized.doubleValue = key.doubleValue;
        const auto it = hash_.find(normalized);
        return it == hash_.end() ? NilValue() : it->second;
    }

    TableKey normalized;
    normalized.type = key.type;
    switch (key.type) {
        case ValueType::Bool:
            normalized.boolValue = key.boolValue;
            break;
        case ValueType::String:
            normalized.pointer = key.string;
            break;
        case ValueType::Table:
            normalized.pointer = key.table;
            break;
        case ValueType::Closure:
            normalized.pointer = key.closure;
            break;
        default:
            return NilValue();
    }
    const auto it = hash_.find(normalized);
    return it == hash_.end() ? NilValue() : it->second;
}

void Table::MigrateFromHash() {
    for (;;) {
        TableKey probe;
        probe.type = ValueType::Int;
        probe.intValue = static_cast<int64_t>(array_.size() + 1);
        const auto it = hash_.find(probe);
        if (it == hash_.end()) {
            break;
        }
        array_.push_back(it->second);
        hash_.erase(it);
    }
}

void Table::Set(const Value& key, const Value& value) {
    if (key.type == ValueType::Nil) {
        return;
    }
    if (key.type == ValueType::Int) {
        const int64_t index = key.intValue;
        if (index >= 1 && static_cast<size_t>(index) <= array_.size()) {
            array_[static_cast<size_t>(index) - 1] = value;
            NotifyMemory();
            return;
        }
        if (index == static_cast<int64_t>(array_.size()) + 1) {
            array_.push_back(value);
            MigrateFromHash();
            NotifyMemory();
            return;
        }
        TableKey normalized;
        normalized.type = ValueType::Int;
        normalized.intValue = index;
        hash_[normalized] = value;
        NotifyMemory();
        return;
    }
    if (key.type == ValueType::Double) {
        int64_t converted = 0;
        if (NormalizeDoubleKey(key.doubleValue, converted)) {
            Value intKey;
            intKey.type = ValueType::Int;
            intKey.intValue = converted;
            Set(intKey, value);
            return;
        }
    }

    TableKey normalized;
    normalized.type = key.type;
    switch (key.type) {
        case ValueType::Bool:
            normalized.boolValue = key.boolValue;
            break;
        case ValueType::Double:
            normalized.doubleValue = key.doubleValue;
            break;
        case ValueType::String:
            normalized.pointer = key.string;
            break;
        case ValueType::Table:
            normalized.pointer = key.table;
            break;
        case ValueType::Closure:
            normalized.pointer = key.closure;
            break;
        default:
            return;
    }
    hash_[normalized] = value;
    NotifyMemory();
}

uint32_t Table::Length() const {
    size_t length = array_.size();
    while (length > 0 && array_[length - 1].type == ValueType::Nil) {
        --length;
    }
    return static_cast<uint32_t>(length);
}

size_t Table::HashCount() const { return hash_.size(); }

Arena::Arena() = default;

Arena::~Arena() {
    Reset();
    for (Block& block : blocks_) {
        delete[] block.data;
    }
    blocks_.clear();
}

bool Arena::AddBlock(size_t minimumBytes) {
    size_t size = kArenaBlockSize;
    while (size < minimumBytes) {
        size *= 2;
    }
    uint8_t* data = new (std::nothrow) uint8_t[size];
    if (data == nullptr) {
        return false;
    }
    Block block;
    block.data = data;
    block.size = size;
    block.used = 0;
    blocks_.push_back(block);
    capacity_ += size;
    return true;
}

void* Arena::Allocate(size_t bytes, size_t alignment) {
    if (alignment == 0) {
        alignment = kPointerSize;
    }
    if (blocks_.empty()) {
        if (!AddBlock(bytes + alignment)) {
            return nullptr;
        }
    }
    Block& block = blocks_.back();
    size_t offset = AlignUp(block.used, alignment);
    if (offset + bytes > block.size) {
        if (!AddBlock(bytes + alignment)) {
            return nullptr;
        }
        Block& fresh = blocks_.back();
        offset = AlignUp(fresh.used, alignment);
        fresh.used = offset + bytes;
        used_ += bytes;
        return fresh.data + offset;
    }
    block.used = offset + bytes;
    used_ += bytes;
    return block.data + offset;
}

void Arena::Reset() {
    ArenaHeader* header = header_;
    while (header != nullptr) {
        ArenaHeader* next = header->next;
        if (header->destroy != nullptr) {
            void* object = reinterpret_cast<uint8_t*>(header) + sizeof(ArenaHeader);
            header->destroy(object);
        }
        header = next;
    }
    header_ = nullptr;
    /* 只保留首个块复用, 其余释放, 避免长会话反复向系统申请大块内存 */
    if (blocks_.size() > 1) {
        for (size_t i = 1; i < blocks_.size(); ++i) {
            capacity_ -= blocks_[i].size;
            delete[] blocks_[i].data;
        }
        blocks_.resize(1);
    }
    if (!blocks_.empty()) {
        blocks_[0].used = 0;
    }
    used_ = 0;
}

StringObject* StringInterner::Find(const char* data, size_t length) const {
    const std::string_view key(data, length);
    const auto it = table_.find(key);
    return it == table_.end() ? nullptr : it->second;
}

StringObject* StringInterner::Intern(Arena& arena, const char* data, size_t length) {
    const std::string_view key(data, length);
    const auto it = table_.find(key);
    if (it != table_.end()) {
        return it->second;
    }
    auto* object = arena.New<StringObject>();
    if (object == nullptr) {
        return nullptr;
    }
    char* copy = static_cast<char*>(arena.Allocate(length + 1, 1));
    if (copy == nullptr) {
        return nullptr;
    }
    if (length > 0) {
        std::memcpy(copy, data, length);
    }
    copy[length] = '\0';
    object->hash = HashBytes(copy, length);
    object->length = static_cast<uint32_t>(length);
    object->data = copy;
    /* 键指向区域内存, 区域重置后整表清空, 不存在悬垂引用 */
    table_.emplace(std::string_view(copy, length), object);
    return object;
}

StringObject* StringInterner::Intern(Arena& arena, const std::string& text) {
    return Intern(arena, text.data(), text.size());
}

void StringInterner::Clear() { table_.clear(); }

}  /* namespace immirror::script */
