#include "game/remote_player.h"

#include <set>

namespace immirror {
namespace net {

void RemotePlayerManager::ReplaceAll(const std::string* uids, const float* xs, const float* ys,
                                     const float* zs, const float* yaws, const float* pitches,
                                     int32_t count) {
    players_.clear();
    if (uids == nullptr || xs == nullptr || ys == nullptr || zs == nullptr) {
        return;
    }
    if (count <= 0) {
        return;
    }
    if (count > kMaxPlayers) {
        count = kMaxPlayers;
    }
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
        player.x = xs[index];
        player.y = ys[index];
        player.z = zs[index];
        player.yaw = yaws != nullptr ? yaws[index] : 0.0f;
        player.pitch = pitches != nullptr ? pitches[index] : 0.0f;
        player.ageSeconds = 0.0;
        players_.push_back(player);
    }
}

void RemotePlayerManager::Update(double deltaSeconds) {
    if (players_.empty()) {
        return;
    }
    size_t write = 0;
    for (size_t read = 0; read < players_.size(); ++read) {
        RemotePlayer& player = players_[read];
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
