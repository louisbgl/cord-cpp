#pragma once

#include <cstddef>
#include <string>

namespace cord {

/**
 * @brief An Error makes the config invalid, a Warning leaves the result usable.
 */
enum class Severity {
    Error,
    Warning,
};

/**
 * @brief A problem found in a config, in any stage.
 */
struct Diagnostic {
    Severity severity;
    std::size_t line;
    std::size_t column;
    std::string path; // Such as entities[0].pos.x. Empty for syntax errors.
    std::string message;
};

} // namespace cord
