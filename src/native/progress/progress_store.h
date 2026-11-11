#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace immirror::progress {

/*
 * 进度落盘.
 *
 * 两份文件放在应用内部存储根目录下的 progress/ 子目录:
 * state 是原生侧自己读写的键值存档, snapshot 是给 Java 界面读的只读快照.
 * 两者都采用先写临时文件再改名的方式发布, 因此界面随时读都不会读到半截内容.
 */
class ProgressStore {
public:
    /*
     * 一条键值记录. 绝大多数记录是整数, 日期与日期列表这类键改用文本存放:
     * text 非空时按文本落盘, 否则写 value.
     */
    struct Entry {
        std::string name;
        int64_t value = 0;
        std::string text;
    };

    /* 指定应用内部存储目录; 空指针或空串返回 false, 重复调用幂等. */
    bool SetDirectory(const char* dataPath);

    bool Ready() const { return !statePath_.empty(); }
    const std::string& Directory() const { return directory_; }
    const std::string& StatePath() const { return statePath_; }
    const std::string& SnapshotPath() const { return snapshotPath_; }

    /* 读取键值存档; 文件不存在时返回 true 并给出空表, 由调用方按首次运行处理. */
    bool LoadState(std::vector<Entry>* out) const;

    /* 写入键值存档, 先写临时文件再改名. */
    bool SaveState(const std::vector<Entry>& entries) const;

    /* 写入界面快照文本, 同样是先写临时文件再改名. */
    bool SaveSnapshot(const std::string& text) const;

private:
    bool WriteAtomic(const std::string& path, const std::string& text) const;

    std::string directory_;
    std::string statePath_;
    std::string snapshotPath_;
};

}  /* namespace immirror::progress */
