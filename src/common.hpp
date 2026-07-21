#pragma once

#include <string>
#include <vector>
#include <type_traits>

namespace cord {

// Error message macro for static_assert failures on unsupported types
#define CORD_UNSUPPORTED_TYPE(context) \
    "\n\n[CORD] Unsupported type for " context "\n" \
    "[CORD] Supported types: bool, int, float, double, std::string, " \
    "std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"

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

} // namespace cord
