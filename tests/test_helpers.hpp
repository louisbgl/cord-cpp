#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <variant>

#include "lexer.hpp"
#include "node.hpp"
#include "overloaded.hpp"

/**
 * @brief Wraps text in backticks with control and non-ASCII bytes spelled out, e.g. `a<CR><LF>`.
 *
 * Backslashes stay as they are, so a real newline can't be confused with an escape in a string.
 */
inline std::string show(std::string_view text) {
    std::string result = "`";
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '\n') {
            result += "<LF>";
        } else if (c == '\r') {
            result += "<CR>";
        } else if (c == '\t') {
            result += "<TAB>";
        } else if (byte < 0x20 || byte >= 0x7F) {
            char hex[8];
            std::snprintf(hex, sizeof(hex), "<%02X>", byte);
            result += hex;
        } else {
            result += c;
        }
    }
    return result + "`";
}

/**
 * @brief The name of a token type, as written in the enum.
 */
inline std::string tokenName(cord::TokenType type) {
    switch (type) {
        case cord::TokenType::Key: return "Key";
        case cord::TokenType::Bool: return "Bool";
        case cord::TokenType::Int: return "Int";
        case cord::TokenType::Float: return "Float";
        case cord::TokenType::String: return "String";
        case cord::TokenType::Equals: return "Equals";
        case cord::TokenType::Comma: return "Comma";
        case cord::TokenType::LBracket: return "LBracket";
        case cord::TokenType::RBracket: return "RBracket";
        case cord::TokenType::LBrace: return "LBrace";
        case cord::TokenType::RBrace: return "RBrace";
        case cord::TokenType::Newline: return "Newline";
        case cord::TokenType::Eof: return "Eof";
        case cord::TokenType::Error: return "Error";
    }
    return "?";
}

/**
 * @brief Prints text as a string literal, so a decoded newline stays on one line.
 */
inline std::string dump_string(std::string_view text) {
    std::string result = "\"";
    for (const char c : text) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += c;
        }
    }
    return result + "\"";
}

inline std::string dump(const cord::Node& node);

/**
 * @brief Prints entries as `key = value`, joined by the separator.
 */
inline std::string dump_entries(const cord::Object& entries, std::string_view separator) {
    std::string result;
    for (std::size_t i = 0; i < entries.size(); i++) {
        if (i > 0) result += separator;
        result += entries[i].key + " = " + dump(entries[i].value);
    }
    return result;
}

/**
 * @brief Prints a node in a normalized form: `[1, 2]`, `{ a = 1, b = 2 }`, `[]`, `{}`.
 */
inline std::string dump(const cord::Node& node) {
    return std::visit(cord::overloaded{
        [](const cord::Bool& value) -> std::string { return value.value ? "true" : "false"; },
        [](const cord::Int& value) -> std::string { return value.text; },
        [](const cord::Float& value) -> std::string { return value.text; },
        [](const cord::String& value) -> std::string { return dump_string(value.value); },
        [](const cord::Array& elements) -> std::string {
            std::string result = "[";
            for (std::size_t i = 0; i < elements.size(); i++) {
                if (i > 0) result += ", ";
                result += dump(elements[i]);
            }
            return result + "]";
        },
        [](const cord::Object& entries) -> std::string {
            if (entries.empty()) return "{}";
            return "{ " + dump_entries(entries, ", ") + " }";
        },
    }, node.value);
}

/**
 * @brief Prints a whole tree, one top-level entry per line.
 */
inline std::string dump(const cord::Object& tree) {
    return dump_entries(tree, "\n");
}
