#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <type_traits>

#include "common.hpp"
#include "exception.hpp"

namespace cord {

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

/**
* @brief Helper function to get the FieldType from a C++ type.
* @tparam T The C++ type for which to get the FieldType.
* @return The corresponding FieldType.
*/
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

/**
 * @brief Represents a value in the schema, which can be of various types.
 *
 * @note Compile-time checks are performed to ensure that only supported types are used.
 */
class Value {
public:
    template<typename T>
    Value(T value) : _value(value) {}

    /**
     * @brief Converts the value to the specified type.
     * @tparam T The type to convert to.
     * @return The converted value.
     *
     * @note This method performs compile-time checks to ensure that the type T is supported.
     */
    template<typename T>
    T as() const {
        static_assert(is_supported_value_type_v<T>, CORD_UNSUPPORTED_TYPE("Value::as<T>()"));
        try {
            return std::get<T>(_value);
        } catch (const std::bad_variant_access&) {
            throw CordException("Type mismatch in as<T>(): value holds a different type");
        }
    }

    /**
     * @brief Gets the type of the value.
     * @return The corresponding FieldType.
     * @throws CordException if the type is unknown.
     */
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

    /**
     * @brief Converts the value to a string representation.
     * @return The string representation of the value.
     * @throws CordException if the type is unknown.
     */
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

/**
 * @brief Interface for a field in the schema.
 */
class IField {
public:
    virtual ~IField() = default;

    virtual std::string getName() const = 0;
    virtual FieldType getType() const = 0;
    virtual bool hasDefault() const = 0;
    virtual Value getDefault() const = 0;
    virtual bool isRequired() const = 0;
};

/**
 * @brief Concrete implementation of a field in the schema.
 * @tparam T The type of the field.
 *
 * @note A field can be marked as required or have a default value, but not both, in which case a CordException is thrown.
 * @note A field can be neither required nor have a default, in which case trying to get the value is unsafe (in result.get()).
 */
template<typename T>
class Field : public IField {
public:
    Field(const std::string& name, std::optional<T> default_value = std::nullopt)
        : _name(name), _default_value(default_value) {}

    // Gets the name of the field
    std::string getName() const override {
        return _name;
    }

    // Gets the type of the field
    FieldType getType() const override {
        return typeOf<T>();
    }

    // Checks if the field has a default value
    bool hasDefault() const override {
        return _default_value.has_value();
    }

    /**
     * @brief Gets the default value of the field.
     * @return The default value.
     */
    Value getDefault() const override {
        return Value(_default_value.value());
    }

    // Checks if the field is required
    bool isRequired() const override {
        return _required;
    }

    // Marks the field as required
    Field<T>& required() {
        if (_default_value.has_value())
            throw CordException("Field '" + _name + "' can't be both required and have a default value");
        _required = true;
        return *this;
    }

    // Sets the default value of the field
    Field<T>& default_(T val) {
        if (_required)
            throw CordException("Field '" + _name + "' can't be both required and have a default value");
        _default_value = val;
        return *this;
    }

private:
    std::string _name;
    std::optional<T> _default_value = std::nullopt;
    bool _required = false;
};

} // namespace cord