#pragma once

#include <cstdint>

/*
 * 由 scripts/gen_buff_data.py 生成, 请勿手改.
 * 客户端可消费的状态效果: 移动速度, 挖掘速度, 跳跃高度, 生命恢复.
 * kind 取值 1 移速 2 挖掘 3 跳跃 4 生命恢复;
 * value 的口径随 kind 变化: 速度是百分比增量, 跳跃是高度增量, 生命恢复是每秒点数.
 */
namespace immirror::player::gen {

struct BuffEntry {
    uint32_t id;
    uint8_t kind;
    uint8_t level;
    float duration; /* 持续秒数 */
    float value;    /* 效果数值, 口径见文件头 */
    const char* name;
    const char* sound; /* 施加时播放的音效路径, 空串表示没有 */
};

constexpr int32_t kBuffEntryCount = 44;

constexpr BuffEntry kBuffEntries[kBuffEntryCount == 0 ? 1 : kBuffEntryCount] = {
    {1001u, 1u, 1u, 3.0f, 20.00f, "示例移动速度", "sounds/buff_buff_speed_1.ogg"},
    {1002u, 1u, 2u, 3.0f, 50.00f, "示例移动速度", "sounds/buff_buff_speed_1.ogg"},
    {1003u, 1u, 3u, 10.0f, 200.00f, "示例移动速度", "sounds/buff_buff_speed_1.ogg"},
    {2001u, 1u, 1u, 3.0f, 20.00f, "示例挖掘速度", "sounds/buff_buff_speed_1.ogg"},
    {3001u, 4u, 1u, 45.0f, 2.50f, "缓慢生命恢复1级", "sounds/buff_buff_hp_1.ogg"},
    {3002u, 4u, 2u, 45.0f, 5.00f, "缓慢生命恢复2级", "sounds/buff_buff_hp_1.ogg"},
    {3003u, 4u, 3u, 45.0f, 10.00f, "缓慢生命恢复3级", "sounds/buff_buff_hp_1.ogg"},
    {4001u, 1u, 1u, 180.0f, 25.00f, "疾跑1级", "sounds/buff_buff_speed_1.ogg"},
    {4002u, 1u, 2u, 180.0f, 50.00f, "疾跑2级", "sounds/buff_buff_speed_1.ogg"},
    {4003u, 1u, 3u, 180.0f, 75.00f, "疾跑3级", "sounds/buff_buff_speed_1.ogg"},
    {4004u, 1u, 4u, 600.0f, 75.00f, "疾跑4级", "sounds/buff_buff_speed_1.ogg"},
    {5001u, 1u, 1u, 60.0f, 25.00f, "速泳1级", "sounds/buff_buff_speed_1.ogg"},
    {5002u, 1u, 2u, 60.0f, 50.00f, "速泳2级", "sounds/buff_buff_speed_1.ogg"},
    {5003u, 1u, 3u, 60.0f, 75.00f, "速泳3级", "sounds/buff_buff_speed_1.ogg"},
    {6001u, 3u, 1u, 60.0f, 4.00f, "跳高高1级", "sounds/buff_buff_speed_1.ogg"},
    {6002u, 3u, 2u, 60.0f, 10.00f, "跳高高2级", "sounds/buff_buff_speed_1.ogg"},
    {6003u, 3u, 3u, 60.0f, 17.00f, "跳高高3级", "sounds/buff_buff_speed_1.ogg"},
    {6004u, 3u, 4u, 10.0f, 24.00f, "跳高高4级", "sounds/buff_buff_speed_1.ogg"},
    {7001u, 2u, 1u, 45.0f, 25.00f, "疯狂挖掘1级", "sounds/buff_buff_speed_1.ogg"},
    {7002u, 2u, 2u, 180.0f, 50.00f, "疯狂挖掘2级", "sounds/buff_buff_speed_1.ogg"},
    {7003u, 2u, 3u, 180.0f, 75.00f, "疯狂挖掘3级", "sounds/buff_buff_speed_1.ogg"},
    {8001u, 1u, 1u, 5.0f, -25.00f, "迟缓1级", "sounds/buff_buff_speed_1.ogg"},
    {8002u, 1u, 2u, 5.0f, -50.00f, "迟缓2级", "sounds/buff_buff_speed_1.ogg"},
    {8003u, 1u, 3u, 5.0f, -75.00f, "迟缓3级", "sounds/buff_buff_speed_1.ogg"},
    {9001u, 2u, 1u, 60.0f, -25.00f, "挖掘疲劳1级", "sounds/buff_buff_attack_1.ogg"},
    {9002u, 2u, 2u, 60.0f, -50.00f, "挖掘疲劳2级", "sounds/buff_buff_attack_1.ogg"},
    {9003u, 2u, 3u, 60.0f, -75.00f, "挖掘疲劳3级", "sounds/buff_buff_attack_1.ogg"},
    {39001u, 1u, 1u, 60.0f, -60.00f, "翻江倒海！1级", "sounds/buff_buff_sick_1.ogg"},
    {45001u, 1u, 1u, 1.5f, -70.00f, "冰冻1级", "sounds/buff_buff_freeze.ogg"},
    {45002u, 1u, 1u, 3.0f, -70.00f, "冰冻2级", "sounds/buff_buff_freeze.ogg"},
    {45003u, 1u, 1u, 3.0f, -33.00f, "冰冻3级", "sounds/buff_buff_freeze.ogg"},
    {50001u, 4u, 1u, 5.0f, 6.00f, "快速生命恢复1级", "sounds/buff_buff_hp_1.ogg"},
    {50002u, 4u, 2u, 5.0f, 12.00f, "快速生命恢复2级", "sounds/buff_buff_hp_1.ogg"},
    {50003u, 4u, 3u, 5.0f, 16.00f, "快速生命恢复3级", "sounds/buff_buff_hp_1.ogg"},
    {50004u, 4u, 4u, 5.0f, 20.00f, "快速生命恢复4级", "sounds/buff_buff_hp_1.ogg"},
    {51001u, 4u, 1u, 30.0f, 10.00f, "快速生命恢复5级", "sounds/buff_buff_hp_1.ogg"},
    {51002u, 4u, 2u, 12.0f, 10.00f, "快速生命恢复2级", "sounds/buff_buff_hp_1.ogg"},
    {52001u, 4u, 1u, 11.0f, 10.00f, "变异红菇的奇妙力量1级", "sounds/buff_buff_hp_1.ogg"},
    {53001u, 2u, 1u, 60.0f, 25.00f, "变异蘑菇的奇妙力量1级", "sounds/buff_buff_speed_1.ogg"},
    {53002u, 2u, 2u, 60.0f, -25.00f, "变异蘑菇的奇妙力量2级", "sounds/buff_buff_attack_1.ogg"},
    {1000001u, 1u, 1u, 180.0f, 100.00f, "加速100%", "sounds/buff_buff_speed_1.ogg"},
    {1000002u, 1u, 1u, 600.0f, 1000.00f, "加速1000%", "sounds/buff_buff_speed_1.ogg"},
    {1000003u, 1u, 3u, 600.0f, -75.00f, "减速", "sounds/buff_buff_speed_1.ogg"},
    {1009006u, 1u, 1u, 3.0f, -25.00f, "减速", "sounds/buff_buff_speed_1.ogg"},
};

/* 按编号取状态条目, 找不到返回空指针. */
inline const BuffEntry* BuffOf(uint32_t id) {
    for (int32_t index = 0; index < kBuffEntryCount; ++index) {
        if (kBuffEntries[index].id == id) {
            return &kBuffEntries[index];
        }
    }
    return nullptr;
}

}  /* namespace immirror::player::gen */
