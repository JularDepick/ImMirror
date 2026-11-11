#include "editor/editor_command.h"
#include "editor/project_format.h"
#include <cstdio>
#include <string>

static void Run(const char* cmd) {
    const std::string reply = immirror::editor::HandleEditorCommand(cmd);
    std::printf("== %s\n%s", cmd, reply.c_str());
}

int main() {
    immirror::editor::ResetEditorCommandStateForTest();
    const std::string root = "temp/editor-diag";
    immirror::editor::MakeDirectories(root);
    Run("init temp/editor-diag");
    Run("new 填充工程");
    Run("projects");
    /* 直接取 projects 行的第一列目录. */
    const std::string list = immirror::editor::HandleEditorCommand("projects");
    const size_t at = list.find("project=");
    const size_t nl = list.find("\n", at);
    std::string dir = list.substr(at + 8, nl - at - 8);
    const size_t pipe = dir.find("|");
    if (pipe != std::string::npos) {
        dir = dir.substr(0, pipe);
    }
    std::printf("== dir=%s\n", dir.c_str());
    Run(("open " + dir).c_str());
    Run("cursor 0 60 0");
    Run("sel a");
    Run("cursor 2 62 2");
    Run("sel b");
    Run("block stone");
    Run("fill");
    Run("status");
    Run("sel clear");
    Run("status");
    return 0;
}
