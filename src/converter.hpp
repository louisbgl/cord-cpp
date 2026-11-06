#pragma once

#include <charconv>
#include <concepts>
#include <string>
#include <system_error>
#include <variant>

#include "context.hpp"
#include "node.hpp"

namespace cord {

/**
 * @brief Converts one value of type T from a tree node. Knows nothing about keys.
 *
 * Each specialization has `static bool read(const Node&, T& out, Context&)`. It returns true when
 * `out` holds the value. Otherwise it has reported why to the context, and `out` is not valid.
 *
 * Not defined on purpose: a type with no specialization is a compile error.
 */
template <class T>
struct Converter;

/** @brief A type cord can read. */
template <class T>
concept Convertible = requires(const Node& node, T& out, Context& ctx) {
    { Converter<T>::read(node, out, ctx) } -> std::same_as<bool>;
};

/** @brief An integer type holding numbers. `bool` and the character types do not count. */
template <class T>
concept Integer = std::integral<T>
    && !std::same_as<T, bool>
    && !std::same_as<T, char>
    && !std::same_as<T, wchar_t>
    && !std::same_as<T, char8_t>
    && !std::same_as<T, char16_t>
    && !std::same_as<T, char32_t>;

template <>
struct Converter<bool> {
    static bool read(const Node& node, bool& out, Context& ctx) {
        const auto* value = std::get_if<Bool>(&node.value);
        if (!value) { ctx.error(node, "Expected a bool"); return false; }
        out = value->value;
        return true;
    }
};

template <Integer T>
struct Converter<T> {
    static bool read(const Node& node, T& out, Context& ctx) {
        const auto* value = std::get_if<Int>(&node.value);
        if (!value) { ctx.error(node, "Expected an integer"); return false; }
        auto [ptr, ec] = std::from_chars(value->text.data(), value->text.data() + value->text.size(), out);
        if (ec != std::errc()) {
            ctx.error(node, value->text + " does not fit in this integer type");
            return false;
        }
        return true;
    }
};

template <std::floating_point T>
struct Converter<T> {
    static bool read(const Node& node, T& out, Context& ctx) {
        const std::string* text = nullptr;
        if (const auto* value = std::get_if<Float>(&node.value)) {
            text = &value->text;
        } else if (const auto* value = std::get_if<Int>(&node.value)) {
            text = &value->text;
        } else {
            ctx.error(node, "Expected a number");
            return false;
        }

        auto [ptr, ec] = std::from_chars(text->data(), text->data() + text->size(), out);
        if (ec != std::errc()) {
            ctx.error(node, *text + " does not fit in this floating point type");
            return false;
        }
        return true;
    }
};

template <>
struct Converter<std::string> {
    static bool read(const Node& node, std::string& out, Context& ctx) {
        const auto* value = std::get_if<String>(&node.value);
        if (!value) { ctx.error(node, "Expected a string"); return false; }
        out = value->value;
        return true;
    }
};

} // namespace cord
