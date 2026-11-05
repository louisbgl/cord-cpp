#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace cord {

struct Bool {
    bool value;
};

/**
 * @brief An INT literal, as written. Its target type is only known at binding.
 */
struct Int {
    std::string text;
};

/**
 * @brief A FLOAT literal, as written. Its target type is only known at binding.
 */
struct Float {
    std::string text;
};

/**
 * @brief A STRING literal, without its quotes and with its escapes decoded.
 */
struct String {
    std::string value;
};

struct Node;
struct Entry;

using Array = std::vector<Node>;

/**
 * @brief Entries in source order. Duplicate keys are kept, binding warns about them.
 */
using Object = std::vector<Entry>;

/**
 * @brief An untyped value of the parsed tree, and where it starts in the source.
 */
struct Node {
    std::variant<Bool, Int, Float, String, Array, Object> value;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Entry {
    std::string key;
    Node value;
};

} // namespace cord
