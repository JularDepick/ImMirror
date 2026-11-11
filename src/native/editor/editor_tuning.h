#pragma once

#include <cstdint>

namespace immirror::editor {

/*
 * 编辑器可调参数. 手感与容量类的数值只在这里定义, 会话, 命令协议与界面
 * 都从这里取值, 不存在第二份系数; 调整编辑器行为只改本文件.
 */
namespace tuning {

/* 撤销栈深度. 里程碑要求至少 20 步, 这里留足余量; 每步是一个手势的方块变更集合. */
constexpr int32_t kUndoDepth = 64;

/* 单个手指手势内允许记录的方块变更上限, 超出后该手势不再并入撤销, 由会话给出提示. */
constexpr int32_t kMaxChangesPerTransaction = 65536;

/* 单次框选填充的方块上限, 与选区体积比较, 防止误触大范围填充拖死主线程. */
constexpr int64_t kMaxFillVolume = 262144;

/* 一个工程允许的区块数量上限. 16x256x16 的区块约 128 KiB, 4096 块约 512 MiB, 只是兜底. */
constexpr int32_t kMaxChunks = 4096;

/* 单个工程的触发器数量上限与单条触发器的动作数量上限. */
constexpr int32_t kMaxTriggers = 256;
constexpr int32_t kMaxActionsPerTrigger = 16;

/* 触发器定时间隔的合法范围, 单位秒. */
constexpr double kMinTimerSeconds = 0.1;
constexpr double kMaxTimerSeconds = 3600.0;

/* 世界坐标允许的编辑范围, 与体素世界高度一致, 水平方向按区块数量的整数倍放宽. */
constexpr int32_t kMinWorldY = 0;
constexpr int32_t kMaxWorldY = 255;
constexpr int32_t kMaxWorldXZ = 32768;

/* 编辑器日志保留的行数, 超出后丢弃最旧的. */
constexpr int32_t kLogLineLimit = 64;

/* 命令协议单条命令的最大字节数, 超出直接拒绝, 避免异常调用撑爆解析. */
constexpr int32_t kMaxCommandBytes = 4096;

/* 单个工程文件的读取上限: 区块 CSV 与触发器 JSON 分别限制. */
constexpr int32_t kMaxChunkFileBytes = 4 * 1024 * 1024;
constexpr int32_t kMaxJsonFileBytes = 1024 * 1024;

/* 工程 id 与显示名的长度上限, id 只允许字母数字与下划线, 因为要落成目录名. */
constexpr int32_t kMaxProjectIdBytes = 48;
constexpr int32_t kMaxProjectNameBytes = 64;

/*
 * 触发器执行时的沙盒限制. 触发器脚本只做几次坐标写入与日志,
 * 因此预算远小于通用脚本的默认值, 保证一次事件分发不会卡住主线程.
 */
constexpr uint64_t kTriggerInstructionBudget = 200000ull;
constexpr uint64_t kTriggerMemoryQuotaBytes = 256ull * 1024ull;
constexpr uint32_t kTriggerTimeoutMillis = 50;
constexpr uint32_t kMaxTimerTriggersPerTick = 8;

/* 界面方块栏展示的方块数量, 与 game::Inventory 的九格对齐, 超出部分滚动查看. */
constexpr int32_t kPalettePageSize = 15;

}  /* namespace tuning */

}  /* namespace immirror::editor */
