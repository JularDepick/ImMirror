#pragma once

namespace immirror::player {

/*
 * 破坏反馈参数. 挖掘所需时长只由方块硬度与本文件的比例系数换算:
 * 秒数 = max(硬度 * kSecondsPerHardness, kMinMineSeconds).
 * 手感调整只改这里, 控制器与界面都从这里取值, 不存在第二份系数.
 */
namespace mining {

/* 每一点硬度对应的挖掘秒数. 草 3.5 约 1.05 秒, 石 8 约 2.4 秒. */
constexpr float kSecondsPerHardness = 0.3f;

/* 最短挖掘时长, 低硬度方块也不会瞬破, 保证进度反馈与音效可见. */
constexpr float kMinMineSeconds = 0.1f;

/* 挖掘音重复间隔, 进度推进期间按这个节拍播放. */
constexpr float kDigSoundIntervalSeconds = 0.22f;

/*
 * 轻点与长按的分界秒数. 按下即开始累计进度, 但按住不足该时长就抬手时
 * 判定为轻点, 结算为放置; 超过该时长才显示进度环, 轻点时不会闪一圈进度.
 */
constexpr float kTapSeconds = 0.22f;

}  /* namespace mining */

}  /* namespace player */
