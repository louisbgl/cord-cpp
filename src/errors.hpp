#pragma once

#include <string>
#include <optional>
#include <vector>

namespace cord {

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

} // namespace cord