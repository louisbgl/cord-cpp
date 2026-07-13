#pragma once

#include <string>
#include <optional>
#include <vector>

namespace cord {

struct ParseError {
    std::string message;
    std::optional<std::string> key;
    std::optional<int> line;
};

class ErrorCollector {
public:
    void addError(const ParseError& error) {
        _errors.push_back(error);
    }

    void addError(const std::string& message, const std::optional<std::string>& key = std::nullopt, const std::optional<int>& line = std::nullopt) {
        _errors.push_back({message, key, line});
    }

    const std::vector<ParseError>& getErrors() const {
        return _errors;
    }

    bool hasErrors() const {
        return !_errors.empty();
    }

private:
    std::vector<ParseError> _errors;
};

} // namespace cord