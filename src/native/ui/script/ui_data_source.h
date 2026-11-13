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
};

class UiDataSource {
public:
    virtual ~UiDataSource() = default;

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
