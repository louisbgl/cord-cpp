#pragma once

#include <cstddef>
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
    Error, // Not in the grammar: invalid input, reported by the parser when it reaches it.
};

/**
 * @brief A single token and where it starts in the source.
 */
struct Token {
    TokenType type;
    std::string_view lexeme;       // Raw slice of the source: a String keeps its quotes and undecoded escapes.
    std::size_t line = 1;          // 1-based.
    std::size_t column = 1;        // 1-based, counted in bytes.
    std::string_view message = {}; // Why the input is invalid. Set only on Error, points to a string literal.
};

/**
 * @brief Turns a config string into tokens, one at a time, on demand.
 *
 * Does not own the source: it must outlive the lexer and every token it returns.
 */
class Lexer {
public:
    explicit Lexer(std::string_view source) : _source(source) {
        // Some Windows editors start files with a UTF-8 byte order mark.
        if (_source.starts_with("\xEF\xBB\xBF")) _source.remove_prefix(3);
    }

    /**
     * @brief Scans and returns the next token.
     *
     * Skips whitespace and comments. Newlines inside brackets are dropped.
     * Once the end of the source is reached, every call returns Eof.
     */
    Token next() {
        _skipCommentedLines();
        _skipWhitespaces();
        if (_lexed >= _source.length()) return _createToken(TokenType::Eof, 0);

        if (_isValidFirstKeyChar(_peek())) return _getKeyToken();
        if (_peek() == '-' || _isDigit(_peek())) return _getNumberToken();
        if (_peek() == '\"') return _getStringToken();

        if (_source.substr(_lexed, 2) == "\r\n") return _createToken(TokenType::Newline, 2);
        switch (_peek()) {
            case '\n': return _createToken(TokenType::Newline, 1);
            case ',': return _createToken(TokenType::Comma, 1);
            case '=': return _createToken(TokenType::Equals, 1);
            case '{': return _createToken(TokenType::LBrace, 1);
            case '}': return _createToken(TokenType::RBrace, 1);
            case '[': return _createToken(TokenType::LBracket, 1);
            case ']': return _createToken(TokenType::RBracket, 1);
        }
        
        return _createToken(TokenType::Error, 1, "Unexpected character");
    }

private:
    std::string_view _source;
    std::size_t _lexed = 0;
    std::size_t _depth = 0; // brace or bracket depth
    std::size_t _line = 1;
    std::size_t _column = 1;

    Token _createToken(TokenType type, std::size_t size, std::string_view msg = {}) {
        Token t = {
            .type = type,
            .lexeme = _source.substr(_lexed, size),
            .line = _line,
            .column = _column,
            .message = msg,
        };
        if (type == TokenType::LBracket) _depth++;
        if (type == TokenType::LBrace)   _depth++;
        if (type == TokenType::RBracket && _depth > 0) _depth--;
        if (type == TokenType::RBrace   && _depth > 0) _depth--;
        _advance(size);
        return t;
    }

    char _peek(std::size_t count = 0) const {
        if (_lexed + count >= _source.length()) return '\0';
        return _source.at(_lexed + count);
    }

    void _advance(std::size_t count = 1) {
        while (count > 0) {
            if (_peek() == '\n') {
                _column = 1;
                _line++;
            } else {
                _column++;
            }
            _lexed++;
            count--;
        }
    }
    
    void _skipWhitespaces() {
        while (true) {
            const char c = _peek();
            if (c == ' ' || c == '\t') {
                _advance();
            } else if (_depth > 0 && _lexed < _source.length() && _isLineEnd()) {
                _advance();
                if (_peek() == '\n') _advance(); // newline was CRLF
            } else {
                break;
            }
        }
    }

    void _skipCommentedLines() {
        while (true) {
            _skipWhitespaces();
            if (_peek() == '#') {
                _advance();
                while (!_isLineEnd()) _advance();
            } else {
                break;
            }
        }
    }

    bool _isLineEnd(std::size_t offset = 0) const {
        if (_lexed + offset >= _source.length()) return true;
        const char c = _peek(offset);
        return c == '\n' || (c == '\r' && _peek(offset + 1) == '\n');
    }

    static bool _isEscapable(char c) {
        return c == '"' || c == '\\' || c == 'n' || c == 't' || c == 'r';
    }

    static bool _isLetter(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static bool _isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    std::size_t _countDigits(std::size_t offset = 0) const {
        std::size_t count = 0;
        while (_isDigit(_peek(count + offset))) count++;
        return count;
    }

    static bool _isNumberJunk(char c) {
        return _isLetter(c) || _isDigit(c) || c == '_' || c == '.';
    }

    static bool _isValidFirstKeyChar(char c) {
        return _isLetter(c) || c == '_';
    }
    
    static bool _isValidKeyChar(char c) {
        return _isLetter(c) || _isDigit(c) || c == '_' || c == '-';
    }

    // Can be a Key or a Bool
    Token _getKeyToken() {
        std::size_t idx = 1; // skip past first char

        while (_isValidKeyChar(_peek(idx))) idx++;
        Token t = _createToken(TokenType::Key, idx);
        if (t.lexeme == "true") t.type = TokenType::Bool;
        if (t.lexeme == "false") t.type = TokenType::Bool;
        return t;
    }

    // Can be an Int, a Float or an Error
    Token _getNumberToken() {
        std::size_t idx = _peek() == '-' ? 1 : 0;

        std::size_t nbDigits = _countDigits(idx);
        if (nbDigits == 0) return _createToken(TokenType::Error, idx, "Expected a digit after '-'");
        idx += nbDigits;

        TokenType type = TokenType::Int;
        if (_peek(idx) == '.') {
            nbDigits = _countDigits(++idx);
            if (nbDigits == 0) return _createToken(TokenType::Error, idx, "Expected a digit after '.' in a number");
            idx += nbDigits;
            type = TokenType::Float;
        }

        if (!_isNumberJunk(_peek(idx))) return _createToken(type, idx);
        while (_isNumberJunk(_peek(idx))) idx++;
        return _createToken(TokenType::Error, idx, "Invalid number");
    }

    // Can be a String or an Error
    Token _getStringToken() {
        std::string_view escapeError;
        std::size_t idx = 1; // skip past opening quote

        while (!_isLineEnd(idx)) {
            const char c = _peek(idx);

            if (c == '"') {
                if (!escapeError.empty()) return _createToken(TokenType::Error, idx + 1, escapeError);
                return _createToken(TokenType::String, idx + 1); // include the closing quote
            }

            if (c != '\\') {
                idx++;
                continue;
            }

            if (_isLineEnd(idx + 1)) {
                idx++;
                continue;
            }

            if (!_isEscapable(_peek(idx + 1)) && escapeError.empty()) escapeError = "Unknown escape sequence";
            idx += 2;
        }

        return _createToken(TokenType::Error, idx, "Unterminated string");
    }

};

} // namespace cord
