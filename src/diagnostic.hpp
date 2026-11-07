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

/**
 * @brief Formats a diagnostic for people to read, e.g. `[CORD] Error 1:5 [port]: Expected an integer`.
 *
 * The position is left out when there is none (0:0, like an absent key), the path when it is empty.
 */
inline std::string toString(const Diagnostic& diagnostic) {
    std::string text = diagnostic.severity == Severity::Error ? "[CORD] Error" : "[CORD] Warning";
    if (diagnostic.line != 0) text += " " + std::to_string(diagnostic.line) + ":" + std::to_string(diagnostic.column);
    if (!diagnostic.path.empty()) text += " [" + diagnostic.path + "]";
    return text + ": " + diagnostic.message;
}

} // namespace cord
