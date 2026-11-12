#pragma once

#include <cstdint>

namespace immirror::render {

/*
 * 生物的程序化体素占位形象.
 *
 * 生物是 3D 模型 (assets/entity 的 omod 容器), 解析工作量极大, 本模块用
 * 少量立方体的组合给每个生物一个可辨识的剪影: 按类别给不同的部件拼法,
 * 按 MonsterDef 的身高/身宽/身厚折算尺寸, 按ID 哈希出该生物的基色.
 * 这是占位形象, omod 解析接入后由真实模型替换.
 *
 * 全部几何在首次使用时预生成进静态数组, 逐帧只做矩阵变换, 单批提交;
 * 着色走独立的 mini 程序 (纯顶点色, 无纹理), 初始化与销毁由显式接口管理.
 */
namespace creature {

/* 程序初始化与销毁. Initialize 必须在当前图形上下文内调用, 重复调用幂等. */
bool Initialize();
void Shutdown();

/*
 * 提交一帧的全部生物形象.
 * 每个生物给: 脚底中心世界坐标, 朝向角 (弧度, 与生物实体的 yaw 同义),
 * 受击闪白强度 0 到 1 (大于 0 时整体颜色向白色提亮), 死亡倒地进度 0 到 1
 * (大于 0 时身体绕横轴逐渐放平), 定义索引 (折算尺寸与配色).
 */
struct CreatureView {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float hurt = 0.0f;
    float death = 0.0f;
    int32_t defIndex = 0;
};

void Draw(const CreatureView* views, int32_t count, const float* viewProjection16);

/*
 * 远端玩家的位置与朝向. 与生物共用同一套体素几何与变换,
 * 只是用一套固定的人形与配色, 玩家之间靠位置区分而不是靠外观.
 */
struct PlayerView {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

/* 提交一帧的全部远端玩家形象, 与 Draw 共用同一个着色器与缓冲. */
void DrawPlayers(const PlayerView* views, int32_t count, const float* viewProjection16);

/* 类别数, 由生成表条目数决定, 供调用方核对. */
int32_t DefCount();

}  /* namespace creature */

}  /* namespace immirror::render */
