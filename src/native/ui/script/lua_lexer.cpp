#include "ui/script/lua_token.h"

#include <cstdlib>

namespace immirror::ui::script {
namespace {

bool IsKeyword(const std::string& word) {
    static const char* const kWords[] = {"and",    "break", "do",     "else", "elseif", "end",
                                         "false",  "for",   "function", "if", "in",     "local",
                                         "nil",    "not",   "or",     "repeat", "return", "then",
                                         "true",   "until", "while"};
    for (const char* known : kWords) {
        if (word == known) {
            return true;
        }
    }
    return false;
}

bool IsNameStart(char value) {
    return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || value == '_';
}

bool IsNamePart(char value) {
    return IsNameStart(value) || (value >= '0' && value <= '9');
}

bool IsDigit(char value) {
    return value >= '0' && value <= '9';
}

class Lexer {
public:
    Lexer(const char* source, size_t length) : source_(source), length_(length) {}

    bool Run(std::vector<Token>& out, std::string* error) {
        while (index_ < length_) {
            const char value = source_[index_];
            if (value == ' ' || value == '\t' || value == '\r' || value == '\n') {
                ++index_;
                continue;
            }
            if (value == '-' && index_ + 1 < length_ && source_[index_ + 1] == '-') {
                if (!SkipComment(error)) {
                    return false;
                }
                continue;
            }
            if (IsNameStart(value)) {
                const size_t begin = index_;
                while (index_ < length_ && IsNamePart(source_[index_])) {
                    ++index_;
                }
                Token token;
                token.text = std::string(source_ + begin, index_ - begin);
                token.kind = IsKeyword(token.text) ? Token::Kind::kKeyword : Token::Kind::kName;
                out.push_back(token);
                continue;
            }
            if (IsDigit(value) ||
                (value == '.' && index_ + 1 < length_ && IsDigit(source_[index_ + 1]))) {
                const size_t begin = index_;
                bool dot = false;
                while (index_ < length_) {
                    const char current = source_[index_];
                    if (IsDigit(current)) {
                        ++index_;
                        continue;
                    }
                    if (current == '.' && !dot) {
                        dot = true;
                        ++index_;
                        continue;
                    }
                    break;
                }
                Token token;
                token.kind = Token::Kind::kNumber;
                token.text = std::string(source_ + begin, index_ - begin);
                token.number = std::atof(token.text.c_str());
                out.push_back(token);
                continue;
            }
            if (value == '"' || value == '\'') {
                if (!ReadString(value, out, error)) {
                    return false;
                }
                continue;
            }
            if (value == '[') {
                /* 长括号: [[ ... ]] 与 [==[ ... ]==]. */
                size_t probe = index_ + 1;
                int32_t level = 0;
                while (probe < length_ && source_[probe] == '=') {
                    ++level;
                    ++probe;
                }
                if (probe < length_ && source_[probe] == '[') {
                    index_ = probe + 1;
                    if (!ReadLong(closingOf(level), out, error)) {
                        return false;
                    }
                    continue;
                }
            }
            const char next = index_ + 1 < length_ ? source_[index_ + 1] : '\0';
            std::string symbol(1, value);
            const bool pair = (value == '=' && next == '=') || (value == '~' && next == '=') ||
                              (value == '<' && next == '=') || (value == '>' && next == '=') ||
                              (value == '.' && next == '.');
            if (pair) {
                symbol += std::string(1, next);
                index_ += 2;
            } else {
                index_ += 1;
            }
            Token token;
            token.kind = Token::Kind::kSymbol;
            token.text = symbol;
            out.push_back(token);
        }
        Token end;
        end.kind = Token::Kind::kEnd;
        out.push_back(end);
        return true;
    }

private:
    static std::string closingOf(int32_t level) {
        std::string closing = "]";
        closing.append(static_cast<size_t>(level), '=');
        closing += "]";
        return closing;
    }

    size_t FindFrom(const std::string& needle) const {
        const std::string rest(source_ + index_, length_ - index_);
        const size_t found = rest.find(needle);
        return found == std::string::npos ? std::string::npos : index_ + found;
    }

    bool SkipComment(std::string* error) {
        index_ += 2;
        if (index_ < length_ && source_[index_] == '[') {
            size_t probe = index_ + 1;
            int32_t level = 0;
            while (probe < length_ && source_[probe] == '=') {
                ++level;
                ++probe;
            }
            if (probe < length_ && source_[probe] == '[') {
                index_ = probe + 1;
                const std::string closing = closingOf(level);
                const size_t end = FindFrom(closing);
                if (end == std::string::npos) {
                    *error = "长注释未闭合";
                    return false;
                }
                index_ = end + closing.size();
                return true;
            }
        }
        while (index_ < length_ && source_[index_] != '\n') {
            ++index_;
        }
        return true;
    }

    bool ReadString(char quote, std::vector<Token>& out, std::string* error) {
        ++index_;
        std::string text;
        while (index_ < length_ && source_[index_] != quote) {
            if (source_[index_] == '\\' && index_ + 1 < length_) {
                const char escape = source_[index_ + 1];
                text.push_back(escape == 'n' ? '\n' : (escape == 't' ? '\t' : escape));
                index_ += 2;
                continue;
            }
            text.push_back(source_[index_]);
            ++index_;
        }
        if (index_ >= length_) {
            *error = "字符串未闭合";
            return false;
        }
        ++index_;
        Token token;
        token.kind = Token::Kind::kString;
        token.text = text;
        out.push_back(token);
        return true;
    }

    bool ReadLong(const std::string& closing, std::vector<Token>& out, std::string* error) {
        const size_t end = FindFrom(closing);
        if (end == std::string::npos) {
            *error = "长字符串未闭合";
            return false;
        }
        Token token;
        token.kind = Token::Kind::kString;
        token.text = std::string(source_ + index_, end - index_);
        out.push_back(token);
        index_ = end + closing.size();
        return true;
    }

    const char* source_ = nullptr;
    size_t length_ = 0;
    size_t index_ = 0;
};

}  /* namespace */

bool Tokenize(const char* source, size_t length, std::vector<Token>& out, std::string* error) {
    Lexer lexer(source, length);
    return lexer.Run(out, error);
}

}  /* namespace immirror::ui::script */
