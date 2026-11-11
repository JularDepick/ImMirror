/* 由 scripts/gen_editor_data.py 从 assets 仓库的 csvdef 生成, 请勿手改. */

#include "editor/generated/editor_data_gen.h"

#include <cstring>

namespace immirror::editor::gen {

const BlockEntry kBlockEntries[] = {
    {0, 0, "air", "空气", -1.0f, 0, 0, 0},
    {1, 100, "grass", "草块", 0.0f, 3, 3, 3},
    {2, 101, "dirt", "土块", 0.0f, 3, 3, 3},
    {3, 104, "stone", "石头", 8.0f, 2, 3, 3},
    {4, 106, "sand", "沙子", 3.0f, 3, 3, 3},
    {5, 4, "water", "水", -1.0f, 0, 2, 2},
    {6, 200, "wood", "果木", 6.0f, 1, 1, 1},
    {7, 218, "leaves", "果木树叶", 0.0f, 0, 1, 1},
    {8, 1, "bedrock", "基石", -1.0f, 0, 3, 3},
    {9, 547, "brick", "红砖块", 12.0f, 2, 4, 4},
    {10, 505, "cobblestone", "石块", 12.0f, 2, 4, 4},
    {11, 115, "snow", "雪", 0.0f, 3, 4, 4},
    {12, 123, "ice", "冰", 5.0f, 2, 3, 3},
    {13, 107, "gravel", "沙砾", 4.0f, 3, 3, 3},
    {14, 206, "planks", "果木板", 6.0f, 1, 4, 4},
};

const int32_t kBlockEntryCount = static_cast<int32_t>(sizeof(kBlockEntries) / sizeof(kBlockEntries[0]));

const TriggerItemEntry kTriggerItems[] = {
    {1030021, "EVENT_ENTER_REGION", "ENTER REGION", "进入区域", 1, 3, 1001, 0},
    {1050002, "EVENT_BLOCK_BROKEN", "BLOCK BROKEN", "被破坏", 1, 5, 0, 0},
    {1020009, "EVENT_TIMER", "TIMER", "计时器变化", 1, 2, 0, 0},
    {3040001, "ACTION_SET_BLOCK", "SET BLOCK", "创建方块", 3, 4, 1002, 1025},
    {3050011, "ACTION_TELEPORT", "TELEPORT", "改变位置", 3, 5, 1018, 1002},
    {3900001, "ACTION_LOG", "LOG", "输出日志", 3, 90, 0, 0},
};

const int32_t kTriggerItemCount = static_cast<int32_t>(sizeof(kTriggerItems) / sizeof(kTriggerItems[0]));

const TriggerParamEntry kTriggerParams[] = {
    {1000, "PARAM_1000", "天气"},
    {1001, "REGION", "区域"},
    {1002, "POSITION", "位置"},
    {1003, "PARAM_1003", "朝向"},
    {1004, "PARAM_1004", "视角"},
    {1005, "NUMBER", "数值"},
    {1006, "COORD", "坐标分量"},
    {1007, "PARAM_1007", "角度单位"},
    {1008, "PARAM_1008", "固定底对数"},
    {1009, "PARAM_1009", "取整方式"},
    {1010, "PARAM_1010", "字符串"},
    {1011, "PARAM_1011", "伤害类型"},
    {1012, "PARAM_1012", "是否"},
    {1013, "PARAM_1013", "开关"},
    {1014, "PARAM_1014", "布尔值"},
    {1015, "PARAM_1015", "最大最小值"},
    {1016, "PARAM_1016", "比较计算符"},
    {1017, "PARAM_1017", "数学运算符"},
    {1018, "PLAYER", "玩家"},
    {1019, "PARAM_1019", "玩家组"},
    {1020, "PARAM_1020", "玩家属性"},
    {1021, "PARAM_1021", "运动状态"},
    {1022, "PARAM_1022", "玩家设置"},
    {1023, "PARAM_1023", "队伍"},
    {1024, "PARAM_1024", "队伍属性"},
    {1025, "BLOCK_TYPE", "方块类型"},
    {1026, "PARAM_1026", "方块设置"},
    {1027, "PARAM_1027", "道具"},
    {1028, "PARAM_1028", "道具类型"},
    {1029, "PARAM_1029", "投掷物类型"},
    {1030, "PARAM_1030", "掉落物"},
    {1031, "PARAM_1031", "生物"},
    {1032, "PARAM_1032", "生物类型"},
    {1033, "PARAM_1033", "生物组"},
    {1034, "PARAM_1034", "生物行动状态"},
    {1035, "PARAM_1035", "生物属性"},
    {1036, "PARAM_1036", "生物设置"},
    {1037, "PARAM_1037", "状态"},
    {1038, "PARAM_1038", "触发器"},
    {1039, "PARAM_1039", "触发器组"},
    {1040, "PARAM_1040", "计时器"},
    {1041, "PARAM_1041", "动画表情"},
    {1042, "PARAM_1042", "私有变量"},
    {1043, "PARAM_1043", "三角函数"},
    {1044, "PARAM_1044", "投掷物"},
    {1045, "PARAM_1045", "特效类型"},
    {1046, "PARAM_1046", "音效"},
    {1047, "PARAM_1047", "方向"},
    {1048, "PARAM_1048", "对象"},
    {1049, "PARAM_1049", "音调"},
    {1050, "PARAM_1050", "脚本"},
    {1051, "PARAM_1051", "按键"},
    {1052, "PARAM_1052", "快捷栏按键"},
    {1053, "PARAM_1053", "任意值"},
    {1054, "PARAM_1054", "装备栏"},
    {1055, "PARAM_1055", "装备类型"},
    {1056, "PARAM_1056", "颜色"},
    {1057, "PARAM_1057", "图文信息"},
    {1058, "PARAM_1058", "图文信息类型"},
    {1059, "PARAM_1059", "商品"},
    {1060, "PARAM_1060", "外观类型"},
};

const int32_t kTriggerParamCount = static_cast<int32_t>(sizeof(kTriggerParams) / sizeof(kTriggerParams[0]));

const BlockEntry& BlockEntryOf(uint16_t clientId) {
    for (int32_t index = 0; index < kBlockEntryCount; ++index) {
        if (kBlockEntries[index].clientId == clientId) {
            return kBlockEntries[index];
        }
    }
    return kBlockEntries[0];
}

const TriggerItemEntry* TriggerItemOfKey(const char* key) {
    if (key == nullptr) {
        return nullptr;
    }
    for (int32_t index = 0; index < kTriggerItemCount; ++index) {
        if (std::strcmp(kTriggerItems[index].key, key) == 0) {
            return &kTriggerItems[index];
        }
    }
    return nullptr;
}

const TriggerItemEntry* TriggerItemOfId(int32_t itemId) {
    for (int32_t index = 0; index < kTriggerItemCount; ++index) {
        if (kTriggerItems[index].itemId == itemId) {
            return &kTriggerItems[index];
        }
    }
    return nullptr;
}

}  /* namespace immirror::editor::gen */
