// cord - Config Reader
#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

// Error message macro for static_assert failures on unsupported types
#define CORD_UNSUPPORTED_TYPE(context) \
    "\n\n[CORD] Unsupported type for " context "\n" \
    "[CORD] Supported types: bool, int, float, double, std::string, " \
    "std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"
// Error message macro for static_assert failures on non-numeric types
#define CORD_NUMERIC_ONLY(context) \
    "\n\n[CORD] " context " is only supported for numeric types (int, float, double)\n"
#define CORD_UNSUPPORTED_TYPE_EXCLUDE_BOOL(context) \
    "\n\n[CORD] Unsupported type for " context "\n" \
    "[CORD] Supported types: int, float, double, std::string, " \
    "std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"

namespace cord {

template<typename T>
struct ParseResult {
    std::optional<T> value;
    std::string error = "";
};

struct VectorElements {
    std::vector<std::string_view> items;
    std::string error;
};

// Type trait to check if T is a numeric cord type
template<typename T>
constexpr bool is_supported_numeric_type_v =
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double>;

// Type trait to check if T is a supported vector type
template<typename T>
constexpr bool is_supported_vector_type_v =
    std::is_same_v<T, std::vector<bool>> ||
    std::is_same_v<T, std::vector<int>> ||
    std::is_same_v<T, std::vector<float>> ||
    std::is_same_v<T, std::vector<double>> ||
    std::is_same_v<T, std::vector<std::string>>;

// Type trait to check if T is a supported type, bool excluded (for min() and max())
template<typename T>
constexpr bool is_supported_type_exclude_bool_v =
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::vector<bool>> ||
    std::is_same_v<T, std::vector<int>> ||
    std::is_same_v<T, std::vector<float>> ||
    std::is_same_v<T, std::vector<double>> ||
    std::is_same_v<T, std::vector<std::string>>;

// Type trait to check if T is a supported cord type
// Also supports const char* and char* for convenience in result.get_or()
template<typename T>
constexpr bool is_supported_type_v =
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, const char*> ||
    std::is_same_v<T, char*> ||
    std::is_same_v<T, std::vector<bool>> ||
    std::is_same_v<T, std::vector<int>> ||
    std::is_same_v<T, std::vector<float>> ||
    std::is_same_v<T, std::vector<double>> ||
    std::is_same_v<T, std::vector<std::string>>;

// Type trait to check if T is a supported cord type
template<typename T>
constexpr bool is_supported_value_type_v =
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::vector<bool>> ||
    std::is_same_v<T, std::vector<int>> ||
    std::is_same_v<T, std::vector<float>> ||
    std::is_same_v<T, std::vector<double>> ||
    std::is_same_v<T, std::vector<std::string>>;

// Converts any supported cord type to its string representation
template<typename T>
std::string valueToString(const T& val) {
    if constexpr (std::is_same_v<T, bool>)
        return val ? "true" : "false";
    else if constexpr (std::is_same_v<T, std::string>)
        return "\"" + val + "\"";
    else if constexpr (is_supported_numeric_type_v<T>)
        return std::to_string(val);
    else if constexpr (std::is_same_v<typename T::value_type, bool>)
        // vector<bool> handled separately — operator[] returns proxy, not bool ref
        return "[" + [&]{ std::string s; for (size_t i = 0; i < val.size(); ++i) { s += (val[i] ? "true" : "false"); if (i < val.size()-1) s += ", "; } return s; }() + "]";
    else {
        // vector<int/float/double/string>
        using Elem = typename T::value_type;
        std::string s = "[";
        for (size_t i = 0; i < val.size(); ++i) {
            s += valueToString<Elem>(val[i]);
            if (i < val.size() - 1) s += ", ";
        }
        return s + "]";
    }
}

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

// Lowercases a string_view into a new std::string
inline std::string toLower(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// Returns the display name for a FieldType (used by Schema::describe())
inline std::string fieldTypeName(FieldType type) {
    switch (type) {
        case FieldType::BOOL:          return "bool";
        case FieldType::INT:           return "int";
        case FieldType::FLOAT:         return "float";
        case FieldType::DOUBLE:        return "double";
        case FieldType::STRING:        return "string";
        case FieldType::VECTOR_BOOL:   return "vector<bool>";
        case FieldType::VECTOR_INT:    return "vector<int>";
        case FieldType::VECTOR_FLOAT:  return "vector<float>";
        case FieldType::VECTOR_DOUBLE: return "vector<double>";
        case FieldType::VECTOR_STRING: return "vector<string>";
    }
    return "unknown"; // unreachable
}

/**
 * @brief Exception class for errors in the cord library.
 */
class CordException : public std::exception {
public:
    explicit CordException(const std::string& message)
        : _message("[CORD] " + message) {}

    explicit CordException(const std::string& file, int line, const std::string& message)
        : _message("[CORD] " + file + ":" + std::to_string(line) + ": " + message) {}

    const char* what() const noexcept override {
        return _message.c_str();
    }

private:
    std::string _message;
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
    T as(std::source_location loc = std::source_location::current()) const {
        static_assert(is_supported_value_type_v<T>, CORD_UNSUPPORTED_TYPE("Value::as<T>()"));
        try {
            return std::get<T>(_value);
        } catch (const std::bad_variant_access&) {
            throw CordException(loc.file_name(), loc.line(), "Type mismatch in as<T>(): value holds a different type");
        }
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
        switch (_value.index()) {
            case 0: return valueToString(std::get<bool>(_value));
            case 1: return valueToString(std::get<int>(_value));
            case 2: return valueToString(std::get<float>(_value));
            case 3: return valueToString(std::get<double>(_value));
            case 4: return valueToString(std::get<std::string>(_value));
            case 5: return valueToString(std::get<std::vector<bool>>(_value));
            case 6: return valueToString(std::get<std::vector<int>>(_value));
            case 7: return valueToString(std::get<std::vector<float>>(_value));
            case 8: return valueToString(std::get<std::vector<double>>(_value));
            case 9: return valueToString(std::get<std::vector<std::string>>(_value));
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

    virtual std::string getName() const = 0;
    virtual FieldType getType() const = 0;
    virtual bool hasDefault() const = 0;
    virtual Value getDefault() const = 0;
    virtual bool isRequired() const = 0;
    virtual std::optional<std::string> checkConstraints(const Value& value) const = 0;
    virtual std::string describeConstraints() const = 0;
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
     */
    Value getDefault() const override {
        return Value(_default_value.value());
    }

    // Checks if the field is required
    bool isRequired() const override {
        return _required;
    }

    std::optional<std::string> checkConstraints(const Value& value) const override {
        if constexpr (is_supported_numeric_type_v<T>) {
            T v = value.as<T>();
            if (_min_value.has_value() && v < *_min_value)
                return "value " + valueToString(v) + " is below minimum " + valueToString(*_min_value);
            if (_max_value.has_value() && v > *_max_value)
                return "value " + valueToString(v) + " exceeds maximum " + valueToString(*_max_value);
        }
        if constexpr (std::is_same_v<T, std::string>) {
            std::string v = value.as<std::string>();
            if (_min_size.has_value() && v.size() < *_min_size)
                return "value " + valueToString(v) + " is shorter than minimum length " + std::to_string(*_min_size);
            if (_max_size.has_value() && v.size() > *_max_size)
                return "value " + valueToString(v) + " is longer than maximum length " + std::to_string(*_max_size);
        }
        if constexpr (is_supported_vector_type_v<T>) {
            T v = value.as<T>();
            if (_min_size.has_value() && v.size() < *_min_size)
                return "vector has too few elements: got " + std::to_string(v.size()) + ", minimum " + std::to_string(*_min_size);
            if (_max_size.has_value() && v.size() > *_max_size)
                return "vector has too many elements: got " + std::to_string(v.size()) + ", maximum " + std::to_string(*_max_size);
        }
        if (_allowed_values.has_value()) {
            T v = value.as<T>();
            if (std::find(_allowed_values->begin(), _allowed_values->end(), v) == _allowed_values->end()) {
                std::string v_str = valueToString(v);
                return "value " + v_str + " is not in allowed values";
            }
        }
        return std::nullopt;
    }

    std::string describeConstraints() const override {
        std::string range;
        std::string choices;

        if constexpr (is_supported_numeric_type_v<T>) {
            if (_min_value.has_value() || _max_value.has_value()) {
                range = "[";
                range += _min_value.has_value() ? valueToString(*_min_value) : "";
                range += "..";
                range += _max_value.has_value() ? valueToString(*_max_value) : "";
                range += "]";
            }
        }
        if constexpr (std::is_same_v<T, std::string> || is_supported_vector_type_v<T>) {
            if (_min_size.has_value() || _max_size.has_value()) {
                range = "[";
                range += _min_size.has_value() ? std::to_string(*_min_size) : "";
                range += "..";
                range += _max_size.has_value() ? std::to_string(*_max_size) : "";
                range += "]";
            }
        }

        if (_allowed_values.has_value()) {
            choices = "[oneOf={";
            for (size_t i = 0; i < _allowed_values->size(); ++i) {
                choices += valueToString((*_allowed_values)[i]);
                if (i < _allowed_values->size() - 1) choices += ", ";
            }
            choices += "}]";
        }

        if (!range.empty() && !choices.empty()) return range + "  " + choices;
        if (!range.empty()) return range;
        return choices;
    }

    // Marks the field as required
    Field<T>& required(std::source_location loc = std::source_location::current()) {
        if (_default_value.has_value())
            throw CordException(loc.file_name(), loc.line(), "Field '" + _name + "' can't be both required and have a default value");
        _required = true;
        return *this;
    }

    // Sets the default value of the field
    Field<T>& default_(T val, std::source_location loc = std::source_location::current()) {
        if (_required)
            throw CordException(loc.file_name(), loc.line(), "Field '" + _name + "' can't be both required and have a default value");
        _default_value = val;
        return *this;
    }

    /**
     * @brief Sets the minimum allowed value (numeric) or minimum length/count (string/vector).
     * @param val The minimum value (inclusive). For numeric types, compared directly. For string/vector, specifies minimum length or element count.
     * @return Reference to this field for chaining.
     */
    Field<T>& min(T val) {
        static_assert(is_supported_numeric_type_v<T>, CORD_NUMERIC_ONLY("min()"));
        _min_value = val;
        return *this;
    }

    /**
     * @brief Sets the minimum length (string) or minimum element count (vector).
     * @param count The minimum size (inclusive).
     * @return Reference to this field for chaining.
     */
    Field<T>& min(size_t count) {
        static_assert(std::is_same_v<T, std::string> || is_supported_vector_type_v<T>, CORD_UNSUPPORTED_TYPE_EXCLUDE_BOOL("min()"));
        _min_size = count;
        return *this;
    }

    /**
     * @brief Sets the maximum allowed value (numeric) or maximum length/count (string/vector).
     * @param val The maximum value (inclusive). For numeric types, compared directly. For string/vector, specifies maximum length or element count.
     * @return Reference to this field for chaining.
     */
    Field<T>& max(T val) {
        static_assert(is_supported_numeric_type_v<T>, CORD_NUMERIC_ONLY("max()"));
        _max_value = val;
        return *this;
    }

    /**
     * @brief Sets the maximum length (string) or maximum element count (vector).
     * @param count The maximum size (inclusive).
     * @return Reference to this field for chaining.
     */
    Field<T>& max(size_t count) {
        static_assert(std::is_same_v<T, std::string> || is_supported_vector_type_v<T>, CORD_UNSUPPORTED_TYPE_EXCLUDE_BOOL("max()"));
        _max_size = count;
        return *this;
    }

    /**
     * @brief Sets the allowed values for the field.
     * @param values The list of allowed values.
     * @return Reference to this field for chaining.
     *
     * @note This method performs compile-time checks to ensure that the type T is supported.
     */
    Field<T>& oneOf(std::initializer_list<T> values) {
        static_assert(is_supported_value_type_v<T>, CORD_UNSUPPORTED_TYPE("oneOf()"));
        _allowed_values = std::vector<T>(values);
        return *this;
    }

private:
    std::string _name;
    std::optional<T> _default_value = std::nullopt;
    bool _required = false;
    std::optional<T> _min_value = std::nullopt;
    std::optional<T> _max_value = std::nullopt;
    std::optional<size_t> _min_size = std::nullopt;
    std::optional<size_t> _max_size = std::nullopt;
    std::optional<std::vector<T>> _allowed_values = std::nullopt;
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
    // Adds a parse-time error with optional key and line number.
    void addError(const std::string& message, const std::optional<std::string>& key = std::nullopt, const std::optional<int>& line = std::nullopt) {
        _errors.push_back({message, key, line});
    }

    // Records a missing required field by name. All missing fields are consolidated into one error.
    void addMissingRequiredKey(const std::string& name) {
        _missing_required.push_back(name);
    }

    /**
     * @brief Returns all errors: consolidated required-fields error first, then parse errors in order.
     * @return A vector of ParseError structs.
     */
    std::vector<ParseError> getErrors() const {
        std::vector<ParseError> result;
        if (!_missing_required.empty()) {
            std::string msg = "missing required fields: ";
            for (size_t i = 0; i < _missing_required.size(); ++i) {
                msg += "'" + _missing_required[i] + "'";
                if (i < _missing_required.size() - 1) msg += ", ";
            }
            result.push_back({msg, std::nullopt, std::nullopt});
        }
        result.insert(result.end(), _errors.begin(), _errors.end());
        return result;
    }

    // Checks if there are any collected errors, including missing required fields.
    bool hasErrors() const {
        return !_errors.empty() || !_missing_required.empty();
    }

private:
    std::vector<ParseError> _errors;
    std::vector<std::string> _missing_required;
};

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

    /**
     * @brief Sets the value associated with a key, or adds it if it doesn't exist.
     * @param key The key to set.
     * @param value The value to set.
     * @return A reference to the Result object.
     *
     * @note Compile-time checks are performed to ensure that only supported types are used.
     */
    template<typename T>
    Result& set(std::string_view key, T value) {
        static_assert(is_supported_type_v<T>, CORD_UNSUPPORTED_TYPE("result.set<T>()"));
        _insert_or_modify_value(key, Value(value));
        return *this;
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

    // Gets a vector of cord::ParseError objects
    const std::vector<ParseError> getErrors() const {
        return _ec.getErrors();
    }

    // Writes the parsed key-value pairs to a string in the format "key + delimiter + val\n"
    // Yes we reuse the Schema delimiter here, because the Result is tied to the Schema that produced it
    std::string write() const {
        std::string output;
        for (const auto& [key, value] : _values) {
            output += key + _delimiter + value.toString() + "\n";
        }
        return output;
    }

    // Writes the parsed key-value pairs to a file in the format "key + delimiter + val\n"
    // Yes we reuse the Schema delimiter here, because the Result is tied to the Schema that produced it
    void writeFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open())
            throw CordException("Failed to open file for writing: " + filename);
        file << write();
    }

private:
    std::vector<std::pair<std::string, Value>> _values;
    ErrorCollector _ec;
    std::string _filepath;
    std::string _delimiter = "=";

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
        result._delimiter = _delimiter;

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
