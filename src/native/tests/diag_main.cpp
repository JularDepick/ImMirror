#include "editor/editor_command.h"
#include "editor/project_format.h"
#include <cstdio>
#include <string>

static std::string Run(const char* cmd) {
    const std::string reply = immirror::editor::HandleEditorCommand(cmd);
    std::printf("== %s\n%s", cmd, reply.c_str());
    return reply;
}

static std::string Field(const std::string& reply, const char* key) {
    std::string needle = std::string(key) + "=";
    const size_t at = reply.find(needle);
    if (at == std::string::npos) return "?";
    const size_t nl = reply.find("\n", at);
    return reply.substr(at + needle.size(), nl - at - needle.size());
}

int main() {
    immirror::editor::ResetEditorCommandStateForTest();
    const std::string root = "temp/editor-diag2";
    immirror::editor::MakeDirectories(root);
    Run("init temp/editor-diag2");
    Run("new 诊断工程");
    const std::string list = Run("projects");
    std::string dir = Field(list, "project");
    const size_t pipe = dir.find("|");
    if (pipe != std::string::npos) dir = dir.substr(0, pipe);
    Run(("open " + dir).c_str());
    Run("block stone");
    Run("cursor 5 70 6");
    Run("apply");
    Run("tool remove");
    Run("apply");
    const std::string s1 = Run("status");
    std::printf(">>> edits after remove = %s\n", Field(s1, "edits").c_str());
    Run("save");
    const std::string s2 = Run("status");
    std::printf(">>> edits after save = %s\n", Field(s2, "edits").c_str());
    /* 撤销链诊断 */
    Run("block dirt");
    Run("cursor 0 200 0");
    Run("sel a");
    Run("cursor 1 201 1");
    Run("sel b");
    Run("fill");
    Run("undo");
    const std::string s3 = Run("status");
    std::printf(">>> edits after undo = %s\n", Field(s3, "edits").c_str());
    std::printf(">>> undo=%s redo=%s\n", Field(s3, "undo").c_str(), Field(s3, "redo").c_str());
    return 0;
}
