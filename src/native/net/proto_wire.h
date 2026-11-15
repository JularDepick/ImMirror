/*
 * Protobuf wire format 编解码: 只实现 proto2 用到的五种 wire type.
 * 不依赖生成代码, 由调用方按字段号与类型读写, 便于静态验证.
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace immirror::net {

/* wire type: 与 protobuf 规范一致. */
enum class WireType : uint8_t {
    kVarint = 0,
    kFixed64 = 1,
    kLengthDelimited = 2,
    kFixed32 = 5,
};

/* 写入器: 追加到一段字节缓冲. */
class ProtoWriter {
public:
    void WriteVarint(uint64_t value) {
        while (value >= 0x80u) {
            buffer_.push_back(static_cast<uint8_t>((value & 0x7Fu) | 0x80u));
            value >>= 7;
        }
        buffer_.push_back(static_cast<uint8_t>(value));
    }

    void WriteTag(uint32_t field, WireType type) {
        WriteVarint((static_cast<uint64_t>(field) << 3) | static_cast<uint64_t>(type));
    }

    void WriteInt32(uint32_t field, int32_t value) {
        WriteTag(field, WireType::kVarint);
        /* 负数按 64 位补码写, 与 protobuf 一致. */
        WriteVarint(static_cast<uint64_t>(static_cast<int64_t>(value)));
    }

    void WriteUInt32(uint32_t field, uint32_t value) {
        WriteTag(field, WireType::kVarint);
        WriteVarint(value);
    }

    void WriteUInt64(uint32_t field, uint64_t value) {
        WriteTag(field, WireType::kVarint);
        WriteVarint(value);
    }

    void WriteInt64(uint32_t field, int64_t value) {
        WriteTag(field, WireType::kVarint);
        WriteVarint(static_cast<uint64_t>(value));
    }

    void WriteDouble(uint32_t field, double value) {
        WriteTag(field, WireType::kFixed64);
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (int32_t i = 0; i < 8; ++i) {
            buffer_.push_back(static_cast<uint8_t>((bits >> (8 * i)) & 0xFFu));
        }
    }

    void WriteBool(uint32_t field, bool value) {
        WriteTag(field, WireType::kVarint);
        WriteVarint(value ? 1u : 0u);
    }

    void WriteFloat(uint32_t field, float value) {
        WriteTag(field, WireType::kFixed32);
        uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (int32_t i = 0; i < 4; ++i) {
            buffer_.push_back(static_cast<uint8_t>((bits >> (8 * i)) & 0xFFu));
        }
    }

    void WriteString(uint32_t field, const std::string& value) {
        WriteTag(field, WireType::kLengthDelimited);
        WriteVarint(value.size());
        buffer_.insert(buffer_.end(), value.begin(), value.end());
    }

    void WriteMessage(uint32_t field, const std::vector<uint8_t>& nested) {
        WriteTag(field, WireType::kLengthDelimited);
        WriteVarint(nested.size());
        buffer_.insert(buffer_.end(), nested.begin(), nested.end());
    }

    const std::vector<uint8_t>& Bytes() const { return buffer_; }

private:
    std::vector<uint8_t> buffer_;
};

/* 读取器: 顺序解析字段. */
class ProtoReader {
public:
    ProtoReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    bool Done() const { return pos_ >= size_; }

    bool ReadTag(uint32_t* field, WireType* type) {
        uint64_t tag = 0;
        if (!ReadVarint(&tag)) {
            return false;
        }
        *field = static_cast<uint32_t>(tag >> 3);
        *type = static_cast<WireType>(tag & 0x7u);
        return true;
    }

    bool ReadVarint(uint64_t* value) {
        uint64_t result = 0;
        int32_t shift = 0;
        while (pos_ < size_ && shift < 64) {
            const uint8_t byte = data_[pos_++];
            result |= static_cast<uint64_t>(byte & 0x7Fu) << shift;
            if ((byte & 0x80u) == 0) {
                *value = result;
                return true;
            }
            shift += 7;
        }
        return false;
    }

    bool ReadInt32(int32_t* value) {
        uint64_t raw = 0;
        if (!ReadVarint(&raw)) {
            return false;
        }
        *value = static_cast<int32_t>(static_cast<int64_t>(raw));
        return true;
    }

    bool ReadUInt64(uint64_t* value) { return ReadVarint(value); }

    bool ReadInt64(int64_t* value) {
        uint64_t raw = 0;
        if (!ReadVarint(&raw)) {
            return false;
        }
        *value = static_cast<int64_t>(raw);
        return true;
    }

    bool ReadUInt32(uint32_t* value) {
        uint64_t raw = 0;
        if (!ReadVarint(&raw)) {
            return false;
        }
        *value = static_cast<uint32_t>(raw);
        return true;
    }

    bool ReadDouble(double* value) {
        if (pos_ + 8 > size_) {
            return false;
        }
        uint64_t bits = 0;
        for (int32_t i = 0; i < 8; ++i) {
            bits |= static_cast<uint64_t>(data_[pos_ + i]) << (8 * i);
        }
        pos_ += 8;
        std::memcpy(value, &bits, sizeof(bits));
        return true;
    }

    bool ReadBool(bool* value) {
        uint64_t raw = 0;
        if (!ReadVarint(&raw)) {
            return false;
        }
        *value = raw != 0;
        return true;
    }

    bool ReadFloat(float* value) {
        if (pos_ + 4 > size_) {
            return false;
        }
        uint32_t bits = 0;
        for (int32_t i = 0; i < 4; ++i) {
            bits |= static_cast<uint32_t>(data_[pos_ + i]) << (8 * i);
        }
        pos_ += 4;
        std::memcpy(value, &bits, sizeof(bits));
        return true;
    }

    bool ReadString(std::string* value) {
        uint64_t length = 0;
        if (!ReadVarint(&length) || pos_ + length > size_) {
            return false;
        }
        value->assign(reinterpret_cast<const char*>(data_ + pos_), static_cast<size_t>(length));
        pos_ += static_cast<size_t>(length);
        return true;
    }

    bool ReadMessage(std::vector<uint8_t>* nested) {
        uint64_t length = 0;
        if (!ReadVarint(&length) || pos_ + length > size_) {
            return false;
        }
        nested->assign(data_ + pos_, data_ + pos_ + length);
        pos_ += static_cast<size_t>(length);
        return true;
    }

    /* 跳过未知字段: 按 wire type 推进. */
    bool Skip(WireType type) {
        switch (type) {
            case WireType::kVarint: {
                uint64_t ignored = 0;
                return ReadVarint(&ignored);
            }
            case WireType::kFixed64:
                if (pos_ + 8 > size_) {
                    return false;
                }
                pos_ += 8;
                return true;
            case WireType::kLengthDelimited: {
                uint64_t length = 0;
                if (!ReadVarint(&length) || pos_ + length > size_) {
                    return false;
                }
                pos_ += static_cast<size_t>(length);
                return true;
            }
            case WireType::kFixed32:
                if (pos_ + 4 > size_) {
                    return false;
                }
                pos_ += 4;
                return true;
        }
        return false;
    }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t pos_ = 0;
};

}  /* namespace immirror::net */
