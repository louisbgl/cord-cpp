#pragma once

#include <cstddef>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>
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
    /**
     * @brief Gets the value associated with a key.
     * @param key The key to look up.
     * @return The Value associated with the key.
     * @throws CordException if the key is not found.
     *
     * @note Recommended to chain with .as<T>() to get the value as the expected type with a one-liner.
     */
    const Value& get(std::string_view key) const {
        auto it = _values.find(std::string(key));
        if (it != _values.end()) {
            return it->second;
        } else {
            throw CordException("Key not found: " + std::string(key));
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
        auto it = _values.find(std::string(key));
        if (it != _values.end()) {
            return it->second;
        }
        return Value(fallback);
    }

    // Checks if there are any parsing errors
    bool hasErrors() const {
        return _ec.hasErrors();
    }

    // Prints all parsing errors to std::cerr
    void printErrors() const {
        for (const auto& error : _ec.getErrors()) {
            std::cerr << "Error";

            if (error.line.has_value()) std::cerr << " line " << error.line.value() << ": ";
            else std::cerr << ": ";

            std::cerr << error.message;

            if (error.key.has_value()) std::cerr << " (key: " << error.key.value() << ")";
            std::cerr << std::endl;
        }
    }

private:
    std::unordered_map<std::string, Value> _values;
    ErrorCollector _ec;
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
     * @throws CordException if the schema is not properly configured.
     */
    Result parse(const std::string_view input) {
        Result result;

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
                result._ec.addError("Missing delimiter (" + std::string(_delimiter) + ") in line: " + std::string(lines[i]), std::nullopt, i + 1);
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
                if (f->getName() == key) {
                    field = f.get();
                    break;
                }
            }

            if (!field) {
                if (_strict) result._ec.addError("Unknown key: " + std::string(key));
                continue;
            }

            // lambda to avoid code duplication
            auto tryParseAndStore = [&](auto tryParseFunc) -> bool {
                auto parsed_value = (this->*tryParseFunc)(value_str);
                if (parsed_value.has_value()) {
                    result._values.insert_or_assign(field->getName(), Value(*parsed_value));
                    return true;
                }
                return false;
            };

            bool parsed = false;
            switch (field->getType()) {
                case FieldType::BOOL:
                    parsed = tryParseAndStore(&Schema::_tryParseBool); break;
                case FieldType::STRING:
                    parsed = tryParseAndStore(&Schema::_tryParseString); break;
                case FieldType::INT:
                    parsed = tryParseAndStore(&Schema::_tryParseInt); break;
                case FieldType::FLOAT:
                    parsed = tryParseAndStore(&Schema::_tryParseFloat); break;
                case FieldType::DOUBLE:
                    parsed = tryParseAndStore(&Schema::_tryParseDouble); break;
                case FieldType::VECTOR_BOOL:
                    parsed = tryParseAndStore(&Schema::_tryParseVectorBool); break;
                case FieldType::VECTOR_INT:
                    parsed = tryParseAndStore(&Schema::_tryParseVectorInt); break;
                case FieldType::VECTOR_FLOAT:
                    parsed = tryParseAndStore(&Schema::_tryParseVectorFloat); break;
                case FieldType::VECTOR_DOUBLE:
                    parsed = tryParseAndStore(&Schema::_tryParseVectorDouble); break;
                case FieldType::VECTOR_STRING:
                    parsed = tryParseAndStore(&Schema::_tryParseVectorString); break;
            }

            if (!parsed) {
                result._ec.addError("Failed to parse value for key: " + std::string(key));
            }
        }

        ensureRequiredFieldsPresent(result);
        applyDefaultValues(result);
        for (const auto& field : _fields) {
            field->validate();
        }
        return result;
    }

    // Thin wrapper around parse() for convenience
    Result parseFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Result result;
            result._ec.addError("Failed to open file: " + filename);
            return result;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str());
    }

    // Prints to std::cout a C-style struct representation of the schema
    void describe() const {
        std::cout << "Schema {" << std::endl;

        for (const auto& field : _fields) {
            switch (field->getType()) {
                case FieldType::BOOL:
                    std::cout << "  bool " << field->getName(); break;
                case FieldType::INT:
                    std::cout << "  int " << field->getName(); break;
                case FieldType::FLOAT:
                    std::cout << "  float " << field->getName(); break;
                case FieldType::DOUBLE:
                    std::cout << "  double " << field->getName(); break;
                case FieldType::STRING:
                    std::cout << "  string " << field->getName(); break;
                case FieldType::VECTOR_BOOL:
                    std::cout << "  vector<bool> " << field->getName(); break;
                case FieldType::VECTOR_INT:
                    std::cout << "  vector<int> " << field->getName(); break;
                case FieldType::VECTOR_FLOAT:
                    std::cout << "  vector<float> " << field->getName(); break;
                case FieldType::VECTOR_DOUBLE:
                    std::cout << "  vector<double> " << field->getName(); break;
                case FieldType::VECTOR_STRING:
                    std::cout << "  vector<string> " << field->getName(); break;
            }

            if (field->hasDefault()) {
                std::cout << " (default = " << field->getDefault().toString() << ")";
            }

            if (field->isRequired()) {
                std::cout << " (required)";
            }

            std::cout << std::endl;
        }
        std::cout << "}" << std::endl;
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
    void setDelimiter(const std::string& delimiter) {
        if (delimiter.empty()) {
            throw CordException("Delimiter cannot be empty");
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
    void setCommentMarker(const std::string& marker) {
        if (marker.empty()) {
            throw CordException("Comment marker cannot be empty");
        }
        _comment_marker = marker;
    }

private:
    std::vector<std::unique_ptr<IField>> _fields;
    bool _strict = false;
    bool _allow_comments = true;
    std::string _delimiter = "=";
    std::string _comment_marker = "#";

    void ensureRequiredFieldsPresent(Result& result) const {
        for (const auto& field : _fields) {
            std::string name = field->getName();
            if (field->isRequired() && result._values.find(name) == result._values.end()) {
                result._ec.addError("Missing required field: " + name);
            }
        }
    }

    void applyDefaultValues(Result& result) const {
        for (const auto& field : _fields) {
            std::string name = field->getName();
            if (!field->hasDefault()) continue;
            if (result._values.find(name) != result._values.end()) continue;
            result._values.emplace(name, field->getDefault());
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

    std::optional<int> _tryParseInt(const std::string_view str) const {
        try {
            size_t idx;
            int value = std::stoi(std::string(str), &idx);
            if (idx != str.size()) {
                return std::nullopt;
            }
            return value;
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<double> _tryParseDouble(const std::string_view str) const {
        try {
            size_t idx;
            double value = std::stod(std::string(str), &idx);
            if (idx != str.size()) {
                return std::nullopt;
            }
            return value;
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<float> _tryParseFloat(const std::string_view str) const {
        auto parsed_double = _tryParseDouble(str);
        if (!parsed_double.has_value()) return std::nullopt;
        return static_cast<float>(*parsed_double);
    }

    std::optional<bool> _tryParseBool(const std::string_view str) const {
        if (str == "true") return true;
        if (str == "false") return false;
        return std::nullopt;
    }

    std::optional<std::string> _tryParseString(const std::string_view str) const {
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            return std::string(str.substr(1, str.size() - 2));
        }
        return std::nullopt;
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

    std::optional<std::vector<std::string_view>> _extractVectorElements(std::string_view str) const {
        if (str.empty() || str.front() != '[') return std::nullopt;

        size_t close_bracket = str.find(']');
        if (close_bracket == std::string_view::npos) return std::nullopt;

        std::string_view inner = _trim(str.substr(1, close_bracket - 1));
        if (inner.empty()) return std::vector<std::string_view>{};

        return _splitCommas(inner);
    }

    std::optional<std::vector<bool>> _tryParseVectorBool(std::string_view str) const {
        auto elements = _extractVectorElements(str);
        if (!elements.has_value()) return std::nullopt;

        if (elements->empty()) return std::vector<bool>{};

        std::vector<bool> result;
        for (const auto& item : elements.value()) {
            auto parsed = _tryParseBool(item);
            if (!parsed.has_value()) return std::nullopt;
            result.push_back(*parsed);
        }
        return result;
    }

    std::optional<std::vector<int>> _tryParseVectorInt(std::string_view str) const {
        auto elements = _extractVectorElements(str);
        if (!elements.has_value()) return std::nullopt;

        if (elements->empty()) return std::vector<int>{};

        std::vector<int> result;
        for (const auto& item : elements.value()) {
            auto parsed = _tryParseInt(item);
            if (!parsed.has_value()) return std::nullopt;
            result.push_back(*parsed);
        }
        return result;
    }

    std::optional<std::vector<float>> _tryParseVectorFloat(std::string_view str) const {
        auto elements = _extractVectorElements(str);
        if (!elements.has_value()) return std::nullopt;

        if (elements->empty()) return std::vector<float>{};

        std::vector<float> result;
        for (const auto& item : elements.value()) {
            auto parsed = _tryParseFloat(item);
            if (!parsed.has_value()) return std::nullopt;
            result.push_back(*parsed);
        }
        return result;
    }

    std::optional<std::vector<double>> _tryParseVectorDouble(std::string_view str) const {
        auto elements = _extractVectorElements(str);
        if (!elements.has_value()) return std::nullopt;

        if (elements->empty()) return std::vector<double>{};

        std::vector<double> result;
        for (const auto& item : elements.value()) {
            auto parsed = _tryParseDouble(item);
            if (!parsed.has_value()) return std::nullopt;
            result.push_back(*parsed);
        }
        return result;
    }

    std::optional<std::vector<std::string>> _tryParseVectorString(std::string_view str) const {
        auto elements = _extractVectorElements(str);
        if (!elements.has_value()) return std::nullopt;

        if (elements->empty()) return std::vector<std::string>{};

        std::vector<std::string> result;
        for (const auto& item : elements.value()) {
            auto parsed = _tryParseString(item);
            if (!parsed.has_value()) return std::nullopt;
            result.push_back(*parsed);
        }
        return result;
    }
};

} // namespace cord