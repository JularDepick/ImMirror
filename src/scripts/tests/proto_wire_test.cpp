/* Protobuf wire format 编解码往返测试. */
#include <cstdio>
#include <string>
#include <vector>

#include "net/proto_wire.h"

using namespace immirror::net;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    /* 一 varint 往返: 正数 */
    {
        ProtoWriter writer;
        writer.WriteInt32(1, 123456);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        check("读出标签", reader.ReadTag(&field, &type));
        check("字段号为 1", field == 1);
        check("类型为 varint", type == WireType::kVarint);
        int32_t value = 0;
        check("读出整数", reader.ReadInt32(&value));
        check("整数值一致", value == 123456);
        check("读完后结束", reader.Done());
    }

    /* 二 varint 往返: 负数 */
    {
        ProtoWriter writer;
        writer.WriteInt32(2, -7);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        int32_t value = 0;
        check("读出负整数", reader.ReadInt32(&value));
        check("负整数值一致", value == -7);
    }

    /* 三 大 uint64 */
    {
        ProtoWriter writer;
        writer.WriteUInt64(3, 0xFFFFFFFFFFFFFFFFull);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        uint64_t value = 0;
        check("读出大整数", reader.ReadUInt64(&value));
        check("大整数值一致", value == 0xFFFFFFFFFFFFFFFFull);
    }

    /* 四 string */
    {
        ProtoWriter writer;
        writer.WriteString(4, "hello proto");
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        check("字符串类型为 length-delimited", type == WireType::kLengthDelimited);
        std::string value;
        check("读出字符串", reader.ReadString(&value));
        check("字符串内容一致", value == "hello proto");
    }

    /* 五 float */
    {
        ProtoWriter writer;
        writer.WriteFloat(5, 3.5f);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        check("浮点类型为 fixed32", type == WireType::kFixed32);
        float value = 0.0f;
        check("读出浮点", reader.ReadFloat(&value));
        check("浮点值一致", value == 3.5f);
    }

    /* 六 嵌套 message */
    {
        ProtoWriter inner;
        inner.WriteInt32(1, 42);
        ProtoWriter outer;
        outer.WriteMessage(6, inner.Bytes());
        const std::vector<uint8_t>& bytes = outer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        std::vector<uint8_t> nested;
        check("读出嵌套", reader.ReadMessage(&nested));
        ProtoReader innerReader(nested.data(), nested.size());
        uint32_t innerField = 0;
        innerReader.ReadTag(&innerField, &type);
        int32_t value = 0;
        innerReader.ReadInt32(&value);
        check("嵌套字段值一致", value == 42);
    }

    /* 七 跳过未知字段 */
    {
        ProtoWriter writer;
        writer.WriteInt32(1, 5);
        writer.WriteString(2, "skip me");
        writer.WriteInt32(3, 9);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        int32_t first = 0;
        reader.ReadTag(&field, &type);
        reader.ReadInt32(&first);
        check("第一个字段为 5", first == 5);
        reader.ReadTag(&field, &type);
        check("跳过字符串字段", reader.Skip(type));
        reader.ReadTag(&field, &type);
        int32_t third = 0;
        reader.ReadInt32(&third);
        check("跳过后的字段为 9", third == 9);
        check("读完后结束", reader.Done());
    }

    /* 八 bool */
    {
        ProtoWriter writer;
        writer.WriteBool(7, true);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        ProtoReader reader(bytes.data(), bytes.size());
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        reader.ReadTag(&field, &type);
        bool value = false;
        check("读出布尔", reader.ReadBool(&value));
        check("布尔值一致", value);
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
