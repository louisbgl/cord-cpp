#pragma once

#include <cctype>
#include <string>
#include <vector>
#include <optional>
#include <type_traits>

namespace cord {

template<typename T>
struct ParseResult {
    std::optional<T> value;
    std::string error = "";
};


// Error message macro for static_assert failures on unsupported types
#define CORD_UNSUPPORTED_TYPE(context) \
    "\n\n[CORD] Unsupported type for " context "\n" \
    "[CORD] Supported types: bool, int, float, double, std::string, " \
    "std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"

// Error message macro for static_assert failures on non-numeric types
#define CORD_NUMERIC_ONLY(context) \
    "\n\n[CORD] " context " is only supported for numeric types (int, float, double)\n"

// Type trait to check if T is a numeric cord type (eligible for min/max)
template<typename T>
constexpr bool is_numeric_type_v =
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double>;

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
    else if constexpr (is_numeric_type_v<T>)
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

} // namespace cord
