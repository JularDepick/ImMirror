#include "ui/layout_draw.h"

#include <algorithm>
#include <deque>
#include <string>

#include "ui/hud_font.h"

namespace immirror::ui {
namespace {

/* 断行结果需要稳定地址: 绘制命令只存指针, deque 的元素地址不会因追加而失效. */
std::deque<std::string>& LinePool() {
    static std::deque<std::string> pool;
    return pool;
}

/* 按帧宽把文字拆成多行; 只有一个字符也放不下时独占一行, 避免死循环. */
void SplitLines(const char* text, float pixelSize, float maxWidth, std::vector<const char*>* out) {
    if (text == nullptr || maxWidth <= 0.0f) {
        return;
    }
    std::deque<std::string>& pool = LinePool();
    std::string line;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor == '\n') {
            pool.push_back(line);
            out->push_back(pool.back().c_str());
            line.clear();
            continue;
        }
        line.push_back(*cursor);
        if (MeasureText(line.c_str(), pixelSize) <= maxWidth) {
            continue;
        }
        /* 放不下了: 把当前字符退回去, 先断行. */
        line.pop_back();
        if (!line.empty()) {
            pool.push_back(line);
            out->push_back(pool.back().c_str());
        }
        line.assign(1, *cursor);
    }
    if (!line.empty()) {
        pool.push_back(line);
        out->push_back(pool.back().c_str());
    }
}

void Emit(const LayoutFrame& frame, int32_t depth, std::vector<LayoutDrawCommand>& out) {
    if (frame.rect.width <= 0.0f || frame.rect.height <= 0.0f) {
        return;
    }
    /* 按钮普通态贴图: 元素自身没给贴图时用它. */
    const char* texture = frame.texture != nullptr ? frame.texture : frame.normalTexture;
    const char* sprite = frame.sprite != nullptr ? frame.sprite : frame.normalSprite;
    /* 既没有颜色也没有贴图也没有文字的帧只是容器, 不产生图元. */
    const bool hasText = frame.text != nullptr && frame.text[0] != '\0';
    if (!frame.hasColor && texture == nullptr && !hasText) {
        return;
    }
    LayoutDrawCommand command;
    command.rect = frame.rect;
    command.layer = frame.layer;
    command.texture = texture;
    command.sprite = sprite;
    command.fontSize = frame.fontSize;
    command.justifyH = frame.justifyH;
    command.justifyV = frame.justifyV;
    command.lineInterval = frame.lineInterval;
    command.outline = frame.outline;
    command.multiLine = frame.multiLine;
    command.bold = frame.bold;
    command.strata = frame.strata;
    command.additive = frame.additive;
    command.shadowRed = frame.shadowRed;
    command.shadowGreen = frame.shadowGreen;
    command.shadowBlue = frame.shadowBlue;
    command.flipU = frame.flipU;
    command.flipV = frame.flipV;
    command.rotateQuarter = frame.rotateQuarter;
    command.tileOffsetX = frame.tileOffsetX;
    command.tileOffsetY = frame.tileOffsetY;
    command.tileWidth = frame.tileWidth;
    command.tileHeight = frame.tileHeight;
    command.insetLeft = frame.insetLeft;
    command.insetRight = frame.insetRight;
    command.insetTop = frame.insetTop;
    command.insetBottom = frame.insetBottom;
    command.text = hasText ? frame.text : nullptr;
    command.name = frame.name;
    command.red = frame.red;
    command.green = frame.green;
    command.blue = frame.blue;
    command.alpha = frame.alpha;
    command.depth = depth;
    /*
     * 多行文本: 超过帧宽时按宽度断行, 每行单独产出绘制命令.
     * 单行文本沿用原路径, 不受影响.
     */
    if (frame.multiLine && hasText && frame.rect.width > 0.0f) {
        std::vector<const char*> lines;
        /*
         * 点阵字形的单个像素边长 = 字号 / 字形高度.
         * 直接把字号当像素边长会让测量宽度放大七倍, 断行会把每个字符拆成一行.
         */
        const float fontSize = command.fontSize > 0.0f ? command.fontSize : 20.0f;
        const float pixelSize = fontSize / static_cast<float>(kGlyphHeight);
        SplitLines(frame.text, pixelSize, frame.rect.width, &lines);
        if (lines.size() > 1) {
            const float lineHeight = pixelSize + frame.lineInterval;
            for (size_t index = 0; index < lines.size(); ++index) {
                LayoutDrawCommand lineCommand = command;
                lineCommand.text = lines[index];
                lineCommand.rect.y = command.rect.y - static_cast<float>(index) * lineHeight;
                lineCommand.depth = depth;
                out.push_back(lineCommand);
            }
            return;
        }
    }
    out.push_back(command);
}

/*
 * 裁剪: 滑动容器只显示自身矩形内的内容.
 * 矩形与裁剪区没有交集时整棵子树跳过, 部分重叠时保留命令, 由渲染阶段裁边.
 */
bool OutsideClip(const LayoutRect& rect, const LayoutRect& clip) {
    return rect.x + rect.width <= clip.x || rect.y + rect.height <= clip.y ||
           rect.x >= clip.x + clip.width || rect.y >= clip.y + clip.height;
}

void Walk(const LayoutFrame& frame, int32_t depth, std::vector<LayoutDrawCommand>& out,
          const LayoutRect* clip = nullptr) {
    /* virtual="true" 的帧只是模板, 自身不绘制, 子树也不展开. */
    if (frame.isVirtual) {
        return;
    }
    if (clip != nullptr && OutsideClip(frame.rect, *clip)) {
        return;
    }
    Emit(frame, depth, out);

    /* 子帧按图层稳定排序: 同层保持布局里的先后, 不因排序打乱. */
    std::vector<const LayoutFrame*> ordered;
    ordered.reserve(frame.children.size());
    for (const LayoutFrame* child : frame.children) {
        if (child != nullptr) {
            ordered.push_back(child);
        }
    }
    /* 先按图层, 同图层内按 framelevel: 数值大的后画, 压在上面. */
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const LayoutFrame* left, const LayoutFrame* right) {
                         const int32_t leftLayer = static_cast<int32_t>(left->layer);
                         const int32_t rightLayer = static_cast<int32_t>(right->layer);
                         if (leftLayer != rightLayer) {
                             return leftLayer < rightLayer;
                         }
                         return left->frameLevel < right->frameLevel;
                     });
    /* 滑动容器把自己的矩形作为子树的裁剪区. */
    const LayoutRect* childClip = frame.clipChildren ? &frame.rect : clip;
    for (const LayoutFrame* child : ordered) {
        Walk(*child, depth + 1, out, childClip);
    }
}

}  /* namespace */

void ResetTextLinePool() {
    LinePool().clear();
}

int32_t CollectDrawCommands(const LayoutFrame& root, std::vector<LayoutDrawCommand>& out) {
    out.clear();
    ResetTextLinePool();
    Walk(root, 0, out);
    return static_cast<int32_t>(out.size());
}

}  /* namespace immirror::ui */
