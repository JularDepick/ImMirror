#pragma once

#include <cstdint>
#include <string>

namespace immirror::ui::script {

/*
 * 界面数据源: 界面脚本读取游戏状态的唯一入口.
 *
 * 求值器不认识游戏对象, 控件宿主也不持有游戏状态; 两者通过这个接口对接,
 * 因此界面可以离线用假数据跑通, 接入游戏时只换实现.
 */
/* 物品定义视图: 界面脚本读到的字段名与物品表列名一致. */
struct ItemDefView {
    bool found = false;
    int32_t id = 0;
    /* 中文名与英文键: 点阵字只有 ASCII, 界面实际显示用英文键. */
    std::string displayName;
    std::string englishName;
    std::string icon;
    /* 类别编号与单格堆叠上限. */
    int32_t category = 0;
    int32_t maxStack = 0;
    /* 界面字段: 原样带出, 由界面脚本按需读取. */
    std::string description;
    int32_t involvedId = 0;
    int32_t unlockFlag = 0;
    int32_t unlockType = 0;
    int32_t createType = 0;
    int32_t model = 0;
    int32_t replaceId = 0;
    int32_t enchantTag = 0;
    int32_t stuffType = 0;
    int32_t quality = 0;
};

/* 方块定义视图: 界面脚本按方块编号读取的字段. */
struct BlockDefView {
    bool found = false;
    int32_t id = 0;
    /* 注册表英文键; 方块表没有独立中文名列, 界面显示同用此键. */
    std::string englishName;
    /* 采集与形态: 硬度, 工具族, 材质族, 所需等级, 形状, 重力方式. */
    float hardness = 0.0f;
    int32_t tool = 0;
    int32_t material = 0;
    int32_t toolLevel = 0;
    int32_t shape = 0;
    int32_t gravity = 0;
    int32_t replaceable = 0;
    /* 掉落概率万分比与爆炸抗性. */
    int32_t dropChance = 0;
    int32_t blastResist = 0;
    /* 摩擦系数, 大于 1 表示打滑. */
    float slipperiness = 1.0f;
};

class UiDataSource {
public:
    virtual ~UiDataSource() = default;

    /* 方块定义: 供界面脚本按方块编号查询形态与采集要求. */
    virtual BlockDefView BlockDef(int32_t id) const = 0;

    /* 方块对应的物品定义: 界面拿方块编号直接要物品信息. */
    virtual ItemDefView BlockItemDef(int32_t blockId) const = 0;

    /* 物品定义: 供界面脚本按编号查询名称与图标. */
    virtual ItemDefView ItemDef(int32_t id) const = 0;

    /* 背包: 槽位数量与逐格内容. 空格返回空串与零数量. */
    virtual int32_t BagSlotCount() const = 0;
    virtual std::string BagSlotName(int32_t slot) const = 0;
    virtual int32_t BagSlotCountAt(int32_t slot) const = 0;
    virtual int32_t BagSlotDurability(int32_t slot) const = 0;

    /* 玩家状态, 均为 0 到 1 的比例. */
    virtual float Health() const = 0;
    virtual float Hunger() const = 0;
    virtual float Stamina() const = 0;

    /* 世界信息. */
    virtual std::string WorldName() const = 0;
    virtual float WorldTimeSeconds() const = 0;

    /* 进度统计: 按统计键取累计值, 未收录返回 0. */
    virtual int64_t ProgressValue(const std::string& key) const = 0;
};

}  /* namespace immirror::ui::script */
