#pragma once

#include <vector>
#include <type_traits>

namespace cord {

template<typename T>
struct is_custom_struct : std::false_type {};

template<typename T>
struct is_vector_of_custom_struct : std::false_type {};

template<typename T>
struct is_vector_of_custom_struct<std::vector<T>>
    : std::bool_constant<is_custom_struct<T>::value> {};

template<typename T>
inline constexpr bool is_custom_struct_v = is_custom_struct<T>::value;

template<typename T>
inline constexpr bool is_vector_of_custom_struct_v = is_vector_of_custom_struct<T>::value;

}

#define CORD_REGISTER_STRUCT(T) \
    namespace cord { \
        template<> struct is_custom_struct<T> : std::true_type {}; \
    }
