#pragma once

#include <cstdint>

/*
 * 由 scripts/gen_achievement_data.py 从 assets 仓库的 autogen/achievement.csv 生成, 请勿手改.
 * 每日任务模板: 每条模板是一种成就条件 (事件类型 + 达成量 + 镜币档位),
 * 每天按日期键确定性地抽取 3 条, 抽取算法见 daily_task_board.cpp.
 * 服务端的 catalog.ts 使用同一份模板表与同一套抽取算法, 两端结果必须一致.
 */
namespace immirror::progress::gen {

/* 单条每日任务模板. */
struct DailyTaskTemplate {
    uint32_t id;        /* 模板 id, 服务端同值 */
    uint8_t eventKind;  /* 计数所用的事件类型 */
    int32_t subject;    /* 对象筛选, 0 表示不限 */
    int32_t target;     /* 当日所需的事件累计量 */
    int32_t coinReward; /* 完成时下发的镜币 */
    const char* name;   /* 任务名, UTF-8 编码 */
};

constexpr int32_t kDailyTaskTemplateCount = 11;

constexpr DailyTaskTemplate kDailyTaskTemplateData[kDailyTaskTemplateCount] = {
    {1u, 1, 0, 20, 40, "开山取石"},
    {2u, 1, 0, 60, 80, "挖掘达人"},
    {3u, 2, 0, 10, 40, "开工建造"},
    {4u, 2, 0, 30, 80, "施工队长"},
    {5u, 4, 0, 10, 40, "采集新手"},
    {6u, 4, 0, 30, 80, "采集行家"},
    {7u, 3, 0, 100, 30, "四处走走"},
    {8u, 3, 0, 300, 60, "长途跋涉"},
    {9u, 5, 0, 900, 40, "上线报到"},
    {10u, 5, 0, 1800, 80, "冒险常客"},
    {11u, 6, 0, 1, 30, "每日签到"},
};

}  // namespace immirror::progress::gen
