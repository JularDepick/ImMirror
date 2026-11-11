#include "progress/progress_store.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "core/log.h"

namespace immirror::progress {
namespace {

constexpr const char* kTag = "ImMirrorProgress";

/* 目录与文件名. 与 settings, worlds 平级, 都在应用内部存储根目录下. */
constexpr const char* kDirectoryName = "progress";
constexpr const char* kStateFileName = "progress.state";
constexpr const char* kSnapshotFileName = "progress_snapshot.tsv";

/* 文件头注释, 便于排查时直接打开查看格式来源. */
constexpr const char* kStateHeader =
    "# ImMirror progress state\n"
    "# format: key=value per line, keys are stable across versions\n";

std::string Trim(const std::string& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end) {
        const char ch = text[begin];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        ++begin;
    }
    while (end > begin) {
        const char ch = text[end - 1];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
            break;
        }
        --end;
    }
    return text.substr(begin, end - begin);
}

int64_t ParseInt(const std::string& text, bool* ok) {
    if (text.empty()) {
        *ok = false;
        return 0;
    }
    char* end = nullptr;
    const long long parsed = std::strtoll(text.c_str(), &end, 10);
    *ok = end != nullptr && *end == '\0';
    return static_cast<int64_t>(parsed);
}

}  /* namespace */

bool ProgressStore::SetDirectory(const char* dataPath) {
    directory_.clear();
    statePath_.clear();
    snapshotPath_.clear();
    if (dataPath == nullptr || dataPath[0] == '\0') {
        return false;
    }
    directory_ = std::string(dataPath) + "/" + kDirectoryName;
    statePath_ = directory_ + "/" + kStateFileName;
    snapshotPath_ = directory_ + "/" + kSnapshotFileName;
    ::mkdir(directory_.c_str(), 0700);
    return true;
}

bool ProgressStore::LoadState(std::vector<Entry>* out) const {
    if (out == nullptr) {
        return false;
    }
    out->clear();
    if (!Ready()) {
        return false;
    }
    std::ifstream stream(statePath_);
    if (!stream.is_open()) {
        return true;
    }
    std::string line;
    while (std::getline(stream, line)) {
        const std::string text = Trim(line);
        if (text.empty() || text[0] == '#') {
            continue;
        }
        const size_t separator = text.find('=');
        if (separator == std::string::npos || separator == 0) {
            continue;
        }
        const std::string name = Trim(text.substr(0, separator));
        const std::string value = Trim(text.substr(separator + 1));
        if (name.empty()) {
            continue;
        }
        bool ok = false;
        const int64_t parsed = ParseInt(value, &ok);
        if (!ok) {
            /* 非整数按文本保留, 日期与日期列表都用这条路径. */
            if (value.empty()) {
                continue;
            }
            Entry textEntry;
            textEntry.name = name;
            textEntry.text = value;
            out->push_back(textEntry);
            continue;
        }
        Entry numericEntry;
        numericEntry.name = name;
        numericEntry.value = parsed;
        out->push_back(numericEntry);
    }
    return true;
}

bool ProgressStore::SaveState(const std::vector<Entry>& entries) const {
    if (!Ready()) {
        return false;
    }
    std::string text = kStateHeader;
    for (const Entry& entry : entries) {
        text += entry.name;
        text += '=';
        text += entry.text.empty() ? std::to_string(entry.value) : entry.text;
        text += '\n';
    }
    return WriteAtomic(statePath_, text);
}

bool ProgressStore::SaveSnapshot(const std::string& text) const {
    if (!Ready()) {
        return false;
    }
    return WriteAtomic(snapshotPath_, text);
}

bool ProgressStore::WriteAtomic(const std::string& path, const std::string& text) const {
    const std::string temporary = path + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            IM_LOGW(kTag, "progress write failed, path=%s", temporary.c_str());
            return false;
        }
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!stream.good()) {
            IM_LOGW(kTag, "progress write failed, path=%s", temporary.c_str());
            return false;
        }
    }
    /*
     * rename 在同一目录内是原子操作, 界面读到的要么是旧文件, 要么是完整的新文件,
     * 不会有半截内容. 失败时退化为先删除目标再改名, 保证不会留下旧快照.
     */
    if (std::rename(temporary.c_str(), path.c_str()) != 0) {
        std::remove(path.c_str());
        if (std::rename(temporary.c_str(), path.c_str()) != 0) {
            IM_LOGW(kTag, "progress rename failed, path=%s", path.c_str());
            std::remove(temporary.c_str());
            return false;
        }
    }
    return true;
}

}  /* namespace immirror::progress */
