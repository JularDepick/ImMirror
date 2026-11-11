#include "editor/editor_json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace immirror::editor {

namespace {

/* 嵌套深度上限, 工程文件的实际嵌套不超过四层, 留出余量后仍然能挡住恶意输入. */
constexpr int32_t kMaxDepth = 32;

/* 解析游标与错误信息. */
struct Parser {
    const char* begin = nullptr;
    const char* cursor = nullptr;
    const char* end = nullptr;
    std::string error;

    bool Fail(const char* message) {
        if (error.empty()) {
            error = message;
        }
        return false;
    }

    void SkipSpace() {
        while (cursor < end) {
            const char c = *cursor;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++cursor;
                continue;
            }
            break;
        }
    }

    bool ParseValue(JsonValue& out, int32_t depth);
    bool ParseObject(JsonValue& out, int32_t depth);
    bool ParseArray(JsonValue& out, int32_t depth);
    bool ParseString(std::string& out);
    bool ParseNumber(JsonValue& out);
    bool ParseLiteral(const char* text);
};

bool Parser::ParseLiteral(const char* text) {
    const size_t length = std::strlen(text);
    if (static_cast<size_t>(end - cursor) < length) {
        return Fail("字面量不完整");
    }
    if (std::memcmp(cursor, text, length) != 0) {
        return Fail("未知的字面量");
    }
    cursor += length;
    return true;
}

bool Parser::ParseString(std::string& out) {
    if (cursor >= end || *cursor != '"') {
        return Fail("字符串缺少起始引号");
    }
    ++cursor;
    out.clear();
    while (cursor < end) {
        const char c = *cursor++;
        if (c == '"') {
            return true;
        }
        if (c != '\\') {
            out.push_back(c);
            continue;
        }
        if (cursor >= end) {
            return Fail("转义序列不完整");
        }
        const char escape = *cursor++;
        switch (escape) {
            case '"':
                out.push_back('"');
                break;
            case '\\':
                out.push_back('\\');
                break;
            case '/':
                out.push_back('/');
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u': {
                /* \uXXXX 按 UTF-8 编码写出, 代理对合成一个码点. */
                if (end - cursor < 4) {
                    return Fail("\\u 转义长度不足");
                }
                char hex[5] = {cursor[0], cursor[1], cursor[2], cursor[3], '\0'};
                const unsigned code = static_cast<unsigned>(std::strtoul(hex, nullptr, 16));
                cursor += 4;
                unsigned codePoint = code;
                if (code >= 0xD800u && code <= 0xDBFFu && end - cursor >= 6 && cursor[0] == '\\' &&
                    cursor[1] == 'u') {
                    char lowHex[5] = {cursor[2], cursor[3], cursor[4], cursor[5], '\0'};
                    const unsigned low = static_cast<unsigned>(std::strtoul(lowHex, nullptr, 16));
                    if (low >= 0xDC00u && low <= 0xDFFFu) {
                        codePoint = 0x10000u + ((code - 0xD800u) << 10) + (low - 0xDC00u);
                        cursor += 6;
                    }
                }
                if (codePoint < 0x80u) {
                    out.push_back(static_cast<char>(codePoint));
                } else if (codePoint < 0x800u) {
                    out.push_back(static_cast<char>(0xC0u | (codePoint >> 6)));
                    out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                } else if (codePoint < 0x10000u) {
                    out.push_back(static_cast<char>(0xE0u | (codePoint >> 12)));
                    out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
                    out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                } else {
                    out.push_back(static_cast<char>(0xF0u | (codePoint >> 18)));
                    out.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3Fu)));
                    out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
                    out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
                }
                break;
            }
            default:
                return Fail("不支持的转义字符");
        }
    }
    return Fail("字符串缺少结束引号");
}

bool Parser::ParseNumber(JsonValue& out) {
    const char* start = cursor;
    if (cursor < end && (*cursor == '-' || *cursor == '+')) {
        ++cursor;
    }
    bool digits = false;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
        ++cursor;
        digits = true;
    }
    if (cursor < end && *cursor == '.') {
        ++cursor;
        while (cursor < end && *cursor >= '0' && *cursor <= '9') {
            ++cursor;
            digits = true;
        }
    }
    if (!digits) {
        return Fail("数字格式不正确");
    }
    if (cursor < end && (*cursor == 'e' || *cursor == 'E')) {
        ++cursor;
        if (cursor < end && (*cursor == '-' || *cursor == '+')) {
            ++cursor;
        }
        while (cursor < end && *cursor >= '0' && *cursor <= '9') {
            ++cursor;
        }
    }
    std::string text(start, static_cast<size_t>(cursor - start));
    out = JsonValue::MakeNumber(std::strtod(text.c_str(), nullptr));
    return true;
}

bool Parser::ParseArray(JsonValue& out, int32_t depth) {
    if (depth >= kMaxDepth) {
        return Fail("嵌套层数超过上限");
    }
    ++cursor; /* 跳过 '[' */
    out = JsonValue::MakeArray();
    SkipSpace();
    if (cursor < end && *cursor == ']') {
        ++cursor;
        return true;
    }
    while (cursor < end) {
        JsonValue element;
        if (!ParseValue(element, depth + 1)) {
            return false;
        }
        out.Push(std::move(element));
        SkipSpace();
        if (cursor < end && *cursor == ',') {
            ++cursor;
            SkipSpace();
            continue;
        }
        if (cursor < end && *cursor == ']') {
            ++cursor;
            return true;
        }
        return Fail("数组缺少逗号或结束方括号");
    }
    return Fail("数组未闭合");
}

bool Parser::ParseObject(JsonValue& out, int32_t depth) {
    if (depth >= kMaxDepth) {
        return Fail("嵌套层数超过上限");
    }
    ++cursor; /* 跳过 '{' */
    out = JsonValue::MakeObject();
    SkipSpace();
    if (cursor < end && *cursor == '}') {
        ++cursor;
        return true;
    }
    while (cursor < end) {
        std::string key;
        if (!ParseString(key)) {
            return false;
        }
        SkipSpace();
        if (cursor >= end || *cursor != ':') {
            return Fail("对象缺少冒号");
        }
        ++cursor;
        SkipSpace();
        JsonValue value;
        if (!ParseValue(value, depth + 1)) {
            return false;
        }
        out.Set(key, std::move(value));
        SkipSpace();
        if (cursor < end && *cursor == ',') {
            ++cursor;
            SkipSpace();
            continue;
        }
        if (cursor < end && *cursor == '}') {
            ++cursor;
            return true;
        }
        return Fail("对象缺少逗号或结束花括号");
    }
    return Fail("对象未闭合");
}

bool Parser::ParseValue(JsonValue& out, int32_t depth) {
    SkipSpace();
    if (cursor >= end) {
        return Fail("内容提前结束");
    }
    const char c = *cursor;
    if (c == '{') {
        return ParseObject(out, depth);
    }
    if (c == '[') {
        return ParseArray(out, depth);
    }
    if (c == '"') {
        std::string text;
        if (!ParseString(text)) {
            return false;
        }
        out = JsonValue::MakeString(text);
        return true;
    }
    if (c == 't') {
        if (!ParseLiteral("true")) {
            return false;
        }
        out = JsonValue::MakeBool(true);
        return true;
    }
    if (c == 'f') {
        if (!ParseLiteral("false")) {
            return false;
        }
        out = JsonValue::MakeBool(false);
        return true;
    }
    if (c == 'n') {
        if (!ParseLiteral("null")) {
            return false;
        }
        out = JsonValue::MakeNull();
        return true;
    }
    return ParseNumber(out);
}

/* 转义为可以安全放进 JSON 字符串的形式, 控制字符按 \u00XX 写出. */
void EscapeInto(std::string& out, const std::string& text) {
    char buffer[8];
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20u) {
                    std::snprintf(buffer, sizeof(buffer), "\\u%04X",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buffer;
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
}

/* 数字输出: 整数值不带小数点, 其余按最短可回读形式输出. */
void WriteNumber(std::string& out, double value) {
    char buffer[64];
    if (std::fabs(value) < 1.0e15 && std::floor(value) == value) {
        std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(value));
        out += buffer;
        return;
    }
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    out += buffer;
    /* 去掉尾部多余的零, 保留至少一位小数. */
    const size_t dot = out.rfind('.');
    if (dot != std::string::npos) {
        size_t last = out.size() - 1;
        while (last > dot + 1 && out[last] == '0') {
            out.erase(last);
            --last;
        }
    }
}

}  /* namespace */

JsonValue JsonValue::MakeNull() { return JsonValue{}; }

JsonValue JsonValue::MakeBool(bool value) {
    JsonValue result;
    result.type_ = JsonType::Bool;
    result.bool_ = value;
    return result;
}

JsonValue JsonValue::MakeNumber(double value) {
    JsonValue result;
    result.type_ = JsonType::Number;
    result.number_ = value;
    return result;
}

JsonValue JsonValue::MakeInt(int64_t value) {
    JsonValue result;
    result.type_ = JsonType::Number;
    result.number_ = static_cast<double>(value);
    return result;
}

JsonValue JsonValue::MakeString(const std::string& value) {
    JsonValue result;
    result.type_ = JsonType::String;
    result.string_ = value;
    return result;
}

JsonValue JsonValue::MakeArray() {
    JsonValue result;
    result.type_ = JsonType::Array;
    return result;
}

JsonValue JsonValue::MakeObject() {
    JsonValue result;
    result.type_ = JsonType::Object;
    return result;
}

bool JsonValue::AsBool(bool fallback) const { return type_ == JsonType::Bool ? bool_ : fallback; }

double JsonValue::AsNumber(double fallback) const {
    return type_ == JsonType::Number ? number_ : fallback;
}

int64_t JsonValue::AsInt(int64_t fallback) const {
    return type_ == JsonType::Number ? static_cast<int64_t>(std::llround(number_)) : fallback;
}

void JsonValue::Push(JsonValue value) {
    if (type_ != JsonType::Array) {
        type_ = JsonType::Array;
        array_.clear();
    }
    array_.push_back(std::move(value));
}

void JsonValue::Set(const std::string& key, JsonValue value) {
    if (type_ != JsonType::Object) {
        type_ = JsonType::Object;
        object_.clear();
    }
    for (auto& member : object_) {
        if (member.first == key) {
            member.second = std::move(value);
            return;
        }
    }
    object_.emplace_back(key, std::move(value));
}

const JsonValue* JsonValue::Find(const std::string& key) const {
    for (const auto& member : object_) {
        if (member.first == key) {
            return &member.second;
        }
    }
    return nullptr;
}

int64_t JsonValue::GetInt(const std::string& key, int64_t fallback) const {
    const JsonValue* value = Find(key);
    return value != nullptr ? value->AsInt(fallback) : fallback;
}

double JsonValue::GetNumber(const std::string& key, double fallback) const {
    const JsonValue* value = Find(key);
    return value != nullptr ? value->AsNumber(fallback) : fallback;
}

bool JsonValue::GetBool(const std::string& key, bool fallback) const {
    const JsonValue* value = Find(key);
    return value != nullptr ? value->AsBool(fallback) : fallback;
}

std::string JsonValue::GetString(const std::string& key, const std::string& fallback) const {
    const JsonValue* value = Find(key);
    return (value != nullptr && value->IsString()) ? value->AsString() : fallback;
}

std::string JsonValue::Serialize(bool pretty) const {
    std::string out;
    WriteValue(out, pretty, 0);
    if (pretty) {
        out.push_back('\n');
    }
    return out;
}

void JsonValue::WriteValue(std::string& out, bool pretty, int32_t depth) const {
    const std::string indent = pretty ? std::string(static_cast<size_t>(depth) * 2u, ' ') : "";
    const std::string innerIndent =
        pretty ? std::string(static_cast<size_t>(depth + 1) * 2u, ' ') : "";
    switch (type_) {
        case JsonType::Null:
            out += "null";
            break;
        case JsonType::Bool:
            out += bool_ ? "true" : "false";
            break;
        case JsonType::Number:
            WriteNumber(out, number_);
            break;
        case JsonType::String:
            out.push_back('"');
            EscapeInto(out, string_);
            out.push_back('"');
            break;
        case JsonType::Array: {
            if (array_.empty()) {
                out += "[]";
                break;
            }
            out.push_back('[');
            for (size_t index = 0; index < array_.size(); ++index) {
                if (index > 0) {
                    out.push_back(',');
                }
                if (pretty) {
                    out.push_back('\n');
                    out += innerIndent;
                }
                array_[index].WriteValue(out, pretty, depth + 1);
            }
            if (pretty) {
                out.push_back('\n');
                out += indent;
            }
            out.push_back(']');
            break;
        }
        case JsonType::Object: {
            if (object_.empty()) {
                out += "{}";
                break;
            }
            out.push_back('{');
            for (size_t index = 0; index < object_.size(); ++index) {
                if (index > 0) {
                    out.push_back(',');
                }
                if (pretty) {
                    out.push_back('\n');
                    out += innerIndent;
                }
                out.push_back('"');
                EscapeInto(out, object_[index].first);
                out += pretty ? "\": " : "\":";
                object_[index].second.WriteValue(out, pretty, depth + 1);
            }
            if (pretty) {
                out.push_back('\n');
                out += indent;
            }
            out.push_back('}');
            break;
        }
    }
}

bool JsonParse(const std::string& text, JsonValue& out, std::string& error) {
    Parser parser;
    parser.begin = text.c_str();
    parser.cursor = text.c_str();
    parser.end = text.c_str() + text.size();
    if (!parser.ParseValue(out, 0)) {
        error = parser.error;
        return false;
    }
    parser.SkipSpace();
    if (parser.cursor != parser.end) {
        error = "顶层值之后还有多余内容";
        return false;
    }
    return true;
}

bool JsonReadFile(const std::string& path, JsonValue& out, std::string& error) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        error = "无法打开文件: " + path;
        return false;
    }
    std::string text;
    char buffer[4096];
    while (true) {
        const size_t read = std::fread(buffer, 1, sizeof(buffer), file);
        if (read > 0) {
            text.append(buffer, read);
        }
        if (read < sizeof(buffer)) {
            break;
        }
    }
    std::fclose(file);
    std::string parseError;
    if (!JsonParse(text, out, parseError)) {
        error = path + ": " + parseError;
        return false;
    }
    return true;
}

bool JsonWriteFile(const std::string& path, const JsonValue& value, std::string& error) {
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        error = "无法写入文件: " + path;
        return false;
    }
    const std::string text = value.Serialize(true);
    const size_t written = std::fwrite(text.data(), 1, text.size(), file);
    std::fclose(file);
    if (written != text.size()) {
        error = "写入不完整: " + path;
        return false;
    }
    return true;
}

}  /* namespace immirror::editor */
