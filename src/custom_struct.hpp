#pragma once

#include "field.hpp"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <source_location>

namespace cord {

template<typename T>
class CustomStruct {
public:
    CustomStruct(const std::string& name) : _name(name) {
        static_assert(is_custom_struct_v<T>, "Must use CORD_REGISTER_STRUCT(T) before using CustomStruct<T>");
    }

    T create() const { return T(); }

    template<typename FieldType>
    Field<FieldType>& add(std::string name, FieldType T::* member_ptr,
                          std::source_location loc = std::source_location::current()) {
        static_assert(is_supported_value_type_v<FieldType>, CORD_UNSUPPORTED_TYPE("CustomStruct::add<T>()"));

        if (name.empty()) {
            throw CordException(loc.file_name(), loc.line(), "Field name cannot be empty");
        }
        for (const auto& f : _fields) {
            if (f->getName() == name) {
                throw CordException(loc.file_name(), loc.line(), "CustomStruct cannot have duplicate field names: " + name);
            }
        }

        auto field  = std::make_shared<Field<FieldType>>(name);
        Field<FieldType>& field_ref = *field;
        _fields.push_back(std::move(field));

        _accessors.push_back({
            name,
            [member_ptr](void* obj, const Value& value) {
                T* typed_obj = static_cast<T*>(obj);
                typed_obj->*member_ptr = value.as<FieldType>();
            },
            [member_ptr](const void* obj) -> Value {
                const T* typed_obj = static_cast<const T*>(obj);
                return Value(typed_obj->*member_ptr);
            }
        });
        return field_ref;
    }

    std::string getName() const { return _name; }
    const std::vector<std::shared_ptr<IField>>& getFields() const { return _fields; }

    void setField(void* obj, const std::string& name, const Value& value,
                  std::source_location loc = std::source_location::current()) const {
        for (const auto& accessor : _accessors) {
            if (accessor.name == name) {
                accessor.setter(obj, value);
                return;
            }
        }
        throw CordException(loc.file_name(), loc.line(), "Field not found in CustomStruct: " + name);
    }

    Value getField(const void* obj, const std::string& name,
                   std::source_location loc = std::source_location::current()) const {
        for (const auto& accessor : _accessors) {
            if (accessor.name == name) {
                return accessor.getter(obj);
            }
        }
        throw CordException(loc.file_name(), loc.line(), "Field not found in CustomStruct: " + name);
    }

private:
    std::string _name;
    std::vector<std::shared_ptr<IField>> _fields;

    struct Accessors {
        std::string name;
        std::function<void(void*, const Value&)> setter;
        std::function<Value(const void*)> getter;
    };
    std::vector<Accessors> _accessors;
};

}