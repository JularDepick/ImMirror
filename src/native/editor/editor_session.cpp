#include "editor/editor_session.h"

#include <mutex>

#include "core/log.h"

namespace immirror::editor {

namespace {

constexpr const char* kTag = "ImMirrorEditor";

/* 请求与回调都在 Java 主线程写入, 原生主线程读取, 用互斥量保护. */
std::mutex g_mutex;
SessionRequest g_pending;
ExitHandler g_exitHandler = nullptr;
void* g_exitUser = nullptr;

}  /* namespace */

void RequestSession(const std::string& projectDirectory, const std::string& dataPath) {
    std::lock_guard<std::mutex> guard(g_mutex);
    g_pending.projectDirectory = projectDirectory;
    g_pending.dataPath = dataPath;
    g_pending.valid = true;
    IM_LOGI(kTag, "editor session requested, dir=%s", projectDirectory.c_str());
}

bool TakeSession(SessionRequest& out) {
    std::lock_guard<std::mutex> guard(g_mutex);
    if (!g_pending.valid) {
        return false;
    }
    out = g_pending;
    g_pending = SessionRequest{};
    return true;
}

void CancelSession() {
    std::lock_guard<std::mutex> guard(g_mutex);
    g_pending = SessionRequest{};
}

bool HasPendingSession() {
    std::lock_guard<std::mutex> guard(g_mutex);
    return g_pending.valid;
}

std::string ProjectRoot(const std::string& dataPath) {
    if (dataPath.empty()) {
        return std::string("editor");
    }
    return dataPath + "/editor";
}

std::string GameWorldPath(const std::string& dataPath, const std::string& projectName) {
    std::string stem;
    for (const char c : projectName) {
        const bool alphaNumeric = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                                  (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (alphaNumeric) {
            stem.push_back(c);
        }
    }
    if (stem.empty()) {
        stem = "project";
    }
    if (dataPath.empty()) {
        return "worlds/" + stem + ".world";
    }
    return dataPath + "/worlds/" + stem + ".world";
}

void SetExitHandler(ExitHandler handler, void* user) {
    std::lock_guard<std::mutex> guard(g_mutex);
    g_exitHandler = handler;
    g_exitUser = user;
}

void ClearExitHandler() {
    std::lock_guard<std::mutex> guard(g_mutex);
    g_exitHandler = nullptr;
    g_exitUser = nullptr;
}

void RequestExitToJava() {
    ExitHandler handler = nullptr;
    void* user = nullptr;
    {
        std::lock_guard<std::mutex> guard(g_mutex);
        handler = g_exitHandler;
        user = g_exitUser;
    }
    if (handler != nullptr) {
        handler(user);
    } else {
        IM_LOGW(kTag, "exit requested but no java handler registered");
    }
}

}  /* namespace immirror::editor */
