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

    /*
     * 天空天体: direction3 是指向太阳的单位向量 (世界空间), skyTimeSeconds 用于云层漂移.
     * 两者都由总装按世界时间折算后下发, 渲染层不读时间也不读存档.
     */
    void SetSkySun(const float* direction3, float skyTimeSeconds);

    /*
     * 天气: kind 与 world::WeatherKind 同值 (0 晴, 1 雨, 2 雪, 3 雷暴),
     * intensity 0 到 1. 只影响天空压暗与降水叠加层, 不改变地形绘制.
     */
    void SetWeather(int32_t kind, float intensity, float seconds);

    /*
     * 设置距离雾密度, 输出亮度与天光混合因子, 每帧由场景组装方折算后下发.
     * density 为 0 表示关闭雾; brightness 为输出颜色的线性系数;
     * skyFactor 夜 0 昼 1, 渲染端按 mix(0.27, 1.0, factor) 压顶点明暗.
     */
    void SetAtmosphere(float fogDensity, float brightness, float skyFactor);

    /*
     * 视口高度, 天空渐变按它把片元纵坐标归一: 分辨率变化后渐变比例不变.
     * 缺省 720, 未下发时按固定高度折算, 只影响渐变曲线的稀疏程度.
     */
    void SetViewportSize(int32_t width, int32_t height);

    /*
     * 水下滤镜强度 0 到 1: 0 表示相机不在水里.
     * 由总装按相机所在方块判定, 渲染层只负责整屏叠加.
     */
    void SetUnderwaterTint(float alpha);

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

    /* 天空渐变: 全屏竖直两色过渡, 在区块之前绘制, 不写深度. */
    void DrawSky();

    /* 降水叠加层: 雨天斜线, 雪天飘点, 雷暴更密; 在区块之后绘制. */
    void DrawPrecipitation();

    /* 水下滤镜: 全屏纯色叠加, 在降水之后绘制. */
    void DrawUnderwaterTint();

    /* 以下成员为本实现的内部状态, 不改变公共接口约定. */
    uint32_t tintProgram_ = 0;
    int32_t uniformTintColor_ = -1;
    float underwaterTint_ = 0.0f;
    uint32_t weatherProgram_ = 0;
    int32_t uniformWeatherIntensity_ = -1;
    int32_t uniformWeatherKind_ = -1;
    int32_t uniformWeatherTime_ = -1;
    int32_t uniformWeatherViewport_ = -1;
    int32_t uniformWeatherAspect_ = -1;
    int32_t weatherKind_ = 0;
    float weatherIntensity_ = 0.0f;
    float weatherSeconds_ = 0.0f;
    uint32_t skyProgram_ = 0;
    int32_t uniformSkyBlend_ = -1;
    int32_t uniformSkyInvViewProjection_ = -1;
    int32_t uniformSkySun_ = -1;
    int32_t uniformSkyCamera_ = -1;
    int32_t uniformSkyTime_ = -1;
    int32_t uniformSkyWeather_ = -1;
    float inverseViewProjection_[16] = {0.0f};
    float sunDirection_[3] = {0.0f, 1.0f, 0.0f};
    float skyTimeSeconds_ = 0.0f;
    int32_t uniformSkyViewHeight_ = -1;
    int32_t skyViewportHeight_ = 720;
    int32_t skyViewportWidth_ = 1280;
    int32_t uniformViewProjection_ = -1;
    int32_t uniformAtlas_ = -1;
    int32_t uniformChunkOrigin_ = -1;
    int32_t uniformCameraPosition_ = -1;
    int32_t uniformFogDensity_ = -1;
    int32_t uniformFogColor_ = -1;
    int32_t uniformBrightness_ = -1;
    int32_t uniformSkyFactor_ = -1;
    /* 云影: 时间与强度, 顶点着色器按世界坐标采样云层噪声. */
    int32_t uniformCloudTime_ = -1;
    int32_t uniformCloudStrength_ = -1;
    float fogDensity_ = 0.0f;
    float brightness_ = 1.0f;
    float skyFactor_ = 1.0f;
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
