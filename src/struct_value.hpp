#pragma once

#include "exception.hpp"

#include <any>
#include <string>
#include <typeinfo>

namespace cord {

class StructValue {
public:
    template<typename T>
    StructValue(T value) : _data(std::move(value)), _type(&typeid(T)) {}

    template<typename T>
    T& as() {
        if (*_type != typeid(T))
            throw CordException("StructValue type mismatch: cannot cast " + std::string(_type->name()) + " to " + std::string(typeid(T).name()));
        return std::any_cast<T&>(_data);
    }

    template<typename T>
    const T& as() const {
        if (*_type != typeid(T))
            throw CordException("StructValue type mismatch: cannot cast " + std::string(_type->name()) + " to " + std::string(typeid(T).name()));
        return std::any_cast<const T&>(_data);
    }

private:
    std::any _data;
    const std::type_info* _type = nullptr;
};

}