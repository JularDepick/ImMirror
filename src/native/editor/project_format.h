#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "editor/editor_types.h"
#include "editor/trigger_model.h"
#include "voxel/voxel_types.h"

namespace immirror::world {
class World;
}

namespace immirror::editor {

/*
 * 工程描述. 对应 project.json, 字段顺序固定,
 * 写盘时每次输出同样的键序, 便于用文本比对查看改动.
 */
struct ProjectMeta {
    std::string name;      /* 显示名, 允许中文 */
    std::string author;    /* 作者, 可空 */
    uint32_t seed = limits::kDefaultSeed;
    float spawnX = 8.0f;   /* 出生点, 工程坐标系 */
    float spawnY = 40.0f;
    float spawnZ = 8.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    int32_t selectedBlock = 1;                 /* 上次选中的客户端方块 id */
    int32_t viewDistance = limits::kDefaultViewDistance;
    std::string createdAt;
    std::string updatedAt;
};

/*
 * 地图工程: 明文目录, 与运行期的 IMWF 世界存档是两套格式.
 *
 * 目录结构固定为:
 *   project.json 工程描述, 明文 JSON
 *   chunks/c.<x>.<z>.vox 区块方块数据, 明文游程文本
 *   triggers/t.<id>.json 触发器, 一条一个明文 JSON 文件
 *   export/<名字>.world 导出产物, 二进制 IMWF, 由游戏存档路径直接加载
 *
 * 工程数据是编辑期的唯一事实来源, 运行时世界只是它的投影:
 * 世界按玩家位置流式加载与卸载区块, 区块卸载后重新生成会回到地形原始状态,
 * 因此本类记住每个方块与生成地形的差异, 每帧把差异重新投影回世界.
 */
class Project {
public:
    Project();
    ~Project();

    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;

    /*
     * 打开已有工程目录; project.json 不存在时按给定名称与种子新建空工程.
     * 返回 false 时 error 给出具体原因, 已加载内容不变.
     */
    bool OpenOrCreate(const std::string& directory, const std::string& name, uint32_t seed,
                      std::string& error);

    /* 读取工程描述与全部区块与触发器. */
    bool Load(const std::string& directory, std::string& error);

    /* 把工程描述, 区块与触发器全部落盘, 目录不存在时一并创建. */
    bool Save(std::string& error);

    const std::string& Directory() const { return directory_; }
    const std::string& Name() const { return meta_.name; }
    uint32_t Seed() const { return meta_.seed; }
    const ProjectMeta& Meta() const { return meta_; }
    ProjectMeta& MutableMeta() { return meta_; }

    /* 方块读写. 读取未涉及的区块时先按种子生成地形, 再记入工程. */
    voxel::BlockId GetBlock(int32_t x, int32_t y, int32_t z);
    bool SetBlock(int32_t x, int32_t y, int32_t z, voxel::BlockId id);

    /*
     * 把工程数据投影回运行时世界, 返回本次真正写入的方块数.
     * 预算限制单帧写入量, 未完成的部分留给下一帧;
     * 区块尚未加载时该格跳过, 等区块加载后补写.
     */
    int32_t ProjectToWorld(world::World& world, int32_t budget);

    /*
     * 导出为可玩世界: 生成 IMWF 二进制存档, 内容为工程里全部区块与出生点.
     * 导出路径记入内存中的最近导出记录, 便于界面显示.
     */
    bool ExportWorld(const std::string& path, std::string& error);

    /* 触发器访问与增删. */
    const std::vector<Trigger>& Triggers() const { return triggers_; }
    std::vector<Trigger>& MutableTriggers() { return triggers_; }
    Trigger* FindTrigger(int32_t id);
    const Trigger* FindTrigger(int32_t id) const;

    /* 新建一条触发器并分配未占用的 id, 超出上限返回空指针. */
    Trigger* AddTrigger();

    /* 删除触发器; 返回是否删除成功. */
    bool RemoveTrigger(int32_t id);

    /* 已保存的区块数与差异格子总数, 供界面显示规模. */
    int32_t ChunkCount() const { return static_cast<int32_t>(chunks_.size()); }
    int64_t ChangedBlockCount() const;

    /* 工程是否已被修改, 退出时据此决定是否落盘. */
    bool Dirty() const { return dirty_; }
    void ClearDirty() { dirty_ = false; }
    void MarkDirty() { dirty_ = true; }

    /* 最近一次导出的路径, 未导出过为空串. */
    const std::string& LastExport() const { return lastExport_; }

private:
    /* 单个区块的工程数据: 完整方块数组 + 与生成地形的差异标记. */
    struct Chunk {
        voxel::ChunkCoord coord;
        std::vector<voxel::BlockId> blocks;   /* 长度固定为 kChunkVolume */
        std::vector<uint8_t> changed;         /* 逐格标记, 1 表示与生成地形不同 */
        std::vector<uint32_t> changedList;    /* 差异格的平铺下标, 供投影按序回放 */
    };

    /* 按地形重算一个区块的差异表, 落盘前调用一次. */
    void RefreshChunkDiff(Chunk& chunk);

    Chunk* FindChunk(voxel::ChunkCoord coord);
    Chunk* EnsureChunk(voxel::ChunkCoord coord);
    static uint64_t ChunkKey(int32_t x, int32_t z);

    /* 区块文件的读写, 明文游程文本. */
    bool LoadChunkFile(Chunk& chunk, const std::string& path, std::string& error);
    bool SaveChunkFile(const Chunk& chunk, const std::string& path, std::string& error) const;

    /* 目录与文件命名. */
    bool EnsureDirectories(std::string& error);
    static std::string ChunkFileName(int32_t x, int32_t z);
    static std::string TriggerFileName(int32_t id);
    void ResetContents();

    std::string directory_;
    ProjectMeta meta_;
    std::unordered_map<uint64_t, Chunk> chunks_;
    std::vector<Trigger> triggers_;
    std::string lastExport_;
    bool dirty_ = false;
};

/* 工程摘要, 供 Java 侧的工程列表显示. */
struct ProjectSummary {
    std::string directory;
    std::string name;
    uint32_t seed = 0;
    int32_t chunkCount = 0;
    int32_t triggerCount = 0;
    std::string updatedAt;
};

/*
 * 工程目录管理. root 是应用内部存储下的编辑器根目录,
 * 每个工程是 root 下的一个子目录: 目录名取安全字符, 显示名存在工程描述里,
 * 因此中文工程名不会影响文件系统路径.
 */
bool ListProjects(const std::string& root, std::vector<ProjectSummary>& out, std::string& error);
bool CreateProject(const std::string& root, const std::string& name, uint32_t seed,
                   std::string& directoryOut, std::string& error);
bool DeleteProject(const std::string& directory, std::string& error);

/* 递归创建目录与递归删除目录, 供工程管理使用. */
bool MakeDirectories(const std::string& path);
bool RemoveDirectoryTree(const std::string& path);

/* 当前时间的 ISO 8601 文本, 形如 2026-11-11T12:00:00+08:00. */
std::string NowTimestamp();

}  /* namespace immirror::editor */
