#pragma once

#include <cctype>
#include <string>
#include <vector>
#include <format>
#include <optional>
#include <type_traits>

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

// int, float, double — numeric constraints (min/max by value)
template<typename T>
constexpr bool is_supported_numeric_type_v =
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double>;

// all five vector variants
template<typename T>
constexpr bool is_supported_vector_type_v =
    std::is_same_v<T, std::vector<bool>> ||
    std::is_same_v<T, std::vector<int>> ||
    std::is_same_v<T, std::vector<float>> ||
    std::is_same_v<T, std::vector<double>> ||
    std::is_same_v<T, std::vector<std::string>>;

// every supported type except scalar bool — used by the size_t overloads of min()/max()
// (string/vector accept a size constraint; scalar bool has no meaningful size)
// note: vector<bool> is included — it supports element-count constraints
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

// all supported cord types, plus const char* / char* which implicitly convert to std::string
// (used by get_or() and set() so callers can pass string literals without an explicit cast)
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
    else if constexpr (std::is_same_v<T, std::string>) {
        // need to re escape escape sequences like a newline to \n
        std::string s = "\"";
        for (char c : val) {
            switch (c) {
                case '\\': s += "\\\\"; break;
                case '\n': s += "\\n"; break;
                case '\t': s += "\\t"; break;
                case '"':  s += "\\\""; break;
                default:   s += c; break;
            }
        }
        s += "\"";
        return s;
    }
    else if constexpr (std::is_same_v<T, int>)
        return std::to_string(val);
    else if constexpr (is_supported_numeric_type_v<T>) // matches float and double
        return std::format("{:g}", val);
    else if constexpr (std::is_same_v<typename T::value_type, bool>) {
        // vector<bool> is special: operator[] returns a proxy object, not a bool&,
        // so we can't pass elements directly to a recursive valueToString<bool> call
        std::string s = "[";
        for (size_t i = 0; i < val.size(); ++i) {
            s += val[i] ? "true" : "false";
            if (i < val.size() - 1) s += ", ";
        }
        return s + "]";
    }
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

} // namespace cord
