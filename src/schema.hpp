#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_map>

#include "field.hpp"
#include "errors.hpp"
#include "exception.hpp"

namespace cord {

class Result {

friend class Schema;

public:
    Value get(std::string_view key) const {
        auto it = _values.find(std::string(key));
        if (it != _values.end()) {
            return it->second;
        } else {
            throw CordException("Key not found: " + std::string(key));
        }
    }

    bool hasErrors() const {
        return _ec.hasErrors();
    }

    std::vector<ParseError> getErrors() const {
        return _ec.getErrors();
    }

private:
    std::unordered_map<std::string, Value> _values;
    ErrorCollector _ec;
};

class Schema {
public:
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

        for (const auto& line : lines) {
            std::string_view trimmed_line = _trim(line);
            if (trimmed_line.empty()) continue;
            if (_allow_comments && trimmed_line[0] == _comment_char) continue;

            size_t equal_pos = trimmed_line.find('=');
            if (equal_pos == std::string_view::npos) {
                result._ec.addError("Missing '=' in line: " + std::string(line));
                continue;
            }

            std::string_view key = _trim(trimmed_line.substr(0, equal_pos));
            std::string_view value_str = _trim(trimmed_line.substr(equal_pos + 1));

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
                    result._values.emplace(field->getName(), Value(*parsed_value));
                    return true;
                }
                return false;
            };

            bool parsed = false;
            switch (field->getType()) {
                case FieldType::BOOL: {
                    parsed = tryParseAndStore(&Schema::_tryParseBool);
                    break;
                }
                case FieldType::STRING: {
                    parsed = tryParseAndStore(&Schema::_tryParseString);
                    break;
                }
                case FieldType::INT: {
                    parsed = tryParseAndStore(&Schema::_tryParseInt);
                    break;
                }
                case FieldType::DOUBLE: {
                    parsed = tryParseAndStore(&Schema::_tryParseDouble);
                    break;
                }
            }

            if (!parsed) {
                result._ec.addError("Failed to parse value for key: " + std::string(key));
            }
        }

        // ensure all required fields are present
        for (const auto& field : _fields) {
            if (field->isRequired() && result._values.find(field->getName()) == result._values.end()) {
                result._ec.addError("Missing required field: " + field->getName());
            }
        }

        // apply default values if needed
        for (const auto& field : _fields) {
            if (!field->hasDefault()) continue;
            if (result._values.find(field->getName()) != result._values.end()) continue;
            result._values.emplace(field->getName(), field->getDefault());
        }

        return result;
    }

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

    void describe() const {
        std::cout << "Schema description:" << std::endl;
        std::cout << "Strict mode: " << (_strict ? "enabled" : "disabled") << std::endl;
        std::cout << "Allow comments: " << (_allow_comments ? "enabled" : "disabled") << std::endl;
        std::cout << std::endl;

        for (const auto& field : _fields) {
            switch (field->getType()) {
                case FieldType::BOOL:
                    std::cout << "bool " << field->getName();
                    break;
                case FieldType::INT:
                    std::cout << "int " << field->getName();
                    break;
                case FieldType::DOUBLE:
                    std::cout << "double " << field->getName();
                    break;
                case FieldType::STRING:
                    std::cout << "string " << field->getName();
                    break;
            }

            if (field->hasDefault()) {
                std::cout << " (default = " << field->getDefault().toString() << ")";
            }

            if (field->isRequired()) {
                std::cout << " (required)";
            }

            std::cout << std::endl;
        }
    }

    template<typename T>
    Field<T>& add(std::string name) {
        auto field = std::make_unique<Field<T>>(name);
        Field<T>& ptr = *field;
        _fields.push_back(std::move(field));
        return ptr;
    }

    void setStrict(bool strict) {
        _strict = strict;
    }

    void setAllowComments(bool allow) {
        _allow_comments = allow;
    }

private:
    std::vector<std::unique_ptr<IField>> _fields;
    bool _strict = false;
    bool _allow_comments = false;
    const char _comment_char = '#';

    std::string_view _trim(std::string_view s) const {
        size_t start = 0;
        while (start < s.size() && std::isspace(s[start])) ++start;
        size_t end = s.size();
        while (end > start && std::isspace(s[end - 1])) --end;
        return s.substr(start, end - start);
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
};

} // namespace cord