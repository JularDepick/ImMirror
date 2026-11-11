#include "tests/test_framework.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "editor/editor_command.h"

#include "editor/project_format.h"

namespace {

using immirror::editor::HandleEditorCommand;
using immirror::editor::ResetEditorCommandStateForTest;

/* 每个用例一个独立的临时工程根目录, 用例结束整体删除. */
std::string MakeTempDir(const char* label) {
    const long long ticks =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    const int tid = static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()) &
                                     0x7fffffff);
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "temp/editor-cmd-test/%s-%d-%lld", label, tid, ticks);
    const std::string path = buffer;
    immirror::editor::MakeDirectories(path);
    return path;
}

/* 递归删除目录树, 直接复用工程格式层的能力. */
void RemoveTree(const std::string& path) {
    immirror::editor::RemoveDirectoryTree(path);
}

/* 取 ReplyFirst 后按竖线切字段. */
std::vector<std::string> RowFields(const std::string& reply, const std::string& key) {
    const std::vector<std::string> rows =
        immirror::editor::test::ReplyAll(reply, key);
    std::vector<std::string> fields;
    if (rows.empty()) {
        return fields;
    }
    std::string row = rows.front();
    size_t start = 0;
    while (start <= row.size()) {
        const size_t pipe = row.find("|", start);
        if (pipe == std::string::npos) {
            fields.push_back(row.substr(start));
            break;
        }
        fields.push_back(row.substr(start, pipe - start));
        start = pipe + 1;
    }
    return fields;
}

/* 用例骨架: 先重置状态, 再建临时目录, 退出时清理. */
struct Fixture {
    Fixture(const char* label) {
        ResetEditorCommandStateForTest();
        root = MakeTempDir(label);
    }
    ~Fixture() {
        ResetEditorCommandStateForTest();
        RemoveTree(root);
    }
    std::string root;
};

/* ---------------------------------------------------------------- 用例 */

TEST_CASE(PingReplysWithoutInit) {
    ResetEditorCommandStateForTest();
    const std::string reply = HandleEditorCommand("ping");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(reply, "message") == "pong");
}

TEST_CASE(EmptyAndUnknownCommands) {
    ResetEditorCommandStateForTest();
    const std::string empty = HandleEditorCommand("   ");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(empty, "code")), 2);
    const std::string unknown = HandleEditorCommand("no_such_command x");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(unknown, "code")), 2);
    CHECK(immirror::editor::test::ReplyFirst(unknown, "message").find("no_such_command")
          != std::string::npos);
}

TEST_CASE(CommandsRejectedBeforeInit) {
    ResetEditorCommandStateForTest();
    for (const char* verb : {"status", "projects", "new demo", "open x", "palette", "save"}) {
        const std::string reply = HandleEditorCommand(verb);
        CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 1);
        CHECK(immirror::editor::test::ReplyFirst(reply, "message").find("init")
              != std::string::npos);
    }
}

TEST_CASE(InitRecordsDataPath) {
    Fixture fixture("init");
    const std::string reply = HandleEditorCommand("init " + fixture.root);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(reply, "dataPath") == fixture.root);
    CHECK(immirror::editor::test::ReplyFirst(reply, "root") == fixture.root + "/editor");
    const std::string status = HandleEditorCommand("status");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(status, "open")), 0);
}

TEST_CASE(PaletteListsChineseNames) {
    Fixture fixture("palette");
    HandleEditorCommand("init " + fixture.root);
    const std::string reply = HandleEditorCommand("palette");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 0);
    const std::vector<std::string> rows = immirror::editor::test::ReplyAll(reply, "block");
    CHECK(!rows.empty());
    bool foundGrass = false;
    for (const std::string& row : rows) {
        const size_t pipe = row.find("|");
        CHECK(pipe != std::string::npos);
        if (row.compare(0, pipe, "grass") == 0) {
            foundGrass = true;
            CHECK(row.find("草块") != std::string::npos);
        }
    }
    CHECK(foundGrass);
}

TEST_CASE(NewCreatesDirectoryAndProjectsListsIt) {
    Fixture fixture("new");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 测试工程");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(created, "code")), 0);
    const std::string directory = immirror::editor::test::ReplyFirst(created, "project");
    CHECK(!directory.empty());
    CHECK(directory.rfind(fixture.root + "/editor/", 0) == 0);
    const std::string list = HandleEditorCommand("projects");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(list, "code")), 0);
    const std::vector<std::string> rows = immirror::editor::test::ReplyAll(list, "project");
    CHECK_INT(static_cast<long long>(rows.size()), 1);
    const std::vector<std::string> fields = RowFields(list, "project");
    CHECK_INT(static_cast<long long>(fields.size()), 6);
    CHECK(fields[0] == directory);
    CHECK(fields[1] == "测试工程");
}

TEST_CASE(NewRejectsEmptyName) {
    Fixture fixture("new-empty");
    HandleEditorCommand("init " + fixture.root);
    const std::string reply = HandleEditorCommand("new ");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 1);
}

TEST_CASE(OpenReportsMeta) {
    Fixture fixture("open");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 元信息工程");
    const std::string directory = immirror::editor::test::ReplyFirst(created, "project");
    const std::string opened = HandleEditorCommand("open " + directory);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(opened, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(opened, "name") == "元信息工程");
    CHECK(immirror::editor::test::ReplyFirst(opened, "project") == directory);
    CHECK(!immirror::editor::test::ReplyFirst(opened, "seed").empty());
    const std::string status = HandleEditorCommand("status");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(status, "open")), 1);
    CHECK(immirror::editor::test::ReplyFirst(status, "project") == directory);
    CHECK(immirror::editor::test::ReplyFirst(status, "tool") == "place");
}

TEST_CASE(OpenRejectsMissingDirectory) {
    Fixture fixture("open-missing");
    HandleEditorCommand("init " + fixture.root);
    const std::string reply =
        HandleEditorCommand("open " + fixture.root + "/not-exist");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reply, "code")), 1);
}

TEST_CASE(FullChainInitNewOpenApplySaveProjects) {
    Fixture fixture("chain");
    /* init -> new -> open -> apply -> save -> projects 全链. */
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("init " + fixture.root), "code")), 0);
    const std::string created = HandleEditorCommand("new 链路工程");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(created, "code")), 0);
    const std::string directory = immirror::editor::test::ReplyFirst(created, "project");
    const std::string opened = HandleEditorCommand("open " + directory);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(opened, "code")), 0);
    const std::string status = HandleEditorCommand("status");
    const std::string blockKey = immirror::editor::test::ReplyFirst(status, "block");
    CHECK(!blockKey.empty());
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("block " + blockKey), "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("cursor 3 200 4"), "code")), 0);
    const std::string applied = HandleEditorCommand("apply");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(applied, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(applied, "cursor") == "3,200,4");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("save"), "code")), 0);
    const std::string list = HandleEditorCommand("projects");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(list, "code")), 0);
    const std::vector<std::string> rows = immirror::editor::test::ReplyAll(list, "project");
    CHECK_INT(static_cast<long long>(rows.size()), 1);
    /* 保存后再打开, 改动应当还在: 落盘链路完整. */
    const std::string reopened = HandleEditorCommand("open " + directory);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reopened, "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(reopened, "edits")), 1);
}

TEST_CASE(ApplyRequiresProjectAndWritesBlock) {
    Fixture fixture("apply");
    HandleEditorCommand("init " + fixture.root);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("apply"),
                                                           "code")), 1);
    const std::string created = HandleEditorCommand("new 应用工程");
    const std::string directory = immirror::editor::test::ReplyFirst(created, "project");
    HandleEditorCommand("open " + directory);
    HandleEditorCommand("cursor 5 200 6");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("tool place"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("block stone"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("apply"),
                                                           "code")), 0);
    /* 原样再放一次是空操作, 仍成功但不重复计数. */
    const std::string again = HandleEditorCommand("apply");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(again, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(again, "message").find("没有改动")
          != std::string::npos);
    /* 删除工具把该格改回空气. */
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("tool remove"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("apply"),
                                                           "code")), 0);
    /* 差异表只在保存时按地形重算: 删除后未保存仍记 1, 保存后归 0. */
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "edits")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("save"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "edits")), 0);
}

TEST_CASE(CursorValidatesInput) {
    Fixture fixture("cursor");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 光标工程");
    HandleEditorCommand("open " + immirror::editor::test::ReplyFirst(created, "project"));
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("cursor 1 2"), "code")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("cursor a b c"), "code")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("cursor 1 999 2"), "code")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("cursor -3 40 -5"), "code")), 0);
    const std::string status = HandleEditorCommand("status");
    CHECK(immirror::editor::test::ReplyFirst(status, "cursor") == "-3,40,-5");
}

TEST_CASE(SelectionAndFill) {
    Fixture fixture("fill");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 填充工程");
    HandleEditorCommand("open " + immirror::editor::test::ReplyFirst(created, "project"));
    /* 未标记 B 点时填充被拒绝. */
    HandleEditorCommand("cursor 0 200 0");
    HandleEditorCommand("sel a");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("fill"),
                                                           "code")), 1);
    HandleEditorCommand("cursor 2 202 2");
    HandleEditorCommand("sel b");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("block stone"),
                                                           "code")), 0);
    const std::string filled = HandleEditorCommand("fill");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(filled, "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(filled, "changed")), 27);
    CHECK(immirror::editor::test::ReplyFirst(filled, "selectionSize") == "3,3,3");
    const std::string status = HandleEditorCommand("status");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(status, "selectionSet")), 1);
    CHECK(immirror::editor::test::ReplyFirst(status, "selectionSize") == "3,3,3");
    CHECK(immirror::editor::test::ReplyFirst(status, "selectionA") == "0,200,0");
    CHECK(immirror::editor::test::ReplyFirst(status, "selectionB") == "2,202,2");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("sel clear"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("status"), "selectionSet")), 0);
}

TEST_CASE(FillUndoRestoresVolume) {
    Fixture fixture("fill-undo");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 撤销工程");
    HandleEditorCommand("open " + immirror::editor::test::ReplyFirst(created, "project"));
    HandleEditorCommand("block stone");
    HandleEditorCommand("cursor 0 200 0");
    HandleEditorCommand("sel a");
    HandleEditorCommand("cursor 1 201 1");
    HandleEditorCommand("sel b");
    const std::string filled = HandleEditorCommand("fill");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(filled, "changed")), 8);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "edits")), 8);
    const std::string undone = HandleEditorCommand("undo");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(undone, "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "undo")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "redo")), 1);
    /* 差异表在保存时按地形重算: 撤销后的内存计数不变, 以撤销深度验证回放. */
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("redo"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "undo")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("status"),
                                                           "redo")), 0);
}

TEST_CASE(ToolAndBlockValidation) {
    Fixture fixture("tool");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 工具工程");
    HandleEditorCommand("open " + immirror::editor::test::ReplyFirst(created, "project"));
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("tool paint"),
                                                           "code")), 1);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("tool place"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("tool remove"),
                                                           "code")), 0);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(
                  HandleEditorCommand("block no_such_block"), "code")), 1);
}

TEST_CASE(TriggerLifecycle) {
    Fixture fixture("trigger");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 触发器工程");
    HandleEditorCommand("open " + immirror::editor::test::ReplyFirst(created, "project"));
    const std::string added = HandleEditorCommand("trigger_add timer");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(added, "code")), 0);
    const std::string id = immirror::editor::test::ReplyFirst(added, "id");
    CHECK(id == "1");
    const std::string list = HandleEditorCommand("triggers");
    const std::vector<std::string> rows = immirror::editor::test::ReplyAll(list, "trigger");
    CHECK_INT(static_cast<long long>(rows.size()), 1);
    CHECK(rows.front().find("定时") != std::string::npos);
    const std::string run = HandleEditorCommand("trigger_run 1");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(run, "code")), 0);
    CHECK(immirror::editor::test::ReplyFirst(run, "script").find("log(") != std::string::npos);
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("trigger_remove 1"),
                                                           "code")), 0);
    CHECK(immirror::editor::test::ReplyAll(HandleEditorCommand("triggers"), "trigger").empty());
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("trigger_run 9"),
                                                           "code")), 1);
}

TEST_CASE(ExportWritesWorldFile) {
    Fixture fixture("export");
    HandleEditorCommand("init " + fixture.root);
    const std::string created = HandleEditorCommand("new 导出工程");
    const std::string directory = immirror::editor::test::ReplyFirst(created, "project");
    HandleEditorCommand("open " + directory);
    /* 没有任何改动时导出被拒绝, 工程格式层没有可写内容. */
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(HandleEditorCommand("export"),
                                                           "code")), 1);
    HandleEditorCommand("block stone");
    HandleEditorCommand("cursor 0 200 0");
    HandleEditorCommand("apply");
    const std::string exported = HandleEditorCommand("export playtest");
    CHECK_INT(std::stoi(immirror::editor::test::ReplyFirst(exported, "code")), 0);
    const std::string world = immirror::editor::test::ReplyFirst(exported, "world");
    CHECK(world.rfind(fixture.root + "/worlds/", 0) == 0);
    FILE* file = std::fopen(world.c_str(), "rb");
    CHECK(file != nullptr);
    if (file != nullptr) {
        char magic[4] = {0, 0, 0, 0};
        CHECK(std::fread(magic, 1, 4, file) == 4);
        CHECK(magic[0] == 'I' && magic[1] == 'M' && magic[2] == 'W' && magic[3] == 'F');
        std::fclose(file);
    }
}

}  /* namespace */
