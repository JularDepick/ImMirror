#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <cstdint>
#include <vector>

namespace immirror::ui {

/*
 * 帧树与锚点求解.
 *
 * 界面布局的范式是"帧 + 锚点 + 图层": 每个帧给出若干锚点, 锚点指到自己身上
 * 的一个点与另一个帧上的一个点, 加上二维偏移; 尺寸另给绝对宽高.
 * 这里只做几何求解, 不涉及绘制与贴图, 因此可以离线用数值断言验证.
 */

/* 帧上的九个参考点. */
enum class AnchorPoint : uint8_t {
    kTopLeft = 0,
    kTop,
    kTopRight,
    kLeft,
    kCenter,
    kRight,
    kBottomLeft,
    kBottom,
    kBottomRight,
};

/* 轴对齐矩形, x 与 y 是左上角, 单位与布局文件一致 (像素). */
struct LayoutRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct LayoutFrame;

/* 一条锚点: 自己的 point 对齐到 relativeTo 的 relativePoint, 再加偏移. */
struct LayoutAnchor {
    AnchorPoint point = AnchorPoint::kTopLeft;
    const LayoutFrame* relativeTo = nullptr;
    AnchorPoint relativePoint = AnchorPoint::kTopLeft;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    /* 布局里写的参照物名字, 用于判断"没写"与"写了但找不到"; 放末尾以免破坏聚合初始化. */
    const char* relativeName = nullptr;
};

/* 图层顺序: 数值小的先画, 与布局里 background / artwork / overlay 一致. */
enum class LayerLevel : uint8_t {
    kBackground = 0,
    kBorders = 1,
    kArtwork = 2,
    kOverlay = 3,
    kHighlight = 4,
};

/*
 * 一帧: 名字, 父帧, 子帧, 锚点, 尺寸与外观.
 * rect 是求解结果; 颜色与贴图来自布局元素本身, 绘制阶段按图层顺序展开.
 */
struct LayoutFrame {
    const char* name = "";
    LayoutFrame* parent = nullptr;
    std::vector<LayoutFrame*> children;
    std::vector<LayoutAnchor> anchors;
    /* 未给尺寸时按 0 处理, 求解结果宽高为 0. */
    float width = 0.0f;
    float height = 0.0f;
    /* 相对尺寸: 按父帧宽高的比例, 绝对尺寸优先. */
    float relWidth = 0.0f;
    float relHeight = 0.0f;
    LayoutRect rect;
    /* 外观: 未给颜色时按不透明白处理, 贴图为空表示只画纯色. */
    bool hasColor = false;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
    /* 贴图相对路径, 空表示无贴图. */
    const char* texture = nullptr;
    LayerLevel layer = LayerLevel::kArtwork;
    /* 布局里标了 hidden="true": 载入时默认不显示, 由脚本按需打开. */
    bool hidden = false;
    /* 模板里的精灵名, 与 texture 一起决定取图集里的哪一格; 空表示整图. */
    const char* sprite = nullptr;
    /* 文本控件的内容; 空表示这个帧不画文字. */
    const char* text = nullptr;
    /* 布局里的 virtual="true": 只作为模板被复制, 自身不参与绘制. */
    bool isVirtual = false;
    /* 字号: 取自 fonttype 里的数字, 0 表示布局没给. */
    float fontSize = 0.0f;
    /*
     * 按钮三态: 布局用 NormalTexture / PushedTexture 子元素声明,
     * 元素自身没给贴图时, 绘制取普通态, 按下时取按下态.
     */
    const char* normalTexture = nullptr;
    const char* normalSprite = nullptr;
    /* 选中态贴图: 按钮被 Checked 后用它. */
    const char* checkedTexture = nullptr;
    const char* checkedSprite = nullptr;
    const char* pushedTexture = nullptr;
    const char* pushedSprite = nullptr;
    /*
     * 文本排版: 水平 0 左 / 1 中 / 2 右, 垂直 0 上 / 1 中 / 2 下,
     * 行距取自布局的 lineInterval, 0 表示按字号.
     */
    int32_t justifyH = 0;
    int32_t justifyV = 0;
    float lineInterval = 0.0f;
    /* 文字描边: fontStyle="border" 时在四周各画一遍深色文字. */
    /* 文字加粗: 同色向右偏移一个点阵像素再画一遍. */
    bool bold = false;
    bool outline = false;
    /* 流式布局: 非空时按这个样式排列子元素. */
    const char* layoutStyle = nullptr;
    float spaceX = 0.0f;
    float spaceY = 0.0f;
    /* 加法混合: alphamode="add" 的元素叠加发光, 其余按普通混合. */
    bool additive = false;
    /* 文字阴影色: 描边用这个颜色, 未写时用黑色. */
    float shadowRed = 0.0f;
    float shadowGreen = 0.0f;
    float shadowBlue = 0.0f;
    /*
     * 跨主帧层级: 0 背景 / 1 中 / 2 高 / 3 全屏 / 4 提示.
     * 决定不同主帧的元素谁画在上面, 与同帧内的 layer 是两个维度.
     */
    int32_t strata = 1;
    /* 布局里的标签名: Frame / Button / Texture / FontString 等, 用于核对控件类型. */
    const char* tag = nullptr;
    /* 同图层内的绘制优先级: 数值大的画在上面. */
    int32_t frameLevel = 0;
    /* 贴图翻转: UVType 写作 mirroeu / mirroev / turn180 时交换 uv 两端. */
    bool flipU = false;
    bool flipV = false;
    /* 直角旋转: UVType 写作 turn90 / turn270 时交换 uv 的两个轴. */
    int32_t rotateQuarter = 0;
    /* 裁剪子元素: 滑动容器只显示自身矩形内的内容. */
    bool clipChildren = false;
    /* 多行文本: 文字超过帧宽时按宽度断行. */
    bool multiLine = false;
    /* 拉伸边缘: 带此属性的纹理铺满父帧(背景蒙版与整屏遮罩常用). */
    bool stretchEdge = false;
    /* 继承的模板名, 空表示不继承; 载入后模板子树会克隆到本帧. */
    const char* inherits = nullptr;
    /* 布局里的 id 属性: 页签与列表行用它区分同类元素. */
    int32_t clientId = 0;
    /* 布局里 OnClick 的脚本片段, 空表示没有点击处理. */
    const char* onClick = nullptr;
    /* 生命周期脚本: OnLoad / OnShow / OnHide 标签的内容. */
    const char* onLoad = nullptr;
    const char* onShow = nullptr;
    const char* onHide = nullptr;
    /*
     * 其余事件脚本: 键是事件名(去掉 On 前缀并转小写, 例如 event / update / mousedown).
     * 用一个表装下所有事件, 比逐个加字段更省事, 也不会漏掉新事件.
     */
    std::map<std::string, const char*>* events = nullptr;
    /* 九宫切片内边距(像素), 来自模板; 全零表示整体拉伸. */
    /* 平铺起始偏移. */
    float tileOffsetX = 0.0f;
    float tileOffsetY = 0.0f;
    /* 平铺块尺寸: 九宫参数里边角为 0 时按这个尺寸重复贴图, 而不是整体拉伸. */
    float tileWidth = 0.0f;
    float tileHeight = 0.0f;
    float insetLeft = 0.0f;
    float insetRight = 0.0f;
    float insetTop = 0.0f;
    float insetBottom = 0.0f;

    void AddChild(LayoutFrame* child) {
        child->parent = this;
        children.push_back(child);
    }
};

/*
 * 精灵像素尺寸解析: 布局不给 Size 的元素按图集精灵的原始像素补齐.
 * 用函数指针而不是直接依赖图集, 布局层才能在离线断言里独立编译.
 */
using SpriteSizeFn = bool (*)(const char* sprite, float* width, float* height);
void SetSpriteSizeResolver(SpriteSizeFn fn);

/* 整图页尺寸解析: 只给贴图、没有精灵名的元素按整图像素尺寸补齐. */
using TextureSizeFn = bool (*)(const char* texture, float* width, float* height);
void SetTextureSizeResolver(TextureSizeFn fn);

/*
 * 缺失参照物计数: 锚点的 relativeTo 在离线树里找不到时, 解算会退化为父帧.
 * 这会让元素位置偏离真实布局, 需要单独计数以便区分"引擎算错"与"缺少参照物".
 */
int32_t MissingAnchorReferenceCount();
void ResetMissingAnchorReferenceCount();
/* 缺失参照物出现过的名字, 用于定位还需要哪些外部布局文件. */
const std::vector<std::string>& MissingAnchorReferenceNames();

/* 点在矩形上的坐标. */
float PointX(const LayoutRect& rect, AnchorPoint point);
float PointY(const LayoutRect& rect, AnchorPoint point);

/*
 * 求解整棵帧树. 根帧按屏幕矩形摆放并忽略自身锚点,
 * 其余帧按第一条锚点定位; 锚点缺失时贴在父帧左上角.
 * 必须从根开始自上而下求解, 父帧未解算时子帧的结果没有意义.
 */
void ResolveLayout(LayoutFrame& root, const LayoutRect& screen);

}  /* namespace immirror::ui */
