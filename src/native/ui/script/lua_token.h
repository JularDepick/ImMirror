#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace immirror::ui::script {

/* 词法单元: 解析器与词法器共用. */
struct Token {
    enum class Kind : uint8_t { kName, kNumber, kString, kSymbol, kKeyword, kEnd };
    Kind kind = Kind::kEnd;
    std::string text;
    double number = 0.0;
    /* 所在行号, 从 1 起; 语法错误按它定位. */
    int32_t line = 0;
};

/* 词法分析: 成功返回 true 并填满 tokens, 末尾追加一个 kEnd. */
bool Tokenize(const char* source, size_t length, std::vector<Token>& out, std::string* error);

}  /* namespace immirror::ui::script */
