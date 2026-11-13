#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace immirror {
namespace net {

/*
 * 远端玩家: 联机骨架 M9 的最小表示, 只有位置与脱敏昵称.
 * uid 为服务端发放的九位数字串, 同时充当脱敏昵称与客户端去重键.
 * 结构体只装数据, 转换与超时策略全部收在 RemotePlayerManager 里.
 */
struct RemotePlayer {
    std::string uid;
    /* 当前渲染位置: 每帧向目标位置按固定速率靠拢, 避免轮询节拍造成的瞬移. */
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    /* 本轮快照给出的目标位置与朝向. */
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
    float targetYaw = 0.0f;
    /* 血量比例, 供头顶血条; 缺省 1 表示满血. */
    float hp = 1.0f;
    /* 快照年龄, 秒; 由管理器按帧累计, 供过期判断与淡出使用. */
    double ageSeconds = 0.0;
};

/*
 * 远端玩家管理器: 持有同世界其他玩家的最新位置快照.
 * 数据来源是 Java 侧 HTTP 轮询的结果, 由总装在低频回调里整批喂入;
 * 逐帧只做老化与查询, 不做任何 IO.
 * 该对象只在原生主线程使用, 不需要加锁.
 */
class RemotePlayerManager {
public:
    /* 快照存活期, 秒; 与服务端 30 秒剔除对齐, 客户端先行过期避免画出离线玩家. */
    static constexpr double kSnapshotTtlSeconds = 30.0;
    /* 一次喂入的快照数量上限, 与服务端内存表容量一致, 防御异常响应撑爆列表. */
    static constexpr int32_t kMaxPlayers = 256;
    /* 跟随速率, 每秒把剩余距离收敛掉的比例; 6.0 约 0.17 秒时间常数. */
    static constexpr double kFollowRate = 6.0;

    /*
     * 整批替换远端玩家快照: 每次轮询成功后调用一次.
     * 输入是三平行数组 (uid 与坐标), 长度以 count 为准, 超长部分截断.
     * 替换而非合并: 服务端返回的列表就是当前在线全集, 直接覆盖最简单且不会积累脏数据.
     */
    /* hps 可为空, 为空时按满血处理, 与旧版客户端兼容. */
    void ReplaceAll(const std::string* uids, const float* xs, const float* ys, const float* zs,
                    const float* yaws, const float* pitches, const float* hps, int32_t count);

    /* 逐帧老化: 累计快照年龄并剔除超期条目. deltaSeconds 为本帧时长. */
    void Update(double deltaSeconds);

    /* 清空全部远端玩家, 退出世界或换世界时调用. */
    void Clear();

    /* 当前可见的远端玩家列表, 已过期条目不会出现. */
    const std::vector<RemotePlayer>& Players() const { return players_; }

    /* 按 uid 定位远端玩家; 未找到返回 false. */
    bool FindByUid(const std::string& uid, RemotePlayer& out) const;

    /* 在线远端玩家数量. */
    int32_t Count() const { return static_cast<int32_t>(players_.size()); }

private:
    std::vector<RemotePlayer> players_;
};

}  /* namespace net */
}  /* namespace immirror */
