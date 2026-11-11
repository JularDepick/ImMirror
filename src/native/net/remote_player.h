#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace immirror::net {

/*
 * 远程玩家快照. 服务端 presence 接口轮询返回, 客户端只读展示,
 * 不参与本地物理与碰撞; 过期条目由管理器按超时剔除.
 */
struct RemotePlayer {
    int32_t uid = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

/*
 * 远程玩家管理器: 存储最近一次轮询的快照列表.
 * 快照整体替换, 不做插值与预测; 展示层直接读列表.
 * 只在原生主线程访问, 不加锁.
 */
class RemotePlayerManager {
public:
    /* 用服务端响应的 JSON 文本整体替换快照列表; 解析失败保留旧列表. */
    void UpdateFromJson(const std::string& json);

    /* 当前快照; 过期剔除在更新时统一处理. */
    const std::vector<RemotePlayer>& Players() const { return players_; }

    int32_t Count() const { return static_cast<int32_t>(players_.size()); }

    /* 清空快照; 退出游戏或断线时调用. */
    void Clear() { players_.clear(); }

private:
    std::vector<RemotePlayer> players_;
};

}  /* namespace immirror::net */
