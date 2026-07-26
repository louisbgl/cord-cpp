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

} // namespace cord
