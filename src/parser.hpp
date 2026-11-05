#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "diagnostic.hpp"
#include "lexer.hpp"
#include "node.hpp"

namespace cord {

/**
 * @brief The parsed tree, and the problems found while building it.
 *
 * With errors, the tree only holds the entries that parsed. It is not meant to be bound.
 */
struct ParseResult {
    Object tree;
    std::vector<Diagnostic> diagnostics;
};

/**
 * @brief Builds the untyped tree of a config. See docs/grammar.md, section 2.
 *
 * Does not own the source: it must outlive the parser. The result does not depend on it.
 */
class Parser {
public:
    explicit Parser(std::string_view source) : _lexer(source), _current(_lexer.next()), _result{} {}

    /**
     * @brief Parses the whole source, resuming at the next line after an error.
     *
     * Stops if the entry right after a failed one fails too.
     */
    ParseResult parse() {
        bool in_recovery = false;
        while (_current.type != TokenType::Eof) {
            if (_current.type == TokenType::Newline) {
                _advance();
                continue;
            }
            if (_parseLine()) {
                in_recovery = false;
                continue;
            }
            if (in_recovery) break;
            in_recovery = true;
            _recover();
        }

        return std::move(_result);
    }

private:
    Lexer _lexer;
    Token _current;
    ParseResult _result;

    void _advance() {
        _current = _lexer.next();
    }

    [[nodiscard]] bool _expect(TokenType type, std::string_view expected) {
        if (_current.type != type) {
            _pushExpected(expected);
            return false;
        }
        _advance();
        return true;
    }

    void _pushDiagnostic(const Token& at, std::string message) {
        _result.diagnostics.push_back(Diagnostic {
            .severity = Severity::Error,
            .line = at.line,
            .column = at.column,
            .path = {},
            .message = std::move(message),
        });
    }

    // A lexer error token already says what is wrong, more precisely than "expected".
    void _pushExpected(std::string_view expected) {
        if (_current.type == TokenType::Error) {
            _pushDiagnostic(_current, std::string{_current.message} + ": " + _describe(_current));
            return;
        }
        _pushDiagnostic(_current, "Expected " + std::string{expected} + ", got: " + _describe(_current));
    }

    static std::string _describe(const Token& token) {
        switch (token.type) {
            case TokenType::Eof: return "end of file";
            case TokenType::Newline: return "end of line";
            default: return "'" + std::string{token.lexeme} + "'";
        }
    }

    std::string _treatLexeme(std::string_view sv) {
        sv.remove_prefix(1);
        sv.remove_suffix(1);

        std::string decoded;
        decoded.reserve(sv.size());
        for (std::size_t i = 0; i < sv.size(); i++) {
            if (sv[i] != '\\') {
                decoded += sv[i];
                continue;
            }
            switch (sv[++i]) {
                case 'n': decoded += '\n'; break;
                case 't': decoded += '\t'; break;
                case 'r': decoded += '\r'; break;
                default: decoded += sv[i]; break;
            }
        }
        return decoded;
    }

    // seeks to the next Newline or Eof for best effort recovery
    void _recover() {
        while (_current.type != TokenType::Newline && _current.type != TokenType::Eof)
            _advance();
    }

    // A valid entry stays in the tree even if garbage follows it, but the line still counts as failed.
    bool _parseLine() {
        auto entry = _parseEntry();
        if (!entry) return false;
        _result.tree.push_back(std::move(*entry));

        if (_current.type == TokenType::Newline || _current.type == TokenType::Eof) return true;
        _pushExpected("a new line after a value");
        return false;
    }

    std::optional<Entry> _parseEntry() {
        std::string_view key = _current.lexeme;
        if (!_expect(TokenType::Key, "a key")) return std::nullopt;
        if (!_expect(TokenType::Equals, "an equal sign ('=')")) return std::nullopt;

        auto value = _parseValue();
        if (!value) return std::nullopt;
        return Entry {
            .key = std::string{key},
            .value = std::move(*value),
        };
    }

    std::optional<Node> _parseValue() {
        Node node{};
        node.line = _current.line;
        node.column = _current.column;

        switch (_current.type) {
            case TokenType::Bool:   node.value = Bool{_current.lexeme == "true"}; break;
            case TokenType::Int:    node.value = Int{std::string{_current.lexeme}}; break;
            case TokenType::Float:  node.value = Float{std::string{_current.lexeme}}; break;
            case TokenType::String: node.value = String{_treatLexeme(_current.lexeme)}; break;
            case TokenType::LBracket: return _parseArray();
            case TokenType::LBrace:   return _parseObject();
            default:
                _pushExpected("a value");
                return std::nullopt;
        }
        _advance();
        return node;
    }

    std::optional<Node> _parseArray() {
        const Token opener = _current;
        _advance();

        Array arr{};
        while (true) {
            if (_current.type == TokenType::RBracket) {
                _advance();
                return Node{std::move(arr), opener.line, opener.column};
            }
            if (_current.type == TokenType::Eof) {
                _pushDiagnostic(opener, "Reached end of file before a closing ']'");
                break;
            }

            auto value = _parseValue();
            if (!value) break;
            arr.push_back(std::move(*value));

            if (_current.type == TokenType::Comma) {
                _advance();
            } else if (_current.type != TokenType::RBracket && _current.type != TokenType::Eof) {
                _pushExpected("',' or ']'");
                break;
            }
        }

        return std::nullopt;
    }

    std::optional<Node> _parseObject() {
        const Token opener = _current;
        _advance();

        Object obj{};
        while (true) {
            if (_current.type == TokenType::RBrace) {
                _advance();
                return Node{std::move(obj), opener.line, opener.column};
            }
            if (_current.type == TokenType::Eof) {
                _pushDiagnostic(opener, "Reached end of file before a closing '}'");
                break;
            }

            auto entry = _parseEntry();
            if (!entry) break;
            obj.push_back(std::move(*entry));

            if (_current.type == TokenType::Comma) {
                _advance();
            } else if (_current.type != TokenType::RBrace && _current.type != TokenType::Eof) {
                _pushExpected("',' or '}'");
                break;
            }
        }

        return std::nullopt;
    }
};

} // namespace cord
