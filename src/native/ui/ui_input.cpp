#include "ui/ui_input.h"

#include <algorithm>

namespace immirror::ui {

void UiInput::SetCanvas(float width, float height) {
    if (width > 0.0f && height > 0.0f) {
        canvasWidth_ = width;
        canvasHeight_ = height;
    }
}

void UiInput::SetScreen(float width, float height) {
    if (width > 0.0f && height > 0.0f) {
        screenWidth_ = width;
        screenHeight_ = height;
    }
}

void UiInput::VisibleRect(float* x, float* y, float* width, float* height) const {
    /* 等比缩放: 取两个方向缩放比例的较小值, 剩余部分居中留黑边. */
    const float scaleX = screenWidth_ / canvasWidth_;
    const float scaleY = screenHeight_ / canvasHeight_;
    const float scale = std::min(scaleX, scaleY);
    const float visibleWidth = canvasWidth_ * scale;
    const float visibleHeight = canvasHeight_ * scale;
    if (x != nullptr) {
        *x = (screenWidth_ - visibleWidth) * 0.5f;
    }
    if (y != nullptr) {
        *y = (screenHeight_ - visibleHeight) * 0.5f;
    }
    if (width != nullptr) {
        *width = visibleWidth;
    }
    if (height != nullptr) {
        *height = visibleHeight;
    }
}

bool UiInput::ToCanvas(float screenX, float screenY, float* canvasX, float* canvasY) const {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float visibleWidth = 0.0f;
    float visibleHeight = 0.0f;
    VisibleRect(&offsetX, &offsetY, &visibleWidth, &visibleHeight);
    if (visibleWidth <= 0.0f || visibleHeight <= 0.0f) {
        return false;
    }
    if (screenX < offsetX || screenY < offsetY || screenX > offsetX + visibleWidth ||
        screenY > offsetY + visibleHeight) {
        return false;
    }
    if (canvasX != nullptr) {
        *canvasX = (screenX - offsetX) / visibleWidth * canvasWidth_;
    }
    if (canvasY != nullptr) {
        *canvasY = (screenY - offsetY) / visibleHeight * canvasHeight_;
    }
    return true;
}

Widget* UiInput::Down(WidgetTree& tree, float screenX, float screenY) {
    float x = 0.0f;
    float y = 0.0f;
    if (!ToCanvas(screenX, screenY, &x, &y)) {
        pressed_ = nullptr;
        return nullptr;
    }
    pressed_ = tree.HitTest(x, y);
    if (pressed_ != nullptr) {
        tree.SetPressed(pressed_, true);
    }
    return pressed_;
}

void UiInput::Move(WidgetTree& tree, float screenX, float screenY) {
    if (pressed_ == nullptr) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    const bool inside = ToCanvas(screenX, screenY, &x, &y);
    Widget* over = inside ? tree.HitTest(x, y) : nullptr;
    if (over != pressed_) {
        /* 指针滑出按下控件: 取消按下态, 与原版按钮的弹起行为一致. */
        tree.SetPressed(pressed_, false);
        pressed_ = nullptr;
    }
}

Widget* UiInput::Up(WidgetTree& tree, float screenX, float screenY) {
    Widget* pressed = pressed_;
    float x = 0.0f;
    float y = 0.0f;
    const bool inside = ToCanvas(screenX, screenY, &x, &y);
    Widget* over = inside ? tree.HitTest(x, y) : nullptr;
    if (pressed != nullptr) {
        tree.SetPressed(pressed, false);
    }
    pressed_ = nullptr;
    return pressed != nullptr && over == pressed ? pressed : nullptr;
}

}  /* namespace immirror::ui */
