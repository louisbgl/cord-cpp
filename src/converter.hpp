#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <optional>

#include "context.hpp"
#include "node.hpp"

namespace cord {

template <class T>
class Struct; // defined in struct.hpp

/**
 * @brief A struct with a `describe(cord::Struct<T>&)` that cord can find.
 *
 * The class check comes first: `Struct<int>` can't even be instantiated, which would be a hard error.
 */
template <class T>
concept Describable = std::is_class_v<T> && requires(Struct<T>& s) { describe(s); };

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

/**
 * @brief T's description, built by its describe() on the first call only, then shared.
 */
template <Describable T>
const Struct<T>& description() {
    static const Struct<T> cached = [] {
        Struct<T> s;
        describe(s);
        return s;
    }();
    return cached;
}

template <Describable T>
struct Converter<T> {
    static bool read(const Node& node, T& out, Context& ctx) {
        const auto* object = std::get_if<Object>(&node.value);
        if (!object) { ctx.error(node, "Expected an object"); return false; }
        return description<T>().read(*object, out, ctx);
    }
};

template <Convertible T>
struct Converter<std::vector<T>> {
    static bool read(const Node& node, std::vector<T>& out, Context& ctx) {
        const auto* array = std::get_if<Array>(&node.value);
        if (!array) { ctx.error(node, "Expected an array"); return false; }
        
        bool ok = true;
        out.clear();
        out.reserve(array->size());
        for (std::size_t i = 0; i < array->size(); i++) {
            ctx.pushIndex(i);
            T element{};
            if (!Converter<T>::read(array->at(i), element, ctx)) ok = false;
            else out.push_back(std::move(element));
            ctx.popPath();
        }
        return ok;
    }
};

template <Convertible T>
struct Converter<std::optional<T>> {
    static bool read(const Node& node, std::optional<T>& out, Context& ctx) {
        T element{};
        if (!Converter<T>::read(node, element, ctx)) {
            out = std::nullopt;
            return false;
        }
        out = std::move(element);
        return true;
    }
};

} // namespace cord
