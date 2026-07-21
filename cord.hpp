// cord - Config Reader
#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cord {

/**
 * @brief Exception class for errors in the cord library.
 */
class CordException : public std::exception {
public:
    explicit CordException(const std::string& message)
        : _message(message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

private:
    std::string _message;
};

/**
* @brief Enum representing the supported field types in the schema.
*/
enum class FieldType {
    BOOL,
    INT,
    FLOAT,
    DOUBLE,
    STRING,
    VECTOR_BOOL,
    VECTOR_INT,
    VECTOR_FLOAT,
    VECTOR_DOUBLE,
    VECTOR_STRING
};

/**
* @brief Helper function to get the FieldType from a C++ type.
* @tparam T The C++ type for which to get the FieldType.
* @return The corresponding FieldType.
*/
template<typename T>
constexpr FieldType typeOf() {
    if constexpr (std::is_same_v<T, bool>) {
        return FieldType::BOOL;
    } else if constexpr (std::is_same_v<T, int>) {
        return FieldType::INT;
    } else if constexpr (std::is_same_v<T, float>) {
        return FieldType::FLOAT;
    } else if constexpr (std::is_same_v<T, double>) {
        return FieldType::DOUBLE;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return FieldType::STRING;
    } else if constexpr (std::is_same_v<T, std::vector<bool>>) {
        return FieldType::VECTOR_BOOL;
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        return FieldType::VECTOR_INT;
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        return FieldType::VECTOR_FLOAT;
    } else if constexpr (std::is_same_v<T, std::vector<double>>) {
        return FieldType::VECTOR_DOUBLE;
    } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        return FieldType::VECTOR_STRING;
    } else {
        throw CordException("Unsupported type");
    }
}

/**
 * @brief Represents a value in the schema, which can be of various types.
 *
 * @note Compile-time checks are performed to ensure that only supported types are used.
 */
class Value {
public:
    template<typename T>
    Value(T value) : _value(value) {}

    /**
     * @brief Converts the value to the specified type.
     * @tparam T The type to convert to.
     * @return The converted value.
     *
     * @note This method performs compile-time checks to ensure that the type T is supported.
     */
    template<typename T>
    T as() const {
        static_assert(
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string> ||
            std::is_same_v<T, std::vector<bool>> ||
            std::is_same_v<T, std::vector<int>> ||
            std::is_same_v<T, std::vector<float>> ||
            std::is_same_v<T, std::vector<double>> ||
            std::is_same_v<T, std::vector<std::string>>,
            "\n\n[CORD] Unsupported type for Value::as<T>()\n[CORD] Supported types: bool, int, float, double, std::string, std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"
        );
        return std::get<T>(_value);
    }

    /**
     * @brief Gets the type of the value.
     * @return The corresponding FieldType.
     * @throws CordException if the type is unknown.
     */
    FieldType getType() const {
        switch (_value.index()) {
            case 0: return FieldType::BOOL;
            case 1: return FieldType::INT;
            case 2: return FieldType::FLOAT;
            case 3: return FieldType::DOUBLE;
            case 4: return FieldType::STRING;
            case 5: return FieldType::VECTOR_BOOL;
            case 6: return FieldType::VECTOR_INT;
            case 7: return FieldType::VECTOR_FLOAT;
            case 8: return FieldType::VECTOR_DOUBLE;
            case 9: return FieldType::VECTOR_STRING;
            default: throw CordException("Unknown type");
        }
    }

    /**
     * @brief Converts the value to a string representation.
     * @return The string representation of the value.
     * @throws CordException if the type is unknown.
     */
    std::string toString() const {
        // lambda to convert vector to string
        auto vectorToString = [](const auto& vec) -> std::string {
            using Elem = typename std::decay_t<decltype(vec)>::value_type;
            std::string result = "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if constexpr (std::is_same_v<Elem, std::string>) {
                    result += "\"" + vec[i] + "\"";
                } else if constexpr (std::is_same_v<Elem, bool>) {
                    result += vec[i] ? "true" : "false";
                } else {
                    result += std::to_string(vec[i]);
                }
                if (i < vec.size() - 1) result += ", ";
            }
            result += "]";
            return result;
        };

        switch (_value.index()) {
            case 0: return std::get<bool>(_value) ? "true" : "false";
            case 1: return std::to_string(std::get<int>(_value));
            case 2: return std::to_string(std::get<float>(_value));
            case 3: return std::to_string(std::get<double>(_value));
            case 4: return "\"" + std::get<std::string>(_value) + "\"";
            case 5: return vectorToString(std::get<std::vector<bool>>(_value));
            case 6: return vectorToString(std::get<std::vector<int>>(_value));
            case 7: return vectorToString(std::get<std::vector<float>>(_value));
            case 8: return vectorToString(std::get<std::vector<double>>(_value));
            case 9: return vectorToString(std::get<std::vector<std::string>>(_value));
            default: throw CordException("Unknown type");
        }
    }

private:
    std::variant<bool, int, float, double, std::string,
        std::vector<bool>, std::vector<int>, std::vector<float>,
        std::vector<double>, std::vector<std::string>
    > _value;
};

/**
 * @brief Interface for a field in the schema.
 */
class IField {
public:
    virtual ~IField() = default;

    virtual void validate() const = 0;
    virtual std::string getName() const = 0;
    virtual FieldType getType() const = 0;
    virtual bool hasDefault() const = 0;
    virtual Value getDefault() const = 0;
    virtual bool isRequired() const = 0;
};

/**
 * @brief Concrete implementation of a field in the schema.
 * @tparam T The type of the field.
 *
 * @note A field can be marked as required or have a default value, but not both, in which case a CordException is thrown.
 * @note A field can be neither required nor have a default, in which case trying to get the value is unsafe (in result.get()).
 */
template<typename T>
class Field : public IField {
public:
    Field(const std::string& name, std::optional<T> default_value = std::nullopt)
        : _name(name), _default_value(default_value) {}

    /**
     * @brief Ensures proper field configuration.
     * @throws CordException if the field is both required and has a default value.
     */
    void validate() const override {
        if (_required && _default_value.has_value()) {
            throw CordException("Field '" + _name + "' is required and has a default value");
        }
    }

    // Gets the name of the field
    std::string getName() const override {
        return _name;
    }

    // Gets the type of the field
    FieldType getType() const override {
        return typeOf<T>();
    }

    // Checks if the field has a default value
    bool hasDefault() const override {
        return _default_value.has_value();
    }

    /**
     * @brief Gets the default value of the field.
     * @return The default value.
     * @throws CordException if no default is set.
     */
    Value getDefault() const override {
        if (!_default_value.has_value()) {
            throw CordException("Field '" + _name + "' does not have a default value");
        }
        return Value(_default_value.value());
    }

    // Checks if the field is required
    bool isRequired() const override {
        return _required;
    }

    // Marks the field as required
    Field<T>& required() {
        _required = true;
        return *this;
    }

    // Sets the default value of the field
    Field<T>& default_(T val) {
        _default_value = val;
        return *this;
    }

private:
    std::string _name;
    std::optional<T> _default_value = std::nullopt;
    bool _required = false;
};

/**
 * @brief Represents a parsing error with an optional key and line number.
 */
struct ParseError {
    std::string message;
    std::optional<std::string> key;
    std::optional<int> line;
};

/**
 * @brief Collects and manages parsing errors.
 */
class ErrorCollector {
public:
    // Adds a parsing error to the collector.
    void addError(const ParseError& error) {
        _errors.push_back(error);
    }

    // Adds a parsing error with a message, optional key, and optional line number.
    void addError(const std::string& message, const std::optional<std::string>& key = std::nullopt, const std::optional<int>& line = std::nullopt) {
        _errors.push_back({message, key, line});
    }

    // Returns the list of collected parsing errors.
    const std::vector<ParseError>& getErrors() const {
        return _errors;
    }

    // Checks if there are any collected parsing errors.
    bool hasErrors() const {
        return !_errors.empty();
    }

private:
    std::vector<ParseError> _errors;
};

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
    Value get(std::string_view key) const {
        auto it = _values.find(std::string(key));
        if (it != _values.end()) {
            return it->second;
        } else {
            throw CordException("Key not found: " + std::string(key));
        }
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
    // Parses the input string according to the schema
    // returns a Result object containing the parsed values and any errors
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
        static_assert(
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string> ||
            std::is_same_v<T, std::vector<bool>> ||
            std::is_same_v<T, std::vector<int>> ||
            std::is_same_v<T, std::vector<float>> ||
            std::is_same_v<T, std::vector<double>> ||
            std::is_same_v<T, std::vector<std::string>>,
            "\n\n[CORD] Unsupported type for schema.add<T>()\n[CORD] Supported types: bool, int, float, double, std::string, vector<bool>, vector<int>, vector<float>, vector<double>, vector<std::string>\n"
        );
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
