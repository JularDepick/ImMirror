#include "render/chunk_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

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
/* 位置 3 为群系染色 vec3: 草方块顶面与树叶按群系草色乘上去. */
constexpr GLuint kAttributeTint = 3;

constexpr GLsizei kVertexStride = static_cast<GLsizei>(sizeof(voxel::Vertex));
constexpr size_t kPositionOffset = 0;
constexpr size_t kTexCoordOffset = sizeof(float) * 3;
constexpr size_t kShadeOffset = sizeof(float) * 5;
constexpr size_t kTintOffset = sizeof(float) * 6;

/*
 * 雾色常量, 取值与 gles_device.cpp 的清屏底色一致 (即天空色):
 * 远处地形融进雾里时与背景无缝衔接, 不出现雾色与天空色的断层.
 */
constexpr float kFogRed = 0.07f;
constexpr float kFogGreen = 0.12f;
constexpr float kFogBlue = 0.20f;

/* 顶点布局是渲染与网格构建之间的硬约定, 这里的断言用于尽早发现接口漂移. */
static_assert(sizeof(voxel::Vertex) == sizeof(float) * 9, "顶点结构必须为 9 个 float, 共 36 字节");
static_assert(sizeof(voxel::Vertex) == 36, "顶点步长必须为 36 字节");

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
layout(location = 3) in vec3 aTint;

uniform mat4 uViewProjection;
uniform vec3 uChunkOrigin;
uniform vec3 uCameraPosition;

out vec2 vTexCoord;
out float vShade;
out vec3 vTint;
out float vViewDistance;

void main() {
    vTexCoord = aTexCoord;
    vShade = aShade;
    vTint = aTint;
    /* 网格顶点是区块局部坐标, 逐个区块加上世界原点后再变换. */
    vec3 worldPosition = aPosition + uChunkOrigin;
    /* 视距在顶点算好传给片元, 片元只做雾混合, 避免逐片元求长度. */
    vViewDistance = length(worldPosition - uCameraPosition);
    gl_Position = uViewProjection * vec4(worldPosition, 1.0);
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
in vec3 vTint;
in float vViewDistance;

uniform sampler2D uAtlas;
uniform float uFogDensity;
uniform vec3 uFogColor;
uniform float uBrightness;
/* 天光混合因子: 夜 0, 昼 1, 晨昏线性过渡; 只压天空贡献, 方块光不在本 shader 内. */
uniform float uSkyFactor;

out vec4 outColor;

/* 只丢弃完全透明的像素, 阈值取在 alpha 量化噪声之上. */
const float kAlphaCutoff = 0.1;

void main() {
    vec4 texel = texture(uAtlas, vTexCoord);
    if (texel.a < kAlphaCutoff) {
        discard;
    }
    /* 群系染色: 草方块顶面与树叶带色, 其余顶点为白色即不改变颜色. */
    vec3 color = texel.rgb * vTint * vShade * mix(0.27, 1.0, clamp(uSkyFactor, 0.0, 1.0));
    /* 指数平方雾: 密度为 0 时雾因子恒为 0, 关雾时不产生混合开销. */
    float fogFactor = 1.0 - exp(-uFogDensity * uFogDensity * vViewDistance * vViewDistance);
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 1.0));
    /* 亮度系数乘在最终颜色上, 滑杆 50 对应 1.0 即原色. */
    outColor = vec4(color * uBrightness, texel.a);
}
)glsl";

/*
 * 天空渐变着色器. 顶点由 gl_VertexID 直接生成全屏三角带, 不需要缓冲对象;
 * 片元按屏幕竖直位置在天顶与地平线两色之间过渡, 再按天光因子在昼夜两套配色间混合.
 * 天空只提供背景, 因此不写深度也不做雾混合, 区块与生物随后覆盖其上.
 */
const char* const kSkyVertexShaderSource = R"glsl(#version 300 es
uniform mat4 uInvViewProjection;

out vec3 vFarPoint;

void main() {
    /*
     * 全屏三角带的四个顶点, 每点反投影到远平面得到世界坐标.
     * 远平面点在屏幕空间是线性插值的, 片元里减去相机位置就得到视线方向,
     * 因此太阳与云层都按真实方向计算, 不受视口比例影响.
     */
    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : -1.0,
                       (gl_VertexID >= 2) ? 1.0 : -1.0);
    vec4 farPoint = uInvViewProjection * vec4(corner, 1.0, 1.0);
    vFarPoint = farPoint.xyz / farPoint.w;
    gl_Position = vec4(corner, 0.0, 1.0);
}
)glsl";

const char* const kSkyFragmentShaderSource = R"glsl(#version 300 es
/*
 * 天空片元用高精度: 星空按视线方向量化成格子取哈希, 中精度在 260 倍放大量级
 * 上分辨不出单颗星, 会退化成整片亮斑. GLES 3.0 起片元着色器必须支持 highp.
 */
precision highp float;

/* 昼夜两套天空配色: 天顶与地平线各一组, 由 uSkyFactor 线性混合. */
const vec3 kDayTop = vec3(0.38, 0.62, 0.92);
const vec3 kDayHorizon = vec3(0.72, 0.87, 0.98);
const vec3 kNightTop = vec3(0.03, 0.05, 0.12);
const vec3 kNightHorizon = vec3(0.09, 0.12, 0.22);

uniform float uSkyFactor;
uniform float uViewportHeight;
uniform vec3 uSunDirection;
uniform vec3 uCameraPosition;
uniform float uSkyTime;
/* 降水强度 0 到 1: 天色压暗并压掉太阳, 云的覆盖度不变. */
uniform float uWeatherIntensity;

in vec3 vFarPoint;

out vec4 outColor;

/* 二维值噪声与四层叠加, 云层用; 采样完全由坐标决定, 不依赖纹理. */
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float valueNoise(vec2 p) {
    vec2 cell = floor(p);
    vec2 weight = fract(p);
    weight = weight * weight * (3.0 - 2.0 * weight);
    float v00 = hash(cell);
    float v10 = hash(cell + vec2(1.0, 0.0));
    float v01 = hash(cell + vec2(0.0, 1.0));
    float v11 = hash(cell + vec2(1.0, 1.0));
    return mix(mix(v00, v10, weight.x), mix(v01, v11, weight.x), weight.y);
}

float cloudDensity(vec2 p) {
    float sum = 0.0;
    float amplitude = 0.55;
    float frequency = 1.0;
    for (int octave = 0; octave < 4; ++octave) {
        sum += valueNoise(p * frequency) * amplitude;
        frequency *= 2.03;
        amplitude *= 0.5;
    }
    return sum;
}

void main() {
    /*
     * 天空底色: gl_FragCoord 原点在左下, y 越大越接近天顶; 开平方让地平线附近
     * 的色彩变化更慢. 昼夜两套配色按天光因子混合.
     */
    float height = clamp(gl_FragCoord.y / max(uViewportHeight, 1.0), 0.0, 1.0);
    float curve = sqrt(height);
    float factor = clamp(uSkyFactor, 0.0, 1.0);
    vec3 top = mix(kNightTop, kDayTop, factor);
    vec3 horizon = mix(kNightHorizon, kDayHorizon, factor);
    vec3 color = mix(horizon, top, curve);

    /* 视线方向: 远平面点减去相机位置; 地平线以下不再画天体. */
    vec3 direction = normalize(vFarPoint - uCameraPosition);
    float up = direction.y;

    /*
     * 云层: 视线与云平面相交后取二维噪声, 按时间平移形成缓慢漂移;
     * 只在地平线以上绘制, 靠近地平线时淡出, 避免出现硬边.
     */
    if (up > 0.015) {
        float planeDistance = (1200.0 - uCameraPosition.y) / max(up, 0.015);
        vec2 cloudPoint = (uCameraPosition.xz + direction.xz * planeDistance) * 0.0045;
        cloudPoint += vec2(uSkyTime * 0.006, uSkyTime * 0.0035);
        float density = cloudDensity(cloudPoint);
        float coverage = smoothstep(0.42, 0.72, density);
        float fade = smoothstep(0.015, 0.18, up);
        vec3 cloudColor = mix(vec3(0.62, 0.66, 0.72), vec3(1.0, 0.99, 0.96), factor);
        color = mix(color, cloudColor, coverage * fade * 0.85);
    }

    /*
     * 月亮: 与太阳反方向的盘面, 夜间可见, 带一圈很淡的光晕.
     * 降水时按强度压掉, 阴天不该看见月亮.
     */
    if (up > 0.0) {
        float moonDot = dot(direction, -normalize(uSunDirection));
        float moonDisc = smoothstep(0.9970, 0.9988, moonDot);
        float moonHalo = pow(max(moonDot, 0.0), 220.0) * 0.20;
        float moonVisible = (1.0 - factor) * (1.0 - clamp(uWeatherIntensity, 0.0, 1.0));
        color = mix(color, vec3(0.88, 0.91, 0.96), (moonDisc + moonHalo) * moonVisible);
    }

    /*
     * 星空: 把视线方向量化成格子后取哈希, 少数格子点亮成星.
     * 只用方向决定, 因此转动视角时星点固定不闪烁; 夜间且高处才可见.
     */
    float night = 1.0 - factor;
    if (night > 0.12 && up > 0.02) {
        vec3 cell = floor(direction * 260.0);
        float seedValue = hash(vec2(cell.x + cell.z * 31.0, cell.y - cell.z * 17.0));
        float star = smoothstep(0.9972, 0.9995, seedValue);
        float skyClear = 1.0 - clamp(uWeatherIntensity, 0.0, 1.0);
        color += vec3(0.86, 0.90, 1.0) * star * night * skyClear * smoothstep(0.02, 0.25, up);
    }

    /* 太阳: 实心盘面加热晕, 夜间由天光因子压掉, 落到地平线以下不画. */
    float sunDot = dot(direction, normalize(uSunDirection));
    float sunDisc = smoothstep(0.9985, 0.9995, sunDot);
    float sunHalo = pow(max(sunDot, 0.0), 64.0) * 0.35;
    float sunVisible = clamp(uSunDirection.y * 4.0 + 0.25, 0.0, 1.0) * factor;
    color += vec3(1.0, 0.94, 0.78) * (sunDisc * 1.0 + sunHalo) * sunVisible;

    /* 雨雪天压暗并去饱和: 阴天没有蓝天, 太阳也被云挡住. */
    float storm = clamp(uWeatherIntensity, 0.0, 1.0);
    float grey = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(color, vec3(grey) * 0.72, storm * 0.85);

    outColor = vec4(color, 1.0);
}
)glsl";

/*
 * 降水叠加层着色器.
 * 顶点由 gl_VertexID 生成全屏三角带; 片元在屏幕空间画斜向雨线或飘雪点.
 * 雨与雪都按强度放大密度, 颜色偏冷白, 混合在半透明通道里完成.
 */
const char* const kWeatherVertexShaderSource = R"glsl(#version 300 es
void main() {
    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : -1.0,
                       (gl_VertexID >= 2) ? 1.0 : -1.0);
    gl_Position = vec4(corner, 0.0, 1.0);
}
)glsl";

const char* const kWeatherFragmentShaderSource = R"glsl(#version 300 es
precision mediump float;

uniform vec2 uViewportSize;
uniform float uAspect;
uniform float uTime;
uniform float uIntensity;
/* 0 雨, 1 雪; 雷暴与雨同一形态, 只提高密度. */
uniform float uSnow;

out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 screen = gl_FragCoord.xy / max(uViewportSize, vec2(1.0));
    vec2 p = vec2(screen.x * uAspect, screen.y);
    float storm = clamp(uIntensity, 0.0, 1.0);
    if (storm <= 0.001) {
        discard;
    }
    float alpha = 0.0;
    if (uSnow < 0.5) {
        /* 雨: 斜向细线, 每条线按列哈希决定是否出现, 随时间向下滚动. */
        float column = floor((p.x * 90.0 + p.y * 26.0));
        float phase = fract((p.y + uTime * 1.9) * 9.0 + hash(vec2(column, 3.0)) * 5.0);
        float present = step(0.72, hash(vec2(column, 7.0)));
        alpha = present * smoothstep(0.0, 0.10, phase) * smoothstep(0.55, 0.15, phase);
    } else {
        /* 雪: 稀疏漂点, 横向按正弦摆动, 下落更慢. */
        vec2 cell = floor(vec2(p.x * 70.0 + sin(uTime * 0.7 + p.y * 12.0) * 0.6,
                               p.y * 45.0 + uTime * 0.35));
        float present = step(0.86, hash(cell));
        float phase = fract(p.y * 45.0 + uTime * 0.35);
        alpha = present * smoothstep(0.0, 0.2, phase) * smoothstep(1.0, 0.6, phase);
    }
    if (alpha <= 0.001) {
        discard;
    }
    vec3 tint = uSnow < 0.5 ? vec3(0.72, 0.80, 0.92) : vec3(0.96, 0.98, 1.0);
    outColor = vec4(tint, alpha * storm * (uSnow < 0.5 ? 0.42 : 0.55));
}
)glsl";

/*
 * 水下滤镜着色器: 全屏三角带加一个纯色片元, 由 uColor 决定颜色与强度.
 * 顶点与降水层同一套写法, 不需要缓冲对象.
 */
const char* const kTintVertexShaderSource = R"glsl(#version 300 es
void main() {
    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : -1.0,
                       (gl_VertexID >= 2) ? 1.0 : -1.0);
    gl_Position = vec4(corner, 0.0, 1.0);
}
)glsl";

const char* const kTintFragmentShaderSource = R"glsl(#version 300 es
precision mediump float;

uniform vec4 uColor;

out vec4 outColor;

void main() {
    outColor = uColor;
}
)glsl";

/*
 * 列主序 4x4 矩阵求逆, 供天空通道把片元反投影成世界视线.
 * 视投影矩阵在场景里恒可逆 (近远平面有效且相机不退化), 不可逆时返回 false,
 * 调用方保留上一帧的逆矩阵, 天空退化为不随视角变化, 不影响地形绘制.
 */
bool InvertMatrix4(const float* source, float* out) {
    const float a00 = source[0], a01 = source[1], a02 = source[2], a03 = source[3];
    const float a10 = source[4], a11 = source[5], a12 = source[6], a13 = source[7];
    const float a20 = source[8], a21 = source[9], a22 = source[10], a23 = source[11];
    const float a30 = source[12], a31 = source[13], a32 = source[14], a33 = source[15];

    const float b00 = a00 * a11 - a01 * a10;
    const float b01 = a00 * a12 - a02 * a10;
    const float b02 = a00 * a13 - a03 * a10;
    const float b03 = a01 * a12 - a02 * a11;
    const float b04 = a01 * a13 - a03 * a11;
    const float b05 = a02 * a13 - a03 * a12;
    const float b06 = a20 * a31 - a21 * a30;
    const float b07 = a20 * a32 - a22 * a30;
    const float b08 = a20 * a33 - a23 * a30;
    const float b09 = a21 * a32 - a22 * a31;
    const float b10 = a21 * a33 - a23 * a31;
    const float b11 = a22 * a33 - a23 * a32;

    const float determinant = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;
    if (determinant > -1.0e-12f && determinant < 1.0e-12f) {
        return false;
    }
    const float inverse = 1.0f / determinant;

    out[0] = (a11 * b11 - a12 * b10 + a13 * b09) * inverse;
    out[1] = (a02 * b10 - a01 * b11 - a03 * b09) * inverse;
    out[2] = (a31 * b05 - a32 * b04 + a33 * b03) * inverse;
    out[3] = (a22 * b04 - a21 * b05 - a23 * b03) * inverse;
    out[4] = (a12 * b08 - a10 * b11 - a13 * b07) * inverse;
    out[5] = (a00 * b11 - a02 * b08 + a03 * b07) * inverse;
    out[6] = (a32 * b02 - a30 * b05 - a33 * b01) * inverse;
    out[7] = (a20 * b05 - a22 * b02 + a23 * b01) * inverse;
    out[8] = (a10 * b10 - a11 * b08 + a13 * b06) * inverse;
    out[9] = (a01 * b08 - a00 * b10 - a03 * b06) * inverse;
    out[10] = (a30 * b04 - a31 * b02 + a33 * b00) * inverse;
    out[11] = (a21 * b02 - a20 * b04 - a23 * b00) * inverse;
    out[12] = (a11 * b07 - a10 * b09 - a12 * b06) * inverse;
    out[13] = (a00 * b09 - a01 * b07 + a02 * b06) * inverse;
    out[14] = (a31 * b01 - a30 * b03 - a32 * b00) * inverse;
    out[15] = (a20 * b03 - a21 * b01 + a22 * b00) * inverse;
    return true;
}

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
    glEnableVertexAttribArray(kAttributeTint);
    glVertexAttribPointer(kAttributeTint, 3, GL_FLOAT, GL_FALSE, kVertexStride,
                          reinterpret_cast<const void*>(kTintOffset));
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
                    GLsizei* outIndexCount, GLsizei* outWaterBegin, GLsizei* outWaterCount) {
    *outIndexCount = 0;
    *outWaterBegin = 0;
    *outWaterCount = 0;
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
    *outWaterBegin = static_cast<GLsizei>(mesh.waterIndexBegin);
    *outWaterCount = static_cast<GLsizei>(mesh.waterIndexCount);
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
    GLsizei waterIndexBegin = 0; /* 半透明段起点, 与 indexCount 同单位 (索引个数) */
    GLsizei waterIndexCount = 0; /* 半透明段索引个数, 0 表示该区块没有液体面 */
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

    /* 天空渐变是可选通道: 编译失败只丢背景, 不影响地形绘制. */
    {
        const GLuint skyVertex = CompileShader(GL_VERTEX_SHADER, kSkyVertexShaderSource, "sky vertex shader");
        const GLuint skyFragment =
            skyVertex != 0 ? CompileShader(GL_FRAGMENT_SHADER, kSkyFragmentShaderSource, "sky fragment shader") : 0;
        if (skyVertex != 0 && skyFragment != 0) {
            const GLuint skyProgram = glCreateProgram();
            if (skyProgram != 0) {
                glAttachShader(skyProgram, skyVertex);
                glAttachShader(skyProgram, skyFragment);
                glLinkProgram(skyProgram);
                GLint skyLinked = GL_FALSE;
                glGetProgramiv(skyProgram, GL_LINK_STATUS, &skyLinked);
                if (skyLinked == GL_TRUE) {
                    skyProgram_ = skyProgram;
                    uniformSkyBlend_ = glGetUniformLocation(skyProgram_, "uSkyFactor");
                    uniformSkyViewHeight_ = glGetUniformLocation(skyProgram_, "uViewportHeight");
                    uniformSkyInvViewProjection_ = glGetUniformLocation(skyProgram_, "uInvViewProjection");
                    uniformSkySun_ = glGetUniformLocation(skyProgram_, "uSunDirection");
                    uniformSkyCamera_ = glGetUniformLocation(skyProgram_, "uCameraPosition");
                    uniformSkyTime_ = glGetUniformLocation(skyProgram_, "uSkyTime");
                    uniformSkyWeather_ = glGetUniformLocation(skyProgram_, "uWeatherIntensity");
                } else {
                    LogObjectInfo("sky program link", skyProgram, true);
                    glDeleteProgram(skyProgram);
                }
            }
        }
        if (skyVertex != 0) {
            glDeleteShader(skyVertex);
        }
        if (skyFragment != 0) {
            glDeleteShader(skyFragment);
        }
        if (skyProgram_ == 0) {
            IM_LOGE(kTag, "sky gradient unavailable, background stays a flat clear color");
        }
    }

    /* 降水叠加层同样可选: 失败时只丢雨雪特效, 天气状态仍然推进. */
    {
        const GLuint weatherVertex =
            CompileShader(GL_VERTEX_SHADER, kWeatherVertexShaderSource, "weather vertex shader");
        const GLuint weatherFragment =
            weatherVertex != 0
                ? CompileShader(GL_FRAGMENT_SHADER, kWeatherFragmentShaderSource, "weather fragment shader")
                : 0;
        if (weatherVertex != 0 && weatherFragment != 0) {
            const GLuint weatherProgram = glCreateProgram();
            if (weatherProgram != 0) {
                glAttachShader(weatherProgram, weatherVertex);
                glAttachShader(weatherProgram, weatherFragment);
                glLinkProgram(weatherProgram);
                GLint linked = GL_FALSE;
                glGetProgramiv(weatherProgram, GL_LINK_STATUS, &linked);
                if (linked == GL_TRUE) {
                    weatherProgram_ = weatherProgram;
                    uniformWeatherIntensity_ = glGetUniformLocation(weatherProgram_, "uIntensity");
                    uniformWeatherKind_ = glGetUniformLocation(weatherProgram_, "uSnow");
                    uniformWeatherTime_ = glGetUniformLocation(weatherProgram_, "uTime");
                    uniformWeatherViewport_ = glGetUniformLocation(weatherProgram_, "uViewportSize");
                    uniformWeatherAspect_ = glGetUniformLocation(weatherProgram_, "uAspect");
                } else {
                    LogObjectInfo("weather program link", weatherProgram, true);
                    glDeleteProgram(weatherProgram);
                }
            }
        }
        if (weatherVertex != 0) {
            glDeleteShader(weatherVertex);
        }
        if (weatherFragment != 0) {
            glDeleteShader(weatherFragment);
        }
        if (weatherProgram_ == 0) {
            IM_LOGE(kTag, "precipitation layer unavailable, weather will not be visible");
        }
    }

    /* 水下滤镜同样可选: 失败时只丢滤镜, 水下仍然能玩. */
    {
        const GLuint tintVertex =
            CompileShader(GL_VERTEX_SHADER, kTintVertexShaderSource, "tint vertex shader");
        const GLuint tintFragment =
            tintVertex != 0
                ? CompileShader(GL_FRAGMENT_SHADER, kTintFragmentShaderSource, "tint fragment shader")
                : 0;
        if (tintVertex != 0 && tintFragment != 0) {
            const GLuint tintProgram = glCreateProgram();
            if (tintProgram != 0) {
                glAttachShader(tintProgram, tintVertex);
                glAttachShader(tintProgram, tintFragment);
                glLinkProgram(tintProgram);
                GLint linked = GL_FALSE;
                glGetProgramiv(tintProgram, GL_LINK_STATUS, &linked);
                if (linked == GL_TRUE) {
                    tintProgram_ = tintProgram;
                    uniformTintColor_ = glGetUniformLocation(tintProgram_, "uColor");
                } else {
                    LogObjectInfo("tint program link", tintProgram, true);
                    glDeleteProgram(tintProgram);
                }
            }
        }
        if (tintVertex != 0) {
            glDeleteShader(tintVertex);
        }
        if (tintFragment != 0) {
            glDeleteShader(tintFragment);
        }
    }

    uniformViewProjection_ = glGetUniformLocation(program_, "uViewProjection");
    uniformAtlas_ = glGetUniformLocation(program_, "uAtlas");
    uniformChunkOrigin_ = glGetUniformLocation(program_, "uChunkOrigin");
    uniformCameraPosition_ = glGetUniformLocation(program_, "uCameraPosition");
    uniformFogDensity_ = glGetUniformLocation(program_, "uFogDensity");
    uniformFogColor_ = glGetUniformLocation(program_, "uFogColor");
    uniformBrightness_ = glGetUniformLocation(program_, "uBrightness");
    uniformSkyFactor_ = glGetUniformLocation(program_, "uSkyFactor");
    if (uniformViewProjection_ < 0 || uniformAtlas_ < 0 || uniformChunkOrigin_ < 0 ||
        uniformCameraPosition_ < 0 || uniformFogDensity_ < 0 || uniformFogColor_ < 0 ||
        uniformBrightness_ < 0) {
        IM_LOGE(kTag, "shader uniform missing, vp=%d atlas=%d origin=%d cam=%d fog=%d color=%d bright=%d",
                uniformViewProjection_, uniformAtlas_, uniformChunkOrigin_, uniformCameraPosition_,
                uniformFogDensity_, uniformFogColor_, uniformBrightness_, uniformSkyFactor_);
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
        if (weatherProgram_ != 0) {
            glDeleteProgram(weatherProgram_);
        }
        if (tintProgram_ != 0) {
            glDeleteProgram(tintProgram_);
        }
        if (skyProgram_ != 0) {
            glDeleteProgram(skyProgram_);
        }
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        if (atlasTexture_ != 0) {
            glDeleteTextures(1, &atlasTexture_);
        }
    } else if (program_ != 0 || skyProgram_ != 0 || atlasTexture_ != 0) {
        /* 上下文先于渲染器销毁时只能放弃显存资源, 由驱动回收. */
        IM_LOGW(kTag, "shutdown without current context, gpu objects dropped");
    }
    if (ready_) {
        IM_LOGI(kTag, "chunk renderer released");
    }
    program_ = 0;
    weatherProgram_ = 0;
    tintProgram_ = 0;
    uniformTintColor_ = -1;
    underwaterTint_ = 0.0f;
    uniformWeatherIntensity_ = -1;
    uniformWeatherKind_ = -1;
    uniformWeatherTime_ = -1;
    uniformWeatherViewport_ = -1;
    uniformWeatherAspect_ = -1;
    skyProgram_ = 0;
    uniformSkyBlend_ = -1;
    uniformSkyViewHeight_ = -1;
    atlasTexture_ = 0;
    uniformViewProjection_ = -1;
    uniformAtlas_ = -1;
    uniformChunkOrigin_ = -1;
    uniformCameraPosition_ = -1;
    uniformFogDensity_ = -1;
    uniformFogColor_ = -1;
    uniformBrightness_ = -1;
    uniformSkyFactor_ = -1;
    fogDensity_ = 0.0f;
    brightness_ = 1.0f;
    syncedWorld_ = nullptr;
    syncedVersion_ = 0;
    drawnChunks_ = 0;
    drawnTriangles_ = 0;
    ready_ = false;
}

bool ChunkRenderer::Ready() const {
    return ready_;
}

void ChunkRenderer::SetSkySun(const float* direction3, float skyTimeSeconds) {
    if (direction3 != nullptr) {
        /* 方向只做归一化, 退化向量回落到天顶, 避免除零后整片天空变黑. */
        const float length = std::sqrt(direction3[0] * direction3[0] + direction3[1] * direction3[1] +
                                       direction3[2] * direction3[2]);
        if (length > 1.0e-4f) {
            sunDirection_[0] = direction3[0] / length;
            sunDirection_[1] = direction3[1] / length;
            sunDirection_[2] = direction3[2] / length;
        } else {
            sunDirection_[0] = 0.0f;
            sunDirection_[1] = 1.0f;
            sunDirection_[2] = 0.0f;
        }
    }
    skyTimeSeconds_ = skyTimeSeconds;
}

void ChunkRenderer::SetViewProjection(const float* matrix16, const float* cameraPosition3) {
    if (matrix16 != nullptr) {
        std::memcpy(viewProjection_, matrix16, sizeof(viewProjection_));
    }
    if (cameraPosition3 != nullptr) {
        /* 相机位置供顶点着色器计算视距, 雾化强度由此推导. */
        cameraPosition_[0] = cameraPosition3[0];
        cameraPosition_[1] = cameraPosition3[1];
        cameraPosition_[2] = cameraPosition3[2];
    }
    /* 天空通道逐片元反投影求视线, 需要视图投影矩阵的逆; 求逆失败时保留上一帧. */
    InvertMatrix4(viewProjection_, inverseViewProjection_);
}

void ChunkRenderer::SetAtmosphere(float fogDensity, float brightness, float skyFactor) {
    fogDensity_ = fogDensity > 0.0f ? fogDensity : 0.0f;
    /* 亮度钳制在 0.5 到 2 之间, 极端值保护视觉可读性. */
    brightness_ = brightness < 0.5f ? 0.5f : (brightness > 2.0f ? 2.0f : brightness);
    /* 天光因子钳制在 0 到 1, 夜间压暗由 shader 的 mix 下限兜底. */
    skyFactor_ = skyFactor < 0.0f ? 0.0f : (skyFactor > 1.0f ? 1.0f : skyFactor);
}

/*
 * 增量同步: 以每个区块的修订号为准.
 * 每帧常态开销是遍历可见区块做一次映射表查找, 不触碰显存.
 *
 * 风险点(单帧可能超过 2 毫秒的地方):
 * 1. 区块上传是同步的 glBufferData, 单个区块约数十 KB 到一百多 KB.
 * 当前世界侧把新建区块限制在每帧 2 个, 上传量因此天然有界;
 * 如果后续放宽该预算, 首次进入视距时可能出现毫秒级的上传尖峰,
 * 届时可以在本函数里加每帧上传区块数上限, 代价是新区块延后一帧出现.
 * 2. 方块编辑会重建被编辑区块及其边界邻块, 单帧最多 5 个区块,
 * 这部分开销由世界侧决定, 渲染侧只做相应的缓冲替换.
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
        GLsizei waterBegin = 0;
        GLsizei waterCount = 0;
        if (!UploadMeshData(buffer.vao, buffer.vbo, buffer.ibo, *mesh, &indexCount, &waterBegin,
                            &waterCount)) {
            buffer.indexCount = 0;
            buffer.waterIndexCount = 0;
            buffer.hasRevision = false;
            continue;
        }
        buffer.indexCount = indexCount;
        buffer.waterIndexBegin = waterBegin;
        buffer.waterIndexCount = waterCount;
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
        /* 半透明索引量单独统计: 水面渲染是否走到第二趟, 看这个数就够了. */
        size_t waterIndices = 0;
        for (const auto& entry : buffers_) {
            waterIndices += static_cast<size_t>(entry.second->waterIndexCount);
        }
        IM_LOGD(kTag, "sync uploaded=%d released=%d alive=%zu waterIndices=%zu worldVersion=%llu",
                uploaded, released, buffers_.size(), waterIndices,
                static_cast<unsigned long long>(syncedVersion_));
    }
}

void ChunkRenderer::SetWeather(int32_t kind, float intensity, float seconds) {
    weatherKind_ = kind;
    weatherIntensity_ = intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity);
    weatherSeconds_ = seconds;
}

/*
 * 降水叠加层: 关深度测试与背面剔除, 开混合, 全屏一次绘制.
 * 必须在全部世界几何之后提交, 否则会挡住地形; HUD 由上层在此之后绘制.
 */
void ChunkRenderer::DrawPrecipitation() {
    if (weatherProgram_ == 0 || weatherIntensity_ <= 0.002f || !HasCurrentContext()) {
        return;
    }
    if (weatherKind_ <= 0) {
        return;
    }
    glUseProgram(weatherProgram_);
    glUniform2f(uniformWeatherViewport_, static_cast<float>(skyViewportWidth_),
                static_cast<float>(skyViewportHeight_));
    glUniform1f(uniformWeatherAspect_, skyViewportHeight_ > 0
                                           ? static_cast<float>(skyViewportWidth_) /
                                                 static_cast<float>(skyViewportHeight_)
                                           : 1.0f);
    glUniform1f(uniformWeatherTime_, weatherSeconds_);
    glUniform1f(uniformWeatherIntensity_, weatherIntensity_);
    glUniform1f(uniformWeatherKind_, weatherKind_ == 2 ? 1.0f : 0.0f);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

/*
 * 水下滤镜: 整屏叠一层冷色, 关深度测试与背面剔除, 开混合.
 * 必须在全部世界几何与降水之后提交, HUD 由上层在此之后绘制.
 */
void ChunkRenderer::DrawUnderwaterTint() {
    if (tintProgram_ == 0 || underwaterTint_ <= 0.002f || !HasCurrentContext()) {
        return;
    }
    glUseProgram(tintProgram_);
    /* 冷蓝偏青: 与水体色接近, 但更暗更压, 避免整屏发白. */
    glUniform4f(uniformTintColor_, 0.10f, 0.30f, 0.45f, underwaterTint_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void ChunkRenderer::SetUnderwaterTint(float alpha) {
    underwaterTint_ = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
}

void ChunkRenderer::SetViewportSize(int32_t width, int32_t height) {
    skyViewportWidth_ = width > 0 ? width : 1280;
    skyViewportHeight_ = height > 0 ? height : 720;
}

/*
 * 天空渐变: 全屏三角带, 关闭深度测试与写入, 也不参与雾与光照.
 * 必须在区块之前绘制, 后续地形与生物按深度覆盖它.
 */
void ChunkRenderer::DrawSky() {
    if (skyProgram_ == 0 || !HasCurrentContext()) {
        return;
    }
    glUseProgram(skyProgram_);
    glUniform1f(uniformSkyBlend_, skyFactor_);
    glUniform1f(uniformSkyViewHeight_, static_cast<float>(skyViewportHeight_));
    glUniformMatrix4fv(uniformSkyInvViewProjection_, 1, GL_FALSE, inverseViewProjection_);
    glUniform3f(uniformSkySun_, sunDirection_[0], sunDirection_[1], sunDirection_[2]);
    glUniform3f(uniformSkyCamera_, cameraPosition_[0], cameraPosition_[1], cameraPosition_[2]);
    glUniform1f(uniformSkyTime_, skyTimeSeconds_);
    glUniform1f(uniformSkyWeather_, weatherIntensity_);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    /* 恢复区块绘制依赖的状态, 避免调用方关心本通道改过什么. */
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
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

    /* 先铺天空, 再画地形; 天空不写深度, 因此不会遮挡任何方块. */
    DrawSky();

    glUseProgram(program_);
    /* 列主序矩阵直接交给 glUniformMatrix4fv, 不需要转置. */
    glUniformMatrix4fv(uniformViewProjection_, 1, GL_FALSE, viewProjection_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTexture_);
    glUniform1i(uniformAtlas_, 0);
    /* 雾与亮度按场景组装方折算好的参数下发, 相机位置供顶点求视距. */
    glUniform1f(uniformFogDensity_, fogDensity_);
    glUniform3f(uniformFogColor_, kFogRed, kFogGreen, kFogBlue);
    glUniform1f(uniformBrightness_, brightness_);
    glUniform1f(uniformSkyFactor_, skyFactor_);
    glUniform3f(uniformCameraPosition_, cameraPosition_[0], cameraPosition_[1], cameraPosition_[2]);

    /*
     * 实心面与液体面分两趟: 第一趟开深度写入画实心面, 第二趟关深度写入并开混合画液体面.
     * 液体面不再排序到像素级, 但按区块由远到近提交, 同一格水面之间不会互相遮挡错误.
     */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(kFrontFace);

    const auto submitOpaque = [this](const ChunkBuffer& buffer) {
        const GLsizei solidCount = buffer.indexCount - buffer.waterIndexCount;
        if (solidCount <= 0) {
            return;
        }
        glUniform3f(uniformChunkOrigin_, buffer.originX, 0.0f, buffer.originZ);
        glBindVertexArray(buffer.vao);
        glDrawElements(GL_TRIANGLES, solidCount, GL_UNSIGNED_INT, nullptr);
        ++drawnChunks_;
        drawnTriangles_ += static_cast<int32_t>(solidCount) / 3;
    };

    const auto submitLiquid = [this](const ChunkBuffer& buffer) {
        if (buffer.waterIndexCount <= 0) {
            return;
        }
        glUniform3f(uniformChunkOrigin_, buffer.originX, 0.0f, buffer.originZ);
        glBindVertexArray(buffer.vao);
        const void* offset = reinterpret_cast<const void*>(
            static_cast<uintptr_t>(buffer.waterIndexBegin) * sizeof(uint32_t));
        glDrawElements(GL_TRIANGLES, buffer.waterIndexCount, GL_UNSIGNED_INT, offset);
        drawnTriangles_ += static_cast<int32_t>(buffer.waterIndexCount) / 3;
    };

    /* 液体区块按相机距离由远到近排列, 近处水面后画, 混合结果才正确. */
    struct LiquidDraw {
        const ChunkBuffer* buffer;
        float distanceSquared;
    };
    static std::vector<LiquidDraw> liquidDraws;
    liquidDraws.clear();

    const auto collect = [this, &submitOpaque](const ChunkBuffer& buffer, bool haveView, float centerX,
                                              float centerZ) {
        submitOpaque(buffer);
        if (buffer.waterIndexCount <= 0) {
            return;
        }
        const float dx = (haveView ? centerX : buffer.originX) - cameraPosition_[0];
        const float dz = (haveView ? centerZ : buffer.originZ) - cameraPosition_[2];
        liquidDraws.push_back(LiquidDraw{&buffer, dx * dx + dz * dz});
    };

    if (syncedWorld_ != nullptr) {
        for (const world::ChunkView& view : syncedWorld_->VisibleChunks()) {
            const auto found = buffers_.find(ChunkKey(view.coord));
            if (found == buffers_.end()) {
                continue;
            }
            collect(*found->second, true, view.originX + 8.0f, view.originZ + 8.0f);
        }
    } else {
        /* 没有同步过世界时退化为按映射表遍历, 便于单独调试渲染路径. */
        for (const auto& entry : buffers_) {
            collect(*entry.second, false, 0.0f, 0.0f);
        }
    }

    if (!liquidDraws.empty()) {
        std::sort(liquidDraws.begin(), liquidDraws.end(),
                  [](const LiquidDraw& left, const LiquidDraw& right) {
                      return left.distanceSquared > right.distanceSquared;
                  });
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        /* 水面多是单层面片, 双面都画才能从水下看到水面; 代价是该趟没有背面剔除. */
        glDisable(GL_CULL_FACE);
        for (const LiquidDraw& draw : liquidDraws) {
            submitLiquid(*draw.buffer);
        }
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    glBindVertexArray(0);

    /* 雨雪叠加在世界几何之后, HUD 之前. */
    DrawPrecipitation();
    /* 水下滤镜在最后: 相机在水里时整屏压一层冷色. */
    DrawUnderwaterTint();
}

int32_t ChunkRenderer::DrawnChunks() const {
    return drawnChunks_;
}

int32_t ChunkRenderer::DrawnTriangles() const {
    return drawnTriangles_;
}

}  /* namespace immirror::render */
