#include "render/chunk_renderer.h"

#include <cstring>
#include <string>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "core/log.h"
#include "render/texture_atlas.h"
#include "voxel/voxel_types.h"
#include "world/world.h"

namespace immirror::render {
namespace {

constexpr const char* kTag = "ImMirrorChunkRenderer";

/*
 * 顶点属性位置, 与顶点着色器里的 layout(location=...) 一一对应.
 * 位置 0 为 vec3, 位置 1 为 vec2, 位置 2 为标量明暗.
 */
constexpr GLuint kAttributePosition = 0;
constexpr GLuint kAttributeTexCoord = 1;
constexpr GLuint kAttributeShade = 2;

constexpr GLsizei kVertexStride = static_cast<GLsizei>(sizeof(voxel::Vertex));
constexpr size_t kPositionOffset = 0;
constexpr size_t kTexCoordOffset = sizeof(float) * 3;
constexpr size_t kShadeOffset = sizeof(float) * 5;

/* 顶点布局是渲染与网格构建之间的硬约定, 这里的断言用于尽早发现接口漂移. */
static_assert(sizeof(voxel::Vertex) == sizeof(float) * 6, "顶点结构必须为 6 个 float, 共 24 字节");
static_assert(sizeof(voxel::Vertex) == 24, "顶点步长必须为 24 字节");

/*
 * 网格三角形绕序. 网格构建侧若以顺时针书写正面, 只需把这里改成 GL_CW,
 * 不必改动着色器. 默认按右手系逆时针为正面.
 */
constexpr GLenum kFrontFace = GL_CCW;

/*
 * 顶点着色器: 顶点属性为位置, 图集坐标与明暗, 只做一次矩阵变换.
 * 明暗已经由网格构建侧按面向算好, 这里不做光照计算.
 * 除视图投影矩阵外还需要区块原点: 网格顶点是区块局部坐标, 由渲染侧补上世界偏移.
 */
const char* const kVertexShaderSource = R"glsl(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float aShade;

uniform mat4 uViewProjection;
uniform vec3 uChunkOrigin;

out vec2 vTexCoord;
out float vShade;

void main() {
    vTexCoord = aTexCoord;
    vShade = aShade;
    /* 网格顶点是区块局部坐标, 逐个区块加上世界原点后再变换. */
    gl_Position = uViewProjection * vec4(aPosition + uChunkOrigin, 1.0);
}
)glsl";

/*
 * 片元着色器: 采样图集后乘以顶点明暗.
 * 图集里树叶含有 alpha 为 0 的像素, 这里只对完全透明的像素做裁剪,
 * 得到镂空的树冠; 半透明边缘仍按不透明绘制, 因此不需要混合与排序.
 * 水的 alpha 约为 0.75, 高于裁剪阈值, 按不透明处理, 与 M4 约定一致.
 */
const char* const kFragmentShaderSource = R"glsl(#version 300 es
precision mediump float;

in vec2 vTexCoord;
in float vShade;

uniform sampler2D uAtlas;

out vec4 outColor;

/* 只丢弃完全透明的像素, 阈值取在 alpha 量化噪声之上. */
const float kAlphaCutoff = 0.1;

void main() {
    vec4 texel = texture(uAtlas, vTexCoord);
    if (texel.a < kAlphaCutoff) {
        discard;
    }
    outColor = vec4(texel.rgb * vShade, texel.a);
}
)glsl";

/*
 * 当前线程是否有可用的图形上下文.
 * 上下文已经销毁时调用任何 GL 函数都是未定义行为, 因此清理与绘制前先判断.
 */
bool HasCurrentContext() {
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

/* 读出并清空 GL 错误队列; 返回 true 表示记录了至少一个错误. */
bool DrainGlError(const char* stage) {
    bool failed = false;
    for (int32_t guard = 0; guard < 8; ++guard) {
        const GLenum error = glGetError();
        if (error == GL_NO_ERROR) {
            break;
        }
        IM_LOGE(kTag, "gl error 0x%x at %s", static_cast<unsigned int>(error), stage);
        failed = true;
    }
    return failed;
}

/* 打印着色器或程序的编译链接日志, 日志缺失时也给出一行说明. */
void LogObjectInfo(const char* stage, GLuint object, bool isProgram) {
    GLint length = 0;
    if (isProgram) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
    } else {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &length);
    }
    if (length <= 1) {
        IM_LOGE(kTag, "%s failed without driver log", stage);
        return;
    }
    std::string log(static_cast<size_t>(length), '\0');
    GLsizei written = 0;
    if (isProgram) {
        glGetProgramInfoLog(object, length, &written, log.data());
    } else {
        glGetShaderInfoLog(object, length, &written, log.data());
    }
    IM_LOGE(kTag, "%s failed: %s", stage, log.c_str());
}

/* 编译单个着色器, 失败返回 0 并打印驱动日志. */
GLuint CompileShader(GLenum type, const char* source, const char* stage) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        IM_LOGE(kTag, "%s: glCreateShader failed", stage);
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        LogObjectInfo(stage, shader, false);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/* 创建并链接区块着色器程序, 失败返回 0. */
GLuint BuildProgram() {
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, kVertexShaderSource, "chunk vertex shader");
    if (vertex == 0) {
        return 0;
    }
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource, "chunk fragment shader");
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    if (program == 0) {
        IM_LOGE(kTag, "glCreateProgram failed");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    /* 链接完成后着色器对象即可释放, 程序本身已持有它们的副本. */
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        LogObjectInfo("chunk program link", program, true);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

/*
 * 上传一张 RGBA8 方形纹理; 采样参数固定为像素画风格.
 * 最近邻采样且不生成 mipmap, 避免相邻格子在缩小时互相渗色;
 * 边缘夹紧避免 uv 越界时取到图集另一侧.
 */
GLuint UploadTexture(const uint8_t* pixels, int32_t sizePixels, const char* label) {
    if (pixels == nullptr || sizePixels <= 0) {
        IM_LOGE(kTag, "%s texture has no pixel data", label);
        return 0;
    }
    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        IM_LOGE(kTag, "glGenTextures for %s failed", label);
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(sizePixels),
                 static_cast<GLsizei>(sizePixels), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (DrainGlError(label)) {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

/*
 * 图集资产缺失或损坏时的占位纹理: 16x16 纯灰.
 * 场景仍然能画出地形轮廓, 不至于只剩清屏色, 同时日志里已经有明确错误.
 */
GLuint CreatePlaceholderTexture() {
    constexpr int32_t kPlaceholderPixels = 16;
    uint8_t pixels[static_cast<size_t>(kPlaceholderPixels) * kPlaceholderPixels * 4u];
    for (size_t i = 0; i < sizeof(pixels) / 4u; ++i) {
        pixels[i * 4u + 0u] = 132u;
        pixels[i * 4u + 1u] = 132u;
        pixels[i * 4u + 2u] = 136u;
        pixels[i * 4u + 3u] = 255u;
    }
    return UploadTexture(pixels, kPlaceholderPixels, "placeholder");
}

/* 从安装包资产加载图集并上传; 加载失败时退回占位纹理. */
GLuint CreateAtlasTexture(AAssetManager* assets) {
    TextureAtlas atlas;
    if (!atlas.LoadFromAssets(assets)) {
        IM_LOGE(kTag, "atlas load failed, chunk rendering continues with placeholder texture");
        return CreatePlaceholderTexture();
    }
    return UploadTexture(atlas.Pixels(), TextureAtlas::kTexturePixels, "atlas");
}

/* 区块坐标打包成 64 位键, 仅用于本模块的映射表, 与世界内部键规则无关. */
uint64_t ChunkKey(const voxel::ChunkCoord& coord) {
    const uint64_t high = static_cast<uint64_t>(static_cast<uint32_t>(coord.x));
    const uint64_t low = static_cast<uint64_t>(static_cast<uint32_t>(coord.z));
    return (high << 32) | low;
}

/* 释放一个区块的 GL 资源, 并把句柄清零以免重复释放. */
void ReleaseBufferObjects(GLuint* vao, GLuint* vbo, GLuint* ibo) {
    if (*vao != 0) {
        glDeleteVertexArrays(1, vao);
        *vao = 0;
    }
    if (*vbo != 0) {
        glDeleteBuffers(1, vbo);
        *vbo = 0;
    }
    if (*ibo != 0) {
        glDeleteBuffers(1, ibo);
        *ibo = 0;
    }
}

/*
 * 惰性创建区块的 VAO/VBO/IBO, 并在创建时固定顶点属性布局.
 * 属性布局只配置一次, 之后重建网格只需重传数据, 避免每帧改状态.
 */
bool EnsureBufferObjects(GLuint* vao, GLuint* vbo, GLuint* ibo) {
    if (*vao != 0) {
        return true;
    }
    GLuint newVao = 0;
    GLuint newVbo = 0;
    GLuint newIbo = 0;
    glGenVertexArrays(1, &newVao);
    glGenBuffers(1, &newVbo);
    glGenBuffers(1, &newIbo);
    if (newVao == 0 || newVbo == 0 || newIbo == 0) {
        IM_LOGE(kTag, "buffer creation failed, vao=%u vbo=%u ibo=%u", newVao, newVbo, newIbo);
        ReleaseBufferObjects(&newVao, &newVbo, &newIbo);
        return false;
    }

    glBindVertexArray(newVao);
    glBindBuffer(GL_ARRAY_BUFFER, newVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newIbo);
    glEnableVertexAttribArray(kAttributePosition);
    glVertexAttribPointer(kAttributePosition, 3, GL_FLOAT, GL_FALSE, kVertexStride,
                          reinterpret_cast<const void*>(kPositionOffset));
    glEnableVertexAttribArray(kAttributeTexCoord);
    glVertexAttribPointer(kAttributeTexCoord, 2, GL_FLOAT, GL_FALSE, kVertexStride,
                          reinterpret_cast<const void*>(kTexCoordOffset));
    glEnableVertexAttribArray(kAttributeShade);
    glVertexAttribPointer(kAttributeShade, 1, GL_FLOAT, GL_FALSE, kVertexStride,
                          reinterpret_cast<const void*>(kShadeOffset));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (DrainGlError("chunk vertex layout")) {
        ReleaseBufferObjects(&newVao, &newVbo, &newIbo);
        return false;
    }
    *vao = newVao;
    *vbo = newVbo;
    *ibo = newIbo;
    return true;
}

/*
 * 把网格整块上传到区块缓冲.
 * 每次重建都整体替换而不是做差量合并, 因为网格重建会改变顶点顺序,
 * 差量合并需要额外的映射表, 得不偿失.
 */
bool UploadMeshData(GLuint vao, GLuint vbo, GLuint ibo, const voxel::MeshData& mesh,
                    GLsizei* outIndexCount) {
    *outIndexCount = 0;
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        /* 空网格只保留缓冲对象, 绘制时按索引数 0 跳过. */
        return true;
    }
    const GLsizeiptr vertexBytes = static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(voxel::Vertex));
    const GLsizeiptr indexBytes = static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, mesh.vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, mesh.indices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (DrainGlError("chunk mesh upload")) {
        return false;
    }
    *outIndexCount = static_cast<GLsizei>(mesh.indices.size());
    return true;
}

}  /* namespace */

/*
 * 单个区块的图形资源.
 * 顶点缓冲与索引缓冲由 VAO 记录绑定关系与属性布局, 绘制时只需绑定 VAO.
 */
struct ChunkRenderer::ChunkBuffer {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    GLsizei indexCount = 0;
    float originX = 0.0f;       /* 区块原点, 绘制时叠加到局部坐标上 */
    float originZ = 0.0f;
    uint64_t revision = 0;      /* 已上传网格对应的区块修订号 */
    bool hasRevision = false;   /* 首次上传前为假, 避免修订号 0 与初始态混淆 */
    bool visibleThisSync = false;
};

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer() {
    Shutdown();
}

bool ChunkRenderer::Initialize(AAssetManager* assets) {
    if (ready_) {
        return true;
    }
    /* 上一次调用可能停在半步失败, 统一先清理, 保证初始化可重复调用. */
    Shutdown();

    if (!HasCurrentContext()) {
        IM_LOGE(kTag, "initialize without current graphics context");
        return false;
    }

    atlasTexture_ = CreateAtlasTexture(assets);
    if (atlasTexture_ == 0) {
        IM_LOGE(kTag, "texture atlas upload failed, chunk rendering disabled");
        return false;
    }

    program_ = BuildProgram();
    if (program_ == 0) {
        /* 着色器为 GLES 3.0 语法, 上下文回退到 GLES 2 时会走到这里. */
        IM_LOGE(kTag, "chunk shader program unavailable (needs GLES 3.0), chunk rendering disabled");
        Shutdown();
        return false;
    }

    uniformViewProjection_ = glGetUniformLocation(program_, "uViewProjection");
    uniformAtlas_ = glGetUniformLocation(program_, "uAtlas");
    uniformChunkOrigin_ = glGetUniformLocation(program_, "uChunkOrigin");
    if (uniformViewProjection_ < 0 || uniformAtlas_ < 0 || uniformChunkOrigin_ < 0) {
        IM_LOGE(kTag, "shader uniform missing, vp=%d atlas=%d origin=%d", uniformViewProjection_,
                uniformAtlas_, uniformChunkOrigin_);
        Shutdown();
        return false;
    }

    /*
     * 尚未收到相机矩阵前先放单位矩阵, 避免零矩阵把所有顶点压成退化几何.
     * 正常情况下 GameShell 在同一帧内就会通过 SetViewProjection 覆盖它.
     */
    std::memset(viewProjection_, 0, sizeof(viewProjection_));
    viewProjection_[0] = 1.0f;
    viewProjection_[5] = 1.0f;
    viewProjection_[10] = 1.0f;
    viewProjection_[15] = 1.0f;
    cameraPosition_[0] = 0.0f;
    cameraPosition_[1] = 0.0f;
    cameraPosition_[2] = 0.0f;

    buffers_.clear();
    syncedWorld_ = nullptr;
    syncedVersion_ = 0;
    drawnChunks_ = 0;
    drawnTriangles_ = 0;
    ready_ = true;
    IM_LOGI(kTag, "chunk renderer ready, atlas=%dpx program=%u", TextureAtlas::kTexturePixels, program_);
    return true;
}

void ChunkRenderer::Shutdown() {
    const bool hasContext = HasCurrentContext();
    for (auto& entry : buffers_) {
        ChunkBuffer& buffer = *entry.second;
        if (hasContext) {
            ReleaseBufferObjects(&buffer.vao, &buffer.vbo, &buffer.ibo);
        }
    }
    buffers_.clear();
    if (hasContext) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        if (atlasTexture_ != 0) {
            glDeleteTextures(1, &atlasTexture_);
        }
    } else if (program_ != 0 || atlasTexture_ != 0) {
        /* 上下文先于渲染器销毁时只能放弃显存资源, 由驱动回收. */
        IM_LOGW(kTag, "shutdown without current context, gpu objects dropped");
    }
    if (ready_) {
        IM_LOGI(kTag, "chunk renderer released");
    }
    program_ = 0;
    atlasTexture_ = 0;
    uniformViewProjection_ = -1;
    uniformAtlas_ = -1;
    uniformChunkOrigin_ = -1;
    syncedWorld_ = nullptr;
    syncedVersion_ = 0;
    drawnChunks_ = 0;
    drawnTriangles_ = 0;
    ready_ = false;
}

bool ChunkRenderer::Ready() const {
    return ready_;
}

void ChunkRenderer::SetViewProjection(const float* matrix16, const float* cameraPosition3) {
    if (matrix16 != nullptr) {
        std::memcpy(viewProjection_, matrix16, sizeof(viewProjection_));
    }
    if (cameraPosition3 != nullptr) {
        /* 相机位置本版本不参与着色, 保留给后续的视距裁剪与雾化. */
        cameraPosition_[0] = cameraPosition3[0];
        cameraPosition_[1] = cameraPosition3[1];
        cameraPosition_[2] = cameraPosition3[2];
    }
}

/*
 * 增量同步: 以每个区块的修订号为准.
 * 每帧常态开销是遍历可见区块做一次映射表查找, 不触碰显存.
 *
 * 风险点(单帧可能超过 2 毫秒的地方):
 * 1. 区块上传是同步的 glBufferData, 单个区块约数十 KB 到一百多 KB.
 *    当前世界侧把新建区块限制在每帧 2 个, 上传量因此天然有界;
 *    如果后续放宽该预算, 首次进入视距时可能出现毫秒级的上传尖峰,
 *    届时可以在本函数里加每帧上传区块数上限, 代价是新区块延后一帧出现.
 * 2. 方块编辑会重建被编辑区块及其边界邻块, 单帧最多 5 个区块,
 *    这部分开销由世界侧决定, 渲染侧只做相应的缓冲替换.
 */
void ChunkRenderer::Sync(const world::World& world) {
    if (!ready_ || program_ == 0 || atlasTexture_ == 0) {
        return;
    }
    if (!HasCurrentContext()) {
        return;
    }
    syncedWorld_ = &world;
    syncedVersion_ = world.Version();

    const std::vector<world::ChunkView>& visible = world.VisibleChunks();
    int32_t uploaded = 0;
    for (const world::ChunkView& view : visible) {
        auto found = buffers_.find(ChunkKey(view.coord));
        if (found == buffers_.end()) {
            std::unique_ptr<ChunkBuffer> created(new ChunkBuffer());
            found = buffers_.emplace(ChunkKey(view.coord), std::move(created)).first;
        }
        ChunkBuffer& buffer = *found->second;
        /* 先标记为本轮可见, 循环结束后据此卸载已经消失的区块. */
        buffer.visibleThisSync = true;
        buffer.originX = view.originX;
        buffer.originZ = view.originZ;

        const voxel::MeshData* mesh = view.mesh;
        if (mesh == nullptr) {
            continue;
        }
        /* 修订号未变的区块直接跳过, 这是每帧的常态路径. */
        if (buffer.hasRevision && buffer.revision == view.revision) {
            continue;
        }
        if (!EnsureBufferObjects(&buffer.vao, &buffer.vbo, &buffer.ibo)) {
            /* 缓冲创建失败: 保持旧内容不动, 后续帧会再次尝试. */
            continue;
        }
        GLsizei indexCount = 0;
        if (!UploadMeshData(buffer.vao, buffer.vbo, buffer.ibo, *mesh, &indexCount)) {
            buffer.indexCount = 0;
            buffer.hasRevision = false;
            continue;
        }
        buffer.indexCount = indexCount;
        buffer.revision = view.revision;
        buffer.hasRevision = true;
        ++uploaded;
    }

    int32_t released = 0;
    for (auto it = buffers_.begin(); it != buffers_.end();) {
        ChunkBuffer& buffer = *it->second;
        if (buffer.visibleThisSync) {
            buffer.visibleThisSync = false;
            ++it;
            continue;
        }
        ReleaseBufferObjects(&buffer.vao, &buffer.vbo, &buffer.ibo);
        it = buffers_.erase(it);
        ++released;
    }

    if (uploaded > 0 || released > 0) {
        IM_LOGD(kTag, "sync uploaded=%d released=%d alive=%zu worldVersion=%llu", uploaded, released,
                buffers_.size(), static_cast<unsigned long long>(syncedVersion_));
    }
}

void ChunkRenderer::Draw() {
    drawnChunks_ = 0;
    drawnTriangles_ = 0;
    if (!ready_ || program_ == 0 || atlasTexture_ == 0) {
        return;
    }
    if (!HasCurrentContext()) {
        return;
    }

    glUseProgram(program_);
    /* 列主序矩阵直接交给 glUniformMatrix4fv, 不需要转置. */
    glUniformMatrix4fv(uniformViewProjection_, 1, GL_FALSE, viewProjection_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture_);
    glUniform1i(uniformAtlas_, 0);

    /* 深度测试与背面剔除常开; 本版本不做透明排序, 因此关闭混合. */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(kFrontFace);

    const auto submit = [this](const ChunkBuffer& buffer) {
        if (buffer.indexCount == 0) {
            return;
        }
        glUniform3f(uniformChunkOrigin_, buffer.originX, 0.0f, buffer.originZ);
        glBindVertexArray(buffer.vao);
        glDrawElements(GL_TRIANGLES, buffer.indexCount, GL_UNSIGNED_INT, nullptr);
        ++drawnChunks_;
        drawnTriangles_ += static_cast<int32_t>(buffer.indexCount) / 3;
    };

    if (syncedWorld_ != nullptr) {
        for (const world::ChunkView& view : syncedWorld_->VisibleChunks()) {
            const auto found = buffers_.find(ChunkKey(view.coord));
            if (found == buffers_.end()) {
                continue;
            }
            submit(*found->second);
        }
    } else {
        /* 没有同步过世界时退化为按映射表遍历, 便于单独调试渲染路径. */
        for (const auto& entry : buffers_) {
            submit(*entry.second);
        }
    }
    glBindVertexArray(0);
}

int32_t ChunkRenderer::DrawnChunks() const {
    return drawnChunks_;
}

int32_t ChunkRenderer::DrawnTriangles() const {
    return drawnTriangles_;
}

}  /* namespace immirror::render */
