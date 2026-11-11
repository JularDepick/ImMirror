#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "editor/editor_types.h"
#include "voxel/voxel_types.h"

namespace immirror::editor {

/* 一次方块改动: 目标格与前后取值, 撤销与重做都在这里回放. */
struct BlockEdit {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    voxel::BlockId before = 0;
    voxel::BlockId after = 0;
};

/*
 * 一批改动. 一次手势 (单击放置, 框选填充) 对应一批,
 * 撤销与重做都以批为单位, 不会把一次填充拆成几百步.
 */
struct EditBatch {
    std::string label;            /* 操作名, 界面显示用 */
    std::vector<BlockEdit> edits;
};

/* 写回一格的回调, 由编辑器实现, 撤销栈不直接接触工程与世界. */
using BlockApplier = void (*)(void* user, int32_t x, int32_t y, int32_t z, voxel::BlockId id);

/*
 * 撤销栈. 只保存方块改动, 容量取 limits::kUndoDepth (默认 32 步, 需求要求至少 20 步).
 * 每次新的改动都会清空重做栈, 与常见编辑器的语义一致.
 */
class UndoStack {
public:
    UndoStack() = default;

    /* 压入一批改动; edits 为空时忽略. */
    void Push(EditBatch batch);

    /* 撤销一步, 返回是否真的撤销了内容. */
    bool Undo(void* user, BlockApplier apply);

    /* 重做一步. */
    bool Redo(void* user, BlockApplier apply);

    int32_t UndoDepth() const { return static_cast<int32_t>(undo_.size()); }
    int32_t RedoDepth() const { return static_cast<int32_t>(redo_.size()); }

    /* 最近一次操作的名称, 没有操作时返回空串. */
    const std::string& LastLabel() const;

    void Clear();

private:
    /* 尾部是最新的一批, 撤销从尾部取, 重做压回尾部. */
    std::vector<EditBatch> undo_;
    std::vector<EditBatch> redo_;
    std::string emptyLabel_;
};

}  /* namespace immirror::editor */
