#pragma once

#include <cstdint>

#include "voxel/voxel_types.h"

namespace immirror::editor {

/*
 * 编辑器侧全部可调参数的唯一来源.
 * 撤销深度, 填充上限与触发器配额只在这里定义一处,
 * 属性面板的说明文本与 scripts/validate_project.py 的校验上限读取同一份语义.
 */
namespace limits {

/* 撤销栈深度. 需求要求至少 20 步, 默认留到 32 步. */
constexpr int32_t kUndoDepth = 32;

/* 新建工程时的默认地形种子与视距档位. */
constexpr uint32_t kDefaultSeed = 20260101u;
constexpr int32_t kDefaultViewDistance = 4;

/* 拾取射线的最长距离, 与游戏内 player::PlayerController 的取值保持一致. */
constexpr float kRayDistance = 6.0f;

/* 拾取射线的最大步进次数, 与游戏内控制器一致, 防御异常方向. */
constexpr int32_t kRaySteps = 96;

/* 单次框选填充允许的最大方块数, 超出时拒绝执行而不是算到卡死主线程. */
constexpr int32_t kMaxFillBlocks = 32768;

/* 单个工程允许的触发器数量与单个触发器的动作数量上限. */
constexpr int32_t kMaxTriggers = 32;
constexpr int32_t kMaxActions = 8;

/* 触发器每帧最多触发次数与单次执行的预算, 脚本写错时不会拖死主循环. */
constexpr int32_t kTriggerFiresPerFrame = 8;
constexpr uint64_t kTriggerInstructionBudget = 200000ull;
constexpr uint32_t kTriggerTimeoutMillis = 40;

/* 每帧最多回放的工程方块变更数, 控制工程数据投影到世界的单帧开销. */
constexpr int32_t kProjectionBudgetPerFrame = 4096;

/* 工程区的上限: 单个工程最多保存的区块文件数与单个区块的方块数. */
constexpr int32_t kMaxChunkFiles = 4096;
constexpr int32_t kChunkVolume = voxel::kChunkVolume;

/* 界面一次显示的状态行数与属性面板行数上限. */
constexpr int32_t kStatusLines = 7;
constexpr int32_t kPanelRows = 12;

}  /* namespace limits */

/* 编辑器运行模式. 编辑模式下改方块, 试玩模式下走游戏玩法并驱动触发器. */
enum class EditorMode {
    kEdit = 0,
    kPlaytest = 1,
};

/*
 * 轴对齐区域, 以方块坐标为单位的中心与尺寸.
 * 区域覆盖 [center - size/2, center + size/2] 的闭区间,
 * 与"进入区域"事件的语义一致 (中心点加长宽高).
 */
struct Region {
    int32_t cx = 0;
    int32_t cy = 0;
    int32_t cz = 0;
    int32_t sx = 1;
    int32_t sy = 1;
    int32_t sz = 1;

    /* 判断世界坐标点是否落在区域内, 进入区域事件据此判定. */
    bool Contains(float x, float y, float z) const;

    /* 把尺寸收敛到合法范围, 至少 1 格. */
    void Normalize();
};

/*
 * 方块调色板: 编辑器可放置方块的唯一列表, 顺序取自生成的参数表.
 * 序号 0 对应调色板首项 (草块), 与客户端方块 id 不是同一套编号.
 */
int32_t PaletteCount();
voxel::BlockId PaletteBlock(int32_t index);
const char* PaletteKey(int32_t index);
const char* PaletteNameZh(int32_t index);

/* 按客户端方块 id 反查调色板序号, 未收录返回 -1. */
int32_t PaletteIndexOf(voxel::BlockId block);

/* 回绕到合法调色板序号, 用于左右切换选中方块. */
int32_t PaletteWrap(int32_t index);

}  /* namespace immirror::editor */
