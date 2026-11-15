/* 协议 message 序列化往返测试. */
#include <cstdio>
#include <string>
#include <vector>

#include "net/generated/proto_codec_gen.h"

using namespace immirror::net;
using namespace immirror::net::gen;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    /* 一 单字段消息 */
    {
        PB_HeartBeatCH value;
        value.BeatCode = 1234567890123ull;
        value.has_BeatCode = true;
        ProtoWriter writer;
        Serialize(value, writer);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        PB_HeartBeatCH restored;
        ProtoReader reader(bytes.data(), bytes.size());
        check("心跳反序列化成功", Deserialize(reader, &restored));
        check("心跳字段有值", restored.has_BeatCode);
        check("心跳值一致", restored.BeatCode == 1234567890123ull);
    }

    /* 二 字符串字段 */
    {
        PB_SyncChunkDataCH value;
        value.trunkmd5 = "abcdef0123456789";
        value.has_trunkmd5 = true;
        ProtoWriter writer;
        Serialize(value, writer);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        PB_SyncChunkDataCH restored;
        ProtoReader reader(bytes.data(), bytes.size());
        check("区块同步反序列化成功", Deserialize(reader, &restored));
        check("md5 有值", restored.has_trunkmd5);
        check("md5 一致", restored.trunkmd5 == "abcdef0123456789");
    }

    /* 三 多字段消息 */
    {
        PB_RoleEnterWorldCH value;
        value.Uin = 346502813;
        value.has_Uin = true;
        value.GeniusLv = 12;
        value.has_GeniusLv = true;
        value.UICtrlMode = 2;
        value.has_UICtrlMode = true;
        value.lang = 1;
        value.has_lang = true;
        value.apiid = 7;
        value.has_apiid = true;
        ProtoWriter writer;
        Serialize(value, writer);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        PB_RoleEnterWorldCH restored;
        ProtoReader reader(bytes.data(), bytes.size());
        check("进入世界反序列化成功", Deserialize(reader, &restored));
        check("Uin 一致", restored.Uin == 346502813);
        check("GeniusLv 一致", restored.GeniusLv == 12);
        check("UICtrlMode 一致", restored.UICtrlMode == 2);
        check("lang 一致", restored.lang == 1);
        check("apiid 一致", restored.apiid == 7);
    }

    /* 四 嵌套消息 */
    {
        PB_SyncChunkDataCH value;
        value.SectionCoord.X = 15;
        value.SectionCoord.has_X = true;
        value.SectionCoord.Y = -25;
        value.SectionCoord.has_Y = true;
        value.SectionCoord.Z = 325;
        value.SectionCoord.has_Z = true;
        value.has_SectionCoord = true;
        ProtoWriter writer;
        Serialize(value, writer);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        PB_SyncChunkDataCH restored;
        ProtoReader reader(bytes.data(), bytes.size());
        check("嵌套消息反序列化成功", Deserialize(reader, &restored));
        check("嵌套有值", restored.has_SectionCoord);
        check("嵌套 X 一致", restored.SectionCoord.X == 15);
        check("嵌套 Y 一致", restored.SectionCoord.Y == -25);
        check("嵌套 Z 一致", restored.SectionCoord.Z == 325);
    }

    /* 五 空消息 */
    {
        PB_HeartBeatCH value;
        ProtoWriter writer;
        Serialize(value, writer);
        const std::vector<uint8_t>& bytes = writer.Bytes();
        check("空消息序列化为零字节", bytes.empty());
        PB_HeartBeatCH restored;
        ProtoReader reader(bytes.data(), bytes.size());
        check("空消息反序列化成功", Deserialize(reader, &restored));
        check("空消息无字段", !restored.has_BeatCode);
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
