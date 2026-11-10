#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <android/asset_manager.h>

namespace immirror::world {
class World;
}

namespace immirror::render {

/*
 * 区块渲染器: 持有纹理图集, 着色器程序与每区块的顶点缓冲,
 * 按世界版本号增量同步网格并提交绘制.
 */
class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();

    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    /* 图集与着色器都从安装包资产加载, assets 由 NativeActivity 提供. */
    bool Initialize(AAssetManager* assets);
    void Shutdown();
    bool Ready() const;

    /* 列主序 4x4 视图投影矩阵与相机世界坐标. */
    void SetViewProjection(const float* matrix16, const float* cameraPosition3);

    /* 按世界版本增量上传脏区块的网格. */
    void Sync(const world::World& world);

    /* 提交全部可见区块. */
    void Draw();

    /* 图集纹理句柄, 供界面层绘制方块图标时共享. */
    uint32_t AtlasTexture() const { return atlasTexture_; }

    int32_t DrawnChunks() const;
    int32_t DrawnTriangles() const;

private:
    struct ChunkBuffer;

    /* 以下成员为本实现的内部状态, 不改变公共接口约定. */
    int32_t uniformViewProjection_ = -1;
    int32_t uniformAtlas_ = -1;
    int32_t uniformChunkOrigin_ = -1;
    const world::World* syncedWorld_ = nullptr;

    bool ready_ = false;
    uint32_t program_ = 0;
    uint32_t atlasTexture_ = 0;
    uint64_t syncedVersion_ = 0;
    float viewProjection_[16] = {0.0f};
    float cameraPosition_[3] = {0.0f};
    int32_t drawnChunks_ = 0;
    int32_t drawnTriangles_ = 0;
    std::unordered_map<uint64_t, std::unique_ptr<ChunkBuffer>> buffers_;
};

}  /* namespace immirror::render */
