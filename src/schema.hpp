#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <memory>
#include <source_location>
#include <cassert>

#include "common.hpp"
#include "field.hpp"
#include "errors.hpp"
#include "exception.hpp"

namespace cord {

/**
 * @brief Represents the result of parsing input.
 */
class Result {

friend class Schema;

public:
    // Checks if a key exists in the result.
    bool contains(std::string_view key) const {
        if (_findValue(key) != -1) return true;
        return false;
    }

    /**
     * @brief Gets the value associated with a key.
     * @param key The key to look up.
     * @return The Value associated with the key.
     * @throws CordException if the key is not found.
     *
     * @note Recommended to chain with .as<T>() to get the value as the expected type with a one-liner.
     */
    const Value& get(std::string_view key,
                     std::source_location loc = std::source_location::current()) const {
        int index = _findValue(key);
        if (index != -1) {
            return _values.at(index).second;
        } else {
            throw CordException(loc.file_name(), loc.line(), "Key not found: " + std::string(key));
        }
    }

    /**
     * @brief Gets the value associated with a key or returns a fallback value.
     * @param key The key to look up.
     * @param fallback The fallback value to return if the key is not found.
     * @return The Value associated with the key or the fallback value.
     *
     * @note Recommended to chain with .as<T>() to get the value as the expected type with a one-liner.
     * @note Compile-time checks are performed to ensure that only supported types are used.
     */
    template<typename T>
    Value get_or(std::string_view key, T fallback) const {
        static_assert(is_supported_type_v<T>, CORD_UNSUPPORTED_TYPE("result.get_or<T>()"));
        int index = _findValue(key);
        if (index != -1) {
            return _values.at(index).second;
        }
        return Value(fallback);
    }

    // Checks if there are any parsing errors
    bool hasErrors() const {
        return _ec.hasErrors();
    }

    // Prints all errors to std::cerr, with file and line context where available.
    void printErrors() const {
        for (const auto& error : _ec.getErrors()) {
            std::cerr << "[CORD] ";

            if (!_filepath.empty() && error.line.has_value())
                std::cerr << _filepath << ":" << error.line.value() << ": ";
            else if (!_filepath.empty())
                std::cerr << _filepath << ": ";
            else if (error.line.has_value())
                std::cerr << "Line " << error.line.value() << ": ";

            std::cerr << error.message << "\n";
        }
    }

    const std::vector<ParseError> getErrors() const {
        return _ec.getErrors();
    }

private:
    std::vector<std::pair<std::string, Value>> _values;
    ErrorCollector _ec;
    std::string _filepath;

    // Finds the index of a key in the _values vector, returns -1 if not found
    int _findValue(std::string_view key) const {
        for (size_t i = 0; i < _values.size(); ++i) {
            if (_values[i].first == key) return static_cast<int>(i);
        }
        return -1;
    }

    // Inserts or modifies a key-value pair in the _values vector
    void _insert_or_modify_value(std::string_view key, const Value& value) {
        int index = _findValue(key);
        if (index != -1) {
            _values[index].second = value;
        } else {
            _values.emplace_back(std::string(key), value);
        }
    }
};

/**
 * @brief Represents a schema for parsing input.
 * Is the main workhorse of the library.
 *
 * @note Compile-time checks are performed to ensure that only supported types are used.
 */
class Schema {
public:
    /**
     * @brief Parses the input string according to the schema.
     * @param input The input string to parse.
     * @return A Result object containing the parsed values and any errors.
     */
    Result parse(const std::string_view input) {
        Result result;
        result._filepath = _filepath;

        // split input into lines
        std::vector<std::string_view> lines;
        size_t start = 0;
        size_t end = 0;
        while ((end = input.find('\n', start)) != std::string_view::npos) {
            lines.push_back(input.substr(start, end - start));
            start = end + 1;
        }
        lines.push_back(input.substr(start));

        for (size_t i = 0; i < lines.size(); ++i) {
            std::string_view trimmed_line = _trim(lines[i]);
            if (trimmed_line.empty()) continue;
            if (_allow_comments && trimmed_line.substr(0, _comment_marker.length()) == _comment_marker) continue;

            size_t delimiter_pos = trimmed_line.find(_delimiter);
            if (delimiter_pos == std::string_view::npos) {
                result._ec.addError("Missing delimiter (" + std::string(_delimiter) + ") on line: \"" + std::string(lines[i]) + "\"", std::nullopt, static_cast<int>(i + 1));
                continue;
            }

            std::string_view cleaned_line = trimmed_line;
            if (_allow_comments) {
                cleaned_line = _removeInlineComment(trimmed_line);
                cleaned_line = _trim(cleaned_line);
            }

            std::string_view key = _trim(cleaned_line.substr(0, delimiter_pos));
            std::string_view value_str = _trim(cleaned_line.substr(delimiter_pos + _delimiter.length()));

            IField* field = nullptr;
            for (const auto& f : _fields) {
                if (_case_insensitive) {
                    if (toLower(f->getName()) != toLower(key)) continue;
                } else {
                    if (f->getName() != key) continue;
                }
                field = f.get();
                break;
            }

            if (!field) {
                if (_strict) result._ec.addError("Unexpected key in strict mode: " + std::string(key), std::nullopt, static_cast<int>(i + 1));
                continue;
            }

            bool parsed = false;
            std::string parse_error;
            switch (field->getType()) {
                case FieldType::BOOL:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseBool); break;
                case FieldType::STRING:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseString); break;
                case FieldType::INT:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseInt); break;
                case FieldType::FLOAT:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseFloat); break;
                case FieldType::DOUBLE:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseDouble); break;
                case FieldType::VECTOR_BOOL:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseVectorBool); break;
                case FieldType::VECTOR_INT:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseVectorInt); break;
                case FieldType::VECTOR_FLOAT:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseVectorFloat); break;
                case FieldType::VECTOR_DOUBLE:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseVectorDouble); break;
                case FieldType::VECTOR_STRING:
                    parsed = _tryParseAndStore(result, field, value_str, parse_error, &Schema::_tryParseVectorString); break;
            }

            if (!parsed) {
                std::string msg = "Invalid value for '" + std::string(key) + "'";
                if (!parse_error.empty()) msg += ": " + parse_error;
                result._ec.addError(msg, std::nullopt, static_cast<int>(i + 1));
            } else {
                checkFieldConstraints(result, field, i + 1);
            }
        }

        ensureRequiredFieldsPresent(result);
        applyDefaultValues(result);
        return result;
    }

    // Thin wrapper around parse() for convenience
    Result parseFile(const std::string& filename) {
        _filepath = filename;
        std::ifstream file(filename);
        if (!file.is_open()) {
            Result result;
            result._filepath = _filepath;
            result._ec.addError("Failed to open file: " + filename);
            return result;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str());
    }

    // Prints to std::cout a C-style struct representation of the schema
    void describe() const {
        static constexpr size_t GROUP_THRESHOLD = 10;

        size_t max_type_len = 0;
        size_t max_name_len = 0;
        for (const auto& f : _fields) {
            max_type_len = std::max(max_type_len, fieldTypeName(f->getType()).size());
            max_name_len = std::max(max_name_len, f->getName().size());
        }

        auto print_field = [&](const std::unique_ptr<IField>& f) {
            std::string type = fieldTypeName(f->getType());
            std::string name = f->getName();
            std::cout << "  " << type << std::string(max_type_len - type.size() + 2, ' ');
            std::cout << name;

            std::string modifier;
            if (f->isRequired())      modifier = "(required)";
            else if (f->hasDefault()) modifier = "(default=" + f->getDefault().toString() + ")";

            if (!modifier.empty()) {
                std::cout << std::string(max_name_len - name.size() + 2, ' ') << modifier;
            }

            std::string constraints = f->describeConstraints();
            if (!constraints.empty()) {
                size_t mod_pad = modifier.empty() ? max_name_len - name.size() + 2 : 2;
                std::cout << std::string(mod_pad, ' ') << constraints;
            }

            std::cout << "\n";
        };

        std::cout << "Schema {\n";

        if (_fields.size() <= GROUP_THRESHOLD) {
            for (const auto& f : _fields) print_field(f);
        } else {
            static const std::vector<std::vector<FieldType>> groups = {
                { FieldType::INT, FieldType::FLOAT, FieldType::DOUBLE },
                { FieldType::BOOL },
                { FieldType::STRING },
                { FieldType::VECTOR_INT, FieldType::VECTOR_FLOAT, FieldType::VECTOR_DOUBLE, FieldType::VECTOR_BOOL, FieldType::VECTOR_STRING },
            };

            bool first_group = true;
            for (const auto& group : groups) {
                bool printed_any = false;
                for (FieldType type : group) {
                    for (const auto& f : _fields) {
                        if (f->getType() != type) continue;
                        if (!printed_any && !first_group) std::cout << "\n";
                        print_field(f);
                        printed_any = true;
                    }
                }
                if (printed_any) first_group = false;
            }
        }

        std::cout << "}\n";
    }

    /**
     * @brief Adds a field to the schema
     * @tparam T The type of the field
     * @param name The name of the field
     * @return A reference to the added field
     *
     * @note Compile-time checks are performed to ensure that only supported types are used.
     */
    template<typename T>
    Field<T>& add(std::string name) {
        static_assert(is_supported_value_type_v<T>, CORD_UNSUPPORTED_TYPE("schema.add<T>()"));
        auto field = std::make_unique<Field<T>>(name);
        Field<T>& ptr = *field;
        _fields.push_back(std::move(field));
        return ptr;
    }

    // Sets the schema to strict mode, where unknown keys will result in errors
    // Defaults to false
    void setStrict(bool strict) {
        _strict = strict;
    }

    // Enables case-insensitive key matching (e.g. "Port" matches field "port")
    // Defaults to false
    void setCaseInsensitive(bool enabled) {
        _case_insensitive = enabled;
    }

    // Sets whether comments are allowed in the input
    // Defaults to true
    void setAllowComments(bool allow) {
        _allow_comments = allow;
    }

    // Sets the delimiter for key-value pairs, '=' is the default
    void setDelimiter(const char delimiter) {
        _delimiter = delimiter;
    }

    /**
     * @brief Sets the delimiter for key-value pairs.
     * @param delimiter The delimiter.
     * @throws CordException if the delimiter is empty.
     *
     * @note "=" is the default.
     */
    void setDelimiter(const std::string& delimiter,
                      std::source_location loc = std::source_location::current()) {
        if (delimiter.empty()) {
            throw CordException(loc.file_name(), loc.line(), "Delimiter cannot be empty");
        }
        _delimiter = delimiter;
    }

    // Sets the comment marker for comments, '#' is the default
    void setCommentMarker(const char marker) {
        _comment_marker = marker;
    }

    /**
     * @brief Sets the comment marker for comments.
     * @param marker The comment marker.
     * @throws CordException if the marker is empty.
     *
     * @note "#" is the default.
     */
    void setCommentMarker(const std::string& marker,
                          std::source_location loc = std::source_location::current()) {
        if (marker.empty()) {
            throw CordException(loc.file_name(), loc.line(), "Comment marker cannot be empty");
        }
        _comment_marker = marker;
    }

private:
    std::vector<std::unique_ptr<IField>> _fields;
    bool _strict = false;
    bool _allow_comments = true;
    bool _case_insensitive = false;
    std::string _delimiter = "=";
    std::string _comment_marker = "#";
    std::string _filepath;

    void checkFieldConstraints(Result& result, IField* field, size_t line) const {
        if (!result.contains(field->getName())) return;
        auto err = field->checkConstraints(result.get(field->getName()));
        if (err.has_value())
            result._ec.addError("Constraint violation for '" + field->getName() + "': " + *err, field->getName(), line);
    }

    void ensureRequiredFieldsPresent(Result& result) const {
        for (const auto& field : _fields) {
            std::string name = field->getName();
            if (field->isRequired() && !result.contains(name)) {
                result._ec.addMissingRequiredKey(name);
            }
        }
    }

    void applyDefaultValues(Result& result) const {
        for (const auto& field : _fields) {
            std::string name = field->getName();
            if (!field->hasDefault()) continue;
            if (result.contains(name)) continue;
            result._insert_or_modify_value(name, field->getDefault());
        }
    }

    std::string_view _trim(std::string_view s) const {
        size_t start = 0;
        while (start < s.size() && std::isspace(s[start])) ++start;
        size_t end = s.size();
        while (end > start && std::isspace(s[end - 1])) --end;
        return s.substr(start, end - start);
    }

    std::string_view _removeInlineComment(std::string_view s) const {
        bool in_quotes = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '"') {
                in_quotes = !in_quotes;
            } else if (s.substr(i, _comment_marker.length()) == _comment_marker && !in_quotes) {
                return s.substr(0, i);
            }
        }
        return s;
    }

    template<typename T>
    bool _tryParseAndStore(Result& result, IField* field, std::string_view value_str,
                           std::string& parse_error,
                           ParseResult<T> (Schema::*parse_fn)(std::string_view) const) const {
        auto res = (this->*parse_fn)(value_str);
        if (res.value.has_value()) {
            result._insert_or_modify_value(field->getName(), Value(*res.value));
            return true;
        }
        parse_error = res.error;
        return false;
    }

    ParseResult<int> _tryParseInt(const std::string_view str) const {
        try {
            size_t idx;
            int value = std::stoi(std::string(str), &idx);
            if (idx != str.size()) return {{}, "\"" + std::string(str) + "\" is not a valid int"};
            return {value};
        } catch (const std::out_of_range&) {
            return {{}, "value out of range for int: \"" + std::string(str) + "\""};
        } catch (const std::invalid_argument&) {
            return {{}, "\"" + std::string(str) + "\" is not a valid int"};
        }
    }

    ParseResult<double> _tryParseDouble(const std::string_view str) const {
        try {
            size_t idx;
            double value = std::stod(std::string(str), &idx);
            if (idx != str.size()) return {{}, "\"" + std::string(str) + "\" is not a valid double"};
            return {value};
        } catch (const std::out_of_range&) {
            return {{}, "value out of range for double: \"" + std::string(str) + "\""};
        } catch (const std::invalid_argument&) {
            return {{}, "\"" + std::string(str) + "\" is not a valid double"};
        }
    }

    ParseResult<float> _tryParseFloat(const std::string_view str) const {
        try {
            size_t idx;
            float value = std::stof(std::string(str), &idx);
            if (idx != str.size()) return {{}, "\"" + std::string(str) + "\" is not a valid float"};
            return {value};
        } catch (const std::out_of_range&) {
            return {{}, "value out of range for float: \"" + std::string(str) + "\""};
        } catch (const std::invalid_argument&) {
            return {{}, "\"" + std::string(str) + "\" is not a valid float"};
        }
    }

    ParseResult<bool> _tryParseBool(const std::string_view str) const {
        if (str == "true") return {true};
        if (str == "false") return {false};
        return {{}, "expected 'true' or 'false', got: \"" + std::string(str) + "\""};
    }

    ParseResult<std::string> _tryParseString(const std::string_view str) const {
        if (str.size() < 2 || str.front() != '"' || str.back() != '"')
            return {{}, "string values must be quoted with \""};

        std::string out;
        out.reserve(str.size() - 2);
        for (size_t i = 1; i < str.size() - 1; ++i) {
            if (str[i] != '\\') { out += str[i]; continue; }
            if (i + 1 >= str.size() - 1)
                return {{}, "trailing backslash in string"};
            switch (str[++i]) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                default:
                    return {{}, std::string("unknown escape sequence: \\") + str[i]};
            }
        }
        return {out};
    }

    std::vector<std::string_view> _splitCommas(std::string_view str) const {
        std::vector<std::string_view> result;
        size_t start = 0;
        while (start < str.size()) {
            size_t end = str.find(',', start);
            if (end == std::string_view::npos) end = str.size();
            std::string_view item = _trim(str.substr(start, end - start));
            result.push_back(item);
            start = end + 1;
        }
        return result;
    }

    VectorElements _extractVectorElements(std::string_view str) const {
        if (str.empty() || str.front() != '[')
            return {{}, "expected '[' to open vector, got: \"" + std::string(str) + "\""};

        size_t close_bracket = str.find(']');
        if (close_bracket == std::string_view::npos)
            return {{}, "missing closing ']' in vector value"};

        std::string_view inner = _trim(str.substr(1, close_bracket - 1));
        if (inner.empty()) return {{}, ""};

        return {_splitCommas(inner), ""};
    }

    template<typename T, typename ParseFn>
    ParseResult<std::vector<T>> _tryParseVector(std::string_view str, ParseFn parse_elem) const {
        auto extracted = _extractVectorElements(str);
        if (!extracted.error.empty()) return {{}, extracted.error};
        std::vector<T> result;
        for (size_t idx = 0; idx < extracted.items.size(); ++idx) {
            auto res = parse_elem(extracted.items[idx]);
            if (!res.value.has_value())
                return {{}, "element at index " + std::to_string(idx) + ": " + res.error};
            result.push_back(*res.value);
        }
        return {result};
    }

    ParseResult<std::vector<bool>> _tryParseVectorBool(std::string_view str) const {
        return _tryParseVector<bool>(str, [this](std::string_view s) { return _tryParseBool(s); });
    }

    ParseResult<std::vector<int>> _tryParseVectorInt(std::string_view str) const {
        return _tryParseVector<int>(str, [this](std::string_view s) { return _tryParseInt(s); });
    }

    ParseResult<std::vector<float>> _tryParseVectorFloat(std::string_view str) const {
        return _tryParseVector<float>(str, [this](std::string_view s) { return _tryParseFloat(s); });
    }

    ParseResult<std::vector<double>> _tryParseVectorDouble(std::string_view str) const {
        return _tryParseVector<double>(str, [this](std::string_view s) { return _tryParseDouble(s); });
    }

    ParseResult<std::vector<std::string>> _tryParseVectorString(std::string_view str) const {
        return _tryParseVector<std::string>(str, [this](std::string_view s) { return _tryParseString(s); });
    }
};

} // namespace cord