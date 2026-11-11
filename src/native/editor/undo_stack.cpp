#include "editor/undo_stack.h"

#include <utility>

namespace immirror::editor {

void UndoStack::Push(EditBatch batch) {
    if (batch.edits.empty()) {
        return;
    }
    /* 新的改动出现时旧的重做记录已经失效. */
    redo_.clear();
    undo_.push_back(std::move(batch));
    /* 超出容量时丢弃最旧的一批, 保证内存占用有上界. */
    while (static_cast<int32_t>(undo_.size()) > limits::kUndoDepth) {
        undo_.erase(undo_.begin());
    }
}

bool UndoStack::Undo(void* user, BlockApplier apply) {
    if (undo_.empty() || apply == nullptr) {
        return false;
    }
    EditBatch batch = std::move(undo_.back());
    undo_.pop_back();
    /* 逆序回放, 同一格被多次改动时也能回到最初取值. */
    for (auto it = batch.edits.rbegin(); it != batch.edits.rend(); ++it) {
        apply(user, it->x, it->y, it->z, it->before);
    }
    redo_.push_back(std::move(batch));
    return true;
}

bool UndoStack::Redo(void* user, BlockApplier apply) {
    if (redo_.empty() || apply == nullptr) {
        return false;
    }
    EditBatch batch = std::move(redo_.back());
    redo_.pop_back();
    for (const BlockEdit& edit : batch.edits) {
        apply(user, edit.x, edit.y, edit.z, edit.after);
    }
    undo_.push_back(std::move(batch));
    return true;
}

const std::string& UndoStack::LastLabel() const {
    if (undo_.empty()) {
        return emptyLabel_;
    }
    return undo_.back().label;
}

void UndoStack::Clear() {
    undo_.clear();
    redo_.clear();
}

}  /* namespace immirror::editor */
