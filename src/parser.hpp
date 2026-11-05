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
 * With an error, the tree only holds the entries parsed before it.
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
     * @brief Parses the whole source, stopping at the first error.
     */
    ParseResult parse() {
        while (_current.type != TokenType::Eof) {
            switch (_current.type) {
                case TokenType::Newline: {
                    _advance();
                    break;
                }
                case TokenType::Key: {
                    auto entry = _parseEntry();
                    if (!entry) return std::move(_result);
                    _result.tree.push_back(entry.value());
                    if (_current.type != TokenType::Newline && _current.type != TokenType::Eof) {
                        _pushDiagnostic("Expected a new line after a value, got: " + _describe(_current));
                        return std::move(_result);
                    }
                    break;
                }
                case TokenType::Error: {
                    _pushDiagnostic();
                    return std::move(_result);
                }
                default: {
                    _pushDiagnostic("Expected a key, got: " + _describe(_current));
                    return std::move(_result);
                }
            }
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

    [[nodiscard]] bool _expect(TokenType type, std::string expected) {
        if (_current.type != type) {
            _pushDiagnostic("Expected " + expected + ", got: " + _describe(_current));
            return false;
        }
        _advance();
        return true;
    }

    // msg overrides the tokens message if provided
    void _pushDiagnostic(std::string msg = {}) {
        if (msg.empty()) msg = std::string{_current.message};
        _result.diagnostics.push_back(Diagnostic {
            .severity = Severity::Error,
            .line = _current.line,
            .column = _current.column,
            .path = {},
            .message = msg,
        });
    }

    // msg overrides the tokens message if provided
    void _pushDiagnostic(Token t, std::string msg = {}) {
        if (msg.empty()) msg = std::string{t.message};
        _result.diagnostics.push_back(Diagnostic {
            .severity = Severity::Error,
            .line = t.line,
            .column = t.column,
            .path = {},
            .message = msg,
        });
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

    std::optional<Entry> _parseEntry() {
        std::string_view key = _current.lexeme;
        if (!_expect(TokenType::Key, "a key")) return std::nullopt;
        if (!_expect(TokenType::Equals, "an equal sign ('=')")) return std::nullopt;
        auto val = _parseValue();
        if (val.has_value()) {
            return Entry {
                .key = std::string{key},
                .value = val.value(),
            };
        }
        return std::nullopt;
    }

    std::optional<Node> _parseValue() {
        Node node{};
        node.line = _current.line;
        node.column = _current.column;
        
        switch (_current.type) {
            case TokenType::Bool: {
                node.value = Bool{(_current.lexeme == "true")};
                _advance();
                break;
            }
            case TokenType::Int: {
                node.value = Int{std::string{_current.lexeme}};
                _advance();
                break;
            }
            case TokenType::Float: {
                node.value = Float{std::string{_current.lexeme}};
                _advance();
                break;
            }
            case TokenType::String: {
                node.value = String{_treatLexeme(_current.lexeme)};
                _advance();
                break;
            }
            case cord::TokenType::LBracket: {
                auto val = _parseArray();
                if (val.has_value()) {
                    node.value = val.value();
                    break;
                } else {
                    return std::nullopt;
                }
            }
            case cord::TokenType::LBrace: {
                auto val = _parseObject();
                if (val.has_value()) {
                    node.value = val.value();
                    break;
                } else {
                    return std::nullopt;
                }
            }
            case TokenType::Newline:
            case TokenType::Eof: {
                _pushDiagnostic("Expected a value after the equal sign ('=')");
                return std::nullopt;
            }
            default: {
                _pushDiagnostic("Could not convert ` " + _describe(_current) + " ` to a value");
                return std::nullopt;
            }
        }
        return node;
    }

    std::optional<Array> _parseArray() {
        const Token opener = _current;
        _advance();
        
        Array arr{};
        while (true) {
            if (_current.type == TokenType::RBracket) {
                _advance();
                return arr;
            }
            if (_current.type == TokenType::Eof) {
                _pushDiagnostic(opener, "Reached end of file before a closing bracket (']')");
                break;
            }
            
            auto value = _parseValue();
            if (!value) break;
            arr.push_back(value.value());

            if (_current.type == TokenType::Comma) {
                _advance();
            } else if (_current.type == TokenType::Eof || _current.type == TokenType::RBracket) {
                continue;
            } else {
                _pushDiagnostic("Expected ',' or ']', got: " + _describe(_current));
                break;
            }
        }
        
        return std::nullopt;
    }

    std::optional<Object> _parseObject() {
        const Token opener = _current;
        _advance();

        Object obj{};
        while (true) {
            if (_current.type == TokenType::RBrace) {
                _advance();
                return obj;
            }
            if (_current.type == TokenType::Eof) {
                _pushDiagnostic(opener, "Reached end of file before a closing brace ('}')");
                break;
            }
            
            auto entry = _parseEntry();
            if (!entry) break;
            obj.push_back(entry.value());

            if (_current.type == TokenType::Comma) {
                _advance();
            } else if (_current.type == TokenType::Eof || _current.type == TokenType::RBrace) {
                continue;
            } else {
                _pushDiagnostic("Expected ',' or ']', got: " + _describe(_current));
                break;
            }
        }

        return std::nullopt;
    }
};

} // namespace cord
