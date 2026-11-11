#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace immirror::editor {

/*
 * 工程文件用的最小 JSON 实现.
 * 只覆盖工程格式需要的子集: 对象按插入顺序保存键值对, 因此写出的文件
 * 每次都是同样的字节序列, 便于文本比对与版本管理; 不引入第三方库.
 */
enum class JsonType {
    Null = 0,
    Bool,
    Number,
    String,
    Array,
    Object,
};

class JsonValue {
public:
    JsonValue() = default;

    static JsonValue MakeNull();
    static JsonValue MakeBool(bool value);
    static JsonValue MakeNumber(double value);
    static JsonValue MakeInt(int64_t value);
    static JsonValue MakeString(const std::string& value);
    static JsonValue MakeArray();
    static JsonValue MakeObject();

    JsonType Type() const { return type_; }
    bool IsObject() const { return type_ == JsonType::Object; }
    bool IsArray() const { return type_ == JsonType::Array; }
    bool IsNumber() const { return type_ == JsonType::Number; }
    bool IsString() const { return type_ == JsonType::String; }
    bool IsBool() const { return type_ == JsonType::Bool; }
    bool IsNull() const { return type_ == JsonType::Null; }

    bool AsBool(bool fallback = false) const;
    double AsNumber(double fallback = 0.0) const;
    int64_t AsInt(int64_t fallback = 0) const;
    const std::string& AsString() const { return string_; }

    /* 数组操作. */
    const std::vector<JsonValue>& Items() const { return array_; }
    std::vector<JsonValue>& Items() { return array_; }
    void Push(JsonValue value);

    /* 对象操作: 同名键覆盖原位置, 新键追加到末尾, 顺序因此稳定. */
    const std::vector<std::pair<std::string, JsonValue>>& Members() const { return object_; }
    void Set(const std::string& key, JsonValue value);
    const JsonValue* Find(const std::string& key) const;

    /* 带默认值的取键辅助, 缺失或类型不符时返回 fallback. */
    int64_t GetInt(const std::string& key, int64_t fallback = 0) const;
    double GetNumber(const std::string& key, double fallback = 0.0) const;
    bool GetBool(const std::string& key, bool fallback = false) const;
    std::string GetString(const std::string& key, const std::string& fallback = std::string()) const;

    /*
     * 序列化成文本. pretty 为真时按两空格缩进换行, 否则压成一行.
     * 数字始终按最简形式输出, 整数不带小数点.
     */
    std::string Serialize(bool pretty) const;

private:
    void WriteValue(std::string& out, bool pretty, int32_t depth) const;

    JsonType type_ = JsonType::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    std::vector<JsonValue> array_;
    std::vector<std::pair<std::string, JsonValue>> object_;
};

/*
 * 解析文本. 失败时返回 false 并在 error 里给出位置与原因,
 * 深度上限防止构造极深的嵌套把栈打穿.
 */
bool JsonParse(const std::string& text, JsonValue& out, std::string& error);

/* 读取与写入整个文件, 失败时 error 给出路径与原因. */
bool JsonReadFile(const std::string& path, JsonValue& out, std::string& error);
bool JsonWriteFile(const std::string& path, const JsonValue& value, std::string& error);

}  /* namespace immirror::editor */
