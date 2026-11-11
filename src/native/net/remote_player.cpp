#include "net/remote_player.h"

#include <cstdlib>

#include "core/log.h"

namespace immirror::net {
namespace {

constexpr const char* kTag = "ImMirrorNet";

/* 极简 JSON 数字提取: 在 json 里找 "key":数字 形式, 只服务 presence 协议. */
bool FindNumber(const std::string& json, const std::string& key, double& out) {
    const std::string pattern = "\"" + key + "\":";
    const size_t keyPos = json.find(pattern);
    if (keyPos == std::string::npos) {
        return false;
    }
    const size_t valueStart = keyPos + pattern.size();
    const char* begin = json.c_str() + valueStart;
    char* end = nullptr;
    const double value = std::strtod(begin, &end);
    if (end == begin) {
        return false;
    }
    out = value;
    return true;
}

bool FindInt(const std::string& json, const std::string& key, int32_t& out) {
    double value = 0.0;
    if (!FindNumber(json, key, value)) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

/* 找下一个对象块的起始与结束 (大括号配平). */
bool NextObject(const std::string& json, size_t& cursor, std::string& out) {
    const size_t open = json.find('{', cursor);
    if (open == std::string::npos) {
        return false;
    }
    int32_t depth = 0;
    for (size_t index = open; index < json.size(); ++index) {
        if (json[index] == '{') {
            depth += 1;
        } else if (json[index] == '}') {
            depth -= 1;
            if (depth == 0) {
                out = json.substr(open, index - open + 1);
                cursor = index + 1;
                return true;
            }
        }
    }
    return false;
}

}  /* namespace */

void RemotePlayerManager::UpdateFromJson(const std::string& json) {
    static const std::string kMarker = "\"others\":";
    const size_t arrayStart = json.find(kMarker);
    if (arrayStart == std::string::npos) {
        IM_LOGW(kTag, "presence json missing others array");
        return;
    }
    std::vector<RemotePlayer> updated;
    size_t cursor = arrayStart + kMarker.size();
    std::string object;
    while (NextObject(json, cursor, object)) {
        RemotePlayer player;
        bool valid = FindInt(object, "uid", player.uid);
        valid = FindNumber(object, "x", reinterpret_cast<double&>(player.x)) && valid;
        valid = FindNumber(object, "y", reinterpret_cast<double&>(player.y)) && valid;
        valid = FindNumber(object, "z", reinterpret_cast<double&>(player.z)) && valid;
        valid = FindNumber(object, "yaw", reinterpret_cast<double&>(player.yaw)) && valid;
        valid = FindNumber(object, "pitch", reinterpret_cast<double&>(player.pitch)) && valid;
        if (valid && player.uid > 0) {
            updated.push_back(player);
        }
    }
    players_ = updated;
}

}  /* namespace immirror::net */
