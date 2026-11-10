#include "common.h"

namespace immirror::script {

namespace {

const char* const kScriptVersion = "IMSC-1";

}  /* namespace */

std::string FormatDiagnostic(const char* chunkName, const Diagnostic& diagnostic) {
    const char* name = (chunkName != nullptr && chunkName[0] != '\0') ? chunkName : "<script>";
    std::string result = name;
    result += ':';
    result += std::to_string(diagnostic.line);
    result += ':';
    result += std::to_string(diagnostic.column);
    result += ": ";
    result += diagnostic.message;
    return result;
}

const char* ScriptVersion() { return kScriptVersion; }

}  /* namespace immirror::script */
