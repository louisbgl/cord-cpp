#pragma once

#include <string_view>

namespace cord {

/**
 * @brief Kinds of token produced by the lexer. See docs/grammar.md, section 1.
 */
enum class TokenType {
    Key,
    Bool,
    Int,
    Float,
    String,
    Equals,
    Comma,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Newline,
    Eof,
};

/**
 * @brief Turns a config string into tokens.
 *
 * The public interface is undecided: full pass over the source or next() on demand.
 */
class Lexer {
public:
    explicit Lexer(std::string_view source) : _source(source) {}

private:
    std::string_view _source;
};

} // namespace cord
