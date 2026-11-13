#include "render/particle_system.h"

#include <cmath>
#include <cstring>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "core/log.h"
#include "voxel/block_registry.h"
#include "voxel/generated/block_color_gen.h"

namespace immirror::render {
namespace particle {
namespace {

constexpr const char* kTag = "ImMirrorParticle";

/* 顶点: 世界坐标 3 个 float 加 RGBA 4 个 float. */
constexpr int32_t kFloatsPerVertex = 7;
/* 每个粒子是一个小立方体: 6 面, 每面 2 三角形, 每三角形 3 顶点. */
constexpr int32_t kVerticesPerParticle = 36;
/* 同时存在的粒子上限: 一次破坏 8 个, 留出连续挖掘的余量. */
constexpr int32_t kMaxParticles = 96;
constexpr int32_t kParticlesPerBreak = 8;
/* 粒子边长, 单位米: 约方块的五分之一, 远看是一撮碎屑. */
constexpr float kParticleSize = 0.14f;
/* 生命与重力. 一秒淡出, 重力与玩家同源. */
constexpr float kLifeSeconds = 1.0f;
constexpr float kGravity = -18.0f;
/* 初速度范围: 水平四散, 竖直向上弹起. */
constexpr float kSpeedHorizontal = 1.8f;
constexpr float kSpeedUp = 2.4f;

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float age = 0.0f;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
};

Particle gParticles[kMaxParticles];
int32_t gCount = 0;

/*
 * 掉落物通道: 方块掉到地上时画成一个小立方体, 比碎屑大且不淡出,
 * 拾取或超时后由游戏层移除. 与碎屑共用着色器与顶点缓冲, 只是存活更久.
 */
constexpr int32_t kMaxDrops = 64;
constexpr float kDropSize = 0.26f;

struct Drop {
    int32_t id = 0;
    voxel::BlockId block = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
};

Drop gDrops[kMaxDrops];
int32_t gDropCount = 0;
/* 上下浮动的相位, 让静止的掉落物看起来是"活着"的. */
float gDropBobPhase = 0.0f;
/* 粒子撒布用的轻量伪随机, 与其它模块同一套 xorshift. */
uint32_t gRandom = 0x9e3779b9u;

bool gInitialized = false;
GLuint gProgram = 0;
GLuint gVao = 0;
GLuint gVbo = 0;
GLint gViewProjectionLocation = -1;

/* 顶点着色器与片元着色器: 纯顶点色, 不做光照与纹理. */
const char* const kVertexShader = R"glsl(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProjection;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)glsl";

const char* const kFragmentShader = R"glsl(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 outColor;
void main() {
    outColor = vColor;
}
)glsl";

bool HasCurrentContext() {
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

GLuint CompileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint BuildProgram() {
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, kVertexShader);
    if (vertex == 0) {
        return 0;
    }
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

float NextUnit() {
    gRandom ^= gRandom << 13;
    gRandom ^= gRandom >> 17;
    gRandom ^= gRandom << 5;
    return static_cast<float>(gRandom & 0x00ffffffu) / 16777216.0f;
}

/* 取 -1 到 1 的对称随机量. */
float NextSigned() {
    return NextUnit() * 2.0f - 1.0f;
}

/* 往顶点缓冲写一个立方体的 36 个顶点, 每个顶点带位置与颜色; half 为半边长. */
int32_t WriteCube(const Particle& particle, float alpha, float half, float* out) {
    const float x0 = particle.x - half;
    const float x1 = particle.x + half;
    const float y0 = particle.y - half;
    const float y1 = particle.y + half;
    const float z0 = particle.z - half;
    const float z1 = particle.z + half;

    /* 六个面的四个角, 每面两个三角形. 顺序只影响绕序, 本通道关闭背面剔除. */
    const float corners[6][4][3] = {
        {{x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}},
        {{x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}},
        {{x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}},
        {{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}},
        {{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}},
        {{x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}},
    };
    static const int32_t kOrder[6] = {0, 1, 2, 0, 2, 3};

    int32_t written = 0;
    for (int32_t face = 0; face < 6; ++face) {
        for (int32_t index = 0; index < 6; ++index) {
            const int32_t corner = kOrder[index];
            float* vertex = out + static_cast<size_t>(written) * kFloatsPerVertex;
            vertex[0] = corners[face][corner][0];
            vertex[1] = corners[face][corner][1];
            vertex[2] = corners[face][corner][2];
            vertex[3] = particle.red;
            vertex[4] = particle.green;
            vertex[5] = particle.blue;
            vertex[6] = alpha;
            ++written;
        }
    }
    return written;
}

}  /* namespace */

bool Initialize() {
    if (gInitialized) {
        return true;
    }
    if (!HasCurrentContext()) {
        return false;
    }
    gProgram = BuildProgram();
    if (gProgram == 0) {
        IM_LOGE(kTag, "particle program unavailable");
        return false;
    }
    gViewProjectionLocation = glGetUniformLocation(gProgram, "uViewProjection");
    if (gViewProjectionLocation < 0) {
        IM_LOGE(kTag, "particle uniform missing");
        glDeleteProgram(gProgram);
        gProgram = 0;
        return false;
    }
    glGenVertexArrays(1, &gVao);
    glGenBuffers(1, &gVbo);
    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kMaxParticles) * kVerticesPerParticle * kFloatsPerVertex *
                     sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);
    const GLsizei stride = kFloatsPerVertex * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    gInitialized = true;
    IM_LOGI(kTag, "particle system ready, capacity=%d", kMaxParticles);
    return true;
}

void Shutdown() {
    if (!HasCurrentContext()) {
        gInitialized = false;
        gCount = 0;
        return;
    }
    if (gVbo != 0) {
        glDeleteBuffers(1, &gVbo);
        gVbo = 0;
    }
    if (gVao != 0) {
        glDeleteVertexArrays(1, &gVao);
        gVao = 0;
    }
    if (gProgram != 0) {
        glDeleteProgram(gProgram);
        gProgram = 0;
    }
    gInitialized = false;
    gCount = 0;
}

void EmitBlockBreak(voxel::BlockId block, float centerX, float centerY, float centerZ) {
    if (!gInitialized) {
        return;
    }
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    const int32_t slot = info.atlasTop == 0xFFFF ? static_cast<int32_t>(info.atlasIndex)
                                                 : static_cast<int32_t>(info.atlasTop);
    const voxel::gen::AtlasCellColor color = voxel::gen::AtlasColorOf(slot);
    const float red = static_cast<float>(color.red) / 255.0f;
    const float green = static_cast<float>(color.green) / 255.0f;
    const float blue = static_cast<float>(color.blue) / 255.0f;

    for (int32_t i = 0; i < kParticlesPerBreak; ++i) {
        if (gCount >= kMaxParticles) {
            /* 满了就覆盖最老的一个, 保证刚破坏的方块一定有碎屑. */
            gParticles[0] = gParticles[gCount - 1];
            gCount = kMaxParticles - 1;
        }
        Particle& particle = gParticles[gCount];
        particle.x = centerX + NextSigned() * 0.3f;
        particle.y = centerY + 0.1f + NextUnit() * 0.5f;
        particle.z = centerZ + NextSigned() * 0.3f;
        particle.vx = NextSigned() * kSpeedHorizontal;
        particle.vy = kSpeedUp * (0.5f + NextUnit() * 0.5f);
        particle.vz = NextSigned() * kSpeedHorizontal;
        particle.age = 0.0f;
        /* 每个碎片的颜色略作扰动, 一撮碎屑不会是一块死色. */
        const float jitter = 0.85f + NextUnit() * 0.3f;
        particle.red = red * jitter > 1.0f ? 1.0f : red * jitter;
        particle.green = green * jitter > 1.0f ? 1.0f : green * jitter;
        particle.blue = blue * jitter > 1.0f ? 1.0f : blue * jitter;
        ++gCount;
    }
    IM_LOGD(kTag, "emit block=%s color=%.2f,%.2f,%.2f active=%d", info.name,
            static_cast<double>(red), static_cast<double>(green), static_cast<double>(blue), gCount);
}

void EmitColored(float red, float green, float blue, float centerX, float centerY, float centerZ,
                 int32_t count) {
    if (!gInitialized || count <= 0) {
        return;
    }
    if (count > kParticlesPerBreak) {
        count = kParticlesPerBreak;
    }
    for (int32_t i = 0; i < count; ++i) {
        if (gCount >= kMaxParticles) {
            /* 满了就覆盖最老的一个, 与方块碎屑同一策略. */
            gParticles[0] = gParticles[gCount - 1];
            gCount = kMaxParticles - 1;
        }
        Particle& particle = gParticles[gCount];
        particle.x = centerX + NextSigned() * 0.35f;
        particle.y = centerY + NextUnit() * 0.6f;
        particle.z = centerZ + NextSigned() * 0.35f;
        particle.vx = NextSigned() * kSpeedHorizontal;
        particle.vy = kSpeedUp * (0.6f + NextUnit() * 0.6f);
        particle.vz = NextSigned() * kSpeedHorizontal;
        particle.age = 0.0f;
        const float jitter = 0.85f + NextUnit() * 0.3f;
        particle.red = red * jitter > 1.0f ? 1.0f : red * jitter;
        particle.green = green * jitter > 1.0f ? 1.0f : green * jitter;
        particle.blue = blue * jitter > 1.0f ? 1.0f : blue * jitter;
        ++gCount;
    }
}

void Update(double deltaSeconds) {
    if (!(deltaSeconds > 0.0)) {
        return;
    }
    /* 掉落物的浮动相位与碎屑生命周期无关, 单独推进. */
    gDropBobPhase += static_cast<float>(deltaSeconds) * 2.0f;
    if (gCount <= 0) {
        return;
    }
    const float step = static_cast<float>(deltaSeconds > 0.1 ? 0.1 : deltaSeconds);
    int32_t alive = 0;
    for (int32_t i = 0; i < gCount; ++i) {
        Particle& particle = gParticles[i];
        particle.age += step;
        if (particle.age >= kLifeSeconds) {
            continue;
        }
        particle.vy += kGravity * step;
        particle.x += particle.vx * step;
        particle.y += particle.vy * step;
        particle.z += particle.vz * step;
        gParticles[alive] = particle;
        ++alive;
    }
    gCount = alive;
}

void Draw(const float* viewProjection16) {
    if (!gInitialized || (gCount <= 0 && gDropCount <= 0) || viewProjection16 == nullptr ||
        !HasCurrentContext()) {
        return;
    }
    static float vertices[(kMaxParticles + kMaxDrops) * kVerticesPerParticle * kFloatsPerVertex];
    int32_t written = 0;
    for (int32_t i = 0; i < gCount; ++i) {
        const Particle& particle = gParticles[i];
        /* 后三分之一生命开始淡出, 前段保持不透明, 碎屑落地时不会突然消失. */
        const float remaining = 1.0f - particle.age / kLifeSeconds;
        const float alpha = remaining > 0.33f ? 1.0f : remaining / 0.33f;
        written += WriteCube(particle, alpha, kParticleSize * 0.5f,
                             vertices + static_cast<size_t>(written) * kFloatsPerVertex);
    }
    /* 掉落物: 上下浮动一点点, 不淡出, 由游戏层决定何时移除. */
    for (int32_t i = 0; i < gDropCount; ++i) {
        const Drop& drop = gDrops[i];
        Particle cube;
        cube.x = drop.x;
        cube.y = drop.y + std::sin(gDropBobPhase + static_cast<float>(i)) * 0.04f;
        cube.z = drop.z;
        cube.red = drop.red;
        cube.green = drop.green;
        cube.blue = drop.blue;
        written += WriteCube(cube, 1.0f, kDropSize * 0.5f,
                             vertices + static_cast<size_t>(written) * kFloatsPerVertex);
    }
    if (written == 0) {
        return;
    }

    glUseProgram(gProgram);
    glUniformMatrix4fv(gViewProjectionLocation, 1, GL_FALSE, viewProjection16);
    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(written) * kFloatsPerVertex * sizeof(float), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* 碎屑是半透明小件: 开混合, 关深度写入, 不写深度避免互相裁切. */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDrawArrays(GL_TRIANGLES, 0, written);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

int32_t Count() {
    return gCount;
}

void AddDrop(int32_t id, voxel::BlockId block, float x, float y, float z) {
    if (!gInitialized) {
        return;
    }
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(block);
    const int32_t slot = info.atlasTop == 0xFFFF ? static_cast<int32_t>(info.atlasIndex)
                                                 : static_cast<int32_t>(info.atlasTop);
    const voxel::gen::AtlasCellColor color = voxel::gen::AtlasColorOf(slot);
    /* 满了就覆盖最老的一条: 掉落物只是表现层, 丢失最老的不影响拾取逻辑. */
    if (gDropCount >= kMaxDrops) {
        for (int32_t i = 1; i < gDropCount; ++i) {
            gDrops[i - 1] = gDrops[i];
        }
        gDropCount = kMaxDrops - 1;
    }
    Drop& drop = gDrops[gDropCount];
    drop.id = id;
    drop.block = block;
    drop.x = x;
    drop.y = y;
    drop.z = z;
    drop.red = static_cast<float>(color.red) / 255.0f;
    drop.green = static_cast<float>(color.green) / 255.0f;
    drop.blue = static_cast<float>(color.blue) / 255.0f;
    ++gDropCount;
}

void MoveDrop(int32_t id, float x, float y, float z) {
    for (int32_t i = 0; i < gDropCount; ++i) {
        if (gDrops[i].id != id) {
            continue;
        }
        gDrops[i].x = x;
        gDrops[i].y = y;
        gDrops[i].z = z;
        return;
    }
}

void RemoveDrop(int32_t id) {
    for (int32_t i = 0; i < gDropCount; ++i) {
        if (gDrops[i].id != id) {
            continue;
        }
        gDrops[i] = gDrops[gDropCount - 1];
        gDropCount -= 1;
        return;
    }
}

void ClearDrops() {
    gDropCount = 0;
}

int32_t DropCount() {
    return gDropCount;
}

}  /* namespace particle */
}  /* namespace immirror::render */
