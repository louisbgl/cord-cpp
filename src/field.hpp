#pragma once

#include <string>
#include <variant>
#include <optional>
#include <type_traits>

#include "exception.hpp"

namespace cord {

enum class FieldType {
    BOOL,
    INT,
    DOUBLE,
    STRING
};

template<typename T>
constexpr FieldType typeOf() {
    if constexpr (std::is_same_v<T, bool>) {
        return FieldType::BOOL;
    } else if constexpr (std::is_same_v<T, int>) {
        return FieldType::INT;
    } else if constexpr (std::is_same_v<T, double>) {
        return FieldType::DOUBLE;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return FieldType::STRING;
    }
}

class Value {
public:
    template<typename T>
    Value(T value) : _value(value) {}

    template<typename T>
    T as() const {
        static_assert(
            std::is_same_v<T, int> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, bool> ||
            std::is_same_v<T, std::string>,
            "\n\n[CORD] Unsupported type for Value::as<T>()\n[CORD] Supported types: int, double, bool, std::string\n"
        );
        return std::get<T>(_value);
    }

    FieldType getType() const {
        switch (_value.index()) {
            case 0: return FieldType::BOOL;
            case 1: return FieldType::INT;
            case 2: return FieldType::DOUBLE;
            case 3: return FieldType::STRING;
            default: throw CordException("Unknown type");
        }
    }

    std::string toString() const {
        switch (_value.index()) {
            case 0: return std::get<bool>(_value) ? "true" : "false";
            case 1: return std::to_string(std::get<int>(_value));
            case 2: return std::to_string(std::get<double>(_value));
            case 3: return "\"" + std::get<std::string>(_value) + "\"";
            default: throw CordException("Unknown type");
        }
    }

private:
    std::variant<bool, int, double, std::string> _value;
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