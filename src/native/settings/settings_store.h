#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace immirror::settings {

/*
 * 设置持久化. 存储形态是应用内部存储目录下的纯文本键值文件,
 * 一行一个 "键=值", 不引入数据库, 也不依赖 Java 侧的 SharedPreferences:
 * 读写都在 native 侧完成, 文件可读可手改, 便于排查线上设置问题.
 *
 * 文件位置: <内部存储目录>/settings/immirror-settings.cfg
 */
class SettingsStore {
public:
    /* 一条落盘条目, 值保持原始整数, 合法范围由 Settings 负责裁剪. */
    struct Entry {
        std::string name;
        int32_t value = 0;
    };

    /* 指定内部存储根目录; 传入空指针或空串时清除落盘位置. 返回是否已有落盘位置. */
    bool SetDirectory(const char* dataPath);

    /* 配置文件绝对路径, 未指定目录时为空串. */
    const std::string& Path() const { return path_; }

    bool Exists() const;

    /*
     * 读取全部条目. 空行与井号开头的注释行忽略, 缺少等号的行忽略,
     * 同名键后出现的覆盖先出现的. 返回是否成功读到了文件.
     */
    bool Load(std::vector<Entry>* out) const;

    /*
     * 原子写入: 先写同目录下的临时文件再改名覆盖,
     * 中途掉电不会留下半个设置文件. 目录不存在时自动创建.
     */
    bool Save(const std::vector<Entry>& entries) const;

private:
    std::string directory_;
    std::string path_;
};

}  /* namespace immirror::settings */
