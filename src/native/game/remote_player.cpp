#include "game/remote_player.h"

#include <algorithm>
#include <set>

namespace immirror {
namespace net {

void RemotePlayerManager::ReplaceAll(const std::string* uids, const float* xs, const float* ys,
                                     const float* zs, const float* yaws, const float* pitches,
                                     const float* hps, int32_t count) {
    if (uids == nullptr || xs == nullptr || ys == nullptr || zs == nullptr || count <= 0) {
        players_.clear();
        return;
    }
    if (count > kMaxPlayers) {
        count = kMaxPlayers;
    }
    /*
     * 保留上一批的当前渲染位置作为起点: 同一 uid 继续平滑跟随新目标,
     * 新出现的玩家直接落在目标位置上, 不从原点滑过来.
     */
    std::vector<RemotePlayer> previous = std::move(players_);
    players_.clear();
    /* 服务端列表本身不含重复 uid, 集合只是防御异常数据画出一堆重叠标记. */
    std::set<std::string> seen;
    players_.reserve(static_cast<size_t>(count));
    for (int32_t index = 0; index < count; ++index) {
        const std::string& uid = uids[index];
        if (uid.empty() || !seen.insert(uid).second) {
            continue;
        }
        RemotePlayer player;
        player.uid = uid;
        player.targetX = xs[index];
        player.targetY = ys[index];
        player.targetZ = zs[index];
        player.targetYaw = yaws != nullptr ? yaws[index] : 0.0f;
        player.pitch = pitches != nullptr ? pitches[index] : 0.0f;
        player.hp = hps != nullptr ? hps[index] : 1.0f;
        player.x = player.targetX;
        player.y = player.targetY;
        player.z = player.targetZ;
        player.yaw = player.targetYaw;
        for (const RemotePlayer& old : previous) {
            if (old.uid != uid) {
                continue;
            }
            /* 同一玩家: 从当前位置继续跟随, 位置与朝向都不跳. */
            player.x = old.x;
            player.y = old.y;
            player.z = old.z;
            player.yaw = old.yaw;
            break;
        }
        player.ageSeconds = 0.0;
        players_.push_back(player);
    }
}

void RemotePlayerManager::Update(double deltaSeconds) {
    if (players_.empty()) {
        return;
    }
    /* 指数跟随: 每帧把剩余距离按速率收敛掉一部分, 与帧率无关. */
    const float alpha = static_cast<float>(std::min(1.0, deltaSeconds * kFollowRate));
    size_t write = 0;
    for (size_t read = 0; read < players_.size(); ++read) {
        RemotePlayer& player = players_[read];
        player.x += (player.targetX - player.x) * alpha;
        player.y += (player.targetY - player.y) * alpha;
        player.z += (player.targetZ - player.z) * alpha;
        player.yaw += (player.targetYaw - player.yaw) * alpha;
        player.ageSeconds += deltaSeconds;
        if (player.ageSeconds <= kSnapshotTtlSeconds) {
            /* 就地压缩保留存活条目, 避免逐帧erase的重复搬运. */
            if (write != read) {
                players_[write] = player;
            }
            ++write;
        }
    }
    players_.resize(write);
}

void RemotePlayerManager::Clear() {
    players_.clear();
}

bool RemotePlayerManager::FindByUid(const std::string& uid, RemotePlayer& out) const {
    for (const RemotePlayer& player : players_) {
        if (player.uid == uid) {
            out = player;
            return true;
        }
    }
    return false;
}

}  /* namespace net */
}  /* namespace immirror */
