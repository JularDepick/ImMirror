#include "settings/settings_store.h"

#include <sys/stat.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/log.h"

namespace immirror::settings {
namespace {

constexpr const char* kTag = "ImMirrorSettings";

/* 目录与文件名. 目录与存档目录平级, 都在应用内部存储根目录下. */
constexpr const char* kDirectoryName = "settings";
constexpr const char* kFileName = "immirror-settings.cfg";

/* 文件头注释, 说明格式来源, 方便排查时直接打开查看. */
constexpr const char* kFileHeader =
    "# ImMirror settings\n"
    "# format: key=value per line, keys and defaults follow original setting.lua\n";

/* 单行读取缓冲. 设置文件只有几十行, 128 字节足够, 超长行按截断处理. */
constexpr int32_t kLineBufferSize = 128;

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

int32_t ParseInt(const std::string& text, bool* ok) {
    if (text.empty()) {
        *ok = false;
        return 0;
    }
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    *ok = end != nullptr && *end == '\0';
    return static_cast<int32_t>(parsed);
}

}  /* namespace */

bool SettingsStore::SetDirectory(const char* dataPath) {
    directory_.clear();
    path_.clear();
    if (dataPath == nullptr || dataPath[0] == '\0') {
        return false;
    }
    directory_ = std::string(dataPath) + "/" + kDirectoryName;
    path_ = directory_ + "/" + kFileName;
    return true;
}

bool SettingsStore::Exists() const {
    if (path_.empty()) {
        return false;
    }
    struct stat info {};
    return ::stat(path_.c_str(), &info) == 0;
}

bool SettingsStore::Load(std::vector<Entry>* out) const {
    if (out == nullptr) {
        return false;
    }
    out->clear();
    if (path_.empty()) {
        return false;
    }
    std::FILE* file = std::fopen(path_.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    char line[kLineBufferSize];
    while (std::fgets(line, static_cast<int>(sizeof(line)), file) != nullptr) {
        const std::string text = Trim(std::string(line));
        if (text.empty() || text[0] == '#') {
            continue;
        }
        const size_t separator = text.find('=');
        if (separator == std::string::npos || separator == 0) {
            continue;
        }
        const std::string name = Trim(text.substr(0, separator));
        const std::string valueText = Trim(text.substr(separator + 1));
        if (name.empty()) {
            continue;
        }
        bool ok = false;
        const int32_t value = ParseInt(valueText, &ok);
        if (!ok) {
            continue;
        }
        /* 同名键以后出现的为准, 与从旧到新的覆盖语义一致. */
        bool replaced = false;
        for (Entry& entry : *out) {
            if (entry.name == name) {
                entry.value = value;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            out->push_back(Entry{name, value});
        }
    }
    std::fclose(file);
    return true;
}

bool SettingsStore::Save(const std::vector<Entry>& entries) const {
    if (path_.empty()) {
        return false;
    }
    /* 目录可能尚未创建: 与存档目录一样按需创建, 已存在时忽略返回值. */
    if (::mkdir(directory_.c_str(), 0700) != 0 && errno != EEXIST) {
        IM_LOGW(kTag, "settings directory create failed, dir=%s", directory_.c_str());
    }
    const std::string temporary = path_ + ".tmp";
    std::FILE* file = std::fopen(temporary.c_str(), "wb");
    if (file == nullptr) {
        IM_LOGE(kTag, "settings temp open failed, path=%s", temporary.c_str());
        return false;
    }
    std::fputs(kFileHeader, file);
    for (const Entry& entry : entries) {
        std::fprintf(file, "%s=%d\n", entry.name.c_str(), static_cast<int>(entry.value));
    }
    const bool flushed = std::fflush(file) == 0;
    std::fclose(file);
    if (!flushed) {
        std::remove(temporary.c_str());
        IM_LOGE(kTag, "settings flush failed, path=%s", temporary.c_str());
        return false;
    }
    /* 改名覆盖是原子的, 读侧要么看到旧文件要么看到完整新文件. */
    if (std::rename(temporary.c_str(), path_.c_str()) != 0) {
        std::remove(temporary.c_str());
        IM_LOGE(kTag, "settings rename failed, path=%s", path_.c_str());
        return false;
    }
    return true;
}

}  /* namespace immirror::settings */
