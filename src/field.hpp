#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <type_traits>

#include "exception.hpp"

namespace cord {

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

class Value {
public:
    template<typename T>
    Value(T value) : _value(value) {}

    template<typename T>
    T as() const {
        static_assert(
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string> ||
            std::is_same_v<T, std::vector<bool>> ||
            std::is_same_v<T, std::vector<int>> ||
            std::is_same_v<T, std::vector<float>> ||
            std::is_same_v<T, std::vector<double>> ||
            std::is_same_v<T, std::vector<std::string>>,
            "\n\n[CORD] Unsupported type for Value::as<T>()\n[CORD] Supported types: bool, int, float, double, std::string, std::vector<bool>, std::vector<int>, std::vector<float>, std::vector<double>, std::vector<std::string>\n"
        );
        return std::get<T>(_value);
    }

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

    std::string toString() const {
        // lambda to convert vector to string
        auto vectorToString = [](const auto& vec) -> std::string {
            using Elem = typename std::decay_t<decltype(vec)>::value_type;
            std::string result = "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if constexpr (std::is_same_v<Elem, std::string>) {
                    result += "\"" + vec[i] + "\"";
                } else if constexpr (std::is_same_v<Elem, bool>) {
                    result += vec[i] ? "true" : "false";
                } else {
                    result += std::to_string(vec[i]);
                }
                if (i < vec.size() - 1) result += ", ";
            }
            result += "]";
            return result;
        };

        switch (_value.index()) {
            case 0: return std::get<bool>(_value) ? "true" : "false";
            case 1: return std::to_string(std::get<int>(_value));
            case 2: return std::to_string(std::get<float>(_value));
            case 3: return std::to_string(std::get<double>(_value));
            case 4: return "\"" + std::get<std::string>(_value) + "\"";
            case 5: return vectorToString(std::get<std::vector<bool>>(_value));
            case 6: return vectorToString(std::get<std::vector<int>>(_value));
            case 7: return vectorToString(std::get<std::vector<float>>(_value));
            case 8: return vectorToString(std::get<std::vector<double>>(_value));
            case 9: return vectorToString(std::get<std::vector<std::string>>(_value));
            default: throw CordException("Unknown type");
        }
    }

private:
    std::variant<bool, int, float, double, std::string,
        std::vector<bool>, std::vector<int>, std::vector<float>,
        std::vector<double>, std::vector<std::string>
    > _value;
};

class IField {
public:
    virtual ~IField() = default;

    virtual void validate() const = 0;
    virtual std::string getName() const = 0;
    virtual FieldType getType() const = 0;
    virtual bool hasDefault() const = 0;
    virtual Value getDefault() const = 0;
    virtual bool isRequired() const = 0;
};

template<typename T>
class Field : public IField {
public:
    Field(const std::string& name, std::optional<T> default_value = std::nullopt)
        : _name(name), _default_value(default_value) {}

    void validate() const override {
        if (_required && _default_value.has_value()) {
            throw CordException("Field '" + _name + "' is required and has a default value");
        }
    }

    std::string getName() const override {
        return _name;
    }

    FieldType getType() const override {
        return typeOf<T>();
    }

    bool hasDefault() const override {
        return _default_value.has_value();
    }

    Value getDefault() const override {
        if (!_default_value.has_value()) {
            throw CordException("Field '" + _name + "' does not have a default value");
        }
        return Value(_default_value.value());
    }

    bool isRequired() const override {
        return _required;
    }

    Field<T>& required() {
        _required = true;
        return *this;
    }

    Field<T>& default_(T val) {
        _default_value = val;
        return *this;
    }

private:
    std::string _name;
    std::optional<T> _default_value = std::nullopt;
    bool _required = false;
};

} // namespace cord