#include "render/creature_visual.h"

#include <cmath>
#include <cstring>
#include <string>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "core/log.h"
#include "entity/generated/monster_data_gen.h"
#include "entity/monster_table.h"

namespace immirror::render {
namespace creature {
namespace {

constexpr const char* kTag = "ImMirrorCreature";

/*
 * 顶点: 世界坐标 (3) + 顶点色 (3), 共 6 个 float. 无纹理无透明, 深度测试
 * 与区块共用, 面剔除按绕序取背面.
 */
constexpr int32_t kFloatsPerVertex = 6;
/* 单个立方体 12 个三角形. 每生物最多 6 个立方体, 生物上限取管理器容量 32. */
/* 每个生物最多 8 个盒子: 躯干, 头, 四肢各一, 再加一对眼睛. */
constexpr int32_t kBoxesPerCreature = 8;
constexpr int32_t kTrianglesPerBox = 12;
constexpr int32_t kVertsPerBox = kTrianglesPerBox * 3;
constexpr int32_t kMaxCreatures = 32;
constexpr int32_t kVertexCapacity = kMaxCreatures * kBoxesPerCreature * kVertsPerBox;

/* 单个立方体的局部几何: 以中心为原点, 半边长由部件给出, 变换在生成期烘焙. */
struct BoxInstance {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float centerZ = 0.0f;
    float halfX = 0.0f;
    float halfY = 0.0f;
    float halfZ = 0.0f;
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
};

/*
 * 每个定义的部件组: 直接存最终的世界局部几何 (以脚底中心为原点, 前向 -Z),
 * 生成一次后逐帧只做平移, 绕 Y 旋转与倒地倾斜.
 */
struct DefVisual {
    int32_t boxCount = 0;
    BoxInstance boxes[kBoxesPerCreature] = {};
    float baseRed = 0.0f;
    float baseGreen = 0.0f;
    float baseBlue = 0.0f;
};

/* 生成表条目数, 编译期从生成头取得. */
constexpr int32_t kDefCount = entity::gen::kMonsterDataCount;

DefVisual gDefVisuals[kDefCount];
bool gInitialized = false;
bool gFailed = false;

uint32_t gProgram = 0;
GLint gViewProjectionLocation = -1;
GLuint gVao = 0;
GLuint gVbo = 0;

/* 顶点色 mini 程序: 纯顶点色, 无纹理, 矩阵在顶点一次变换. */
const char* const kVertexSource = R"(#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 uViewProjection;
out vec3 vColor;
void main() {
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    vColor = aColor;
}
)";

const char* const kFragmentSource = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

/* 当前线程是否有可用的图形上下文, 上下文销毁后任何 GL 调用都是未定义行为. */
bool HasCurrentContext() {
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

/* 编译单个着色器, 失败返回 0 并打印驱动日志. */
uint32_t CompileShader(GLenum type, const char* source, const char* stage) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        IM_LOGE(kTag, "%s: glCreateShader failed", stage);
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(length > 0 ? length : 1), '\0');
        if (length > 0) {
            glGetShaderInfoLog(shader, length, nullptr, log.data());
        }
        IM_LOGE(kTag, "%s compile failed: %s", stage, log.c_str());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/* 32 位 FNV-1a, 与 progress 侧实现同族, 用于配色哈希. */
uint32_t HashId(uint32_t value) {
    uint32_t hash = 2166136261u;
    for (int32_t shift = 0; shift < 4; ++shift) {
        hash ^= (value >> (shift * 8)) & 0xFFu;
        hash *= 16777619u;
    }
    return hash;
}

/*
 * 类别基色: 按ID 哈希后在类别色域内取值, 保证同类生物色相相近,
 * 不同个体的明暗有差别, 视觉上可辨识又成族群.
 */
void BaseColorOf(uint32_t sourceId, uint8_t type, float* outRgb) {
    const uint32_t hash = HashId(sourceId);
    const float variation = static_cast<float>(hash % 1000u) / 1000.0f;
    switch (type) {
        case 0: /* 怪物: 暗红到暗紫域 */
            outRgb[0] = 0.55f + 0.15f * variation;
            outRgb[1] = 0.18f + 0.10f * variation;
            outRgb[2] = 0.20f + 0.18f * variation;
            break;
        case 1: /* 动物: 土黄到草绿域 */
            outRgb[0] = 0.62f + 0.15f * variation;
            outRgb[1] = 0.52f + 0.20f * variation;
            outRgb[2] = 0.25f + 0.10f * variation;
            break;
        case 2: /* 稀有动物: 蓝青域 */
            outRgb[0] = 0.20f + 0.10f * variation;
            outRgb[1] = 0.45f + 0.15f * variation;
            outRgb[2] = 0.65f + 0.20f * variation;
            break;
        case 3: /* 水生物: 深海蓝青域 */
            outRgb[0] = 0.15f + 0.10f * variation;
            outRgb[1] = 0.35f + 0.20f * variation;
            outRgb[2] = 0.55f + 0.25f * variation;
            break;
        case 6: /* boss: 黑金域 */
            outRgb[0] = 0.35f + 0.20f * variation;
            outRgb[1] = 0.28f + 0.10f * variation;
            outRgb[2] = 0.10f + 0.08f * variation;
            break;
        case 8: /* 飞行生物: 亮黄白域 */
            outRgb[0] = 0.85f + 0.10f * variation;
            outRgb[1] = 0.80f + 0.15f * variation;
            outRgb[2] = 0.40f + 0.20f * variation;
            break;
        default:
            outRgb[0] = 0.5f;
            outRgb[1] = 0.5f;
            outRgb[2] = 0.5f;
            break;
    }
    if (outRgb[0] > 1.0f) outRgb[0] = 1.0f;
    if (outRgb[1] > 1.0f) outRgb[1] = 1.0f;
    if (outRgb[2] > 1.0f) outRgb[2] = 1.0f;
}

void PushBox(DefVisual& visual, float cx, float cy, float cz,
             float hx, float hy, float hz, float shade) {
    if (visual.boxCount >= kBoxesPerCreature) {
        return;
    }
    BoxInstance& box = visual.boxes[visual.boxCount];
    box.centerX = cx;
    box.centerY = cy;
    box.centerZ = cz;
    box.halfX = hx;
    box.halfY = hy;
    box.halfZ = hz;
    box.red = visual.baseRed * shade;
    box.green = visual.baseGreen * shade;
    box.blue = visual.baseBlue * shade;
    visual.boxCount += 1;
}

/*
 * 独立着色的盒子: 眼睛这类部件不能跟随身体底色, 单独给色.
 * 与 PushBox 分开是为了让身体配色与部件配色各走各的路径, 互不干扰.
 */
void PushColoredBox(DefVisual& visual, float cx, float cy, float cz, float hx, float hy, float hz,
                    float red, float green, float blue) {
    if (visual.boxCount >= kBoxesPerCreature) {
        return;
    }
    BoxInstance& box = visual.boxes[visual.boxCount];
    box.centerX = cx;
    box.centerY = cy;
    box.centerZ = cz;
    box.halfX = hx;
    box.halfY = hy;
    box.halfZ = hz;
    box.red = red;
    box.green = green;
    box.blue = blue;
    visual.boxCount += 1;
}

/*
 * 一对眼睛: 贴在头部前表面 (-Z 为前向) 的左右两侧.
 * 敌对生物用发光的红黄, 其余用深色, 远看也能一眼分出敌我.
 */
void PushEyePair(DefVisual& visual, float headX, float headY, float faceZ, float headHalf,
                 bool hostile) {
    const float eyeHalf = headHalf * 0.22f;
    const float offsetX = headHalf * 0.45f;
    /* faceZ 是头部前表面的 z 坐标, 眼睛再往前贴半个眼厚. */
    const float eyeZ = faceZ - eyeHalf * 0.5f;
    float red = 0.10f;
    float green = 0.10f;
    float blue = 0.12f;
    if (hostile) {
        red = 1.0f;
        green = 0.35f;
        blue = 0.15f;
    }
    PushColoredBox(visual, headX - offsetX, headY, eyeZ, eyeHalf, eyeHalf, eyeHalf * 0.6f, red,
                   green, blue);
    PushColoredBox(visual, headX + offsetX, headY, eyeZ, eyeHalf, eyeHalf, eyeHalf * 0.6f, red,
                   green, blue);
}

/*
 * 按类别生成部件组合. 尺寸从 MonsterDef 折算:
 * 身高拆成头 (约四分之一) 与躯干 (约二分之一), 腿补足剩余;
 * 宽厚取 def 的宽度厚度, 部件略收让轮廓有分段感.
 */
void BuildDefVisual(int32_t index, const entity::MonsterDef& def) {
    DefVisual& visual = gDefVisuals[index];
    visual.boxCount = 0;
    /* 部件数上限按盒子容量收敛, 眼睛等新增部件不能把身体挤出容量. */
    BaseColorOf(def.sourceId, def.type, &visual.baseRed);

    const float width = def.width > 0.05f ? def.width : 0.3f;
    const float thickness = def.thickness > 0.05f ? def.thickness : 0.3f;
    const float height = def.height > 0.1f ? def.height : 0.5f;
    const float halfW = width * 0.5f * 0.8f;
    const float halfT = thickness * 0.5f * 0.8f;

    switch (def.type) {
        case 1: {
            /* 四足兽: 横放躯干 + 抬头 + 四条短腿, 头在前端略上. */
            const float bodyH = height * 0.45f;
            const float legH = height * 0.25f;
            const float bodyY = legH + bodyH * 0.5f;
            const float bodyL = thickness * 0.4f;
            PushBox(visual, 0.0f, bodyY, 0.0f, halfW, bodyH * 0.5f, bodyL, 1.0f);
            /* 头: 前端 ( -Z 为前向 ), 略高于躯干. */
            const float headY = bodyY + bodyH * 0.35f;
            const float headZ = -bodyL - halfW * 0.5f;
            const float headHalf = halfW * 0.7f;
            PushBox(visual, 0.0f, headY, headZ, headHalf, headHalf, headHalf, 1.1f);
            PushEyePair(visual, 0.0f, headY, headZ - headHalf, headHalf, def.hostile);
            /* 四肢: 前后各一对, 短柱. */
            const float legOffX = halfW * 0.6f;
            const float legOffZ = bodyL * 0.6f;
            PushBox(visual, -legOffX, legH * 0.5f, -legOffZ, halfW * 0.25f, legH * 0.5f, halfW * 0.25f, 0.8f);
            PushBox(visual, legOffX, legH * 0.5f, -legOffZ, halfW * 0.25f, legH * 0.5f, halfW * 0.25f, 0.8f);
            PushBox(visual, -legOffX, legH * 0.5f, legOffZ, halfW * 0.25f, legH * 0.5f, halfW * 0.25f, 0.8f);
            PushBox(visual, legOffX, legH * 0.5f, legOffZ, halfW * 0.25f, legH * 0.5f, halfW * 0.25f, 0.8f);
            break;
        }
        case 3: {
            /* 水生物: 前后两段纺锤体 + 尾鳍, 整体横放. */
            const float bodyH = height * 0.5f;
            PushBox(visual, 0.0f, height * 0.55f, -thickness * 0.2f,
                    halfW, bodyH * 0.5f, thickness * 0.35f, 1.0f);
            PushBox(visual, 0.0f, height * 0.55f, thickness * 0.3f,
                    halfW * 0.7f, bodyH * 0.4f, thickness * 0.25f, 0.9f);
            /* 尾鳍: 薄板. */
            PushBox(visual, 0.0f, height * 0.55f, thickness * 0.55f,
                    halfW * 0.5f, bodyH * 0.35f, thickness * 0.06f, 0.8f);
            break;
        }
        case 8: {
            /* 飞行生物: 小身 + 双翼薄板, 身体整体抬高悬浮. */
            const float bodyY = height * 0.65f;
            const float flyHalf = halfW * 0.7f;
            PushBox(visual, 0.0f, bodyY, 0.0f, flyHalf, flyHalf, halfT * 0.7f, 1.0f);
            PushEyePair(visual, 0.0f, bodyY, -halfT * 0.7f, flyHalf, def.hostile);
            PushBox(visual, -halfW * 1.6f, bodyY + height * 0.05f, 0.0f,
                    halfW * 0.9f, halfW * 0.08f, halfT * 0.5f, 0.9f);
            PushBox(visual, halfW * 1.6f, bodyY + height * 0.05f, 0.0f,
                    halfW * 0.9f, halfW * 0.08f, halfT * 0.5f, 0.9f);
            break;
        }
        default: {
            /* 直立人形 (怪物/稀有/boss): 头 + 躯干 + 双腿, boss 加双臂. */
            const float headH = height * 0.24f;
            const float torsoH = height * 0.46f;
            const float legH = height - headH - torsoH;
            const float legY = legH * 0.5f;
            const float torsoY = legH + torsoH * 0.5f;
            const float headY = legH + torsoH + headH * 0.5f;
            PushBox(visual, 0.0f, torsoY, 0.0f, halfW, torsoH * 0.5f, halfT, 1.0f);
            const float faceHalf = halfT * 0.85f;
            PushBox(visual, 0.0f, headY, 0.0f, halfW * 0.85f, headH * 0.5f, faceHalf, 1.15f);
            /* 人形的眼睛贴在头部前表面, 前向为 -Z. */
            PushEyePair(visual, 0.0f, headY, -faceHalf, halfW * 0.85f, def.hostile);
            const float legOffX = halfW * 0.55f;
            PushBox(visual, -legOffX, legY, 0.0f, halfW * 0.35f, legH * 0.5f, halfT * 0.8f, 0.75f);
            PushBox(visual, legOffX, legY, 0.0f, halfW * 0.35f, legH * 0.5f, halfT * 0.8f, 0.75f);
            if (def.type == 6 && visual.boxCount < kBoxesPerCreature - 1) {
                /* boss 双臂: 躯干两侧长条. */
                PushBox(visual, -halfW * 1.3f, torsoY + torsoH * 0.1f, 0.0f,
                        halfW * 0.3f, torsoH * 0.55f, halfT * 0.6f, 0.9f);
                PushBox(visual, halfW * 1.3f, torsoY + torsoH * 0.1f, 0.0f,
                        halfW * 0.3f, torsoH * 0.55f, halfT * 0.6f, 0.9f);
            }
            break;
        }
    }
}

/* 把一个立方体的 12 个三角形按变换写入顶点流. 旋转只含绕 Y 与倒地绕 X 两级. */
int32_t WriteCreature(const CreatureView& view, const DefVisual& visual, float* out) {
    const float cosYaw = std::cos(view.yaw);
    const float sinYaw = std::sin(view.yaw);
    /* 死亡倒地: 绕横轴渐变到 90 度, 从竖直站立倒向前方. */
    const float fallAngle = view.death * 1.5707963f;
    const float cosFall = std::cos(fallAngle);
    const float sinFall = std::sin(fallAngle);
    /* 受击闪白: 颜色向白色线性提亮, 亮度上限 2 倍在 shader 外钳制. */
    const float brighten = view.hurt > 0.0f ? (view.hurt > 1.0f ? 1.0f : view.hurt) : 0.0f;

    int32_t written = 0;
    for (int32_t b = 0; b < visual.boxCount; ++b) {
        const BoxInstance& box = visual.boxes[b];
        /* 立方体 8 个顶点, 逐面写成 12 个三角形; 顶点级旋转在循环内做. */
        const float lx[2] = {-box.halfX, box.halfX};
        const float ly[2] = {-box.halfY, box.halfY};
        const float lz[2] = {-box.halfZ, box.halfZ};
        /* 面表: 每面 4 个角点的 (x,y,z) 分量索引, 按逆时针绕序拆两三角. */
        const int32_t faces[6][4][3] = {
            {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}, /* -Z 前 */
            {{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}, /* +Z 后 */
            {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}}, /* -X 左 */
            {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}, /* +X 右 */
            {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, /* +Y 顶 */
            {{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}}, /* -Y 底 */
        };
        const float faceShade[6] = {0.85f, 0.85f, 0.7f, 0.7f, 1.0f, 0.55f};
        for (int32_t f = 0; f < 6; ++f) {
            const int32_t cornerOrder[6] = {0, 1, 2, 0, 2, 3};
            for (int32_t v = 0; v < 6; ++v) {
                const int32_t* corner = faces[f][cornerOrder[v]];
                const float px = lx[corner[0]];
                const float py = ly[corner[1]];
                const float pz = lz[corner[2]];
                /* 倒地: 局部先绕 X 轴 (过脚底的横轴) 放平. */
                const float rx = px;
                const float ry = py * cosFall - pz * sinFall;
                const float rz = py * sinFall + pz * cosFall;
                /* 朝向: 绕 Y 轴旋转, 与实体 yaw 同义 (-Z 为前向). */
                const float wx = rx * cosYaw + rz * sinYaw;
                const float wy = ry;
                const float wz = -rx * sinYaw + rz * cosYaw;
                float* target = out + written * kFloatsPerVertex;
                target[0] = view.x + wx;
                target[1] = view.y + wy;
                target[2] = view.z + wz;
                /* 亮度 = 面明暗 * 受击提亮, 向 1 混合. */
                const float shade = faceShade[f];
                target[3] = box.red * shade + (1.0f - shade) * brighten;
                target[4] = box.green * shade + (1.0f - shade) * brighten;
                target[5] = box.blue * shade + (1.0f - shade) * brighten;
                written += 1;
            }
        }
    }
    return written;
}

}  /* namespace */

bool Initialize() {
    if (gInitialized || gFailed) {
        return gInitialized;
    }
    if (!HasCurrentContext()) {
        return false;
    }

    /* 几何预生成: 全部定义的部件组一次性烘焙, 运行期零生成开销. */
    const entity::MonsterTable& table = entity::MonsterTable::Instance();
    for (int32_t i = 0; i < kDefCount; ++i) {
        BuildDefVisual(i, table.Def(i));
    }

    const uint32_t vertex = CompileShader(GL_VERTEX_SHADER, kVertexSource, "creature vertex");
    if (vertex == 0) {
        gFailed = true;
        return false;
    }
    const uint32_t fragment = CompileShader(GL_FRAGMENT_SHADER, kFragmentSource, "creature fragment");
    if (fragment == 0) {
        glDeleteShader(vertex);
        gFailed = true;
        return false;
    }
    gProgram = glCreateProgram();
    glAttachShader(gProgram, vertex);
    glAttachShader(gProgram, fragment);
    glLinkProgram(gProgram);
    glDetachShader(gProgram, vertex);
    glDetachShader(gProgram, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = GL_FALSE;
    glGetProgramiv(gProgram, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        IM_LOGE(kTag, "creature program link failed");
        glDeleteProgram(gProgram);
        gProgram = 0;
        gFailed = true;
        return false;
    }
    gViewProjectionLocation = glGetUniformLocation(gProgram, "uViewProjection");
    glGenVertexArrays(1, &gVao);
    glGenBuffers(1, &gVbo);
    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferData(GL_ARRAY_BUFFER, kVertexCapacity * kFloatsPerVertex * sizeof(float), nullptr,
                 GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kFloatsPerVertex * sizeof(float),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    gInitialized = true;
    IM_LOGI(kTag, "creature visuals ready, defs=%d", kDefCount);
    return true;
}

void Shutdown() {
    if (!gInitialized) {
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
    gViewProjectionLocation = -1;
    gInitialized = false;
}

void Draw(const CreatureView* views, int32_t count, const float* viewProjection16) {
    if (!gInitialized || views == nullptr || viewProjection16 == nullptr || count <= 0) {
        return;
    }
    if (!HasCurrentContext()) {
        return;
    }
    static float vertices[kVertexCapacity * kFloatsPerVertex];
    int32_t written = 0;
    for (int32_t i = 0; i < count && written + kBoxesPerCreature * kVertsPerBox <= kVertexCapacity; ++i) {
        if (views[i].defIndex < 0 || views[i].defIndex >= kDefCount) {
            continue;
        }
        /* 视锥粗剔除: 相机后方的个体直接跳过. */
        const float clipW = viewProjection16[3] * views[i].x + viewProjection16[7] * views[i].y +
                            viewProjection16[11] * views[i].z + viewProjection16[15];
        if (clipW <= 0.1f) {
            continue;
        }
        written += WriteCreature(views[i], gDefVisuals[views[i].defIndex],
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
    /* 深度测试常开与区块一致; 不透明小件不做混合, 面剔除减一半片元. */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDrawArrays(GL_TRIANGLES, 0, written);
    glBindVertexArray(0);
    glDisable(GL_CULL_FACE);
}

int32_t DefCount() {
    return kDefCount;
}

}  /* namespace creature */

}  /* namespace immirror::render */
